#!/bin/bash
set -e

BUILD_TYPE="${1:-Debug}"

if [[ "$BUILD_TYPE" != "Debug" && "$BUILD_TYPE" != "Release" ]]; then
    echo "Usage: $0 [Debug|Release]"
    exit 1
fi

BUILD_DIR="build"

echo "==> Clean build ($BUILD_TYPE)"
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"

export PATH="/opt/gcc-arm-11.2-2022.02-x86_64-arm-none-eabi/bin:$PATH"

cmake -B "$BUILD_DIR" \
    -DCMAKE_TOOLCHAIN_FILE=cmake/gcc-arm-none-eabi.cmake \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
cmake --build "$BUILD_DIR" -j"$(nproc)"

echo "==> Done"
