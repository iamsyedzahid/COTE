#!/usr/bin/env bash
set -e

echo "=== Cloud Task Orchestration Engine — Build Script ==="

if ! command -v cmake &>/dev/null; then
    echo "ERROR: cmake not found. Install cmake >= 3.16"
    exit 1
fi

BUILD_TYPE="${1:-Release}"
BUILD_DIR="build"

echo "[1/3] Configuring ($BUILD_TYPE)..."
cmake -S . -B "$BUILD_DIR" \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

echo "[2/3] Building..."
cmake --build "$BUILD_DIR" --parallel "$(nproc 2>/dev/null || sysctl -n hw.logicalcpu)"

echo "[3/3] Done!"
echo ""
echo "Run with:  ./$BUILD_DIR/orchestrator"
