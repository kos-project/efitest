cmake_minimum_required(VERSION 3.20)
project(efitest LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_C_STANDARD 23)

option(EFITEST_BUILD_TESTS "Build unit tests for libefitest" OFF)
option(EFITEST_SUB_BUILD "Set automatically if this is a sub-build" OFF)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(cmx-bootstrap)
include(cmx-efi)

file(GLOB_RECURSE EFITEST_SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c")
add_library(efitest STATIC ${EFITEST_SOURCE_FILES})
cmx_include_efi(efitest PUBLIC)
target_include_directories(efitest PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
if ((CMX_COMPILER_GCC OR CMX_COMPILER_CLANG) AND CMX_CPU_X86 AND CMX_CPU_64_BIT)
    target_compile_options(efitest PRIVATE -march=x86-64) # Disable SSE/AVX
endif ()

add_executable(efitest-discoverer "discoverer.cpp")
cmx_include_cxxopts(efitest-discoverer PRIVATE)
cmx_include_fmt(efitest-discoverer PRIVATE)
if ((CMX_COMPILER_GCC OR CMX_COMPILER_CLANG) AND CMX_CPU_X86 AND CMX_CPU_64_BIT)
    target_compile_options(efitest-discoverer PUBLIC -march=x86-64-v3) # Enable SSE/AVX
endif ()

if (NOT EFITEST_SUB_BUILD)
    execute_process(COMMAND ${CMAKE_COMMAND}
            -DCC=$ENV{CC}
            -DCXX=$ENV{CXX}
            -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
            -DPARENT_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}
            -DPARENT_BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}
            -P "cmake/efitest-prebuild.cmake"
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endif ()

if (EFITEST_BUILD_TESTS)
    set(EFITEST_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
    set(EFITEST_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR})
    include(efitest)
    efitest_add_tests(efitest-test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/test")
endif ()