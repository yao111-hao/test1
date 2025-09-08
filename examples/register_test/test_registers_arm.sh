#!/bin/bash
#==============================================================================
# Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
# SPDX-License-Identifier: MIT
#
#==============================================================================
#
# ARM-Optimized RecoNIC Register Test Script
# Specifically designed for NVIDIA Jetson and other ARM platforms
#
#==============================================================================

# Default values - Updated for your Jetson system
DEVICE="/dev/reconic-mm"
PCIE_RESOURCE="/sys/bus/pci/devices/0005:01:00.0/resource2"
VERBOSE=""
DEBUG=""

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Function to print colored output
print_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

print_debug() {
    echo -e "${CYAN}[DEBUG]${NC} $1"
}

# Function to show usage
usage() {
    echo "Usage: $0 [OPTIONS]"
    echo ""
    echo "ARM-Optimized RecoNIC Register Test Script"
    echo "Designed for NVIDIA Jetson and other ARM platforms"
    echo ""
    echo "Options:"
    echo "  -d DEVICE        Device file (default: $DEVICE)"
    echo "  -p PCIE_RESOURCE PCIe resource file (default: $PCIE_RESOURCE)"
    echo "  -v               Verbose output"
    echo "  -g               Debug mode"
    echo "  -h               Show this help"
    echo ""
    echo "Examples:"
    echo "  $0                                    # Run ARM-optimized tests"
    echo "  $0 -v                                # Run with verbose output"
    echo "  $0 -p /sys/bus/pci/devices/0005:01:00.0/resource2 -v"
    echo ""
    echo "Key differences from x86 version:"
    echo "  - Uses memory barriers for proper ARM memory ordering"
    echo "  - Includes cache-aware register access"
    echo "  - Optimized for PCIe on ARM platforms"
    echo ""
}

# Parse command line arguments
while getopts "d:p:vgh" opt; do
    case $opt in
        d)
            DEVICE="$OPTARG"
            ;;
        p)
            PCIE_RESOURCE="$OPTARG"
            ;;
        v)
            VERBOSE="-V"
            ;;
        g)
            DEBUG="-g"
            VERBOSE="-V"
            ;;
        h)
            usage
            exit 0
            ;;
        \?)
            echo "Invalid option: -$OPTARG" >&2
            usage
            exit 1
            ;;
    esac
done

# Check if the ARM-optimized program exists
if [ ! -f "./register_test_arm" ]; then
    print_error "register_test_arm executable not found. Please run 'make register_test_arm' first."
    exit 1
fi

# Check if PCIe resource file exists
if [ ! -f "$PCIE_RESOURCE" ]; then
    print_error "PCIe resource file not found: $PCIE_RESOURCE"
    print_info "Please check your PCIe device path with 'lspci | grep Xilinx'"
    print_info "For ARM systems, the path might be different (e.g., 0005:01:00.0 instead of 0000:d8:00.0)"
    exit 1
fi

# Check if device file exists
if [ ! -c "$DEVICE" ]; then
    print_warning "Device file not found: $DEVICE"
    print_info "Register tests will continue (device file only needed for DMA operations)"
fi

# Check if we're running on ARM
ARCH=$(uname -m)
if [[ "$ARCH" == "aarch64" || "$ARCH" == "arm64" ]]; then
    print_success "Detected ARM64 architecture - using ARM-optimized register access"
else
    print_warning "Not running on ARM64 ($ARCH detected) - ARM optimizations may not be needed"
fi

print_info "Starting ARM-Optimized RecoNIC Register Tests"
print_info "Device: $DEVICE"
print_info "PCIe Resource: $PCIE_RESOURCE"
print_info "Architecture: $ARCH"
echo ""

# Function to run a test and check result
run_test() {
    local test_name="$1"
    local cmd="$2"
    
    print_info "Running test: $test_name"
    echo "Command: $cmd"
    echo ""
    
    if eval "$cmd"; then
        print_success "Test completed: $test_name"
    else
        print_error "Test failed: $test_name"
        return 1
    fi
    echo ""
    echo "----------------------------------------"
    echo ""
}

# ARM-specific system checks
print_info "=== ARM System Information ==="
print_debug "CPU Information:"
lscpu | grep -E "(Architecture|Model name|CPU MHz)" || echo "lscpu not available"
echo ""

print_debug "Memory Information:"
free -h || echo "free command not available"
echo ""

print_debug "PCIe Information:"
lspci | grep -i xilinx || print_warning "No Xilinx devices found with lspci"
echo ""

# Test 1: List predefined registers
print_info "=== Test 1: List Predefined Registers ==="
run_test "List Registers" "./register_test_arm -l"

# Test 2: Read version register (should always work)
print_info "=== Test 2: Read Version Register ==="
run_test "Read Version Register" "./register_test_arm -p \"$PCIE_RESOURCE\" -a 0x102000 -r $VERBOSE $DEBUG"

# Test 3: Read template register in SCR space
print_info "=== Test 3: Read SCR Template Register ==="
run_test "Read SCR Template" "./register_test_arm -p \"$PCIE_RESOURCE\" -a 0x102200 -r $VERBOSE $DEBUG"

# Test 4: Write and read back template register in SCR space
print_info "=== Test 4: Write/Read SCR Template Register ==="
TEST_VALUES=("0x12345678" "0xDEADBEEF" "0xCAFEBABE" "0x55AA55AA" "0x00000000" "0xFFFFFFFF")

