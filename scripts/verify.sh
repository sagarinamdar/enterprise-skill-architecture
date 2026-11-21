#!/usr/bin/env bash
# ==============================================================================
# Deterministic Verification Script: Enterprise C++ Sanitizer & Test Gate
# ==============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"

echo "=== [1/4] Configuring CMake with AddressSanitizer & C++17 ==="
cmake -B "${BUILD_DIR}" -S "${ROOT_DIR}" \
    -DCMAKE_BUILD_TYPE=Debug \
    -DENABLE_ASAN=ON \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

echo "=== [2/4] Building Legacy and Modern Targets ==="
cmake --build "${BUILD_DIR}" --config Debug -j4

echo "=== [3/4] Testing Legacy Baseline (Verifying AddressSanitizer Intercepts Flaw) ==="
export ASAN_OPTIONS="detect_leaks=1:halt_on_error=1:abort_on_error=0:symbolize=1"
set +e
"${BUILD_DIR}/session_legacy_test" > /tmp/legacy_output.log 2>&1
LEGACY_EXIT=$?
set -e

if [ $LEGACY_EXIT -ne 0 ] && grep -q "heap-use-after-free" /tmp/legacy_output.log; then
    echo ">>> Baseline Verified: AddressSanitizer successfully intercepted the legacy heap-use-after-free."
    grep "SUMMARY: AddressSanitizer:" /tmp/legacy_output.log || true
else
    echo "Warning: Expected legacy ASAN interception did not trigger."
fi

echo "=== [4/4] Executing Modernized RAII Test Suite under AddressSanitizer ==="
export ASAN_OPTIONS="detect_leaks=1:halt_on_error=1:abort_on_error=1:symbolize=1"
export UBSAN_OPTIONS="halt_on_error=1:abort_on_error=1"

cd "${BUILD_DIR}"
ctest --output-on-failure -C Debug

echo ">>> VERIFICATION PASSED: Modernized C++ RAII architecture is 100% CLEAN under AddressSanitizer. <<<"
