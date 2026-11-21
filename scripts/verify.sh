#!/usr/bin/env bash
# ==============================================================================
# Deterministic Verification Script: Enterprise C++ Sanitizer & Test Gate
# 
# Design Principle:
# Never allow the LLM to hallucinate or guess compiler flags, include paths,
# or sanitizer settings. The agent executes this deterministic script and
# receives machine-checked ground truth.
# ==============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"

echo "=== [1/3] Configuring CMake with AddressSanitizer & C++17 ==="
cmake -B "${BUILD_DIR}" -S "${ROOT_DIR}" \
    -DCMAKE_BUILD_TYPE=Debug \
    -DENABLE_ASAN=ON \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

echo "=== [2/3] Building Test Targets ==="
cmake --build "${BUILD_DIR}" --config Debug -j4

echo "=== [3/3] Executing CTest with Sanitizers Active ==="
export ASAN_OPTIONS="detect_leaks=1:halt_on_error=1:abort_on_error=1:symbolize=1"
export UBSAN_OPTIONS="halt_on_error=1:abort_on_error=1"

cd "${BUILD_DIR}"
ctest --output-on-failure -C Debug

echo ">>> VERIFICATION PASSED: Build, tests, and AddressSanitizer are 100% CLEAN. <<<"
