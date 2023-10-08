cmake_minimum_required(VERSION 3.20)
project(efitest-discoverer LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_C_STANDARD 23)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")
include(cmx-bootstrap)

add_executable(efitest-discoverer "discoverer.cpp")
cmx_include_cxxopts(efitest-discoverer PRIVATE)
cmx_include_fmt(efitest-discoverer PRIVATE)
if ((CMX_COMPILER_GCC OR CMX_COMPILER_CLANG) AND CMX_CPU_X86 AND CMX_CPU_64_BIT)
    target_compile_options(efitest-discoverer PUBLIC -march=x86-64-v3) # Enable SSE/AVX
endif ()