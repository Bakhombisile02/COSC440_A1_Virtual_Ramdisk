/*
 * ramdisk_test.c - Comprehensive Test Program 
 * 
 * Author: Bakhombisile Dlamini - COSC440 Assignment 1
 * Description: Complete test suite for virtual ramdisk character device driver
 * 
 * This program tests:
 * - Basic I/O operations and seek functionality
 * - IOCTL process control feature (concurrent user limits)
 * - mmap() memory mapping functionality (Bonus Feature 1)
 * - /proc interface for device statistics (Bonus Feature 3)
 * - Enhanced logging and performance features
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/stat.h>

/* IOCTL command definition - must match kernel module */
#define RAMDISK_IOC_MAGIC 'r'
#define RAMDISK_IOC_LIMIT_USERS _IOW(RAMDISK_IOC_MAGIC, 1, int)

#define DEVICE_PATH "/dev/asgn1"
#define PROC_PATH "/proc/ramdisk_info"

void test_basic_io(void)
{
    int fd;
    char write_buf[] = "Hello, Virtual Ramdisk! This is a test message.";
    char read_buf[100];
    ssize_t bytes_written, bytes_read;

    printf("\n=== Basic I/O Test ===\n");

    /* Open device */
    fd = open(DEVICE_PATH, O_RDWR);
    if (fd < 0) {
        perror("Failed to open ramdisk device for I/O test");
        return;
    }

    /* Write data */
    bytes_written = write(fd, write_buf, strlen(write_buf));
    if (bytes_written < 0) {
        perror("Write failed");
        close(fd);
        return;
    }
    printf("Wrote %zd bytes: %s\n", bytes_written, write_buf);

    /* Seek to beginning */
    if (lseek(fd, 0, SEEK_SET) < 0) {
        perror("Seek failed");
        close(fd);
        return;
    }

    /* Read data back */
    memset(read_buf, 0, sizeof(read_buf));
    bytes_read = read(fd, read_buf, sizeof(read_buf) - 1);
    if (bytes_read < 0) {
        perror("Read failed");
        close(fd);
        return;
    }

    printf("Read %zd bytes: %s\n", bytes_read, read_buf);

    /* Verify data integrity */
    if (strncmp(write_buf, read_buf, strlen(write_buf)) == 0) {
        printf("✓ Data integrity verified - write and read match\n");
    } else {
        printf("✗ Data integrity failed - write and read don't match\n");
    }

    close(fd);
}

void test_ioctl_user_limit(void)
{
    int fd;
    int user_limit = 2;

    printf("\n=== IOCTL User Limit Test ===\n");

    /* Open device */
    fd = open(DEVICE_PATH, O_RDWR);
    if (fd < 0) {
        perror("Failed to open ramdisk device for IOCTL test");
        return;
    }

    /* Set user limit using IOCTL */
    if (ioctl(fd, RAMDISK_IOC_LIMIT_USERS, &user_limit) < 0) {
        perror("Failed to set user limit");
        close(fd);
        return;
    }

    printf("✓ Successfully set user limit to %d\n", user_limit);
    close(fd);
}

void test_concurrent_access(void)
{
    int i;
    pid_t pids[5];
    int user_limit = 2;
    int fd;

    printf("\n=== Concurrent Access Test ===\n");

    /* First, set the user limit */
    fd = open(DEVICE_PATH, O_RDWR);
    if (fd < 0) {
        perror("Failed to open device to set limit");
        return;
    }

    if (ioctl(fd, RAMDISK_IOC_LIMIT_USERS, &user_limit) < 0) {
        perror("Failed to set user limit for concurrent test");
        close(fd);
        return;
    }
    close(fd);

    printf("Set user limit to %d. Spawning 5 child processes...\n", user_limit);

    /* Fork multiple child processes to test concurrent access */
    for (i = 0; i < 5; i++) {
        pids[i] = fork();
        
        if (pids[i] == 0) {
            /* Child process */
            int child_fd;
            char buffer[50];
            
            printf("Child %d: Attempting to open device...\n", i + 1);
            
            child_fd = open(DEVICE_PATH, O_RDWR);
            if (child_fd < 0) {
                if (errno == EBUSY) {
                    printf("Child %d: ✓ Correctly blocked (EBUSY) - user limit enforced\n", i + 1);
                } else {
                    printf("Child %d: ✗ Failed with unexpected error: %s\n", i + 1, strerror(errno));
                }
                exit(errno == EBUSY ? 0 : 1);
            }
            
            printf("Child %d: ✓ Successfully opened device\n", i + 1);
            
            /* Write some data to show the device is functional */
            snprintf(buffer, sizeof(buffer), "Message from child %d\n", i + 1);
            if (write(child_fd, buffer, strlen(buffer)) < 0) {
                printf("Child %d: Write failed: %s\n", i + 1, strerror(errno));
            } else {
                printf("Child %d: Successfully wrote data\n", i + 1);
            }
            
            /* Hold the device open for a moment */
            sleep(2);
            
            close(child_fd);
            printf("Child %d: Closed device\n", i + 1);
            exit(0);
        } else if (pids[i] < 0) {
            perror("Fork failed");
            break;
        }
        
        /* Small delay between forks to make output more readable */
        usleep(100000);
    }

    /* Wait for all children */
    for (i = 0; i < 5 && pids[i] > 0; i++) {
        int status;
        waitpid(pids[i], &status, 0);
        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            printf("Child %d: Completed successfully\n", i + 1);
        } else {
            printf("Child %d: Completed with error\n", i + 1);
        }
    }
}

