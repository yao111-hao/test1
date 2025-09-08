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

# Executable and sources
EXECUTABLE = register_test
SOURCES = register_test.c
OBJECTS = $(SOURCES:.c=.o)

# Default target
all: $(EXECUTABLE)

# Build the executable
$(EXECUTABLE): $(OBJECTS) ../../lib/libreconic.a
	$(CC) $(OBJECTS) $(LDFLAGS) $(LDLIBS) -o $@

# Compile source files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Build the library if it doesn't exist
../../lib/libreconic.a:
	@echo "Building libreconic..."
	@$(MAKE) -C ../../lib

# Clean build artifacts
clean:
	rm -f $(OBJECTS) $(EXECUTABLE)

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
	@echo "  all        - Build the register test program (default)"
	@echo "  clean      - Remove build artifacts"
	@echo "  distclean  - Remove build artifacts and clean library"
	@echo "  install    - Install to /usr/local/bin (requires sudo)"
	@echo "  uninstall  - Remove from /usr/local/bin (requires sudo)"
	@echo "  help       - Show this help message"
	@echo ""
	@echo "Usage examples:"
	@echo "  make                    # Build the program"
	@echo "  make clean             # Clean build files"
	@echo "  sudo make install      # Install system-wide"

.PHONY: all clean distclean install uninstall help
