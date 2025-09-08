#!/bin/bash
#==============================================================================
# Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
# SPDX-License-Identifier: MIT
#
#==============================================================================
#
# RecoNIC Register Test Script
# This script demonstrates various register read/write operations
#
#==============================================================================

# Default values
DEVICE="/dev/reconic-mm"
PCIE_RESOURCE="/sys/bus/pci/devices/0000:d8:00.0/resource2"
VERBOSE=""
DEBUG=""

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
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

# Function to show usage
usage() {
    echo "Usage: $0 [OPTIONS]"
    echo ""
    echo "Options:"
    echo "  -d DEVICE        Device file (default: $DEVICE)"
    echo "  -p PCIE_RESOURCE PCIe resource file (default: $PCIE_RESOURCE)"
    echo "  -v               Verbose output"
    echo "  -g               Debug mode"
    echo "  -h               Show this help"
    echo ""
    echo "Examples:"
    echo "  $0                                    # Run basic tests"
    echo "  $0 -v                                # Run with verbose output"
    echo "  $0 -p /sys/bus/pci/devices/0000:d8:00.0/resource2 -v"
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

# Check if the program exists
if [ ! -f "./register_test" ]; then
    print_error "register_test executable not found. Please run 'make' first."
    exit 1
fi

# Check if PCIe resource file exists
if [ ! -f "$PCIE_RESOURCE" ]; then
    print_error "PCIe resource file not found: $PCIE_RESOURCE"
    print_info "Please check your PCIe device path with 'lspci | grep Xilinx'"
    exit 1
fi

# Check if device file exists
if [ ! -c "$DEVICE" ]; then
    print_warning "Device file not found: $DEVICE"
    print_info "Register tests will continue (device file only needed for DMA operations)"
fi

print_info "Starting RecoNIC Register Tests"
print_info "Device: $DEVICE"
print_info "PCIe Resource: $PCIE_RESOURCE"
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

# Test 1: List predefined registers
print_info "=== Test 1: List Predefined Registers ==="
run_test "List Registers" "./register_test -l"

# Test 2: Read version register (should always work)
print_info "=== Test 2: Read Version Register ==="
run_test "Read Version Register" "./register_test -p \"$PCIE_RESOURCE\" -a 0x102000 -r $VERBOSE $DEBUG"

# Test 3: Read template register in SCR space
print_info "=== Test 3: Read SCR Template Register ==="
run_test "Read SCR Template" "./register_test -p \"$PCIE_RESOURCE\" -a 0x102200 -r $VERBOSE $DEBUG"

# Test 4: Write and read back template register in SCR space
print_info "=== Test 4: Write/Read SCR Template Register ==="
TEST_VALUE="0x12345678"
run_test "Write SCR Template" "./register_test -p \"$PCIE_RESOURCE\" -a 0x102200 -v $TEST_VALUE -w $VERBOSE $DEBUG"

# Test 5: Read CLR template register
print_info "=== Test 5: Read CLR Template Register ==="
run_test "Read CLR Template" "./register_test -p \"$PCIE_RESOURCE\" -a 0x103200 -r $VERBOSE $DEBUG"

# Test 6: Write and read back CLR template register
print_info "=== Test 6: Write/Read CLR Template Register ==="
TEST_VALUE="0xDEADBEEF"
run_test "Write CLR Template" "./register_test -p \"$PCIE_RESOURCE\" -a 0x103200 -v $TEST_VALUE -w $VERBOSE $DEBUG"

# Test 7: Read some RDMA registers (these might be read-only)
print_info "=== Test 7: Read RDMA Configuration Register ==="
run_test "Read RDMA Config" "./register_test -p \"$PCIE_RESOURCE\" -a 0x60000 -r $VERBOSE $DEBUG"

# Test 8: Test error handling with invalid address
print_info "=== Test 8: Test with High Address (Boundary Test) ==="
run_test "Boundary Address Test" "./register_test -p \"$PCIE_RESOURCE\" -a 0x3FFFFF0 -r $VERBOSE $DEBUG"

# Test 9: Sequential register read test
print_info "=== Test 9: Sequential Register Reads ==="
print_info "Reading multiple consecutive registers..."

ADDRESSES=("0x102000" "0x102004" "0x102008" "0x10200C" "0x102010")
for addr in "${ADDRESSES[@]}"; do
    print_info "Reading address $addr..."
    ./register_test -p "$PCIE_RESOURCE" -a "$addr" -r $VERBOSE
    echo ""
done

print_success "Sequential register test completed"
echo ""
echo "----------------------------------------"
echo ""

# Summary
print_info "=== Test Summary ==="
print_success "All register tests completed!"
print_info "Check the output above for any warnings or errors."
echo ""
print_info "Useful commands for manual testing:"
echo "  List registers:     ./register_test -l"
echo "  Read version:       ./register_test -p $PCIE_RESOURCE -a 0x102000 -r"
echo "  Write template:     ./register_test -p $PCIE_RESOURCE -a 0x102200 -v 0x12345678 -w"
echo "  Debug mode:         ./register_test -p $PCIE_RESOURCE -a 0x102000 -r -g"
echo ""
print_info "For more options, run: ./register_test -h"
