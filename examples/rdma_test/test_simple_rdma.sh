#!/bin/bash
#==============================================================================
# Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
# SPDX-License-Identifier: MIT
#
#==============================================================================
#
# Simple RDMA Read Test Script
# Provides easy testing and demonstration of the simplified RDMA read functionality
#
#==============================================================================

# Default configuration
SERVER_IP="192.168.1.100"
CLIENT_IP="192.168.1.101"
PCIE_RESOURCE="/sys/bus/pci/devices/0005:01:00.0/resource2"
DEVICE="/dev/reconic-mm"
PAYLOAD_SIZE="1024"
QP_LOCATION="host_mem"
VERBOSE=""
DEBUG=""

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
PURPLE='\033[0;35m'
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

print_step() {
    echo -e "${PURPLE}[STEP]${NC} $1"
}

# Function to show usage
usage() {
    echo "Usage: $0 [OPTIONS] MODE"
    echo ""
    echo "Simple RDMA Read Test Script for RecoNIC"
    echo ""
    echo "Modes:"
    echo "  server         Run as RDMA server (data provider)"
    echo "  client         Run as RDMA client (data reader)"
    echo "  demo           Run automated demo (requires two terminals)"
    echo "  test           Run comprehensive tests"
    echo ""
    echo "Options:"
    echo "  -s SERVER_IP   Server IP address (default: $SERVER_IP)"
    echo "  -c CLIENT_IP   Client IP address (default: $CLIENT_IP)"
    echo "  -p PCIE_PATH   PCIe resource path (default: $PCIE_RESOURCE)"
    echo "  -d DEVICE      Device file (default: $DEVICE)"
    echo "  -z SIZE        Payload size in bytes (default: $PAYLOAD_SIZE)"
    echo "  -l LOCATION    QP location: host_mem|dev_mem (default: $QP_LOCATION)"
    echo "  -v             Verbose output"
    echo "  -g             Debug mode"
    echo "  -h             Show this help"
    echo ""
    echo "Examples:"
    echo "  # Terminal 1 (Server):"
    echo "  $0 -s 192.168.1.100 -c 192.168.1.101 -v server"
    echo ""
    echo "  # Terminal 2 (Client):"
    echo "  $0 -s 192.168.1.100 -c 192.168.1.101 -v client"
    echo ""
    echo "  # Run comprehensive tests:"
    echo "  $0 test"
    echo ""
}

# Parse command line arguments
while getopts "s:c:p:d:z:l:vgh" opt; do
    case $opt in
        s)
            SERVER_IP="$OPTARG"
            ;;
        c)
            CLIENT_IP="$OPTARG"
            ;;
        p)
            PCIE_RESOURCE="$OPTARG"
            ;;
        d)
            DEVICE="$OPTARG"
            ;;
        z)
            PAYLOAD_SIZE="$OPTARG"
            ;;
        l)
            if [[ "$OPTARG" == "host_mem" || "$OPTARG" == "dev_mem" ]]; then
                QP_LOCATION="$OPTARG"
            else
                print_error "Invalid QP location. Use 'host_mem' or 'dev_mem'"
                exit 1
            fi
            ;;
        v)
            VERBOSE="-v"
            ;;
        g)
            DEBUG="-g"
            VERBOSE="-v"
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

shift $((OPTIND-1))
MODE="$1"

if [ -z "$MODE" ]; then
    print_error "Mode is required"
    usage
    exit 1
fi

# Check if the simple_read program exists
if [ ! -f "./simple_read" ]; then
    print_error "simple_read executable not found. Building it now..."
    if ! make -f Makefile_simple; then
        print_error "Failed to build simple_read program"
        exit 1
    fi
    print_success "Built simple_read program successfully"
fi

# System checks
check_system() {
    print_step "Performing system checks..."
    
    # Check if running as root
    if [ "$EUID" -ne 0 ]; then
        print_warning "Not running as root. You may need sudo for device access."
    fi
    
    # Check PCIe resource file
    if [ ! -f "$PCIE_RESOURCE" ]; then
        print_error "PCIe resource file not found: $PCIE_RESOURCE"
        print_info "Available PCIe devices:"
        lspci | grep -i xilinx || print_warning "No Xilinx devices found"
        return 1
    fi
    
    # Check device file
    if [ ! -c "$DEVICE" ]; then
        print_warning "Device file not found: $DEVICE"
        print_info "This may be normal if the driver is not loaded"
    fi
    
    # Check network connectivity
    if ! ping -c 1 -W 1 "$SERVER_IP" >/dev/null 2>&1; then
        print_warning "Cannot ping server IP $SERVER_IP"
    fi
    
    if ! ping -c 1 -W 1 "$CLIENT_IP" >/dev/null 2>&1; then
        print_warning "Cannot ping client IP $CLIENT_IP"
    fi
    
    print_success "System checks completed"
    return 0
}

