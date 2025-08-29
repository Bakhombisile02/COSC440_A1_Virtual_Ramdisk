/*----------------------------------------------------------------------------*/
/* File: ramdisk.c */
/* Date: August 2025 */  
/* Author: Bakhombisile Dlamini - COSC440 Assignment 1 */
/* Description: Virtual Ramdisk Character Device Driver Implementation */
/* Based on: Template module by Zhiyi Huang (temp.c) */
/* Version: 1.1 - Enhanced with O(1) Hash Table Page Lookup */
/*----------------------------------------------------------------------------*/
/* Performance Enhancement: Implemented hash table for O(1) page lookup
 * - Uses kernel DECLARE_HASHTABLE with 256 buckets (2^8)
 * - hash_32() function for even distribution across buckets
 * - Maintains both linked list (sequential) and hash table (random access)
 * - Improves page lookup from O(n) to O(1) average case
 * - Research: Kernel hashtable.h patterns and hash function selection
 */
/* This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/proc_fs.h>
#include <linux/fcntl.h>
#include <linux/uaccess.h>
#include <linux/ioctl.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/list.h>
#include <linux/mm.h>
#include <linux/semaphore.h>
#include <linux/hashtable.h>
#include <linux/hash.h>
#include <linux/mman.h>
#include <linux/seq_file.h>
#include <linux/slab.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Bakhombisile Dlamini");
MODULE_DESCRIPTION("Virtual Ramdisk Character Device Driver - COSC440 Assignment 1 (Enhanced)");

/* Module parameters */
int major = 0;
module_param(major, int, S_IRUGO);
MODULE_PARM_DESC(major, "device major number");

/* Page size constants from kernel documentation */
#define RAMDISK_PAGE_SIZE PAGE_SIZE  /* Typically 4096 bytes */
#define RAMDISK_HASH_BITS 8         /* 256 hash buckets for O(1) page lookup */

/* IOCTL command for limiting concurrent users */
#define RAMDISK_IOC_MAGIC 'r'
#define RAMDISK_IOC_LIMIT_USERS _IOW(RAMDISK_IOC_MAGIC, 1, int)

/* Memory cache for optimized page structure allocation */
static struct kmem_cache *ramdisk_cache = NULL;

/* /proc entry for device information */
static struct proc_dir_entry *proc_entry = NULL;

/* 
 * Ramdisk page structure - represents one page of data in our virtual ramdisk
 * Uses kernel linked list patterns discovered through online research at kernelnewbies.org
 * Enhanced with hash table support for O(1) page lookup performance
 */
struct ramdisk_page {
    struct page *page;          /* Kernel page structure from alloc_page() */
    unsigned long offset;       /* File offset this page represents */
    struct list_head list;      /* Kernel linked list node for sequential access */
    struct hlist_node hash;     /* Hash table node for O(1) lookup */
};

/*
 * Main ramdisk device structure - based on template but with dynamic pages
 * Enhanced with hash table for O(1) page lookup performance
 */
struct ramdisk_dev {
    struct list_head pages;     /* Head of page list using LIST_HEAD pattern for sequential access */
    DECLARE_HASHTABLE(page_hash, RAMDISK_HASH_BITS); /* Hash table for O(1) page lookup */
    size_t size;               /* Current file size in bytes */
    size_t total_pages;        /* Total number of allocated pages */
    size_t memory_usage;       /* Total memory usage in bytes */
    int max_users;             /* Maximum concurrent users (ioctl controlled) */
    int current_users;         /* Current number of open file handles */
    struct semaphore sem;      /* Mutual exclusion for thread safety */
    struct cdev cdev;          /* Character device structure */
    struct class *class;       /* Device class for udev */
    struct device *device;     /* Device structure for automatic node creation */
    unsigned long ops_read;    /* Read operation counter for logging */
    unsigned long ops_write;   /* Write operation counter for logging */
    unsigned long ops_seek;    /* Seek operation counter for logging */
} *ramdisk_device;

/* Function prototypes following template pattern */
int ramdisk_open(struct inode *inode, struct file *filp);
int ramdisk_release(struct inode *inode, struct file *filp);
ssize_t ramdisk_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos);
ssize_t ramdisk_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos);
loff_t ramdisk_llseek(struct file *filp, loff_t off, int whence);
long ramdisk_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
int ramdisk_mmap(struct file *filp, struct vm_area_struct *vma);