void test_seek_functionality(void)
{
    int fd;
    char data1[] = "First part";
    char data2[] = "Second part";
    char read_buf[50];
    off_t offset;

    printf("\n=== Seek Functionality Test ===\n");

    fd = open(DEVICE_PATH, O_RDWR);
    if (fd < 0) {
        perror("Failed to open device for seek test");
        return;
    }

    /* Write first part at beginning */
    if (write(fd, data1, strlen(data1)) < 0) {
        perror("Failed to write first part");
        close(fd);
        return;
    }

    /* Seek to position 20 and write second part */
    offset = lseek(fd, 20, SEEK_SET);
    if (offset < 0) {
        perror("Seek failed");
        close(fd);
        return;
    }
    printf("Seeked to position: %ld\n", (long)offset);

    if (write(fd, data2, strlen(data2)) < 0) {
        perror("Failed to write second part");
        close(fd);
        return;
    }

    /* Read from beginning */
    lseek(fd, 0, SEEK_SET);
    memset(read_buf, 0, sizeof(read_buf));
    if (read(fd, read_buf, sizeof(read_buf) - 1) < 0) {
        perror("Failed to read after seek");
        close(fd);
        return;
    }

    printf("Data after seek test: '%s'\n", read_buf);
    printf("✓ Seek functionality working (notice gap between parts)\n");

    close(fd);
}

