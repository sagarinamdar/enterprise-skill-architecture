#include "session_registry.h"
#include <cassert>
#include <iostream>
#include <memory>
#include <vector>

// Synthetic Legacy Test Fixture demonstrating "Context Poisoning"
// A standard LLM given this file copies:
// 1. Raw pointer allocation (new) without smart pointer wrappers
// 2. One-shot self-deletion inside callbacks (triggering heap-use-after-free)
// 3. Manual delete in tearDown bypassed on early assertion failure

class OneShotSessionListener : public ISessionListener {
public:
    OneShotSessionListener(SessionRegistry& registry) 
        : m_registry(registry), m_eventReceived(false) {}

    void onSessionEvent(int sessionId, const std::string& eventType) override {
        m_eventReceived = true;
        std::cout << "[LISTENER] Processing event: " << eventType << " for session " << sessionId << std::endl;
        
        // Legacy Anti-Pattern: Unregisters and frees own memory inside callback
        m_registry.unregisterListener(sessionId, this);
        delete this; // Triggers heap-use-after-free in SessionRegistry::triggerEvent
    }

    bool isNotificationComplete() const override { 
        return m_eventReceived; 
    }

private:
    SessionRegistry& m_registry;
    bool m_eventReceived;
};

void run_flaky_legacy_lifecycle_test() {
    std::cout << "[TEST] Running session lifecycle test with raw one-shot listener..." << std::endl;

    SessionRegistry registry;
    const int targetSessionId = 42;

    // Legacy pattern: manually allocated raw pointer
    auto* listener = new OneShotSessionListener(registry);
    registry.registerListener(targetSessionId, listener);

    assert(registry.getListenerCount(targetSessionId) == 1);

    // This triggers AddressSanitizer heap-use-after-free:
    // listener deletes itself during onSessionEvent, but SessionRegistry reads
    // listener->isNotificationComplete() immediately after the call!
    registry.triggerEvent(targetSessionId, "DISCONNECT");

    std::cout << "[TEST] Test completed successfully." << std::endl;
}

int main() {
    run_flaky_legacy_lifecycle_test();
    return 0;
}