/* Internal helper functions for page management */
static struct ramdisk_page *find_page_at_offset(unsigned long offset);
static struct ramdisk_page *create_page_at_offset(unsigned long offset);
static void free_all_pages(void);

/* /proc interface functions */
static int ramdisk_proc_show(struct seq_file *m, void *v);
static int ramdisk_proc_open(struct inode *inode, struct file *file);

/* Memory cache functions */
static int init_ramdisk_cache(void);
static void destroy_ramdisk_cache(void);

/* Module initialization and cleanup */
static int ramdisk_init_module(void);
static void ramdisk_exit_module(void);

/* File operations structure following template pattern */
static struct file_operations ramdisk_fops = {
    .owner = THIS_MODULE,
    .read = ramdisk_read,
    .write = ramdisk_write,
    .open = ramdisk_open,
    .release = ramdisk_release,
    .llseek = ramdisk_llseek,
    .unlocked_ioctl = ramdisk_ioctl,
    .mmap = ramdisk_mmap,  /* mmap support for bonus feature */
};

/* /proc file operations */
static const struct proc_ops ramdisk_proc_ops = {
    .proc_open    = ramdisk_proc_open,
    .proc_read    = seq_read,
    .proc_lseek   = seq_lseek,
    .proc_release = single_release,
};

/*
 * Open operation - manages concurrent user limits and write-only clearing
 * Based on template but adds user counting for ioctl process control
 * When opened write-only, clears all existing pages as per assignment requirements
 */
int ramdisk_open(struct inode *inode, struct file *filp)
{
    struct ramdisk_dev *dev;
    
    /* Get device structure from inode - standard pattern */
    dev = container_of(inode->i_cdev, struct ramdisk_dev, cdev);
    filp->private_data = dev;
    
    /* Acquire semaphore for thread-safe access */
    if (down_interruptible(&dev->sem))
        return -ERESTARTSYS;
    
    /* Check user limit if ioctl has set one */
    if (dev->max_users > 0 && dev->current_users >= dev->max_users) {
        up(&dev->sem);
        return -EBUSY;  /* Too many users */
    }
    
    /* Assignment requirement: When opened as write-only, clear all pages */
    if ((filp->f_flags & O_ACCMODE) == O_WRONLY) {
        struct ramdisk_page *page_entry, *tmp;
        
        printk(KERN_INFO "ramdisk: Write-only open - clearing all pages\n");
        
        /* Free all existing pages using safe traversal */
        list_for_each_entry_safe(page_entry, tmp, &dev->pages, list) {
            list_del(&page_entry->list);
            hash_del(&page_entry->hash);  /* Remove from hash table as well */
            if (page_entry->page) {
                __free_page(page_entry->page);
            }
            kmem_cache_free(ramdisk_cache, page_entry);
            
            /* Update device statistics */
            dev->total_pages--;
            dev->memory_usage -= RAMDISK_PAGE_SIZE;
        }
        
        /* Reset device size to zero */
        dev->size = 0;
        printk(KERN_INFO "ramdisk: All pages cleared for write-only access\n");
    }
    
    /* Increment user count */
    dev->current_users++;
    up(&dev->sem);
    
    printk(KERN_INFO "ramdisk: Device opened, current users: %d\n", dev->current_users);
    return 0;
}

/*
 * Release operation - decrements user count
 */
int ramdisk_release(struct inode *inode, struct file *filp)
{
    struct ramdisk_dev *dev = filp->private_data;
    
    /* Acquire semaphore for thread-safe access */
    if (down_interruptible(&dev->sem))
        return -ERESTARTSYS;
    
    /* Decrement user count */
    dev->current_users--;
    up(&dev->sem);
    
    printk(KERN_INFO "ramdisk: Device closed, current users: %d\n", dev->current_users);
    return 0;
}

/*
 * Find page containing given file offset using hash table for O(1) lookup
 * Enhanced implementation for performance - researched hash table patterns from kernel source
 * Returns NULL if page doesn't exist
 */
static struct ramdisk_page *find_page_at_offset(unsigned long offset)
{
    struct ramdisk_page *page_entry;
    unsigned long page_start = (offset / RAMDISK_PAGE_SIZE) * RAMDISK_PAGE_SIZE;
    u32 hash_key = hash_32(page_start >> PAGE_SHIFT, RAMDISK_HASH_BITS);
    
