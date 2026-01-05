# Makefile for DRACHMA Blockchain
# This is a convenience wrapper around CMake, similar to Bitcoin Core's build system

# Default prefix for installation (can be overridden)
PREFIX ?= /usr/local
BUILD_DIR ?= build
CMAKE_BUILD_TYPE ?= Release

# Default target
.DEFAULT_GOAL := all

# Phony targets
.PHONY: all install clean help test check distclean uninstall

all:
	@echo "Building DRACHMA binaries..."
	@mkdir -p $(BUILD_DIR)
	cmake -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=$(CMAKE_BUILD_TYPE) -DCMAKE_INSTALL_PREFIX=$(PREFIX)
	cmake --build $(BUILD_DIR) --parallel

install:
	@echo "Installing DRACHMA to $(PREFIX)..."
	cmake --build $(BUILD_DIR) --target install

clean:
	@echo "Cleaning build directory..."
	@if [ -d "$(BUILD_DIR)" ]; then \
		cmake --build $(BUILD_DIR) --target clean 2>/dev/null || true; \
	fi

distclean:
	@echo "Removing build directory..."
	@rm -rf $(BUILD_DIR)

test: check

check:
	@echo "Running tests..."
	@if [ ! -d "$(BUILD_DIR)" ]; then \
		echo "Build directory not found. Run 'make' first."; \
		exit 1; \
	fi
	ctest --test-dir $(BUILD_DIR) --output-on-failure

uninstall:
	@echo "Uninstalling DRACHMA from $(PREFIX)..."
	@if [ -f "$(BUILD_DIR)/install_manifest.txt" ]; then \
		xargs rm -f < $(BUILD_DIR)/install_manifest.txt; \
		echo "Uninstall complete."; \
	else \
		echo "No install manifest found. Cannot uninstall."; \
		exit 1; \
	fi

help:
	@echo "DRACHMA Blockchain Build System"
	@echo ""
	@echo "Usage:"
	@echo "  make [target] [OPTION=value]"
	@echo ""
	@echo "Targets:"
	@echo "  all         Build all binaries (default)"
	@echo "  install     Install binaries and files to PREFIX"
	@echo "  clean       Clean build artifacts"
	@echo "  distclean   Remove build directory entirely"
	@echo "  test        Run test suite"
	@echo "  check       Alias for 'test'"
	@echo "  uninstall   Remove installed files"
	@echo "  help        Show this help message"
	@echo ""
	@echo "Options:"
	@echo "  PREFIX=<path>          Installation prefix (default: /usr/local)"
	@echo "  BUILD_DIR=<path>       Build directory (default: build)"
	@echo "  CMAKE_BUILD_TYPE=<type> Build type: Release, Debug, RelWithDebInfo (default: Release)"
	@echo ""
	@echo "Examples:"
	@echo "  make                   Build with default settings"
	@echo "  make PREFIX=/opt/drachma install"
	@echo "  make BUILD_DIR=mybuild all"
	@echo "  make test"
