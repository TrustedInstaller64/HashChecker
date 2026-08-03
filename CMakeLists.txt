cmake_minimum_required(VERSION 3.16)
project(HashChecker VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── Options ──────────────────────────────────────────────
option(ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(ENABLE_TSAN "Enable ThreadSanitizer" OFF)
option(ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)
option(BUILD_TESTS "Build test suite" ON)

find_package(Threads REQUIRED)

# MSVC: force UTF-8 source encoding
if(MSVC)
    add_compile_options(/utf-8 /wd4819)
endif()

# ── Helper: apply sanitizer flags ────────────────────────
function(add_sanitizers target)
    if(ENABLE_ASAN)
        target_compile_options(${target} PRIVATE -fsanitize=address -fno-omit-frame-pointer)
        target_link_options(${target} PRIVATE -fsanitize=address)
    endif()
    if(ENABLE_TSAN)
        target_compile_options(${target} PRIVATE -fsanitize=thread -fno-omit-frame-pointer)
        target_link_options(${target} PRIVATE -fsanitize=thread)
    endif()
    if(ENABLE_UBSAN)
        target_compile_options(${target} PRIVATE -fsanitize=undefined -fno-omit-frame-pointer)
        target_link_options(${target} PRIVATE -fsanitize=undefined)
    endif()
endfunction()

# ── Core library ─────────────────────────────────────────
set(LIB_SOURCES
    src/app.cpp
    src/compare/comparator.cpp
    src/hash/hasher_registry.cpp
    src/hash/parallel_computer.cpp
    src/infra/thread_pool.cpp
    src/report/json_reporter.cpp
    src/report/quiet_reporter.cpp
    src/report/text_reporter.cpp
    src/scan/file_scanner.cpp
)

add_library(hashchecker_core STATIC ${LIB_SOURCES})
target_include_directories(hashchecker_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(hashchecker_core PUBLIC Threads::Threads)
add_sanitizers(hashchecker_core)

# ── CLI executable ───────────────────────────────────────
add_executable(hashchecker src/main.cpp)
target_link_libraries(hashchecker PRIVATE hashchecker_core)
add_sanitizers(hashchecker)

# ── Tests ────────────────────────────────────────────────
if(BUILD_TESTS)
    enable_testing()

    add_subdirectory(tests)
endif()