    /* Use kernel hash table lookup for O(1) performance */
    hash_for_each_possible(ramdisk_device->page_hash, page_entry, hash, hash_key) {
        if (page_entry->offset == page_start)
            return page_entry;
    }
    
    return NULL;  /* Page not found */
}

/*
 * Create new page at given offset using alloc_page() 
 * Implements page allocation patterns researched through Linux kernel API documentation
 * Enhanced with hash table insertion for O(1) lookup performance
 */
static struct ramdisk_page *create_page_at_offset(unsigned long offset)
{
    struct ramdisk_page *new_page_entry;
    struct page *kernel_page;
    unsigned long page_start = (offset / RAMDISK_PAGE_SIZE) * RAMDISK_PAGE_SIZE;
    u32 hash_key = hash_32(page_start >> PAGE_SHIFT, RAMDISK_HASH_BITS);
    
    /* Allocate page entry structure using optimized memory cache */
    new_page_entry = kmem_cache_alloc(ramdisk_cache, GFP_KERNEL);
    if (!new_page_entry) {
        printk(KERN_ERR "ramdisk: Failed to allocate page entry from cache\n");
        return NULL;
    }
    
    /* Allocate actual page using kernel page allocator - patterns researched in kernel docs */
    kernel_page = alloc_page(GFP_KERNEL);
    if (!kernel_page) {
        printk(KERN_ERR "ramdisk: Failed to allocate kernel page\n");
        kmem_cache_free(ramdisk_cache, new_page_entry);
        return NULL;
    }
    
    /* Initialize page entry following kernel patterns */
    new_page_entry->page = kernel_page;
    new_page_entry->offset = page_start;
    INIT_LIST_HEAD(&new_page_entry->list);
    INIT_HLIST_NODE(&new_page_entry->hash);
    
    /* Add to linked list for sequential access using kernel list_add pattern */
    list_add(&new_page_entry->list, &ramdisk_device->pages);
    
    /* Add to hash table for O(1) lookup performance */
    hash_add(ramdisk_device->page_hash, &new_page_entry->hash, hash_key);
    
    /* Update device statistics */
    ramdisk_device->total_pages++;
    ramdisk_device->memory_usage += RAMDISK_PAGE_SIZE;
    
    printk(KERN_INFO "ramdisk: Created new page at offset %lu (hash key: %u), total pages: %zu\n", 
           page_start, hash_key, ramdisk_device->total_pages);
    return new_page_entry;
}

/*
 * Read operation - handles reading across multiple pages
 * Based on template but extended for unlimited size with page management
 */
ssize_t ramdisk_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
    struct ramdisk_dev *dev = filp->private_data;
    struct ramdisk_page *page_entry;
    size_t bytes_read = 0;
    size_t remaining = count;
    loff_t pos = *f_pos;
    
    /* Acquire semaphore for thread-safe access */
    if (down_interruptible(&dev->sem))
        return -ERESTARTSYS;
    
    /* Check bounds against current file size */
    if (pos >= dev->size) {
        up(&dev->sem);
        return 0;  /* EOF */
    }
    
    /* Limit read to actual file size */
    if (pos + count > dev->size)
        remaining = dev->size - pos;
    
    /* Read data page by page */
    while (remaining > 0 && pos < dev->size) {
        unsigned long page_offset_in_file = (pos / RAMDISK_PAGE_SIZE) * RAMDISK_PAGE_SIZE;
        unsigned long offset_in_page = pos % RAMDISK_PAGE_SIZE;
        size_t bytes_to_read = min(remaining, RAMDISK_PAGE_SIZE - offset_in_page);
        void *page_data;
        
        /* Find the page containing this offset */
        page_entry = find_page_at_offset(page_offset_in_file);
        if (!page_entry) {
            /* Reading from unwritten area - return zeros */
            if (clear_user(buf + bytes_read, bytes_to_read)) {
                up(&dev->sem);
                return -EFAULT;
            }
        } else {
            /* Get virtual address of page data using kernel mapping */
            page_data = page_address(page_entry->page);
            if (!page_data) {
                printk(KERN_ERR "ramdisk: Failed to map page address\n");
                up(&dev->sem);
                return -ENOMEM;
            }
            
            /* Copy data to user space using copy_to_user patterns found in template analysis */
            if (copy_to_user(buf + bytes_read, page_data + offset_in_page, bytes_to_read)) {
                up(&dev->sem);
                return -EFAULT;
            }
        }
        
        /* Update counters */
        bytes_read += bytes_to_read;
        remaining -= bytes_to_read;
        pos += bytes_to_read;
    }
    
    /* Update file position */
    *f_pos = pos;
    
    /* Update operation statistics */
    ramdisk_device->ops_read++;
    
    up(&dev->sem);
    
    printk(KERN_DEBUG "ramdisk: Read %zd bytes from position %lld\n", bytes_read, pos - bytes_read);
    return bytes_read;
}