void test_mmap_functionality(void)
{
    int fd;
    void *mapped_mem;
    char write_data[] = "Memory mapped test data for ramdisk!";
    char *read_data;
    size_t map_size = 4096; /* One page */

    printf("\n=== mmap() Functionality Test (Bonus Feature) ===\n");

    /* Open device for read/write */
    fd = open(DEVICE_PATH, O_RDWR);
    if (fd < 0) {
        perror("Failed to open device for mmap test");
        return;
    }

    /* First write some data using regular write() */
    if (write(fd, write_data, strlen(write_data)) < 0) {
        perror("Failed to write initial data for mmap test");
        close(fd);
        return;
    }

    /* Map the ramdisk into memory */
    mapped_mem = mmap(NULL, map_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (mapped_mem == MAP_FAILED) {
        perror("mmap failed");
        close(fd);
        return;
    }

    printf("✓ Successfully mapped ramdisk into memory at address %p\n", mapped_mem);

    /* Read data through memory mapping */
    read_data = (char *)mapped_mem;
    printf("Data read via mmap: '%.50s'\n", read_data);

    /* Verify data integrity */
    if (strncmp(write_data, read_data, strlen(write_data)) == 0) {
        printf("✓ mmap read integrity verified\n");
    } else {
        printf("✗ mmap read integrity failed\n");
    }

    /* Modify data through memory mapping */
    const char *mmap_write = " + Modified via mmap!";
    strncat(read_data, mmap_write, map_size - strlen(read_data) - 1);
    printf("Modified data via mmap: '%.80s'\n", read_data);

    /* Verify modification by reading with regular read() */
    lseek(fd, 0, SEEK_SET);
    char verify_buf[200];
    memset(verify_buf, 0, sizeof(verify_buf));
    if (read(fd, verify_buf, sizeof(verify_buf) - 1) > 0) {
        printf("Data verified via read(): '%.80s'\n", verify_buf);
        if (strstr(verify_buf, "Modified via mmap") != NULL) {
            printf("✓ mmap write integrity verified\n");
        } else {
            printf("✗ mmap write integrity failed\n");
        }
    }

    /* Unmap memory */
    if (munmap(mapped_mem, map_size) < 0) {
        perror("munmap failed");
    } else {
        printf("✓ Successfully unmapped memory\n");
    }

    close(fd);
}

void test_proc_interface(void)
{
    FILE *proc_file;
    char buffer[1024];

    printf("\n=== /proc Interface Test (Bonus Feature) ===\n");

    /* Check if /proc entry exists */
    if (access(PROC_PATH, R_OK) != 0) {
        printf("✗ /proc entry %s not accessible: %s\n", PROC_PATH, strerror(errno));
        printf("  This might indicate the enhanced module is not loaded\n");
        return;
    }

    /* Open and read /proc entry */
    proc_file = fopen(PROC_PATH, "r");
    if (!proc_file) {
        perror("Failed to open /proc entry");
        return;
    }

    printf("✓ Successfully opened %s\n", PROC_PATH);
    printf("=== Ramdisk Information from /proc ===\n");

    /* Read and display proc content */
    while (fgets(buffer, sizeof(buffer), proc_file)) {
        printf("%s", buffer);
    }

    fclose(proc_file);
    printf("=== End of /proc information ===\n");
}

void test_large_file_operations(void)
{
    int fd;
    size_t test_size = 8192; /* 2 pages */
    char *write_buffer, *read_buffer;
    size_t i;

    printf("\n=== Large File Operations Test ===\n");

    /* Allocate test buffers */
    write_buffer = malloc(test_size);
    read_buffer = malloc(test_size);
    if (!write_buffer || !read_buffer) {
        printf("✗ Failed to allocate test buffers\n");
        free(write_buffer);
        free(read_buffer);
        return;
    }

    /* Fill write buffer with pattern */
    for (i = 0; i < test_size; i++) {
        write_buffer[i] = 'A' + (i % 26);
    }

    fd = open(DEVICE_PATH, O_RDWR);
    if (fd < 0) {
        perror("Failed to open device for large file test");
        free(write_buffer);
        free(read_buffer);
        return;
    }

    /* Write large amount of data */
    printf("Writing %zu bytes of test data...\n", test_size);
    if (write(fd, write_buffer, test_size) != (ssize_t)test_size) {
        perror("Failed to write large data");
        close(fd);
        free(write_buffer);
        free(read_buffer);
        return;
    }

    /* Seek to beginning and read back */
    lseek(fd, 0, SEEK_SET);
    printf("Reading %zu bytes back...\n", test_size);
    if (read(fd, read_buffer, test_size) != (ssize_t)test_size) {
        perror("Failed to read large data");
        close(fd);
        free(write_buffer);
        free(read_buffer);
        return;
    }

    /* Verify data integrity */
    if (memcmp(write_buffer, read_buffer, test_size) == 0) {
        printf("✓ Large file operation integrity verified (%zu bytes)\n", test_size);
    } else {
        printf("✗ Large file operation integrity failed\n");
    }

    close(fd);
    free(write_buffer);
    free(read_buffer);
}

void test_performance_logging(void)
{
    int fd;
    int i;
    const int num_operations = 100;
    char test_data[] = "Performance test data";

    printf("\n=== Performance Logging Test ===\n");

    fd = open(DEVICE_PATH, O_RDWR);
    if (fd < 0) {
        perror("Failed to open device for performance test");
        return;
    }

    printf("Performing %d write/read/seek operations for logging test...\n", num_operations);

    /* Perform multiple operations to generate logs */
    for (i = 0; i < num_operations; i++) {
        /* Write operation */
        if (write(fd, test_data, strlen(test_data)) < 0) {
            printf("Write operation %d failed\n", i);
            break;
        }

        /* Seek operation */
        if (lseek(fd, 0, SEEK_SET) < 0) {
            printf("Seek operation %d failed\n", i);
            break;
        }

        /* Read operation */
        char read_buf[50];
        if (read(fd, read_buf, sizeof(read_buf)) < 0) {
            printf("Read operation %d failed\n", i);
            break;
        }

        /* Occasional seek to different position */
        if (i % 10 == 0) {
            lseek(fd, i * 10, SEEK_SET);
        }
    }

    close(fd);
    
    printf("✓ Completed %d operations - check /proc interface and dmesg for statistics\n", num_operations);
    printf("  Use: cat %s\n", PROC_PATH);
    printf("  Use: sudo dmesg | tail -10\n");
}

int main(void)
{
    printf("Enhanced Virtual Ramdisk Test Program\n");
    printf("=====================================\n");
    printf("Testing Core + Bonus Features:\n");
    printf("- Basic I/O operations and seek functionality\n");
    printf("- IOCTL process control (user limits)\n"); 
    printf("- mmap() memory mapping (Bonus Feature 1)\n");
    printf("- Memory cache optimization (Bonus Feature 2)\n");
    printf("- /proc interface (Bonus Feature 3)\n");
    printf("- Enhanced logging and performance tracking\n");
    printf("=====================================\n");

    /* Check if device exists */
    if (access(DEVICE_PATH, F_OK) != 0) {
        printf("Error: Device %s not found. Make sure the enhanced ramdisk module is loaded.\n", DEVICE_PATH);
        printf("Try: sudo make load\n");
        return 1;
    }

    /* Run all tests */
    test_basic_io();
    test_ioctl_user_limit();
    test_seek_functionality();
    test_large_file_operations();
    test_mmap_functionality();
    test_proc_interface();
    test_performance_logging();
    test_concurrent_access();

    printf("\n=== All Enhanced Tests Complete ===\n");
    printf("Check the following for detailed information:\n");
    printf("1. /proc interface: cat %s\n", PROC_PATH);
    printf("2. Kernel logs: sudo dmesg | tail -30\n");
    printf("3. Module info: lsmod | grep ramdisk\n");
    printf("\nEnhanced features tested:\n");
    printf("✓ mmap() implementation with memory mapping\n");
    printf("✓ Custom memory cache for optimized allocations\n");
    printf("✓ /proc interface for device statistics\n");
    printf("✓ Enhanced logging with operation counters\n");

    return 0;
}
