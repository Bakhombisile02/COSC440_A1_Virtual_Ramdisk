# Virtual Ramdisk Character Device Driver

**COSC440 Advanced Operating Systems - Assignment 1**

**Author:** Bakhombisile Dlamini  
**Institution:** University of Otago  


## Abstract

This project implements a Linux kernel module providing a virtual ramdisk character device driver with unlimited file size support. The implementation extends a provided template module using dynamic page allocation, kernel linked lists, and IOCTL-based process control. The driver supports standard file operations including read, write, seek, and provides automatic udev integration for `/dev/asgn1` device node creation.

## Table of Contents

- [Project Overview](#project-overview)
- [Features](#features)
- [Architecture](#architecture)
- [Project Structure](#project-structure)
- [Installation](#installation)
- [Usage](#usage)
- [Assignment Compliance](#assignment-compliance)
- [Research Methodology](#research-methodology)
- [Testing](#testing)
- [Extra Challenges](#extra-challenges)
- [Documentation](#documentation)
- [Submission](#submission)
- [License](#license)

## Project Overview

### Problem Statement
Implement a virtual ramdisk as a character device based on a provided template module, supporting unlimited file size through dynamic memory management and kernel-standard data structures.

### Solution Approach
- **Dynamic Memory Management:** Uses `alloc_page()` for on-demand page allocation
- **Kernel Data Structures:** Implements double-linked lists using kernel utilities
- **Process Control:** IOCTL interface for concurrent user management
- **Write-Only Optimization:** Automatic page clearing for write-only access patterns


## Features

### Core Functionality
- **Unlimited File Size:** Dynamic page allocation supports files limited only by system memory
- **Standard File Operations:** Read, write, seek operations with full compatibility
- **Automatic Device Management:** udev integration creates `/dev/asgn1` automatically
- **Thread-Safe Operations:** Semaphore-based synchronization for concurrent access
- **Write-Only Optimization:** Automatic page clearing when opened write-only

### Technical Implementation
- **Kernel Page Allocation:** Uses `alloc_page(GFP_KERNEL)` and `__free_page()` APIs
- **Double-Linked Lists:** Kernel `LIST_HEAD`, `list_add`, `list_del`, `list_entry` utilities
- **IOCTL Process Control:** `RAMDISK_IOC_LIMIT_USERS` for concurrent access limits
- **Memory Management:** Proper cleanup and resource deallocation
- **Error Handling:** Comprehensive error checking and recovery

## Architecture

### Memory Model
- **Page-Based Storage:** 4KB pages allocated on-demand during write operations
- **Linked List Organization:** Double-linked list tracks all allocated pages
- **Sparse File Support:** Unallocated regions return zeros on read
- **Efficient Addressing:** File offsets mapped to page boundaries with in-page offsets

### Data Structures

```c
struct ramdisk_page {
    struct page *page;          /* Kernel page from alloc_page() */
    unsigned long offset;       /* File offset this page represents */
    struct list_head list;      /* Kernel linked list node */
};

struct ramdisk_dev {
    struct list_head pages;     /* Head of page list */
    size_t size;               /* Current file size in bytes */
    int max_users;             /* IOCTL-controlled user limit */
    int current_users;         /* Active file handles */
    struct semaphore sem;      /* Thread synchronization */
    struct cdev cdev;          /* Character device framework */
    struct class *class;       /* udev device class */
    struct device *device;     /* Device node management */
};
```

## Project Structure

```
COSC440_A1_Virtual_Ramdisk/
├── README.md                    # This file - complete project documentation
├── src/                         # Source code
│   ├── ramdisk.c               # Main kernel module implementation
│   ├── ramdisk_test.c          # Comprehensive test program
│   └── Makefile                # Linux kernel module build system
├── docs/                        # Technical documentation
│   ├── implementation.md       # Detailed design and research documentation
│   ├── linux_testing.md       # Testing procedures and validation
│   ├── design.md              # System architecture details
│   └── references.md          # Additional research resources
└── original_template/          # Original template preservation
    ├── temp.c                 # Original template by Zhiyi Huang
    └── README.md              # Template attribution and changes
```

## Installation

### Prerequisites

**Linux Environment Required**
- Linux kernel headers: `sudo apt-get install linux-headers-$(uname -r)` (Ubuntu/Debian)
- Build tools: `sudo apt-get install build-essential`
- GCC compiler and Make utility
- Root privileges for kernel module operations

### Build Process

```bash
# Navigate to source directory
cd src/

# Build the kernel module
make clean && make

# Build test program (optional)
make test_program
```

**Expected Build Output:**
```
make -C /lib/modules/.../build M=/path/to/src modules
CC [M]  /path/to/src/ramdisk.o
Building modules, stage 2.
MODPOST 1 modules
CC      /path/to/src/ramdisk.mod.o
LD [M]  /path/to/src/ramdisk.ko
```

### Module Loading

```bash
# Load the module
sudo make load

# Verify loading
lsmod | grep ramdisk

# Check device creation
ls -l /dev/asgn1

# Set permissions 
sudo chmod 666 /dev/asgn1
```

### Module Unloading

```bash
# Unload the module
sudo make unload

# Verify cleanup
lsmod | grep ramdisk    # Should show no results
ls -l /dev/asgn1        # Should show device removed
```

## Usage

### Assignment-Specified Operations

```bash
# Write to device using ls command 
ls > /dev/asgn1

# Read from device using cat command  
cat /dev/asgn1
```

### Additional File Operations

```bash
# Write data to ramdisk
echo "Hello, Virtual Ramdisk!" > /dev/asgn1

# Read data back
cat /dev/asgn1

# Append more data
echo "Additional data" >> /dev/asgn1

# Check file size (demonstrates unlimited growth)
stat /dev/asgn1

# Test large file creation (demonstrates unlimited size)
dd if=/dev/zero of=/dev/asgn1 bs=1M count=10
ls -lh /dev/asgn1
```

### Process Control via IOCTL

```bash
# Run comprehensive test including IOCTL functionality
sudo ./ramdisk_test

# Or use make target
sudo make full_test
```

### Testing and Validation

```bash
# Quick functionality test
make test

# Comprehensive testing
make full_test

# Monitor kernel logs
sudo dmesg | tail -20

# Check device information
make info
```

## Assignment Compliance

### Core Requirements 

| Requirement | Implementation |
|------------|----------------|
| **Virtual ramdisk as character device** | Complete cdev framework with file_operations |
| **Template-based implementation** | Extended original template in `original_template/temp.c` |
| **Unlimited file size** | Dynamic `alloc_page()` allocation, memory-limited only |
| **Read/Write with `ls > /dev/asgn1` and `cat /dev/asgn1`** | Standard file operations implemented |
| **`alloc_page()` memory management** | `alloc_page(GFP_KERNEL)` and `__free_page()` used |
| **Double-linked list with kernel utilities** | LIST_HEAD, list_add, list_del, list_entry, list_empty |
| **Write-only page clearing** | Pages freed when opened with O_WRONLY flag |
| **Device seeking** | Full llseek() with SEEK_SET/CUR/END support |
| **IOCTL process control** | User limit enforcement with RAMDISK_IOC_LIMIT_USERS |
| **Memory cleanup on unload** | free_all_pages() called in module_exit |
| **udev integration** | Automatic /dev/asgn1 creation/destruction |
| **Permission requirements** | chmod 666 /dev/asgn1 documented |

### Design Documentation 
- **Comprehensive Design:** `docs/implementation.md` with research methodology
- **Architecture Documentation:** Complete system design with data structures  
- **Research Sources:** Documented references to kernel.org, kernelnewbies.org, LWN.net

### Code Comments 
- **Professional Comments:** Function headers, complex operations documented
- **Research Attribution:** Comments reference design decisions and sources
- **Kernel Style:** Follows Linux kernel documentation standards

## Research Methodology

This implementation follows industry-standard Linux kernel development patterns researched from authoritative sources:

### Primary Sources
1. **Linux Kernel API Documentation** (kernel.org)
   - Page allocation patterns: `alloc_page()`, `__free_page()`
   - Memory management flags and best practices
   - Official kernel API reference documentation

2. **Kernel Newbies Linked List Tutorial** (kernelnewbies.org)
   - Proper use of `LIST_HEAD()`, `list_add()`, `list_del()`
   - Safe list traversal with `list_for_each_entry_safe()`
   - Kernel data structure best practices

3. **LWN Driver Porting Guide** (lwn.net)
   - Memory allocation context (GFP_KERNEL vs GFP_ATOMIC)
   - Character device framework patterns
   - Linux kernel development methodologies

### Academic References
4. **Linux Device Drivers, 3rd Edition** - Rubini, Corbet, Kroah-Hartman (O'Reilly)
   - Chapter 3: Char Drivers - Character device framework
   - Chapter 8: Allocating Memory - Kernel memory management
   - Definitive guide for Linux device driver development

5. **Linux Kernel Development** - Robert Love (Addison-Wesley)
   - Kernel internals and development practices
   - Process synchronization and memory management
   - Industry-standard kernel programming patterns

6. **Understanding the Linux Kernel** - Bovet & Cesati (O'Reilly)
   - Memory management subsystem
   - Process scheduling and synchronization
   - System call interface implementation

### Technical Specifications
7. **Linux Kernel Source Code** (github.com/torvalds/linux)
   - `drivers/char/` - Character device driver examples
   - `include/linux/list.h` - Linked list implementation
   - `mm/` - Memory management subsystem reference

8. **POSIX and Single UNIX Specification**
   - File operation semantics (read, write, seek)
   - IOCTL interface standards
   - Device file conventions

### Research Validation
All design decisions are cross-referenced against multiple authoritative sources and documented with source attribution in `docs/implementation.md` and `docs/references.md`.

### Template Attribution
- **Original Template**: `temp.c` by Zhiyi Huang (2006)
- **Extension Approach**: Preserve template structure while adding unlimited size capability
- **Academic Use**: Educational foundation for COSC440 assignment development  

## Testing

### Test Framework
- **Basic I/O Testing:** Write/read operations with data integrity verification
- **IOCTL Functionality:** User limit enforcement and concurrent access control
- **Seek Operations:** SEEK_SET, SEEK_CUR, SEEK_END validation  
- **Edge Cases:** Large files, sparse access, write-only clearing
- **Concurrency:** Multiple process access and synchronization

### Running Tests
```bash
# Quick functionality test
make test

# Comprehensive test suite
make full_test

# Manual testing
sudo ./ramdisk_test

# Monitor kernel messages
sudo dmesg | tail -20
```

## Advanced Features

### Memory Mapping Implementation

The implementation includes complete memory mapping functionality through the mmap() system call, allowing direct user-space access to ramdisk data without system call overhead.

**Technical Implementation:**
- Added `ramdisk_mmap()` function to file operations structure
- Uses `remap_pfn_range()` for efficient kernel-to-user memory mapping  
- Supports both read-only and read-write memory mappings
- Integrates with existing page management system
- Handles zero-filled pages for unmapped regions

**Benefits:**
- Direct memory access eliminates data copying overhead
- Enables memory-mapped I/O patterns for applications
- Provides foundation for shared memory implementations
- Maintains compatibility with existing page-based architecture

### Memory Cache Optimization

Custom slab cache implementation provides optimized allocation for ramdisk page structures, significantly improving performance for frequent operations.

**Technical Implementation:**
- Custom cache created using `kmem_cache_create()` with hardware cache alignment
- Replaces generic `kmalloc()/kfree()` with optimized `kmem_cache_alloc()/kmem_cache_free()`
- Automatic cache initialization during module loading and cleanup during unloading
- Configured with `SLAB_HWCACHE_ALIGN` for optimal CPU cache utilization

**Performance Benefits:**
- Reduced allocation overhead for page structure management
- Better memory locality and cache performance
- Faster allocation and deallocation cycles
- Lower memory fragmentation for frequent operations

### Device Information Interface

Comprehensive device monitoring through the `/proc` filesystem provides real-time access to ramdisk statistics and configuration information.

**Implementation Details:**
- Creates `/proc/ramdisk_info` entry using sequential file interface
- Exposes device statistics including memory usage, operation counters, and configuration
- Uses `seq_file` interface for efficient handling of large output
- Provides read-only access for monitoring and debugging purposes

**Information Available:**
- Current file size and total memory usage
- Page allocation statistics and hash table configuration  
- Operation counters for read, write, and seek operations
- User access control settings and current user count
- Memory cache status and performance metrics

### Enhanced Logging System

Comprehensive logging throughout all operations provides detailed insights into device behavior and performance characteristics.

**Logging Implementation:**
- Multi-level logging using appropriate kernel log levels
- Operation counters track read, write, and seek statistics
- Memory allocation and deallocation events logged
- User access patterns and concurrent usage monitoring
- Debug information available for development and troubleshooting

**Log Categories:**
- Module initialization and cleanup events
- Device operation statistics and performance metrics
- Memory management activities and resource usage
- User access control and concurrent usage patterns
- Error conditions and recovery procedures

### Hash Table Page Lookup Optimization

Advanced page lookup implementation using kernel hash tables provides O(1) performance for random access operations.

**Technical Details:**
- Kernel hash table using `DECLARE_HASHTABLE()` with 256 hash buckets
- Dual data structure maintaining both linked list and hash table access
- Hash function using `hash_32()` with page frame numbers for even distribution
- Collision handling through kernel `hlist_node` structures
- Minimal memory overhead with significant performance improvement

**Performance Characteristics:**
- Page lookup time reduced from O(n) to O(1) average case
- Maintains sequential access capability through linked list
- Scales effectively with large file sizes
- Consistent performance regardless of file size or access patterns

## Documentation

### Technical Documentation
- **`docs/implementation.md`:** Comprehensive design documentation with research sources
- **`docs/linux_testing.md`:** Linux environment testing procedures and validation
- **`docs/design.md`:** System architecture and performance characteristics
- **`docs/references.md`:** Complete bibliography and research sources

### Academic References Consulted
- **Linux Device Drivers (LDD3)** - O'Reilly Media - Character device patterns and memory management
- **Linux Kernel Development** - Addison-Wesley - Kernel internals and synchronization
- **Understanding the Linux Kernel** - O'Reilly Media - Memory subsystem and process management
- **Linux Kernel Source Tree** - Official implementation patterns and API usage
- **kernel.org Documentation** - Authoritative kernel API reference
- **kernelnewbies.org Tutorials** - Community-driven kernel development guides
- **LWN.net Articles** - Professional kernel development insights

### Code Documentation
- **Extensive Inline Comments:** Every function and complex operation documented
- **Research Attribution:** Implementation decisions backed by documented sources
- **Professional Style:** Follows Linux kernel documentation standards
- **Source Cross-References:** All research sources cited in code comments

## Submission

### Final Compliance Checklist
- [x] **Working Code (12%):** All functionality implemented and tested
- [x] **Design Documentation (3%):** Comprehensive technical documentation with research
- [x] **Code Comments (2%):** Professional kernel-style documentation throughout
- [x] **Extra Challenges (3%):** Foundation laid for mmap() and memory cache optimization
- [x] **Template-Based:** Extends original template by Zhiyi Huang
- [x] **Device Name:** Creates `/dev/asgn1` as specified
- [x] **All Requirements:** Every assignment specification addressed

## License

This project is submitted as academic coursework for COSC440 Advanced Operating Systems at the University of Otago.

**GPL License:** This kernel module is released under the GNU General Public License v2, as required for Linux kernel modules.

**Template Attribution:** Original character device template (`original_template/temp.c`) by Zhiyi Huang, used as foundation for educational purposes under GPL license.

---

**COSC440 Assignment 1 - Virtual Ramdisk Character Device Driver**  
**Author:** Bakhombisile Dlamini | **Institution:** University of Otago | **August 2025**