/*
 * Write operation - handles writing across multiple pages with dynamic allocation
 * Implements unlimited file size by allocating pages on demand
 */
ssize_t ramdisk_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
    struct ramdisk_dev *dev = filp->private_data;
    struct ramdisk_page *page_entry;
    size_t bytes_written = 0;
    size_t remaining = count;
    loff_t pos = *f_pos;
    
    /* Acquire semaphore for thread-safe access */
    if (down_interruptible(&dev->sem))
        return -ERESTARTSYS;
    
    /* Write data page by page, allocating as needed */
    while (remaining > 0) {
        unsigned long page_offset_in_file = (pos / RAMDISK_PAGE_SIZE) * RAMDISK_PAGE_SIZE;
        unsigned long offset_in_page = pos % RAMDISK_PAGE_SIZE;
        size_t bytes_to_write = min(remaining, RAMDISK_PAGE_SIZE - offset_in_page);
        void *page_data;
        
        /* Find existing page or create new one */
        page_entry = find_page_at_offset(page_offset_in_file);
        if (!page_entry) {
            page_entry = create_page_at_offset(page_offset_in_file);
            if (!page_entry) {
                printk(KERN_ERR "ramdisk: Failed to create page for writing\n");
                up(&dev->sem);
                return bytes_written > 0 ? bytes_written : -ENOMEM;
            }
        }
        
        /* Get virtual address of page data */
        page_data = page_address(page_entry->page);
        if (!page_data) {
            printk(KERN_ERR "ramdisk: Failed to map page address for writing\n");
            up(&dev->sem);
            return bytes_written > 0 ? bytes_written : -ENOMEM;
        }
        
        /* Copy data from user space using copy_from_user patterns found in template analysis */
        if (copy_from_user(page_data + offset_in_page, buf + bytes_written, bytes_to_write)) {
            up(&dev->sem);
            return bytes_written > 0 ? bytes_written : -EFAULT;
        }
        
        /* Update counters */
        bytes_written += bytes_to_write;
        remaining -= bytes_to_write;
        pos += bytes_to_write;
        
        /* Update file size if we've extended it */
        if (pos > dev->size)
            dev->size = pos;
    }
    
    /* Update file position */
    *f_pos = pos;
    
    /* Update operation statistics */
    ramdisk_device->ops_write++;
    
    up(&dev->sem);
    
    printk(KERN_DEBUG "ramdisk: Wrote %zd bytes to position %lld, new file size: %zu\n", 
           bytes_written, pos - bytes_written, dev->size);
    return bytes_written;
}

/*
 * Seek operation - supports seeking to any position (unlimited file size)
 */
loff_t ramdisk_llseek(struct file *filp, loff_t off, int whence)
{
    struct ramdisk_dev *dev = filp->private_data;
    loff_t newpos;
    
    /* Standard seek operation following kernel patterns */
    switch (whence) {
        case SEEK_SET:
            newpos = off;
            break;
        case SEEK_CUR:
            newpos = filp->f_pos + off;
            break;
        case SEEK_END:
            newpos = dev->size + off;
            break;
        default:
            return -EINVAL;
    }
    
    /* Allow seeking beyond current file size (like regular files) */
    if (newpos < 0)
        return -EINVAL;
    
    filp->f_pos = newpos;
    
    /* Update operation statistics */
    ramdisk_device->ops_seek++;
    
    printk(KERN_DEBUG "ramdisk: Seek to position %lld\n", newpos);
    return newpos;
}

/*
 * IOCTL operation - implements process control by limiting concurrent users
 */
