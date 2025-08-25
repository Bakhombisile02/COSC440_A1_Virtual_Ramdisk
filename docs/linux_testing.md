# Linux Testing Guide for Virtual Ramdisk

**Author**: Bakhombisile Dlamini  
**Assignment**: COSC440 Assignment 1 - Virtual Ramdisk  
**Purpose**: Linux environment testing procedures and validation  

## Prerequisites for Linux Testing

### Required System Setup
```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install build-essential linux-headers-$(uname -r)

# CentOS/RHEL/Fedora
sudo yum groupinstall "Development Tools"
sudo yum install kernel-devel kernel-headers

# Or for newer versions:
sudo dnf groupinstall "Development Tools"
sudo dnf install kernel-devel kernel-headers
```

### Verify Kernel Headers
```bash
ls /lib/modules/$(uname -r)/build
# Should show kernel build directory
```

## Step-by-Step Testing Process

### 1. Build the Module
```bash
cd src/
make clean          # Clean any previous builds
make                # Build the kernel module

# Expected output:
# make -C /lib/modules/.../build M=/path/to/src modules
# CC [M]  /path/to/src/ramdisk.o
# Building modules, stage 2.
# MODPOST 1 modules
# CC      /path/to/src/ramdisk.mod.o
# LD [M]  /path/to/src/ramdisk.ko
```

### 2. Check Module Info
```bash
make info
# or manually:
modinfo ramdisk.ko
file ramdisk.ko
```

### 3. Load the Module
```bash
sudo make load      # Load the module
# or manually:
sudo insmod ramdisk.ko

# Check if loaded:
lsmod | grep ramdisk
```

### 4. Verify Device Creation and Set Permissions
```bash
ls -l /dev/asgn1
# Should show: crw-rw-rw- 1 root root major,0 date /dev/asgn1

# Set permissions as noted in assignment requirements
# "you need to change its permission to allow everyone to read and write from it"
sudo chmod 666 /dev/asgn1

# Verify permissions
ls -l /dev/asgn1

# Check major number:
cat /proc/devices | grep ramdisk
```

### 5. Basic Functionality Test
```bash
make test           # Simple test
# or manually:
echo "Hello Virtual Ramdisk!" > /dev/asgn1
cat /dev/asgn1
```

### 6. Comprehensive Testing
```bash
make full_test      # Complete test suite
# or manually:
sudo ./ramdisk_test
```

### 7. Monitor Kernel Logs
```bash
# In another terminal, monitor kernel messages:
sudo dmesg -w

# Or after testing:
sudo dmesg | tail -20
```

### 8. Unload Module
```bash
sudo make unload    # Unload module
# or manually:
sudo rmmod ramdisk
```

## Expected Test Results

### Module Loading
```
ramdisk: Initializing Virtual Ramdisk module
ramdisk: Virtual Ramdisk initialized successfully
ramdisk: Major number: XXX, Device node: /dev/asgn1
```

### Basic I/O Test
```
ramdisk: Device opened, current users: 1
ramdisk: Created new page at offset 0
ramdisk: Device closed, current users: 0
```

### Full Test Program Results
```
Virtual Ramdisk Test Program
============================

=== Basic I/O Test ===
Wrote 47 bytes: Hello, Virtual Ramdisk! This is a test message.
Read 47 bytes: Hello, Virtual Ramdisk! This is a test message.
✓ Data integrity verified - write and read match

=== IOCTL User Limit Test ===
✓ Successfully set user limit to 2

=== Seek Functionality Test ===
Seeked to position: 20
Data after seek test: 'First part Second part'
✓ Seek functionality working (notice gap between parts)

=== Concurrent Access Test ===
Set user limit to 2. Spawning 5 child processes...
Child 1: ✓ Successfully opened device
Child 2: ✓ Successfully opened device  
Child 3: ✓ Correctly blocked (EBUSY) - user limit enforced
Child 4: ✓ Correctly blocked (EBUSY) - user limit enforced
Child 5: ✓ Correctly blocked (EBUSY) - user limit enforced
```

### Module Unloading
```
ramdisk: Cleaning up Virtual Ramdisk module
ramdisk: Freed page at offset 0
ramdisk: All pages freed
ramdisk: Virtual Ramdisk module removed successfully
```

## Troubleshooting

### Common Issues

1. **Module won't load**
   ```bash
   # Check kernel log for errors:
   sudo dmesg | tail
   
   # Verify kernel headers match:
   uname -r
   ls /lib/modules/$(uname -r)/build
   ```

2. **Device node not created**
   ```bash
   # Check if module loaded:
   lsmod | grep ramdisk
   
   # Manually create device node if needed:
   sudo mknod /dev/asgn1 c MAJOR 0
   # (Replace MAJOR with the number from /proc/devices)
   ```

3. **Permission denied**
   ```bash
   # Fix permissions:
   sudo chmod 666 /dev/asgn1
   ```

4. **Compilation errors**
   ```bash
   # Ensure kernel headers installed:
   sudo apt-get install linux-headers-$(uname -r)
   
   # Clean and rebuild:
   make clean
   make
   ```

## Performance Testing

### Large File Test
```bash
# Test unlimited size capability:
dd if=/dev/zero of=/dev/asgn1 bs=1M count=10
ls -lh /dev/asgn1
cat /dev/asgn1 | wc -c
```

### Memory Usage Monitoring
```bash
# Monitor memory usage during testing:
watch -n 1 'free -h; echo "Pages:"; cat /proc/meminfo | grep -i page'
```

### Stress Testing
```bash
# Multiple concurrent writers:
for i in {1..5}; do
  (echo "Process $i data" >> /dev/asgn1) &
done
wait

cat /dev/asgn1
```

## Advanced Testing

### Kernel Module Debugging
```bash
# Enable dynamic debug (if available):
echo 'module ramdisk +p' > /sys/kernel/debug/dynamic_debug/control

# Or add printk messages and recompile
```

### Memory Leak Detection
```bash
# Before loading:
cat /proc/meminfo | grep -i page

# After loading and testing:
cat /proc/meminfo | grep -i page

# Check for page leaks
```

## Validation Checklist

- [ ] Module compiles without warnings
- [ ] Module loads successfully  
- [ ] Device node appears in /dev/
- [ ] Basic read/write operations work
- [ ] File size grows beyond 4KB (multiple pages)
- [ ] Seeking works correctly
- [ ] IOCTL user limiting functions
- [ ] Concurrent access control works
- [ ] Module unloads cleanly
- [ ] All pages freed on unload
- [ ] No kernel error messages


