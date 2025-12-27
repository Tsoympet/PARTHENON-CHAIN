#!/bin/bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"
BUILD_TYPE="Debug"

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

cmake -DCMAKE_BUILD_TYPE="$BUILD_TYPE" -DDRACHMA_BUILD_TESTS=ON -DDRACHMA_BUILD_GUI=OFF ..
cmake --build . -- -j"$(nproc || echo 2)"
ctest --output-on-failure
