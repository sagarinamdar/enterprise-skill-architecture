# Failure Memory & Pattern Catalog: Modern C++ RAII Patterns

This catalog documents recurring failure signatures encountered in enterprise C++ codebases and provides their verified, modern RAII replacements.

---

## Pattern 1: Scoped Connection Guard (Replacing Raw Observer Registrations)

### Failure Signature
- **Symptoms**: `AddressSanitizer: heap-use-after-free` or `SEGV on unknown address` during event dispatch.
- **Root Cause**: The observer object is destroyed while its raw pointer remains in the registry, or the observer unregisters itself during the dispatch loop, invalidating the container iterator.

### Anti-Pattern (Legacy)
```cpp
// Legacy: Raw pointer registration with manual unregister
registry.registerListener(id, this);
// Caller must remember to call unregisterListener in destructor
```

### Verified Modern Solution (RAII Connection Token)
```cpp
#include <functional>
#include <memory>

class ScopedConnection {
public:
    ScopedConnection() = default;
    explicit ScopedConnection(std::function<void()> disconnectFn)
        : m_disconnect(std::move(disconnectFn)) {}

    ~ScopedConnection() {
        disconnect();
    }

    // Move-only semantics prevent accidental double-disconnect
    ScopedConnection(ScopedConnection&& other) noexcept
        : m_disconnect(std::move(other.m_disconnect)) {
        other.m_disconnect = nullptr;
    }

    ScopedConnection& operator=(ScopedConnection&& other) noexcept {
        if (this != &other) {
            disconnect();
            m_disconnect = std::move(other.m_disconnect);
            other.m_disconnect = nullptr;
        }
        return *this;
    }

    ScopedConnection(const ScopedConnection&) = delete;
    ScopedConnection& operator=(const ScopedConnection&) = delete;

    void disconnect() {
        if (m_disconnect) {
            m_disconnect();
            m_disconnect = nullptr;
        }
    }

private:
    std::function<void()> m_disconnect;
};
```

### Registry Safe Dispatch Idiom
```cpp
// Safe dispatch: iterate over a local copy of listeners or valid weak handles
void triggerEventSafe(int sessionId, const std::string& eventType) {
    auto it = m_listeners.find(sessionId);
    if (it == m_listeners.end()) return;

    // Snapshot copy prevents iterator invalidation if a callback modifies the collection
    auto snapshot = it->second;
    for (auto* listener : snapshot) {
        if (listener) {
            listener->onSessionEvent(sessionId, eventType);
        }
    }
}
```

---

## Pattern 2: RAII Fixture Scoped Cleanup (Preventing Test Leaks)

### Failure Signature
- **Symptoms**: Memory leak reported by ASAN (`detect_leaks=1`) on failed test assertions.
- **Root Cause**: Tests allocate raw pointers and place `delete` at the end of the test function. When an assertion (`assert()`, `EXPECT_TRUE()`) fails early, cleanup is bypassed.

### Anti-Pattern (Legacy)
```cpp
void test_feature() {
    auto* item = new Item();
    assert(item->isValid()); // If this fails, item leaks!
    delete item;
}
```

### Verified Modern Solution
```cpp
void test_feature() {
    auto item = std::make_unique<Item>();
    assert(item->isValid()); // Safe: unique_ptr automatically cleans up regardless of exit path
}
```
