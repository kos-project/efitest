cmake_minimum_required(VERSION 3.20)
project(efitest LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_C_STANDARD 23)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(cmx-bootstrap)

option(EFITEST_BUILD_TESTS "Build unit tests for libefitest" OFF)
option(EFITEST_SUB_BUILD "Set automatically if this is a sub-build" OFF)
set(EFITEST_TARGET_ARCH "${CMX_CPU_ARCH}" CACHE STRING "Specify the target architecture to build for")
set(EFI_TARGET_ARCH "${EFITEST_TARGET_ARCH}")

include(cmx-efi)

file(GLOB_RECURSE EFITEST_SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c")
add_library(efitest STATIC ${EFITEST_SOURCE_FILES})
cmx_include_efi(efitest PUBLIC)
target_include_directories(efitest PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

if ("${EFITEST_TARGET_ARCH}" STREQUAL "x86_64")
    target_compile_definitions(efitest PUBLIC ETEST_ARCH_AMD64)
    target_compile_options(efitest PUBLIC -march=x86-64)
elseif ("${EFITEST_TARGET_ARCH}" STREQUAL "x86")
    target_compile_definitions(efitest PUBLIC ETEST_ARCH_IA32)
    target_compile_options(efitest PUBLIC -march=i686 -mno-mmx -mno-sse)
elseif ("${EFITEST_TARGET_ARCH}" STREQUAL "arm64")
    target_compile_definitions(efitest PUBLIC ETEST_ARCH_ARM64)
elseif ("${EFITEST_TARGET_ARCH}" STREQUAL "arm")
    target_compile_definitions(efitest PUBLIC ETEST_ARCH_ARM)
elseif ("${EFITEST_TARGET_ARCH}" STREQUAL "riscv64")
    target_compile_definitions(efitest PUBLIC ETEST_ARCH_RISCV64)
else ()
    message(FATAL_ERROR "CPU architecture ${LCPU_TARGET_ARCH} is not supported right now")
endif ()

if (NOT EFITEST_SUB_BUILD)
    execute_process(COMMAND ${CMAKE_COMMAND}
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