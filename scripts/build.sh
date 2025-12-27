#!/bin/bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"
BUILD_TYPE="Release"
GENERATOR="Unix Makefiles"

usage() {
  cat <<USAGE
Usage: $0 [-d] [-g <generator>]
  -d                Build in Debug mode (default Release)
  -g <generator>    CMake generator (default: ${GENERATOR})

The script creates a local ./build directory, configures CMake,
then compiles. It is deterministic and fails fast on errors.
USAGE
}

while getopts "dg:" opt; do
  case "$opt" in
    d) BUILD_TYPE="Debug" ;;
    g) GENERATOR="$OPTARG" ;;
    *) usage; exit 1 ;;
  esac
done

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

cmake -G "$GENERATOR" -DCMAKE_BUILD_TYPE="$BUILD_TYPE" ..
cmake --build . -- -j"$(nproc || echo 2)"

cat <<INFO
Build complete.
Artifacts (if targets are defined) reside in: $BUILD_DIR
INFO
