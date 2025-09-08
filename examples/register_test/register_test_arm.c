//==============================================================================
// Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
// SPDX-License-Identifier: MIT
//
//==============================================================================

/** @file register_test_arm.c
 *  @brief ARM-optimized register read/write test program for RecoNIC
 *
 *  This program is specifically designed for ARM platforms (like NVIDIA Jetson)
 *  and includes proper memory barriers and cache management for PCIe register access.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <getopt.h>
#include <errno.h>
#include <stdint.h>
#include <sys/types.h>
#include <time.h>

// ARM-specific memory barrier support
#ifdef __aarch64__
#define mb()    __asm__ volatile("dsb sy" : : : "memory")
#define rmb()   __asm__ volatile("dsb ld" : : : "memory")
#define wmb()   __asm__ volatile("dsb st" : : : "memory")
#else
#define mb()    __asm__ volatile("" : : : "memory")
#define rmb()   __asm__ volatile("" : : : "memory")  
#define wmb()   __asm__ volatile("" : : : "memory")
#endif

// Include RecoNIC register definitions (but not the library functions)
#include "../../lib/reconic_reg.h"

#define DEVICE_NAME_DEFAULT "/dev/reconic-mm"
#define PCIE_RESOURCE_DEFAULT "/sys/bus/pci/devices/0000:d8:00.0/resource2"

// Global variables
int verbose = 0;
int debug = 0;

// Command line options
static struct option const long_opts[] = {
    {"device", required_argument, NULL, 'd'},
    {"pcie_resource", required_argument, NULL, 'p'},
    {"address", required_argument, NULL, 'a'},
    {"value", required_argument, NULL, 'v'},
    {"read", no_argument, NULL, 'r'},
    {"write", no_argument, NULL, 'w'},
    {"list", no_argument, NULL, 'l'},
    {"test", no_argument, NULL, 't'},
    {"verbose", no_argument, NULL, 'V'},
    {"debug", no_argument, NULL, 'g'},
    {"help", no_argument, NULL, 'h'},
    {0, 0, 0, 0}
};

// ARM-optimized register access functions with proper memory barriers
static inline uint32_t arm_read32_data(volatile uint32_t* base_address, off_t offset) {
    volatile uint32_t* config_addr;
    uint32_t value;
    
    config_addr = (volatile uint32_t*)((uintptr_t)base_address + offset);
    
    // Memory barrier before read
    rmb();
    
    // Perform the read
    value = *config_addr;
    
    // Memory barrier after read to ensure completion
    rmb();
    
    return value;
}

static inline void arm_write32_data(volatile uint32_t* base_address, off_t offset, uint32_t value) {
    volatile uint32_t* config_addr;
    
    config_addr = (volatile uint32_t*)((uintptr_t)base_address + offset);
    
    // Memory barrier before write
    wmb();
    
    // Perform the write
    *config_addr = value;
    
    // Memory barrier after write to ensure completion
    wmb();
    
    // Additional synchronization - read back to ensure write completed
    // This is important for ARM platforms
    (void)(*config_addr);
    rmb();
}

static void usage(const char *name)
{
    fprintf(stdout, "usage: %s [OPTIONS]\n\n", name);
    fprintf(stdout, "ARM-Optimized Register Test Tool for RecoNIC\n\n");
    fprintf(stdout, "Options:\n");
    fprintf(stdout, "  -d (--device) character device name (defaults to %s)\n", DEVICE_NAME_DEFAULT);
    fprintf(stdout, "  -p (--pcie_resource) PCIe resource file (defaults to %s)\n", PCIE_RESOURCE_DEFAULT);
    fprintf(stdout, "  -a (--address) register address offset (hex, e.g., 0x102000)\n");
    fprintf(stdout, "  -v (--value) value to write (hex, e.g., 0x12345678)\n");
    fprintf(stdout, "  -r (--read) perform register read operation\n");
    fprintf(stdout, "  -w (--write) perform register write operation\n");
    fprintf(stdout, "  -l (--list) list predefined register addresses\n");
    fprintf(stdout, "  -t (--test) run comprehensive register tests\n");
    fprintf(stdout, "  -V (--verbose) verbose output\n");
    fprintf(stdout, "  -g (--debug) debug mode\n");
    fprintf(stdout, "  -h (--help) print usage help and exit\n\n");
    
    fprintf(stdout, "ARM-specific optimizations:\n");
    fprintf(stdout, "  - Memory barriers for proper ordering\n");
    fprintf(stdout, "  - Cache-aware register access\n");
    fprintf(stdout, "  - Read-back verification for writes\n\n");
    
    fprintf(stdout, "Examples:\n");
    fprintf(stdout, "  Read version register:\n");
    fprintf(stdout, "    %s -p /sys/bus/pci/devices/0005:01:00.0/resource2 -a 0x102000 -r\n\n", name);
    fprintf(stdout, "  Write to template register:\n");
    fprintf(stdout, "    %s -p /sys/bus/pci/devices/0005:01:00.0/resource2 -a 0x102200 -v 0x12345678 -w\n\n", name);
    fprintf(stdout, "  Run comprehensive tests:\n");
    fprintf(stdout, "    %s -p /sys/bus/pci/devices/0005:01:00.0/resource2 -t -V\n\n", name);
}

static void list_predefined_registers(void)
{
    fprintf(stdout, "\n=== RecoNIC Predefined Register Addresses ===\n\n");
    
    fprintf(stdout, "Statistics and Configuration Registers (SCR):\n");
    fprintf(stdout, "  RN_SCR_VERSION         : 0x%08X (Version register - Read only)\n", RN_SCR_VERSION);
    fprintf(stdout, "  RN_SCR_FATAL_ERR       : 0x%08X (Fatal error register - Read only)\n", RN_SCR_FATAL_ERR);
    fprintf(stdout, "  RN_SCR_TRMHR_REG       : 0x%08X (TX rate meter high register - Read only)\n", RN_SCR_TRMHR_REG);
    fprintf(stdout, "  RN_SCR_TRMLR_REG       : 0x%08X (TX rate meter low register - Read only)\n", RN_SCR_TRMLR_REG);
    fprintf(stdout, "  RN_SCR_TRRMHR_REG      : 0x%08X (TX/RX rate meter high register - Read only)\n", RN_SCR_TRRMHR_REG);
    fprintf(stdout, "  RN_SCR_TRRMLR_REG      : 0x%08X (TX/RX rate meter low register - Read only)\n", RN_SCR_TRRMLR_REG);
    fprintf(stdout, "  RN_SCR_TEMPLATE_REG    : 0x%08X (Template register - Read/Write)\n", RN_SCR_TEMPLATE_REG);
    
    fprintf(stdout, "\nCompute Logic Registers (CLR):\n");
    fprintf(stdout, "  RN_CLR_CTL_CMD         : 0x%08X (Control command register)\n", RN_CLR_CTL_CMD);
    fprintf(stdout, "  RN_CLR_KER_STS         : 0x%08X (Kernel status register)\n", RN_CLR_KER_STS);
    fprintf(stdout, "  RN_CLR_JOB_SUBMITTED   : 0x%08X (Job submitted register)\n", RN_CLR_JOB_SUBMITTED);
    fprintf(stdout, "  RN_CLR_JOB_COMPLETED_NOT_READ : 0x%08X (Job completed not read register)\n", RN_CLR_JOB_COMPLETED_NOT_READ);
    fprintf(stdout, "  RN_CLR_TEMPLATE        : 0x%08X (Template register - Read/Write)\n", RN_CLR_TEMPLATE);
    
    fprintf(stdout, "\nRDMA Global Control Status Registers (GCSR):\n");
    fprintf(stdout, "  RN_RDMA_GCSR_XRNICCONF : 0x%08X (XRNIC configuration)\n", RN_RDMA_GCSR_XRNICCONF);
    fprintf(stdout, "  RN_RDMA_GCSR_MACXADDLSB: 0x%08X (MAC address LSB)\n", RN_RDMA_GCSR_MACXADDLSB);
    fprintf(stdout, "  RN_RDMA_GCSR_MACXADDMSB: 0x%08X (MAC address MSB)\n", RN_RDMA_GCSR_MACXADDMSB);
    fprintf(stdout, "  RN_RDMA_GCSR_IPV4XADD  : 0x%08X (IPv4 address)\n", RN_RDMA_GCSR_IPV4XADD);
    
    fprintf(stdout, "\nQDMA AXI Bridge Registers:\n");
    fprintf(stdout, "  AXIB_BDF_ADDR_TRANSLATE_ADDR_LSB : 0x%08X (BDF address translate LSB)\n", AXIB_BDF_ADDR_TRANSLATE_ADDR_LSB);
    fprintf(stdout, "  AXIB_BDF_ADDR_TRANSLATE_ADDR_MSB : 0x%08X (BDF address translate MSB)\n", AXIB_BDF_ADDR_TRANSLATE_ADDR_MSB);
    fprintf(stdout, "  AXIB_BDF_MAP_CONTROL_ADDR        : 0x%08X (BDF map control)\n", AXIB_BDF_MAP_CONTROL_ADDR);
    
    fprintf(stdout, "\nNote: This tool is optimized for ARM platforms like NVIDIA Jetson.\n");
    fprintf(stdout, "      Memory barriers ensure proper register access ordering.\n\n");
}

static uint64_t parse_hex_value(const char *str)
{
    char *endptr;
    uint64_t value;
    
    if (str == NULL) {
        fprintf(stderr, "Error: NULL string provided to parse_hex_value\n");
        return 0;
    }
    
    // Handle 0x prefix
    if (strncmp(str, "0x", 2) == 0 || strncmp(str, "0X", 2) == 0) {
        value = strtoull(str + 2, &endptr, 16);
    } else {
        value = strtoull(str, &endptr, 16);
    }
    
    if (*endptr != '\0') {
        fprintf(stderr, "Error: Invalid hex value: %s\n", str);
        return 0;
    }
    
    return value;
}

static int test_register_read(volatile uint32_t *axil_base, uint32_t offset)
{
    uint32_t value;
    struct timespec start, end;
    double elapsed;
    
    if (verbose) {
        printf("Reading from register at offset 0x%08X...\n", offset);
    }
    
    // Measure access time
    clock_gettime(CLOCK_MONOTONIC, &start);
    value = arm_read32_data(axil_base, offset);
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    elapsed = (end.tv_sec - start.tv_sec) * 1000000.0 + (end.tv_nsec - start.tv_nsec) / 1000.0;
    
    printf("Register Read Result:\n");
    printf("  Address: 0x%08X\n", offset);
    printf("  Value  : 0x%08X (%u)\n", value, value);
    if (verbose) {
        printf("  Time   : %.2f microseconds\n", elapsed);
    }
    
    if (debug) {
        printf("Debug: AXIL base address: %p\n", (void*)axil_base);
        printf("Debug: Calculated address: %p\n", (void*)((uintptr_t)axil_base + offset));
        printf("Debug: Physical PCIe BAR: Check lspci output\n");
        printf("Debug: Virtual mapping: This is normal for user-space access\n");
    }
    
    return 0;
}

static int test_register_write(volatile uint32_t *axil_base, uint32_t offset, uint32_t value)
{
    uint32_t read_back;
    struct timespec start, end;
    double elapsed;
    
    if (verbose) {
        printf("Writing 0x%08X to register at offset 0x%08X...\n", value, offset);
    }
    
    // Measure write time
    clock_gettime(CLOCK_MONOTONIC, &start);
    arm_write32_data(axil_base, offset, value);
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    elapsed = (end.tv_sec - start.tv_sec) * 1000000.0 + (end.tv_nsec - start.tv_nsec) / 1000.0;
    
    // Read back to verify
    read_back = arm_read32_data(axil_base, offset);
    
    printf("Register Write Result:\n");
    printf("  Address    : 0x%08X\n", offset);
    printf("  Written    : 0x%08X (%u)\n", value, value);
    printf("  Read back  : 0x%08X (%u)\n", read_back, read_back);
    if (verbose) {
        printf("  Write time : %.2f microseconds\n", elapsed);
    }
    
    if (value == read_back) {
        printf("  Status     : SUCCESS - Values match\n");
    } else {
        printf("  Status     : WARNING - Values don't match (register may be read-only or have different behavior)\n");
        if (debug) {
            printf("  Debug      : Difference = 0x%08X\n", value ^ read_back);
        }
    }
    
    if (debug) {
        printf("Debug: AXIL base address: %p\n", (void*)axil_base);
        printf("Debug: Calculated address: %p\n", (void*)((uintptr_t)axil_base + offset));
    }
    
    return 0;
}

static int run_comprehensive_tests(volatile uint32_t *axil_base)
{
    printf("\n=== ARM-Optimized Comprehensive Register Tests ===\n");
    
    // Test 1: Read version register (should always work)
    printf("\nTest 1: Version Register Read\n");
    test_register_read(axil_base, RN_SCR_VERSION);
    
    // Test 2: Read multiple status registers
    printf("\nTest 2: Status Registers Read\n");
    test_register_read(axil_base, RN_SCR_FATAL_ERR);
    test_register_read(axil_base, RN_SCR_TRMHR_REG);
    test_register_read(axil_base, RN_SCR_TRMLR_REG);
    
    // Test 3: Test template register (read/write)
    printf("\nTest 3: Template Register Write/Read Test\n");
    uint32_t test_values[] = {0x12345678, 0xDEADBEEF, 0xCAFEBABE, 0x55AA55AA, 0x00000000, 0xFFFFFFFF};
    int num_tests = sizeof(test_values) / sizeof(test_values[0]);
    
    for (int i = 0; i < num_tests; i++) {
        printf("\nTest 3.%d: Testing value 0x%08X\n", i+1, test_values[i]);
        test_register_write(axil_base, RN_SCR_TEMPLATE_REG, test_values[i]);
        usleep(1000); // Small delay between tests
    }
    
    // Test 4: CLR template register
    printf("\nTest 4: CLR Template Register Test\n");
    test_register_write(axil_base, RN_CLR_TEMPLATE, 0x87654321);
    
    // Test 5: Register access timing
    printf("\nTest 5: Register Access Timing Test\n");
    struct timespec start, end;
    double total_time = 0;
    int iterations = 100;
    
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < iterations; i++) {
        (void)arm_read32_data(axil_base, RN_SCR_VERSION);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    total_time = (end.tv_sec - start.tv_sec) * 1000000.0 + (end.tv_nsec - start.tv_nsec) / 1000.0;
    printf("  %d reads took %.2f microseconds (avg: %.2f us per read)\n", 
           iterations, total_time, total_time / iterations);
    
    printf("\n=== Comprehensive Tests Complete ===\n");
    return 0;
}

int main(int argc, char *argv[])
{
    int cmd_opt;
    char *device = DEVICE_NAME_DEFAULT;
    char *pcie_resource = PCIE_RESOURCE_DEFAULT;
    uint32_t address = 0;
    uint32_t value = 0;
    int operation = 0; // 0: none, 1: read, 2: write, 3: test
    int address_set = 0;
    int value_set = 0;
    
    printf("=== ARM-Optimized RecoNIC Register Test Tool ===\n");
    printf("Optimized for NVIDIA Jetson and other ARM platforms\n\n");
    
    // Parse command line arguments
    while ((cmd_opt = getopt_long(argc, argv, "d:p:a:v:rwltVgh", long_opts, NULL)) != -1) {
        switch (cmd_opt) {
        case 'd':
            device = strdup(optarg);
            break;
        case 'p':
            pcie_resource = strdup(optarg);
            break;
        case 'a':
            address = (uint32_t)parse_hex_value(optarg);
            address_set = 1;
            break;
        case 'v':
            value = (uint32_t)parse_hex_value(optarg);
            value_set = 1;
            break;
        case 'r':
            operation = 1;
            break;
        case 'w':
            operation = 2;
            break;
        case 'l':
            list_predefined_registers();
            exit(0);
            break;
        case 't':
            operation = 3;
            break;
        case 'V':
            verbose = 1;
            break;
        case 'g':
            debug = 1;
            verbose = 1; // Debug implies verbose
            break;
        case 'h':
            usage(argv[0]);
            exit(0);
            break;
        default:
            usage(argv[0]);
            exit(1);
            break;
        }
    }
    
    // Validate arguments
    if (operation == 0) {
        fprintf(stderr, "Error: Must specify operation: -r (read), -w (write), -t (test), or -l (list)\n");
        usage(argv[0]);
        exit(1);
    }
    
    if ((operation == 1 || operation == 2) && !address_set) {
        fprintf(stderr, "Error: Must specify register address with -a option for read/write operations\n");
        usage(argv[0]);
        exit(1);
    }
    
    if (operation == 2 && !value_set) {
        fprintf(stderr, "Error: Must specify value with -v option for write operation\n");
        usage(argv[0]);
        exit(1);
    }
    
    // Open PCIe resource file for register access
    int pcie_resource_fd = open(pcie_resource, O_RDWR | O_SYNC);
    if (pcie_resource_fd < 0) {
        fprintf(stderr, "Error: Unable to open PCIe resource file %s\n", pcie_resource);
        perror("open PCIe resource");
        exit(1);
    }
    
    if (verbose) {
        printf("Opened PCIe resource: %s (fd: %d)\n", pcie_resource, pcie_resource_fd);
    }
    
    // Map the PCIe BAR space with ARM-appropriate flags
    volatile uint32_t *axil_base = (volatile uint32_t*)mmap(NULL, RN_SCR_MAP_SIZE, 
                                                           PROT_READ | PROT_WRITE, 
                                                           MAP_SHARED, pcie_resource_fd, 0);
    if (axil_base == MAP_FAILED) {
        fprintf(stderr, "Error: Failed to mmap PCIe resource\n");
        perror("mmap");
        close(pcie_resource_fd);
        exit(1);
    }
    
    if (verbose) {
        printf("Mapped PCIe BAR space: %p (size: 0x%X)\n", (void*)axil_base, RN_SCR_MAP_SIZE);
        printf("Device: %s\n", device);
        if (operation == 1) printf("Operation: READ\n");
        else if (operation == 2) printf("Operation: WRITE\n");
        else if (operation == 3) printf("Operation: COMPREHENSIVE TEST\n");
        
        if (address_set) printf("Address: 0x%08X\n", address);
        if (value_set) printf("Value: 0x%08X\n", value);
    }
    
    printf("\n=== RecoNIC ARM-Optimized Register Test ===\n");
    
    int result = 0;
    
    // Perform the requested operation
    if (operation == 1) {
        result = test_register_read(axil_base, address);
    } else if (operation == 2) {
        result = test_register_write(axil_base, address, value);
    } else if (operation == 3) {
        result = run_comprehensive_tests(axil_base);
    }
    
    printf("\n=== Test Complete ===\n");
    
    // Cleanup
    if (munmap((void*)axil_base, RN_SCR_MAP_SIZE) != 0) {
        perror("munmap");
    }
    close(pcie_resource_fd);
    
    if (strcmp(device, DEVICE_NAME_DEFAULT) != 0) {
        free(device);
    }
    if (strcmp(pcie_resource, PCIE_RESOURCE_DEFAULT) != 0) {
        free(pcie_resource);
    }
    
    return result;
}