for i in "${!TEST_VALUES[@]}"; do
    TEST_VALUE="${TEST_VALUES[$i]}"
    print_info "Test 4.$((i+1)): Testing value $TEST_VALUE"
    run_test "Write SCR Template ($TEST_VALUE)" "./register_test_arm -p \"$PCIE_RESOURCE\" -a 0x102200 -v $TEST_VALUE -w $VERBOSE $DEBUG"
done

# Test 5: Read CLR template register
print_info "=== Test 5: Read CLR Template Register ==="
run_test "Read CLR Template" "./register_test_arm -p \"$PCIE_RESOURCE\" -a 0x103200 -r $VERBOSE $DEBUG"

# Test 6: Write and read back CLR template register
print_info "=== Test 6: Write/Read CLR Template Register ==="
TEST_VALUE="0x87654321"
run_test "Write CLR Template" "./register_test_arm -p \"$PCIE_RESOURCE\" -a 0x103200 -v $TEST_VALUE -w $VERBOSE $DEBUG"

# Test 7: Read some status registers
print_info "=== Test 7: Read Status Registers ==="
run_test "Read Fatal Error Register" "./register_test_arm -p \"$PCIE_RESOURCE\" -a 0x102004 -r $VERBOSE $DEBUG"
run_test "Read TX Rate Meter High" "./register_test_arm -p \"$PCIE_RESOURCE\" -a 0x102008 -r $VERBOSE $DEBUG"
run_test "Read TX Rate Meter Low" "./register_test_arm -p \"$PCIE_RESOURCE\" -a 0x10200C -r $VERBOSE $DEBUG"

# Test 8: Comprehensive test suite
print_info "=== Test 8: Comprehensive ARM-Optimized Tests ==="
run_test "Comprehensive Test Suite" "./register_test_arm -p \"$PCIE_RESOURCE\" -t $VERBOSE $DEBUG"

# Test 9: Edge case testing
print_info "=== Test 9: Edge Case Testing ==="
print_info "Testing boundary addresses..."

# Test some boundary addresses
BOUNDARY_ADDRESSES=("0x102000" "0x1020FC" "0x102200" "0x1022FC" "0x103000" "0x1030FC" "0x103200" "0x1032FC")

for addr in "${BOUNDARY_ADDRESSES[@]}"; do
    print_info "Testing boundary address $addr..."
    ./register_test_arm -p "$PCIE_RESOURCE" -a "$addr" -r $VERBOSE 2>/dev/null
    if [ $? -eq 0 ]; then
        print_success "Address $addr accessible"
    else
        print_warning "Address $addr not accessible (this may be normal)"
    fi
done

echo ""
echo "----------------------------------------"
echo ""

# Performance comparison
print_info "=== Test 10: ARM Performance Analysis ==="
print_info "Measuring register access performance on ARM..."

# Create a simple performance test
echo "Testing register access timing..."
time_output=$(./register_test_arm -p "$PCIE_RESOURCE" -a 0x102000 -r -V 2>&1 | grep "microseconds" || echo "No timing info available")
print_info "Register access timing: $time_output"

# Summary
print_info "=== ARM Test Summary ==="
print_success "All ARM-optimized register tests completed!"
print_info "Key ARM optimizations applied:"
print_info "  ✓ Memory barriers (dsb) for proper ordering"
print_info "  ✓ Volatile pointer access"
print_info "  ✓ Read-back verification for writes"
print_info "  ✓ Cache-aware register access"
echo ""

print_info "Check the output above for any warnings or errors."
echo ""
print_info "Useful ARM-specific commands for manual testing:"
echo "  List registers:     ./register_test_arm -l"
echo "  Read version:       ./register_test_arm -p $PCIE_RESOURCE -a 0x102000 -r"
echo "  Write template:     ./register_test_arm -p $PCIE_RESOURCE -a 0x102200 -v 0x12345678 -w"
echo "  Comprehensive:      ./register_test_arm -p $PCIE_RESOURCE -t -V"
echo "  Debug mode:         ./register_test_arm -p $PCIE_RESOURCE -a 0x102000 -r -g"
echo ""

print_info "ARM-specific debugging tips:"
echo "  1. Check 'dmesg' for PCIe-related messages"
echo "  2. Verify IOMMU configuration if applicable"
echo "  3. Check PCIe link status: lspci -vv -s 0005:01:00.0"
echo "  4. Monitor system performance: iostat, vmstat"
echo ""

print_info "For more options, run: ./register_test_arm -h"

# Check for common ARM-specific issues
print_info "=== ARM-Specific System Checks ==="

# Check if IOMMU is enabled
if [ -d "/sys/kernel/iommu_groups" ]; then
    iommu_groups=$(find /sys/kernel/iommu_groups -name "devices" -exec ls {} \; 2>/dev/null | wc -l)
    if [ $iommu_groups -gt 0 ]; then
        print_info "IOMMU is active with $iommu_groups device groups"
    else
        print_info "IOMMU directories exist but no device groups found"
    fi
else
    print_info "IOMMU not detected or not enabled"
fi

# Check PCIe link status
print_info "PCIe Link Status:"
if lspci -vv -s 0005:01:00.0 2>/dev/null | grep -E "(LnkSta|Width|Speed)" | head -3; then
    print_success "PCIe link information retrieved successfully"
else
    print_warning "Could not retrieve PCIe link information"
fi

print_success "ARM-optimized register testing complete!"