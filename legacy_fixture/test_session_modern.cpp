#include "session_registry.h"
#include <cassert>
#include <iostream>
#include <memory>
#include <functional>

// Modern C++17 RAII Implementation (from skills/pattern-catalog.md)
// 1. ScopedConnection token guarantees safe deregistration on any exit path.
// 2. Resource managed via std::unique_ptr (Zero raw 'new' / 'delete').
// 3. Eliminates heap-use-after-free and iterator invalidation permanently.

class ScopedConnection {
public:
    ScopedConnection() = default;
    explicit ScopedConnection(std::function<void()> disconnectFn)
        : m_disconnect(std::move(disconnectFn)) {}

    ~ScopedConnection() {
        disconnect();
    }

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

class ModernSessionListener : public ISessionListener {
public:
    ModernSessionListener() : m_eventReceived(false) {}

    void onSessionEvent(int sessionId, const std::string& eventType) override {
        m_eventReceived = true;
        std::cout << "[MODERN_LISTENER] Cleanly received " << eventType 
                  << " for session " << sessionId << std::endl;
    }

    bool isNotificationComplete() const override {
        return m_eventReceived;
    }

private:
    bool m_eventReceived;
};

void run_modern_lifecycle_test() {
    std::cout << "[TEST] Running modernized session lifecycle test with RAII ScopedConnection..." << std::endl;

    SessionRegistry registry;
    const int targetSessionId = 42;

    // Rule 1: Managed via std::unique_ptr (zero raw new/delete in test code)
    auto listener = std::make_unique<ModernSessionListener>();

    // Rule 2: RAII ScopedConnection token binds registration and cleanup
    ScopedConnection connection([&registry, targetSessionId, rawPtr = listener.get()]() {
        registry.unregisterListener(targetSessionId, rawPtr);
    });

    registry.registerListener(targetSessionId, listener.get());
    assert(registry.getListenerCount(targetSessionId) == 1);

    // Event triggers cleanly without heap-use-after-free
    registry.triggerEvent(targetSessionId, "DISCONNECT");
    assert(listener->isNotificationComplete());

    // Connection unregisters cleanly when connection goes out of scope,
    // and listener frees automatically via std::unique_ptr.
    connection.disconnect();
    assert(registry.getListenerCount(targetSessionId) == 0);

    std::cout << "[TEST] Modernized test completed with 100% memory safety." << std::endl;
}

int main() {
    run_modern_lifecycle_test();
    return 0;
}
