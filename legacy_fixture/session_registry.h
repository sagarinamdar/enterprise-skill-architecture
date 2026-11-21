#pragma once

#include <string>
#include <vector>
#include <functional>
#include <unordered_map>

// Synthetic Legacy C++ Anti-Pattern:
// Uses raw observer pointers, manual deregistration, and unchecked lifetime coupling.
// In brownfield codebases, agents prompted without policies copy this pattern into new tests.

class ISessionListener {
public:
    virtual ~ISessionListener() = default;
    virtual void onSessionEvent(int sessionId, const std::string& eventType) = 0;
    virtual bool isNotificationComplete() const { return true; }
};

class SessionRegistry {
public:
    SessionRegistry() = default;
    ~SessionRegistry();

    // Legacy registration pattern: takes raw pointer, caller responsible for deregistering
    void registerListener(int sessionId, ISessionListener* listener);
    void unregisterListener(int sessionId, ISessionListener* listener);

    void triggerEvent(int sessionId, const std::string& eventType);
    void clearAllSessions();

    size_t getListenerCount(int sessionId) const;

private:
    std::unordered_map<int, std::vector<ISessionListener*>> m_listeners;
};
