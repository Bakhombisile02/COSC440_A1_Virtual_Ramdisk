# Original Template Module

## Attribution
- **Author**: Zhiyi Huang
- **Date**: 13/03/2006
- **File**: temp.c
- **Version**: 0.1

## Description
This is the original template character device driver module provided as the foundation for COSC440 Assignment 1. The template demonstrates basic Linux kernel module structure and character device operations with a fixed-size buffer of 3071 bytes.

## Template Features
- Basic character device registration
- Fixed buffer of 3071 bytes
- Simple read/write operations
- Device class and udev integration
- Semaphore-based mutual exclusion

## Modifications Made for Assignment
The template was extended to create a virtual ramdisk with the following enhancements:
1. Unlimited file size using dynamic page allocation
2. Kernel linked list for memory management
3. IOCTL support for process control
4. Cross-page I/O operations
5. Proper cleanup and error handling

## Academic Use
This template is provided under GPL license and was used as the foundation for educational purposes in COSC440 Assignment 1 - Virtual Ramdisk Implementation by Bakhombisile Dlamini.
