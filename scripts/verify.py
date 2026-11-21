#!/usr/bin/env python3
"""
Portable Deterministic Verification Engine: Enterprise C++ & Skill Policy Auditor
Cross-platform verification script that checks both static architecture invariants
and dynamic AddressSanitizer/CTest runs.
"""

import os
import sys
import re
import subprocess
import shutil

ROOT_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
LEGACY_DIR = os.path.join(ROOT_DIR, "legacy_fixture")
SKILLS_DIR = os.path.join(ROOT_DIR, "skills")

def check_static_policy(filepath):
    """Audits C++ source file against skills/instructions.md invariants."""
    findings = []
    with open(filepath, "r", encoding="utf-8") as f:
        lines = f.readlines()

    for idx, line in enumerate(lines, 1):
        stripped = line.strip()
        # Check 1: Raw new
        if re.search(r'\bnew\s+[A-Za-z0-9_]+', stripped) and not stripped.startswith("//"):
            findings.append((idx, "RAW_NEW_USAGE", "Violation: Explicit 'new' expression forbidden under Policy Rule 1."))
        # Check 2: Raw delete
        if re.search(r'\bdelete\s+[A-Za-z0-9_]+', stripped) and not stripped.startswith("//"):
            findings.append((idx, "RAW_DELETE_USAGE", "Violation: Explicit 'delete' expression forbidden under Policy Rule 1."))
        # Check 3: Raw delete this
        if "delete this;" in stripped and not stripped.startswith("//"):
            findings.append((idx, "DELETE_THIS_UAF_RISK", "Violation: 'delete this;' causes severe heap-use-after-free risk."))

    return findings

def run_cmake_asan():
    """Attempts to configure and run CMake with AddressSanitizer if available."""
    cmake_bin = shutil.which("cmake")
    if not cmake_bin:
        # Check WSL
        wsl_bin = shutil.which("wsl")
        if wsl_bin:
            res = subprocess.run(["wsl", "which", "cmake"], capture_output=True, text=True)
            if res.returncode == 0:
                print(">>> [CI] Detected CMake inside WSL. Running containerized verification...")
                wsl_root = subprocess.run(["wsl", "wslpath", "-a", ROOT_DIR], capture_output=True, text=True).stdout.strip()
                cmd = ["wsl", "bash", "-c", f"cd '{wsl_root}' && ./scripts/verify.sh"]
                proc = subprocess.run(cmd)
                return proc.returncode == 0
        return None

    build_dir = os.path.join(ROOT_DIR, "build")
    print(">>> [CI] Configuring CMake with AddressSanitizer...")
    cfg = subprocess.run([cmake_bin, "-B", build_dir, "-S", ROOT_DIR, "-DCMAKE_BUILD_TYPE=Debug", "-DENABLE_ASAN=ON"])
    if cfg.returncode != 0:
        return False

    print(">>> [CI] Compiling targets...")
    build = subprocess.run([cmake_bin, "--build", build_dir, "--config", "Debug"])
    if build.returncode != 0:
        return False

    print(">>> [CI] Running CTest under AddressSanitizer...")
    test = subprocess.run(["ctest", "--output-on-failure", "-C", "Debug"], cwd=build_dir)
    return test.returncode == 0

def main():
    print("================================================================================")
    print("  Enterprise Skill Verification Harness: Policy & Sanitizer Audit")
    print("================================================================================")

    # Step 1: Static Policy Audit across legacy_fixture
    print("\n[Phase 1/2] Auditing Source against skills/instructions.md Invariants...")
    total_violations = 0
    for root, _, files in os.walk(LEGACY_DIR):
        for file in files:
            if file.endswith((".h", ".cpp")):
                p = os.path.join(root, file)
                rel_path = os.path.relpath(p, ROOT_DIR)
                violations = check_static_policy(p)
                if violations:
                    print(f"  [FLAGGED] {rel_path}: {len(violations)} policy violation(s)")
                    for line_no, vtype, msg in violations:
                        print(f"    - Line {line_no:3d} [{vtype}]: {msg}")
                        total_violations += 1
                else:
                    print(f"  [CLEAN]   {rel_path}: Invariant compliant (RAII enforced).")

    print(f"\nPhase 1 Result: {total_violations} architectural policy violation(s) detected in baseline.")

    # Step 2: Dynamic Sanitizer Verification
    print("\n[Phase 2/2] Checking Dynamic Compilation & AddressSanitizer...")
    dynamic_res = run_cmake_asan()
    if dynamic_res is None:
        print("  [NOTE] Native CMake toolchain not in host PATH. Native static policy analysis complete.")
        print("  [NOTE] Run './scripts/verify.sh' in any Linux/WSL/Docker container with Clang for full ASAN abort.")
    elif dynamic_res is False:
        print("  [FAIL] Compilation or AddressSanitizer intercepted memory safety violations.")
    else:
        print("  [PASS] All targets compiled cleanly and passed AddressSanitizer checks.")

    print("\n================================================================================")
    print("  Verification summary logged. Machine truth established.")
    print("================================================================================")

if __name__ == "__main__":
    main()
