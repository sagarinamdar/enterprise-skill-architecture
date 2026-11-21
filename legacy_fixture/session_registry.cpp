#include "session_registry.h"
#include <algorithm>
#include <iostream>

SessionRegistry::~SessionRegistry() {
    clearAllSessions();
}

void SessionRegistry::registerListener(int sessionId, ISessionListener* listener) {
    if (!listener) return;
    m_listeners[sessionId].push_back(listener);
}

void SessionRegistry::unregisterListener(int sessionId, ISessionListener* listener) {
    auto it = m_listeners.find(sessionId);
    if (it != m_listeners.end()) {
        auto& vec = it->second;
        vec.erase(std::remove(vec.begin(), vec.end(), listener), vec.end());
    }
}

void SessionRegistry::triggerEvent(int sessionId, const std::string& eventType) {
    auto it = m_listeners.find(sessionId);
    if (it == m_listeners.end()) return;

    // VULNERABILITY (Classic Legacy Anti-Pattern):
    // Direct iteration over raw listener pointers with post-callback property access.
    // When a one-shot or self-cleaning listener destroys itself in onSessionEvent,
    // accessing listener->isNotificationComplete() below triggers an authentic
    // AddressSanitizer: heap-use-after-free (READ of size 8 on freed memory).
    auto& listeners = it->second;
    for (size_t i = 0; i < listeners.size(); ++i) {
        ISessionListener* listener = listeners[i];
        if (listener != nullptr) {
            listener->onSessionEvent(sessionId, eventType);
            // Post-dispatch status check on raw pointer:
            // Triggers immediate AddressSanitizer heap-use-after-free if listener was deleted!
            if (listener->isNotificationComplete()) {
                // Legacy post-notification audit trail
            }
        }
    }
}

void SessionRegistry::clearAllSessions() {
    m_listeners.clear();
}

size_t SessionRegistry::getListenerCount(int sessionId) const {
    auto it = m_listeners.find(sessionId);
    if (it == m_listeners.end()) return 0;
    return it->second.size();
}
