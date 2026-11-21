# Architecture Policy: Modern C++ Standards & Zero Context Poisoning

This policy document defines the non-negotiable architectural constraints for all AI coding agents working on this codebase.

---

## 1. Core Mandate: Immunity to Legacy Anti-Patterns
When modifying or extending existing components, the agent MUST NOT propagate legacy idioms present in the existing code.

1. **Zero Raw Owning Pointers**:
   - `new` and `delete` expressions are strictly prohibited in application logic and test code.
   - Resource management must strictly use `std::unique_ptr` for exclusive ownership and `std::shared_ptr` / `std::weak_ptr` for shared or observed ownership.
   - For callbacks and listener subscriptions, use RAII tokens (`ScopedConnection`) that automatically unbind upon destruction.

2. **Mandatory RAII Resource Lifetime**:
   - Every system resource (file descriptors, sockets, thread handles, listener registrations) must be encapsulated in an RAII container.
   - Test teardown logic must NEVER depend on manual cleanup calls that are bypassed on assertion failures or exceptions.

3. **Compiler and Sanitizer Cleanliness**:
   - Code must compile with zero warnings under `-Wall -Wextra -Wpedantic`.
   - Code must pass AddressSanitizer (ASAN) and UndefinedBehaviorSanitizer (UBSAN) with **zero detected errors, memory leaks, or heap-use-after-free events**.

4. **Deterministic Verification Gate**:
   - The agent MUST NEVER attempt to guess compilation commands or run ad-hoc `g++` / `clang++` commands.
   - The agent MUST run `./scripts/verify.sh` to obtain the authoritative machine verdict.