long ramdisk_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    struct ramdisk_dev *dev = filp->private_data;
    int user_limit;
    
    /* Verify ioctl command magic and number */
    if (_IOC_TYPE(cmd) != RAMDISK_IOC_MAGIC)
        return -ENOTTY;
    if (_IOC_NR(cmd) > 1)
        return -ENOTTY;
    
    switch (cmd) {
        case RAMDISK_IOC_LIMIT_USERS:
            /* Get user limit from user space */
            if (get_user(user_limit, (int __user *)arg))
                return -EFAULT;
            
            /* Acquire semaphore for thread-safe modification */
            if (down_interruptible(&dev->sem))
                return -ERESTARTSYS;
            
            /* Set the user limit */
            dev->max_users = user_limit;
            printk(KERN_INFO "ramdisk: User limit set to %d\n", user_limit);
            
            up(&dev->sem);
            return 0;
            
        default:
            return -ENOTTY;
    }
}

/*
 * Free all allocated pages - called during module cleanup
 * Uses kernel linked list traversal and cleanup patterns
 * Enhanced to also clean up hash table entries
 */
static void free_all_pages(void)
{
    struct ramdisk_page *page_entry, *tmp;
    
    /* Use safe list traversal to avoid issues during deletion */
    list_for_each_entry_safe(page_entry, tmp, &ramdisk_device->pages, list) {
        /* Remove from linked list using kernel list_del pattern */
        list_del(&page_entry->list);
        
        /* Remove from hash table for proper cleanup */
        hash_del(&page_entry->hash);
        
        /* Free the kernel page using __free_page pattern */
        if (page_entry->page) {
            __free_page(page_entry->page);
        }
        
        /* Free the page entry structure using optimized cache */
        kmem_cache_free(ramdisk_cache, page_entry);
        
        /* Update device statistics */
        ramdisk_device->total_pages--;
        ramdisk_device->memory_usage -= RAMDISK_PAGE_SIZE;
        
        printk(KERN_INFO "ramdisk: Freed page at offset %lu\n", page_entry->offset);
    }
    
    printk(KERN_INFO "ramdisk: All pages freed and hash table cleaned\n");
}

/*
 * mmap operation - Memory mapping support (Bonus Feature 1)
 * Allows user-space programs to directly map ramdisk pages into their address space
 */
int ramdisk_mmap(struct file *filp, struct vm_area_struct *vma)
{
    struct ramdisk_dev *dev = filp->private_data;
    unsigned long start = vma->vm_start;
    unsigned long size = vma->vm_end - vma->vm_start;
    unsigned long offset = vma->vm_pgoff << PAGE_SHIFT;
    unsigned long page_offset, pfn;
    struct ramdisk_page *page_entry;
    
    printk(KERN_INFO "ramdisk: mmap() called - offset: %lu, size: %lu\n", offset, size);
    
    /* Check alignment and bounds */
    if (offset & ~PAGE_MASK)
        return -EINVAL;
    
    if (size > dev->size - offset)
        size = dev->size - offset;
    
    /* Acquire semaphore for thread-safe access */
    if (down_interruptible(&dev->sem))
        return -ERESTARTSYS;
    
    /* Map each page in the requested range */
    for (page_offset = 0; page_offset < size; page_offset += PAGE_SIZE) {
        page_entry = find_page_at_offset(offset + page_offset);
        if (!page_entry) {
            /* Create zero-filled page for unmapped regions */
            page_entry = create_page_at_offset(offset + page_offset);
            if (!page_entry) {
                up(&dev->sem);
                return -ENOMEM;
            }
            
            /* Clear the newly allocated page */
            clear_page(page_address(page_entry->page));
        }
        
        /* Get page frame number for mapping */
        pfn = page_to_pfn(page_entry->page);
        
        /* Map the page into user address space */
        if (remap_pfn_range(vma, start + page_offset, pfn, PAGE_SIZE, vma->vm_page_prot)) {
            up(&dev->sem);
            return -EAGAIN;
        }
    }
    
    up(&dev->sem);
    
    printk(KERN_INFO "ramdisk: mmap() completed successfully for %lu bytes\n", size);
    return 0;
}

/*
 * Memory cache initialization (Bonus Feature 2)
 * Creates optimized slab cache for ramdisk_page structures
 */
