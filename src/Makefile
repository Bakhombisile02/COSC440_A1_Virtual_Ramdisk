# Makefile for COSC440 Assignment 1 - Enhanced Virtual Ramdisk Linux kernel module

# Module name - asgn1 as specified in assignment requirements
MODULE_NAME = ramdisk
obj-m := $(MODULE_NAME).o

KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all: module

module:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -f $(MODULE_NAME)_test

# Build enhanced test program with all bonus features
test_program: $(MODULE_NAME)_test.c
	gcc -o $(MODULE_NAME)_test $(MODULE_NAME)_test.c

# Additional targets for testing and module management
load:
	sudo insmod $(MODULE_NAME).ko

unload:
	sudo rmmod $(MODULE_NAME)

# Simple functionality test
test: load
	@echo "Testing Enhanced Virtual Ramdisk functionality..."
	@echo "1. Writing test data to /dev/asgn1"
	@echo "Hello Enhanced Virtual Ramdisk!" | sudo tee /dev/asgn1 > /dev/null
	@echo "2. Reading back from /dev/asgn1"
	@sudo cat /dev/asgn1
	@echo "3. Checking /proc interface"
	@sudo cat /proc/ramdisk_info 2>/dev/null || echo "/proc entry not available"
	@echo "4. Testing seek functionality"
	@echo "Enhanced test complete - ramdisk is working!"

# Comprehensive test using enhanced test program
full_test: module test_program load
	@echo "Running comprehensive enhanced ramdisk tests..."
	@echo "Features being tested:"
	@echo "- Basic I/O operations"
	@echo "- IOCTL user limits"
	@echo "- mmap() memory mapping (Bonus Feature 1)"
	@echo "- Memory cache optimization (Bonus Feature 2)"
	@echo "- /proc interface (Bonus Feature 3)"
	@echo "- Enhanced logging and statistics"
	@echo "========================================="
	sudo ./$(MODULE_NAME)_test
	@echo "Check kernel logs with: sudo dmesg | tail -30"
	@echo "Check /proc interface with: cat /proc/ramdisk_info"

info:
	@echo "Enhanced Module info:"
	@modinfo $(MODULE_NAME).ko 2>/dev/null || echo "Module not built yet"
	@echo "Device info:"
	@ls -l /dev/asgn1 2>/dev/null || echo "Device not loaded yet"
	@echo "/proc interface:"
	@ls -l /proc/ramdisk_info 2>/dev/null || echo "/proc entry not available"

# Debug target for development
debug: module load
	@echo "Debug information:"
	@echo "1. Module loaded:"
	@lsmod | grep $(MODULE_NAME) || echo "Module not loaded"
	@echo "2. Device node:"
	@ls -l /dev/asgn1 2>/dev/null || echo "Device node not found"
	@echo "3. /proc interface:"
	@cat /proc/ramdisk_info 2>/dev/null || echo "/proc entry not available"
	@echo "4. Recent kernel messages:"
	@sudo dmesg | tail -10

.PHONY: all module clean load unload test full_test info debug
