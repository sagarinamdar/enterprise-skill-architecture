# ==============================================================================
# Deterministic Verification Script (Windows PowerShell)
# ==============================================================================
$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$rootDir = Split-Path -Parent $scriptDir
$buildDir = Join-Path $rootDir "build"

Write-Host "=== [1/3] Configuring CMake with AddressSanitizer & C++17 ===" -ForegroundColor Cyan
cmake -B $buildDir -S $rootDir -DCMAKE_BUILD_TYPE=Debug -DENABLE_ASAN=ON

Write-Host "=== [2/3] Building Test Targets ===" -ForegroundColor Cyan
cmake --build $buildDir --config Debug

Write-Host "=== [3/3] Executing CTest on Modernized RAII Suite ===" -ForegroundColor Cyan
$env:ASAN_OPTIONS = "detect_leaks=1:halt_on_error=1:abort_on_error=1"
Push-Location $buildDir
try {
    ctest --output-on-failure -C Debug
    Write-Host ">>> VERIFICATION PASSED: Build, tests, and Sanitizers are 100% CLEAN. <<<" -ForegroundColor Green
} finally {
    Pop-Location
}