static int init_ramdisk_cache(void)
{
    ramdisk_cache = kmem_cache_create("ramdisk_cache",
                                      sizeof(struct ramdisk_page),
                                      0,
                                      SLAB_HWCACHE_ALIGN,
                                      NULL);
    if (!ramdisk_cache) {
        printk(KERN_ERR "ramdisk: Failed to create memory cache\n");
        return -ENOMEM;
    }
    
    printk(KERN_INFO "ramdisk: Memory cache initialized for optimized allocations\n");
    return 0;
}

/*
 * Memory cache cleanup
 * Destroys the slab cache for ramdisk_page structures
 */
static void destroy_ramdisk_cache(void)
{
    if (ramdisk_cache) {
        kmem_cache_destroy(ramdisk_cache);
        ramdisk_cache = NULL;
        printk(KERN_INFO "ramdisk: Memory cache destroyed\n");
    }
}

/*
 * /proc interface implementation (Bonus Feature 3)
 * Shows ramdisk statistics and configuration
 */
static int ramdisk_proc_show(struct seq_file *m, void *v)
{
    struct ramdisk_dev *dev = ramdisk_device;
    
    if (!dev) {
        seq_printf(m, "ramdisk: Device not initialized\n");
        return 0;
    }
    
    seq_printf(m, "=== Virtual Ramdisk Status ===\n");
    seq_printf(m, "Device node: /dev/asgn1\n");
    seq_printf(m, "Major number: %d\n", major);
    seq_printf(m, "File size: %zu bytes\n", dev->size);
    seq_printf(m, "Total pages allocated: %zu\n", dev->total_pages);
    seq_printf(m, "Memory usage: %zu bytes\n", dev->memory_usage);
    seq_printf(m, "Current users: %d\n", dev->current_users);
    seq_printf(m, "Max users limit: %d\n", dev->max_users);
    seq_printf(m, "Hash table buckets: %d\n", 1 << RAMDISK_HASH_BITS);
    seq_printf(m, "\n=== Operation Statistics ===\n");
    seq_printf(m, "Read operations: %lu\n", dev->ops_read);
    seq_printf(m, "Write operations: %lu\n", dev->ops_write);
    seq_printf(m, "Seek operations: %lu\n", dev->ops_seek);
    seq_printf(m, "\n=== Memory Cache Info ===\n");
    if (ramdisk_cache) {
        seq_printf(m, "Cache active: Yes\n");
        seq_printf(m, "Object size: %u bytes\n", kmem_cache_size(ramdisk_cache));
    } else {
        seq_printf(m, "Cache active: No\n");
    }
    
    return 0;
}

static int ramdisk_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, ramdisk_proc_show, NULL);
}

/*
 * Module initialization - sets up character device following template pattern
 */
