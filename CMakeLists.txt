cmake_minimum_required(VERSION 3.16)
project(xavrx_n64 C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(XAVRX_BUILD_TESTS "Build host-side unit tests" ON)

add_library(xavrx_core
    src/cpu.c
    src/bus.c
    src/cartridge.c
)
target_include_directories(xavrx_core PUBLIC include)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(xavrx_core PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
endif()

if(XAVRX_BUILD_TESTS)
    enable_testing()
    add_executable(xavrx_tests tests/test_core.c)
    target_link_libraries(xavrx_tests PRIVATE xavrx_core)
    add_test(NAME xavrx_core_tests COMMAND xavrx_tests)
endif()
