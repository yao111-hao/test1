# RecoNIC Register Test

This directory contains a comprehensive register test tool for the RecoNIC RDMA smart NIC. The tool allows reading and writing arbitrary registers in the PCIe BAR space using the RecoNIC control API.

## Overview

The register test tool provides the following functionality:

- **Register Reading**: Read any register in the RecoNIC PCIe BAR space
- **Register Writing**: Write values to writable registers
- **Predefined Register List**: Display commonly used register addresses
- **Verification**: Read-back verification for write operations
- **Debug Support**: Verbose and debug output modes

## Files

- `register_test.c` - Main test program source code
- `Makefile` - Build configuration
- `test_registers.sh` - Automated test script
- `README.md` - This documentation

## Building

### Prerequisites

1. RecoNIC library must be built first:
```bash
cd ../../lib
make
```

2. Ensure you have appropriate permissions to access PCIe resources.

### Compilation

```bash
# Build the register test program
make

# Clean build files
make clean

# Clean everything including library
make distclean
```

## Usage

### Basic Usage

```bash
# Show help
./register_test -h

# List predefined register addresses
./register_test -l

# Read a register
./register_test -p /sys/bus/pci/devices/0000:d8:00.0/resource2 -a 0x102000 -r

# Write to a register
./register_test -p /sys/bus/pci/devices/0000:d8:00.0/resource2 -a 0x102200 -v 0x12345678 -w

# Verbose mode
./register_test -p /sys/bus/pci/devices/0000:d8:00.0/resource2 -a 0x102000 -r -V

# Debug mode
./register_test -p /sys/bus/pci/devices/0000:d8:00.0/resource2 -a 0x102000 -r -g
```

### Command Line Options

| Option | Description |
|--------|-------------|
| `-d, --device` | Character device name (default: `/dev/reconic-mm`) |
| `-p, --pcie_resource` | PCIe resource file path |
| `-a, --address` | Register address offset (hex format) |
| `-v, --value` | Value to write (hex format) |
| `-r, --read` | Perform read operation |
| `-w, --write` | Perform write operation |
| `-l, --list` | List predefined register addresses |
| `-V, --verbose` | Enable verbose output |
| `-g, --debug` | Enable debug mode |
| `-h, --help` | Show help message |

## Register Address Spaces

The RecoNIC contains several register address spaces:

### Statistics and Configuration Registers (SCR): 0x102000 - 0x102FFF
- **RN_SCR_VERSION** (0x102000): Version register (Read-only)
- **RN_SCR_FATAL_ERR** (0x102004): Fatal error register (Read-only)  
- **RN_SCR_TEMPLATE_REG** (0x102200): Template register (Read/Write)

### Compute Logic Registers (CLR): 0x103000 - 0x103FFF
- **RN_CLR_CTL_CMD** (0x103000): Control command register
- **RN_CLR_KER_STS** (0x103004): Kernel status register
- **RN_CLR_TEMPLATE** (0x103200): Template register (Read/Write)

### RDMA Global Control Status Registers (GCSR): 0x60000 - 0x8FFFF
- **RN_RDMA_GCSR_XRNICCONF** (0x60000): XRNIC configuration
- **RN_RDMA_GCSR_MACXADDLSB** (0x60010): MAC address LSB
- **RN_RDMA_GCSR_IPV4XADD** (0x60070): IPv4 address

## Automated Testing

Use the provided test script for comprehensive testing:

```bash
# Run all tests with default settings
./test_registers.sh

# Run tests with verbose output
./test_registers.sh -v

# Run tests with debug output
./test_registers.sh -g

# Run tests with custom PCIe resource path
./test_registers.sh -p /sys/bus/pci/devices/0000:d8:00.0/resource2 -v
```

### Test Script Options

| Option | Description |
|--------|-------------|
| `-d DEVICE` | Device file path |
| `-p PCIE_RESOURCE` | PCIe resource file path |
| `-v` | Verbose output |
| `-g` | Debug mode |
| `-h` | Show help |

