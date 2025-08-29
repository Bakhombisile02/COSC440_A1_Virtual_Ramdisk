# Virtual Ramdisk Character Device Driver

**COSC440 Advanced Operating Systems - Assignment 1**

**Author:** Bakhombisile Dlamini  
**Institution:** University of Otago  


## Abstract

This project implements a Linux kernel module providing a virtual ramdisk character device driver with unlimited file size support. The implementation extends a provided template module using dynamic page allocation, kernel linked lists, and IOCTL-based process control. The driver supports standard file operations including read, write, seek, and provides automatic udev integration for `/dev/asgn1` device node creation.

## Table of Contents

- [Quick Start Guide](#quick-start-guide)
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

## Quick Start Guide

**For a brand new Linux system, follow these steps in order:**

1. **Install Prerequisites:**
   ```bash
   sudo apt update
   sudo apt install -y build-essential linux-headers-$(uname -r) kmod
   ```

2. **Build and Load Module:**
   ```bash
   cd src/
   make clean && make module && make test_program
   sudo insmod ramdisk.ko
   sudo mknod /dev/asgn1 c 239 0
   sudo chmod 666 /dev/asgn1
   ```

3. **Verify Installation:**
   ```bash
   lsmod | grep ramdisk
   ls -l /dev/asgn1
   cat /proc/ramdisk_info
   ```

4. **Run Assignment Tests:**
   ```bash
   ls > /dev/asgn1
   cat /dev/asgn1
   sudo make full_test
   ```

5. **Clean Up:**
   ```bash
   sudo rmmod ramdisk
   rm /dev/asgn1 2>/dev/null
   ```

**If any step fails, see the detailed Installation and Troubleshooting sections below.**

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

### Prerequisites and System Setup

**Fresh Linux Environment Setup (Ubuntu/Debian)**

For a brand new Linux container or machine, install all required dependencies:

```bash
# Update package manager
sudo apt update

# Install essential build tools
sudo apt install -y build-essential

# Install kernel headers for current running kernel
sudo apt install -y linux-headers-$(uname -r)

# Install kernel module utilities (insmod, rmmod, lsmod)
sudo apt install -y kmod

# Verify kernel version and headers match
uname -r
ls /lib/modules/$(uname -r)/build/
```

**Container and Virtual Environment Notes:**

- **Docker Containers**: Kernel modules cannot be loaded inside standard Docker containers due to security restrictions. Use privileged containers or run on host system.
- **WSL (Windows Subsystem for Linux)**: WSL2 supports kernel modules, but may require enabling kernel module loading in the WSL kernel configuration.
- **GitHub Codespaces**: Works correctly as demonstrated in testing. Kernel headers are installable and modules can be loaded.
- **Cloud VMs**: Should work on most cloud providers (AWS, Azure, GCP) with proper permissions.
- **Shared Hosting**: May not work due to restricted kernel access. Use dedicated VMs or containers with appropriate privileges.

**Virtual Environment Troubleshooting:**

```bash
# Check if running in container
if [ -f /.dockerenv ]; then
    echo "Running in Docker - may need privileged mode"
fi

# Check kernel capabilities
if ! [ -w /proc/sys ]; then
    echo "Limited kernel access - may need elevated privileges"
fi

# Check if /dev can be written to
if ! sudo touch /dev/test_write 2>/dev/null; then
    echo "Cannot create device nodes - check container privileges"
else
    sudo rm /dev/test_write
    echo "Device node creation available"
fi
```

**Alternative Linux Distributions:**
```bash
# CentOS/RHEL/Fedora
sudo yum install kernel-devel kernel-headers gcc make
# or for newer versions:
sudo dnf install kernel-devel kernel-headers gcc make

# Arch Linux
sudo pacman -S linux-headers base-devel
```

**System Requirements:**
- Linux kernel version 3.0 or higher
- GCC compiler version 4.8 or higher
- Make utility
- Root/sudo privileges for kernel module operations
- At least 16MB RAM for testing (module uses dynamic allocation)

### Build Process

```bash
# Navigate to project root
cd /path/to/COSC440_A1_Virtual_Ramdisk

# Navigate to source directory
cd src/

# Clean any previous builds
make clean

# Build the kernel module
make module

# Build comprehensive test program
make test_program
```

**Expected Build Output:**
```
make -C /lib/modules/6.8.0-1030-azure/build M=/workspaces/COSC440_A1_Virtual_Ramdisk/src modules
warning: the compiler differs from the one used to build the kernel
  CC [M]  /workspaces/COSC440_A1_Virtual_Ramdisk/src/ramdisk.o
  MODPOST /workspaces/COSC440_A1_Virtual_Ramdisk/src/Module.symvers
  CC [M]  /workspaces/COSC440_A1_Virtual_Ramdisk/src/ramdisk.mod.o
  LD [M]  /workspaces/COSC440_A1_Virtual_Ramdisk/src/ramdisk.ko
```

**Common Build Issues and Solutions:**

1. **Missing kernel headers:**
   ```bash
   # Error: No such file or directory: /lib/modules/.../build
   # Solution: Install correct kernel headers
   sudo apt install linux-headers-$(uname -r)
   ```

2. **class_create compatibility (newer kernels 6.4+):**
   ```bash
   # Error: too many arguments to function 'class_create'
   # Error: passing argument 1 of 'class_create' from incompatible pointer type
   # This issue has been fixed in the provided code
   # The fix changes: class_create(THIS_MODULE, "asgn1") 
   # To: class_create("asgn1")
   # If you encounter this error, update line 742 in ramdisk.c
   ```

3. **Module verification warnings (safe to ignore):**
   ```bash
   # Warning: loading out-of-tree module taints kernel
   # Warning: module verification failed: signature and/or required key missing
   # These warnings are normal for development modules and can be ignored
   ```

4. **Compiler version warnings (safe to ignore):**
   ```bash
   # Warning: the compiler differs from the one used to build the kernel
   # This is normal in container environments and will not affect functionality
   ```

### Module Loading and Device Setup

```bash
# Method 1: Using make targets (recommended)
sudo make load

# Method 2: Manual loading
sudo insmod ramdisk.ko

# Verify module loaded successfully
lsmod | grep ramdisk

# Create device node if not automatically created
sudo mknod /dev/asgn1 c 239 0

# Set proper permissions for device access
sudo chmod 666 /dev/asgn1

# Verify device node creation
ls -l /dev/asgn1

# Check kernel messages for successful loading
sudo dmesg | tail -10
```

**Expected Success Messages:**
```
ramdisk: Initializing Virtual Ramdisk module with enhanced features
ramdisk: Memory cache initialized for optimized allocations
ramdisk: Enhanced Virtual Ramdisk initialized successfully
ramdisk: Major number: 239, Device node: /dev/asgn1
ramdisk: /proc/ramdisk_info entry created successfully
```

### Troubleshooting Module Loading

**Common Issues and Solutions:**

1. **Module loading fails - insmod not found:**
   ```bash
   # Install kernel module utilities
   sudo apt install -y kmod
   # Verify installation
   which insmod
   ```

2. **Permission denied errors:**
   ```bash
   # Ensure running with sudo privileges
   sudo insmod ramdisk.ko
   # Check if SELinux/AppArmor is interfering (if applicable)
   ```

3. **Device node not created automatically:**
   ```bash
   # Create manually with correct major number (239)
   sudo mknod /dev/asgn1 c 239 0
   sudo chmod 666 /dev/asgn1
   ```

4. **Module verification warnings (safe to ignore):**
   ```bash
   # These warnings are normal for development modules:
   # "loading out-of-tree module taints kernel"
   # "module verification failed: signature and/or required key missing"
   ```

5. **Device busy or resource conflicts:**
   ```bash
   # Check if device already exists
   ls -l /dev/asgn1
   # Remove conflicting device node
   sudo rm /dev/asgn1
   # Reload module
   sudo rmmod ramdisk && sudo insmod ramdisk.ko
   ```

### Module Unloading

```bash
# Method 1: Using make targets (recommended)
sudo make unload

# Method 2: Manual unloading
sudo rmmod ramdisk

# Clean up device node (if manually created)
sudo rm /dev/asgn1

# Verify complete cleanup
lsmod | grep ramdisk    # Should show no results
ls -l /dev/asgn1        # Should show "No such file or directory"
cat /proc/ramdisk_info  # Should show "No such file or directory"
```

### Complete Installation Verification

**System Readiness Checklist:**

```bash
# Check kernel version compatibility
uname -r

# Verify kernel headers are installed
ls /lib/modules/$(uname -r)/build/ >/dev/null && echo "Kernel headers: OK" || echo "Kernel headers: MISSING"

# Verify build tools
which gcc make >/dev/null && echo "Build tools: OK" || echo "Build tools: MISSING"

# Verify kernel module utilities
which insmod rmmod lsmod >/dev/null && echo "Module utilities: OK" || echo "Module utilities: MISSING"

# Check sudo privileges
sudo -v && echo "Sudo access: OK" || echo "Sudo access: REQUIRED"
```

**If any checks fail, install missing components:**

```bash
# For Ubuntu/Debian systems
sudo apt update
sudo apt install -y build-essential linux-headers-$(uname -r) kmod

# For CentOS/RHEL/Fedora systems
sudo yum install kernel-devel kernel-headers gcc make
# or for newer versions:
sudo dnf install kernel-devel kernel-headers gcc make

# For Arch Linux systems
sudo pacman -S linux-headers base-devel
```

### Common Issues and Complete Solutions

1. **"No such file or directory" during build:**
   ```bash
   # Problem: Missing kernel headers
   # Solution:
   sudo apt install linux-headers-$(uname -r)
   # Verify installation:
   ls /lib/modules/$(uname -r)/build/
   ```

2. **"insmod: command not found":**
   ```bash
   # Problem: Missing kernel module utilities
   # Solution:
   sudo apt install kmod
   # Verify installation:
   which insmod
   ```

3. **"Permission denied" when accessing /dev/asgn1:**
   ```bash
   # Problem: Incorrect device permissions
   # Solution:
   sudo chmod 666 /dev/asgn1
   # Verify:
   ls -l /dev/asgn1
   # Should show: crw-rw-rw- 1 root root 239, 0
   ```

4. **Device not created automatically:**
   ```bash
   # Problem: udev may not create device node
   # Solution: Create manually
   sudo mknod /dev/asgn1 c 239 0
   sudo chmod 666 /dev/asgn1
   ```

5. **Module fails to load with "Invalid module format":**
   ```bash
   # Problem: Kernel version mismatch
   # Solution: Ensure headers match running kernel
   uname -r
   dpkg -l | grep linux-headers
   # Reinstall correct headers if needed:
   sudo apt install --reinstall linux-headers-$(uname -r)
   # Rebuild module:
   make clean && make module
   ```

6. **"Device or resource busy" when unloading:**
   ```bash
   # Problem: Module in use
   # Solution: Close all handles and wait
   sudo lsof /dev/asgn1  # Check what's using device
   # Kill processes if necessary, then:
   sudo rmmod ramdisk
   ```

7. **Build warnings about compiler differences:**
   ```bash
   # This warning is normal and can be ignored:
   # "warning: the compiler differs from the one used to build the kernel"
   # The module will still function correctly
   ```

8. **Tests fail with "No such device":**
   ```bash
   # Complete diagnostic and fix:
   lsmod | grep ramdisk || echo "Module not loaded"
   ls -l /dev/asgn1 || echo "Device node missing"
   
   # Fix sequence:
   sudo rmmod ramdisk 2>/dev/null
   sudo insmod ramdisk.ko
   sudo mknod /dev/asgn1 c 239 0 2>/dev/null
   sudo chmod 666 /dev/asgn1
   
   # Test:
   echo "test" > /dev/asgn1 && cat /dev/asgn1
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

### Complete Testing Guide for New Systems

**Step-by-Step Testing Process:**

1. **Initial Setup Verification:**
   ```bash
   # Ensure you're in the correct directory
   cd /path/to/COSC440_A1_Virtual_Ramdisk/src
   
   # Verify all build files are present
   ls -la ramdisk.ko ramdisk_test
   
   # Check module is loaded
   lsmod | grep ramdisk
   
   # Verify device node exists with correct permissions
   ls -l /dev/asgn1
   ```

2. **Basic Assignment Requirements Testing:**
   ```bash
   # Test assignment-specified commands
   ls > /dev/asgn1
   cat /dev/asgn1
   
   # Expected: Should display directory listing
   # If this fails, check device permissions: sudo chmod 666 /dev/asgn1
   ```

3. **Core Functionality Testing:**
   ```bash
   # Test basic write/read
   echo "Hello Virtual Ramdisk!" > /dev/asgn1
   cat /dev/asgn1
   
   # Test write-only clearing (assignment requirement)
   echo "New content after clearing" > /dev/asgn1
   cat /dev/asgn1
   
   # Test unlimited file size
   dd if=/dev/zero of=/dev/asgn1 bs=1K count=100
   cat /proc/ramdisk_info | head -10
   ```

4. **Advanced Feature Testing:**
   ```bash
   # Run comprehensive test program
   sudo ./ramdisk_test
   
   # Or use make target
   sudo make full_test
   ```

5. **Monitoring and Verification:**
   ```bash
   # Check /proc interface
   cat /proc/ramdisk_info
   
   # Monitor kernel logs
   sudo dmesg | tail -30
   
   # Verify module information
   make info
   ```

### Test Framework Components
- **Basic I/O Testing:** Write/read operations with data integrity verification
- **IOCTL Functionality:** User limit enforcement and concurrent access control
- **Seek Operations:** SEEK_SET, SEEK_CUR, SEEK_END validation  
- **Edge Cases:** Large files, sparse access, write-only clearing
- **Concurrency:** Multiple process access and synchronization
- **Memory Management:** Page allocation/deallocation testing
- **Enhanced Features:** mmap, memory cache, /proc interface testing

### Test Commands Reference

```bash
# Quick functionality test (basic requirements only)
make test

# Comprehensive test suite (all features including bonuses)
make full_test

# Manual comprehensive testing
sudo ./ramdisk_test

# Debug information
make debug

# Module information
make info

# Monitor kernel messages during testing
sudo dmesg | tail -20
```

### Expected Test Results

**Successful Basic Test Output:**
```
Testing Enhanced Virtual Ramdisk functionality...
1. Writing test data to /dev/asgn1
2. Reading back from /dev/asgn1
Hello Enhanced Virtual Ramdisk!
3. Checking /proc interface
=== Virtual Ramdisk Status ===
Device node: /dev/asgn1
Major number: 239
File size: 29 bytes
4. Testing seek functionality
Enhanced test complete - ramdisk is working!
```

**Comprehensive Test Success Indicators:**
- All I/O operations complete successfully
- IOCTL user limits enforced (blocks excess users with EBUSY)
- Seek operations work with all three modes
- Large file operations (8KB+) complete without errors
- mmap functionality maps and unmaps successfully
- /proc interface shows accurate device statistics
- Concurrent access properly controlled
- Memory cleanup verified on module unload

### Troubleshooting Test Failures

1. **Device not accessible:**
   ```bash
   # Check if device node exists
   ls -l /dev/asgn1 || sudo mknod /dev/asgn1 c 239 0
   # Fix permissions
   sudo chmod 666 /dev/asgn1
   ```

2. **Module not loaded:**
   ```bash
   # Reload module
   sudo rmmod ramdisk 2>/dev/null; sudo insmod ramdisk.ko
   ```

3. **Permission denied on tests:**
   ```bash
   # Run test with sudo
   sudo make full_test
   sudo ./ramdisk_test
   ```

4. **/proc interface not available:**
   ```bash
   # Check kernel messages for errors
   sudo dmesg | grep ramdisk
   # Reload module if necessary
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
