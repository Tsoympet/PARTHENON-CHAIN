#!/bin/bash
# Build and Test Verification Script for DRACHMA
# This script documents and verifies the build and test process
# Part of Enhanced Documentation recommendations (LAUNCH-ACTION-ITEMS.md)

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

BUILD_DIR="${BUILD_DIR:-build}"
VERBOSE="${VERBOSE:-0}"

echo "================================================"
echo "DRACHMA Build & Test Verification"
echo "================================================"
echo ""
echo "This script verifies that:"
echo "  1. Build system is configured correctly"
echo "  2. Code compiles without errors"
echo "  3. All tests pass"
echo "  4. Supply cap constants are correct"
echo ""

# Function to print step header
step() {
    echo ""
    echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo -e "${BLUE}▶ $1${NC}"
    echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
}

# Step 1: Check dependencies
step "Step 1/5: Checking build dependencies"

check_command() {
    if command -v "$1" &> /dev/null; then
        echo -e "${GREEN}✓${NC} $1 found"
        return 0
    else
        echo -e "${RED}✗${NC} $1 not found"
        return 1
    fi
}

deps_ok=true
check_command "cmake" || deps_ok=false
check_command "make" || deps_ok=false
check_command "g++" || check_command "clang++" || deps_ok=false

if [ "$deps_ok" = false ]; then
    echo ""
    echo -e "${RED}Missing required dependencies!${NC}"
    echo "Please install: cmake, make, g++/clang++"
    echo "See docs/getting-started/building.md for details"
    exit 1
fi

# Step 2: Configure build
step "Step 2/5: Configuring build with CMake"

cd "$REPO_ROOT"

if [ -d "$BUILD_DIR" ]; then
    echo "Build directory exists, reconfiguring..."
else
    echo "Creating build directory..."
    mkdir -p "$BUILD_DIR"
fi

if [ "$VERBOSE" = "1" ]; then
    cmake -S . -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release
else
    cmake -S . -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release > /dev/null 2>&1
fi

echo -e "${GREEN}✓${NC} CMake configuration successful"

# Step 3: Build
step "Step 3/5: Building project"

echo "Building with parallel jobs..."
if [ "$VERBOSE" = "1" ]; then
    cmake --build "$BUILD_DIR" --parallel
else
    cmake --build "$BUILD_DIR" --parallel > /dev/null 2>&1
fi

echo -e "${GREEN}✓${NC} Build successful"

# Step 4: Run tests
step "Step 4/5: Running tests"

echo "Running test suite..."
if [ "$VERBOSE" = "1" ]; then
    ctest --test-dir "$BUILD_DIR" --output-on-failure
else
    test_output=$(ctest --test-dir "$BUILD_DIR" --output-on-failure 2>&1)
    test_result=$?
    
    # Extract summary
    passed=$(echo "$test_output" | grep "tests passed" | grep -oP '\d+(?= tests passed)' || echo "?")
    total=$(echo "$test_output" | grep "tests passed" | grep -oP '(?<=out of )\d+' || echo "?")
    
    if [ $test_result -eq 0 ]; then
        echo -e "${GREEN}✓${NC} All tests passed ($passed/$total)"
    else
        echo -e "${RED}✗${NC} Some tests failed"
        echo "$test_output"
        exit 1
    fi
fi

# Step 5: Verify supply constants
step "Step 5/5: Verifying supply cap constants in code"

echo "Checking layer1-core/consensus/params.cpp..."

# Check DRM supply cap
if grep -q "41000000ULL \* COIN" "$REPO_ROOT/layer1-core/consensus/params.cpp"; then
    echo -e "${GREEN}✓${NC} DRM max supply: 41,000,000 (correct)"
else
    echo -e "${RED}✗${NC} DRM max supply not found or incorrect"
    exit 1
fi

# Check TLN supply cap
if grep -q "21000000ULL \* COIN" "$REPO_ROOT/layer1-core/consensus/params.cpp"; then
    echo -e "${GREEN}✓${NC} TLN max supply: 21,000,000 (correct)"
else
    echo -e "${RED}✗${NC} TLN max supply not found or incorrect"
    exit 1
fi

# Check OBL supply cap
if grep -q "61000000ULL \* COIN" "$REPO_ROOT/layer1-core/consensus/params.cpp"; then
    echo -e "${GREEN}✓${NC} OBL max supply: 61,000,000 (correct)"
else
    echo -e "${RED}✗${NC} OBL max supply not found or incorrect"
    exit 1
fi

# Check halving interval
if grep -q "2102400" "$REPO_ROOT/layer1-core/consensus/params.cpp"; then
    echo -e "${GREEN}✓${NC} Halving interval: 2,102,400 blocks (~4 years)"
else
    echo -e "${RED}✗${NC} Halving interval not found or incorrect"
    exit 1
fi

# Summary
echo ""
echo "================================================"
echo "VERIFICATION SUMMARY"
echo "================================================"
echo -e "${GREEN}✓ ALL CHECKS PASSED${NC}"
echo ""
echo "Build system status:"
echo "  ✓ Dependencies installed"
echo "  ✓ CMake configuration successful"
echo "  ✓ Build completed without errors"
echo "  ✓ All tests passing"
echo "  ✓ Supply constants verified"
echo ""
echo "The project is ready for:"
echo "  - Local testing and development"
echo "  - Integration testing"
echo "  - Further security review"
echo ""
echo "Next steps:"
echo "  - Run extended validation: ./scripts/verify-genesis.sh"
echo "  - Review LAUNCH-ACTION-ITEMS.md for pre-launch tasks"
echo "  - Consider running: make install (requires sudo)"
echo ""