## Examples

### Example 1: Read Version Register
```bash
sudo ./register_test -p /sys/bus/pci/devices/0000:d8:00.0/resource2 -a 0x102000 -r
```

Expected output:
```
=== RecoNIC Register Test ===
Register Read Result:
  Address: 0x00102000
  Value  : 0x12345678 (305419896)

=== Test Complete ===
```

### Example 2: Write and Verify Template Register
```bash
sudo ./register_test -p /sys/bus/pci/devices/0000:d8:00.0/resource2 -a 0x102200 -v 0xDEADBEEF -w
```

Expected output:
```
=== RecoNIC Register Test ===
Register Write Result:
  Address    : 0x00102200
  Written    : 0xDEADBEEF (3735928559)
  Read back  : 0xDEADBEEF (3735928559)
  Status     : SUCCESS - Values match

=== Test Complete ===
```

### Example 3: List All Predefined Registers
```bash
./register_test -l
```

This will display all predefined register addresses with descriptions.

## Troubleshooting

### Common Issues

1. **Permission Denied**
   ```
   Error: Unable to open PCIe resource file
   ```
   **Solution**: Run with `sudo` or ensure proper permissions.

2. **PCIe Resource Not Found**
   ```
   Error: PCIe resource file not found
   ```
   **Solution**: Check PCIe device path with `lspci | grep Xilinx`

3. **Memory Mapping Failed**
   ```
   Error: Failed to mmap PCIe resource
   ```
   **Solution**: Ensure the FPGA is properly configured and accessible.

4. **Values Don't Match After Write**
   ```
   Status: WARNING - Values don't match
   ```
   **Solution**: This is normal for read-only registers or registers with special behavior.

### Finding Your PCIe Device

```bash
# Find Xilinx devices
lspci | grep Xilinx

# Get detailed information
sudo lspci -vv -s d8:00.0

# Check NUMA node (for performance optimization)
sudo lspci -vv -s d8:00.0 | grep 'NUMA node'
```

### Checking Permissions

```bash
# Check PCIe resource permissions
ls -la /sys/bus/pci/devices/0000:d8:00.0/resource*

# Check device file permissions
ls -la /dev/reconic-mm
```

## Performance Considerations

For optimal performance, especially when running many register operations:

1. **CPU Affinity**: Bind the process to CPUs on the same NUMA node as the PCIe device
2. **Batch Operations**: Use the test script for multiple operations
3. **Debug Mode**: Disable debug output for performance testing

```bash
# Example with CPU affinity
taskset -c 1,3,5,7 ./register_test -p /sys/bus/pci/devices/0000:d8:00.0/resource2 -a 0x102000 -r
```

## Integration with Other Tests

This register test can be combined with other RecoNIC tests:

```bash
# Run DMA test first
cd ../dma_test
make && sudo ./dma_test -d /dev/reconic-mm

# Then run register test
cd ../register_test
make && sudo ./register_test -p /sys/bus/pci/devices/0000:d8:00.0/resource2 -a 0x102000 -r

# Finally run RDMA tests
cd ../rdma_test
make && sudo ./read -r 192.168.1.1 -i 192.168.1.2 -p /sys/bus/pci/devices/0000:d8:00.0/resource2
```

## Safety Notes

⚠️ **Important Safety Information**:

1. **Read-Only Registers**: Some registers are read-only. Writing to them may have no effect or could cause unexpected behavior.

2. **System Registers**: Be cautious when writing to system configuration registers. Incorrect values could affect system stability.

3. **Concurrent Access**: Avoid concurrent access to the same registers from multiple processes.

4. **Hardware State**: Ensure the FPGA is in a proper state before accessing registers.

## Support

For issues and questions:

1. Check the main RecoNIC documentation
2. Verify PCIe device setup
3. Ensure proper driver installation
4. Review system logs for error messages
