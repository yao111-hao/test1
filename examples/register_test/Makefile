#==============================================================================
# Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
# SPDX-License-Identifier: MIT
#
#==============================================================================
#
# Makefile for RecoNIC Register Test
#
#==============================================================================

CC ?= gcc

# Compiler flags
CFLAGS += -g -Wall -Wextra -std=c99
CFLAGS += -I../../lib
CFLAGS += -I.

# Linker flags - use static linking by default
LDFLAGS += -L../../lib
LDLIBS += -lreconic -static

# Executables and sources
EXECUTABLE = register_test
EXECUTABLE_ARM = register_test_arm
SOURCES = register_test.c
SOURCES_ARM = register_test_arm.c
OBJECTS = $(SOURCES:.c=.o)
OBJECTS_ARM = $(SOURCES_ARM:.c=.o)

# Default target - build both versions
all: $(EXECUTABLE) $(EXECUTABLE_ARM)

# Build the standard executable
$(EXECUTABLE): $(OBJECTS) ../../lib/libreconic.a
	$(CC) $(OBJECTS) $(LDFLAGS) $(LDLIBS) -o $@

# Build the ARM-optimized executable (standalone, no library dependency)
$(EXECUTABLE_ARM): $(OBJECTS_ARM)
	$(CC) $(OBJECTS_ARM) -o $@

# ARM version doesn't need the library
$(EXECUTABLE_ARM).o: $(SOURCES_ARM)
	$(CC) $(CFLAGS) -c $(SOURCES_ARM) -o $@

# Compile source files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Build the library if it doesn't exist
../../lib/libreconic.a:
	@echo "Building libreconic..."
	@$(MAKE) -C ../../lib

# Clean build artifacts
clean:
	rm -f $(OBJECTS) $(OBJECTS_ARM) $(EXECUTABLE) $(EXECUTABLE_ARM)

# Clean everything including library
distclean: clean
	@$(MAKE) -C ../../lib clean

# Install (copy to system location - optional)
install: $(EXECUTABLE)
	@echo "Installing $(EXECUTABLE) to /usr/local/bin/"
	@sudo cp $(EXECUTABLE) /usr/local/bin/
	@sudo chmod +x /usr/local/bin/$(EXECUTABLE)

# Uninstall
uninstall:
	@echo "Removing $(EXECUTABLE) from /usr/local/bin/"
	@sudo rm -f /usr/local/bin/$(EXECUTABLE)

# Help target
help:
	@echo "Available targets:"
	@echo "  all              - Build both register test programs (default)"
	@echo "  register_test    - Build standard version (requires libreconic)"
	@echo "  register_test_arm- Build ARM-optimized version (standalone)"
	@echo "  clean            - Remove build artifacts"
	@echo "  distclean        - Remove build artifacts and clean library"
	@echo "  install          - Install to /usr/local/bin (requires sudo)"
	@echo "  uninstall        - Remove from /usr/local/bin (requires sudo)"
	@echo "  help             - Show this help message"
	@echo ""
	@echo "Available programs:"
	@echo "  register_test     - Standard version for x86/general use"
	@echo "  register_test_arm - ARM-optimized for NVIDIA Jetson/ARM platforms"
	@echo ""
	@echo "Usage examples:"
	@echo "  make                      # Build both versions"
	@echo "  make register_test_arm    # Build ARM-optimized version only"
	@echo "  make clean               # Clean build files"
	@echo "  sudo make install        # Install system-wide"
	@echo ""
	@echo "For ARM platforms (NVIDIA Jetson):"
	@echo "  ./register_test_arm -p /sys/bus/pci/devices/0005:01:00.0/resource2 -t -V"
	@echo "  ./test_registers_arm.sh -v"

.PHONY: all clean distclean install uninstall help