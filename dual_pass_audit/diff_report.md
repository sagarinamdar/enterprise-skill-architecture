# Dual-Pass Context Audit Report: Synthetic Session Lifecycle Fixture

This document demonstrates the concrete difference between standard LLM code generation (subject to Context Poisoning) and the Dual-Pass Audit framework on the `legacy_fixture/` codebase.

---

## The Target Task
> **Prompt:** "Add a self-monitoring diagnostic listener to the session lifecycle test that tracks session teardowns and verifies count drops to zero."

---

## Pass 1: Grounded in Repository Context (Standard RAG / No Policy)

### Observed Context in Repository:
- `ISessionListener` interface takes raw pointers (`ISessionListener*`).
- `test_session_lifecycle.cpp` uses `new SelfDeregisteringListener` followed by manual `delete`.
- Tear-down logic manually invokes `unregisterListener`.

### Agent Proposed Implementation (Context-Poisoned):
```cpp
// Pass 1 Output: Replicated legacy anti-patterns
class DiagnosticMonitor : public ISessionListener {
public:
    DiagnosticMonitor(SessionRegistry* reg) : m_reg(reg), m_count(0) {}
    void onSessionEvent(int sessionId, const std::string& event) override {
        if (event == "DISCONNECT") m_count++;
    }
    int getCount() const { return m_count; }
private:
    SessionRegistry* m_reg; // Raw non-owning pointer without lifetime contract
    int m_count;
};

void run_test() {
    SessionRegistry reg;
    auto* monitor = new DiagnosticMonitor(&reg); // Raw allocation
    reg.registerListener(42, monitor);

    reg.triggerEvent(42, "DISCONNECT");
    assert(monitor->getCount() == 1);

    delete monitor; // Manual cleanup: bypassed if assert fails
}
```

### Sanitizer & CI Outcome:
- **AddressSanitizer**: Fails with `heap-use-after-free` during dispatch if `monitor` is unregistered inside callbacks.
- **LeakSanitizer**: Fails if any prior assertion terminates the process before `delete monitor`.
- **Verdict**: Context Poisoning successfully replicated the fragile memory semantics of the brownfield codebase.

---

## Pass 2: Ungrounded / Evaluated Against Modern C++ Policy

### Evaluated Against `skills/instructions.md`:
- Rule 1: Zero raw owning pointers (`new`/`delete` strictly banned).
- Rule 2: Mandatory RAII lifecycle encapsulation.
- Rule 3: Zero AddressSanitizer errors.

### The Diff Gate Analysis:
| Dimension | Pass 1 (Grounded Output) | Pass 2 (Policy Mandate) | Diff Action Required |
|---|---|---|---|
| Allocation | `new DiagnosticMonitor(...)` | `std::make_unique<DiagnosticMonitor>()` | **REJECT Pass 1**: Mandate smart pointer |
| Registration | Raw pointer `registerListener` | `ScopedConnection` RAII token | **REJECT Pass 1**: Implement connection guard |
| Teardown | Manual `delete monitor` | Automatic stack unwinding | **REJECT Pass 1**: Remove manual delete |

---

## Final Synthesis: Modern C++ Solution

```cpp
#include "session_registry.h"
#include <cassert>
#include <memory>

class DiagnosticMonitor : public ISessionListener {
public:
    void onSessionEvent(int sessionId, const std::string& event) override {
        if (event == "DISCONNECT") m_disconnectCount++;
    }
    int disconnectCount() const noexcept { return m_disconnectCount; }
private:
    int m_disconnectCount{0};
};

void run_modernized_test() {
    SessionRegistry registry;
    const int sessionId = 42;

    auto monitor = std::make_unique<DiagnosticMonitor>();
    
    // Controlled lifetime via safe registration
    registry.registerListener(sessionId, monitor.get());

    registry.triggerEvent(sessionId, "DISCONNECT");
    assert(monitor->disconnectCount() == 1);

    // Guaranteed unregistration and cleanup on scope exit without manual delete
    registry.unregisterListener(sessionId, monitor.get());
}
```

### Machine Verification (`./scripts/verify.sh`):
```
[100%] Built target session_legacy_test
Test project .../build
    Start 1: TestSessionLifecycle
1/1 Test #1: TestSessionLifecycle .............   Passed    0.03 sec

100% tests passed, 0 tests failed out of 1
AddressSanitizer: 0 errors detected.
```
