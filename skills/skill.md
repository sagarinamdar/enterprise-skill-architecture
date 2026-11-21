---
name: enterprise-cpp-modernization
description: Eliminates context poisoning and legacy anti-patterns in enterprise C++ codebases using deterministic script hooks and a dual-pass audit.
---

# Execution Playbook: Enterprise C++ Modernization & Verification

Follow this staged workflow for all bug fixes, test stabilization, and feature additions in legacy C++ components.

## Phase 1: Machine Baseline Diagnostic
Do not inspect the code or make assumptions first. Run the deterministic verification script to establish the ground truth:

```bash
./scripts/verify.sh
```

Capture the compiler and AddressSanitizer output. Identify whether the failure is:
- Compilation / syntax error
- Memory corruption / heap-use-after-free (ASAN error)
- Resource leak on exit
- Test assertion failure

## Phase 2: Dual-Pass Context Audit (Grounded vs. Ungrounded)
Before generating code, perform a conscious Dual-Pass evaluation:

1. **Pass 1 (Grounded Assessment):**
   - How does the legacy repository currently handle this lifecycle?
   - Identify existing idioms (e.g., `registerListener(raw_ptr)`, manual `delete`).
2. **Pass 2 (Ungrounded Standards Check):**
   - Evaluate against `skills/instructions.md`.
   - Ask: *If this were written in Modern C++17 with RAII and zero raw pointers, how would it be designed?*
3. **Diff Gate:**
   - Explicitly list the anti-patterns in Pass 1 that MUST be rejected.
   - If Pass 1 proposes a quick patch using raw pointers or manual cleanup, REJECT it immediately.

## Phase 3: Pattern Catalog Lookup
Consult `skills/pattern-catalog.md` to map the diagnosed failure signature to its verified Modern C++ pattern:
- **Dangling observer / use-after-free**: See *Pattern 1: Scoped Connection Guard*.
- **Leaked test fixture state**: See *Pattern 2: RAII Fixture Scoped Cleanup*.

## Phase 4: Implementation
Apply the minimal Modern C++ refactoring:
- Encapsulate subscription lifetimes using RAII.
- Copy listener collections or use snapshot dispatch to avoid iterator invalidation during mutation.
- Replace raw allocations with smart pointers.

## Phase 5: Machine-Checked Verification Gate
Execute the deterministic verification script again:

```bash
./scripts/verify.sh
```

Only declare the task complete when `./scripts/verify.sh` exits with code 0 and reports 0 sanitizer errors.