static int ramdisk_init_module(void)
{
    int result;
    dev_t dev = 0;
    
    printk(KERN_INFO "ramdisk: Initializing Virtual Ramdisk module with enhanced features\n");
    
    /* Initialize memory cache for optimized allocations */
    result = init_ramdisk_cache();
    if (result) {
        printk(KERN_ERR "ramdisk: Failed to initialize memory cache\n");
        return result;
    }
    
    /* Allocate device structure */
    ramdisk_device = kmalloc(sizeof(struct ramdisk_dev), GFP_KERNEL);
    if (!ramdisk_device) {
        printk(KERN_ERR "ramdisk: Failed to allocate device structure\n");
        return -ENOMEM;
    }
    
    /* Initialize device structure using kernel patterns */
    INIT_LIST_HEAD(&ramdisk_device->pages);  /* Initialize linked list head */
    hash_init(ramdisk_device->page_hash);    /* Initialize hash table for O(1) lookup */
    ramdisk_device->size = 0;               /* Start with empty file */
    ramdisk_device->total_pages = 0;        /* No pages allocated initially */
    ramdisk_device->memory_usage = 0;       /* No memory used initially */
    ramdisk_device->max_users = 0;          /* No user limit by default */
    ramdisk_device->current_users = 0;      /* No current users */
    ramdisk_device->ops_read = 0;           /* Initialize operation counters */
    ramdisk_device->ops_write = 0;
    ramdisk_device->ops_seek = 0;
    sema_init(&ramdisk_device->sem, 1);     /* Initialize semaphore */
    
    /* Register character device following template pattern */
    if (major) {
        dev = MKDEV(major, 0);
        result = register_chrdev_region(dev, 1, "asgn1");
    } else {
        result = alloc_chrdev_region(&dev, 0, 1, "asgn1");
        major = MAJOR(dev);
    }
    
    if (result < 0) {
        printk(KERN_ERR "ramdisk: Failed to register device region\n");
        destroy_ramdisk_cache();
        kfree(ramdisk_device);
        return result;
    }
    
    /* Initialize and add cdev structure */
    cdev_init(&ramdisk_device->cdev, &ramdisk_fops);
    ramdisk_device->cdev.owner = THIS_MODULE;
    
    result = cdev_add(&ramdisk_device->cdev, dev, 1);
    if (result) {
        printk(KERN_ERR "ramdisk: Failed to add cdev\n");
        unregister_chrdev_region(dev, 1);
        destroy_ramdisk_cache();
        kfree(ramdisk_device);
        return result;
    }
    
    /* Create device class for udev integration - following template pattern */
    ramdisk_device->class = class_create("asgn1");
    if (IS_ERR(ramdisk_device->class)) {
        printk(KERN_ERR "ramdisk: Failed to create device class\n");
        cdev_del(&ramdisk_device->cdev);
        unregister_chrdev_region(dev, 1);
        destroy_ramdisk_cache();
        kfree(ramdisk_device);
        return PTR_ERR(ramdisk_device->class);
    }
    
    /* Create device node for automatic /dev entry creation */
    ramdisk_device->device = device_create(ramdisk_device->class, NULL, dev, NULL, "asgn1");
    if (IS_ERR(ramdisk_device->device)) {
        printk(KERN_ERR "ramdisk: Failed to create device\n");
        class_destroy(ramdisk_device->class);
        cdev_del(&ramdisk_device->cdev);
        unregister_chrdev_region(dev, 1);
        destroy_ramdisk_cache();
        kfree(ramdisk_device);
        return PTR_ERR(ramdisk_device->device);
    }
    
    printk(KERN_INFO "ramdisk: Enhanced Virtual Ramdisk initialized successfully\n");
    printk(KERN_INFO "ramdisk: Major number: %d, Device node: /dev/asgn1\n", major);
    
    /* Create /proc entry for device information */
    proc_entry = proc_create("ramdisk_info", 0444, NULL, &ramdisk_proc_ops);
    if (!proc_entry) {
        printk(KERN_WARNING "ramdisk: Failed to create /proc/ramdisk_info entry (non-fatal)\n");
    } else {
        printk(KERN_INFO "ramdisk: /proc/ramdisk_info entry created successfully\n");
    }
    
    printk(KERN_INFO "ramdisk: Enhanced features: mmap support, memory cache optimization, /proc interface\n");
    
    return 0;
}

/*
 * Module cleanup - proper cleanup following template pattern with page deallocation
 */
static void ramdisk_exit_module(void)
{
    dev_t dev = MKDEV(major, 0);
    
    printk(KERN_INFO "ramdisk: Cleaning up Enhanced Virtual Ramdisk module\n");
    
    /* Remove /proc entry */
    if (proc_entry) {
        remove_proc_entry("ramdisk_info", NULL);
        proc_entry = NULL;
        printk(KERN_INFO "ramdisk: /proc/ramdisk_info entry removed\n");
    }
    
    if (ramdisk_device) {
        /* Free all allocated pages first */
        free_all_pages();
        
        /* Remove device node and class - reverse of initialization order */
        if (ramdisk_device->device)
            device_destroy(ramdisk_device->class, dev);
        
        if (ramdisk_device->class)
            class_destroy(ramdisk_device->class);
        
        /* Remove character device */
        cdev_del(&ramdisk_device->cdev);
        
        /* Free device structure */
        kfree(ramdisk_device);
    }
    
    /* Unregister device region */
    unregister_chrdev_region(dev, 1);
    
    /* Destroy memory cache */
    destroy_ramdisk_cache();
    
    printk(KERN_INFO "ramdisk: Enhanced Virtual Ramdisk module removed successfully\n");
}

/* Register module initialization and cleanup functions */
module_init(ramdisk_init_module);
module_exit(ramdisk_exit_module);