# Run server mode
run_server() {
    print_step "Starting RDMA Server Mode"
    print_info "Server IP: $SERVER_IP"
    print_info "Client IP: $CLIENT_IP"
    print_info "Payload Size: $PAYLOAD_SIZE bytes"
    print_info "QP Location: $QP_LOCATION"
    echo ""
    
    CMD="./simple_read -r $SERVER_IP -i $CLIENT_IP -p $PCIE_RESOURCE -d $DEVICE -z $PAYLOAD_SIZE -l $QP_LOCATION -s $VERBOSE $DEBUG"
    
    print_info "Executing: $CMD"
    echo ""
    
    exec $CMD
}

# Run client mode
run_client() {
    print_step "Starting RDMA Client Mode"
    print_info "Server IP: $SERVER_IP"
    print_info "Client IP: $CLIENT_IP"
    print_info "Payload Size: $PAYLOAD_SIZE bytes"
    print_info "QP Location: $QP_LOCATION"
    echo ""
    
    CMD="./simple_read -r $CLIENT_IP -i $SERVER_IP -p $PCIE_RESOURCE -d $DEVICE -z $PAYLOAD_SIZE -l $QP_LOCATION -c $VERBOSE $DEBUG"
    
    print_info "Executing: $CMD"
    echo ""
    
    exec $CMD
}

# Run demo mode
run_demo() {
    print_step "RDMA Read Demo Mode"
    echo ""
    print_info "This demo shows how to run the RDMA read test."
    print_info "You need to run server and client in separate terminals."
    echo ""
    
    print_step "Step 1: Start the server (in terminal 1):"
    echo "  $0 -s $SERVER_IP -c $CLIENT_IP -v server"
    echo ""
    
    print_step "Step 2: Start the client (in terminal 2):"
    echo "  $0 -s $SERVER_IP -c $CLIENT_IP -v client"
    echo ""
    
    print_step "Expected flow:"
    print_info "1. Server starts and waits for client connection"
    print_info "2. Client connects and receives buffer address from server"
    print_info "3. Client performs RDMA read operation"
    print_info "4. Client verifies received data"
    print_info "5. Performance metrics are displayed"
    echo ""
    
    print_step "Configuration used:"
    print_info "Server IP: $SERVER_IP"
    print_info "Client IP: $CLIENT_IP"
    print_info "PCIe Resource: $PCIE_RESOURCE"
    print_info "Device: $DEVICE"
    print_info "Payload Size: $PAYLOAD_SIZE bytes"
    print_info "QP Location: $QP_LOCATION"
}

# Run comprehensive tests
run_tests() {
    print_step "Running Comprehensive RDMA Tests"
    
    if ! check_system; then
        print_error "System checks failed. Please fix the issues and try again."
        return 1
    fi
    
    # Test different payload sizes
    PAYLOAD_SIZES=("64" "256" "1024" "4096" "16384")
    QP_LOCATIONS=("host_mem" "dev_mem")
    
    print_step "Test Configuration:"
    print_info "Server IP: $SERVER_IP"
    print_info "Client IP: $CLIENT_IP"
    print_info "Payload sizes: ${PAYLOAD_SIZES[*]}"
    print_info "QP locations: ${QP_LOCATIONS[*]}"
    echo ""
    
    print_warning "NOTE: These tests require manual coordination between server and client."
    print_warning "This script will show you the commands to run in separate terminals."
    echo ""
    
    for qp_loc in "${QP_LOCATIONS[@]}"; do
        for size in "${PAYLOAD_SIZES[@]}"; do
            print_step "Test: Payload=$size bytes, QP Location=$qp_loc"
            echo ""
            print_info "Terminal 1 (Server):"
            echo "  ./simple_read -r $SERVER_IP -i $CLIENT_IP -p $PCIE_RESOURCE -z $size -l $qp_loc -s -v"
            echo ""
            print_info "Terminal 2 (Client):"
            echo "  ./simple_read -r $CLIENT_IP -i $SERVER_IP -p $PCIE_RESOURCE -z $size -l $qp_loc -c -v"
            echo ""
            echo "Press Enter to continue to next test..."
            read
        done
    done
    
    print_success "All test configurations displayed"
}

# Main execution
print_info "Simple RDMA Read Test Script"
print_info "============================"
echo ""

case "$MODE" in
    "server")
        check_system && run_server
        ;;
    "client")
        check_system && run_client
        ;;
    "demo")
        run_demo
        ;;
    "test")
        run_tests
        ;;
    *)
        print_error "Invalid mode: $MODE"
        usage
        exit 1
        ;;
esac