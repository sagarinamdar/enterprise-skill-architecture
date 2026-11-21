cmake_minimum_required(VERSION 3.16)
project(EnterpriseSkillProof CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable AddressSanitizer and UndefinedBehaviorSanitizer option
option(ENABLE_ASAN "Enable AddressSanitizer and UBSan" ON)

if(ENABLE_ASAN)
    if(MSVC)
        add_compile_options(/fsanitize=address /Zi)
    else()
        add_compile_options(-fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer -g)
        add_link_options(-fsanitize=address -fsanitize=undefined)
    endif()
endif()

enable_testing()

include_directories(legacy_fixture)

add_library(session_core STATIC
    legacy_fixture/session_registry.cpp
)

# 1. Legacy Baseline Executable (Demonstrates AddressSanitizer UAF intercept)
add_executable(session_legacy_test
    legacy_fixture/test_session_lifecycle.cpp
)
target_link_libraries(session_legacy_test PRIVATE session_core)

# 2. Modernized RAII Executable (Demonstrates 100% clean memory safety)
add_executable(session_modern_test
    legacy_fixture/test_session_modern.cpp
)
target_link_libraries(session_modern_test PRIVATE session_core)

# CTest Suite:
# Test 1 verifies AddressSanitizer intercepts the legacy bug (WILL_FAIL TRUE = expected abort)
add_test(NAME TestLegacyLifecycle_ExpectedFailure COMMAND session_legacy_test)
set_tests_properties(TestLegacyLifecycle_ExpectedFailure PROPERTIES WILL_FAIL TRUE)

# Test 2 verifies Modern C++ RAII architecture passes cleanly with 0 ASAN errors
add_test(NAME TestModernLifecycle_CleanPass COMMAND session_modern_test)
