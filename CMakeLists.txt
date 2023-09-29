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

macro (efitest_add_tests target access)
    # Search for source files to transform/copy
    foreach (directory IN ITEMS ${ARGN})
        file(GLOB_RECURSE source_files "${directory}/*.c*")
        list(APPEND all_source_files ${source_files})
    endforeach ()
    string(REPLACE ";" "," file_flags "${all_source_files}")
    # Set up directories
    set(generated_dir "${CMAKE_CURRENT_BINARY_DIR}/efitest-generated")
    if(NOT EXISTS ${generated_dir})
        file(MAKE_DIRECTORY ${generated_dir})
    endif()
    # Discover the tests while configuring
    execute_process(COMMAND "${CMAKE_CURRENT_BINARY_DIR}/efitest-prebuild/efitest-discoverer"
            -o ${generated_dir}
            -f ${file_flags}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
    # Define actual test executable
    cmx_add_efi_executable(${target} ${access} ${generated_dir})
    target_link_libraries(${target} PRIVATE efitest)
    # Define image targets for the test executable
    cmx_add_esp_image("${target}-esp"
            BOOT_FILE "${target}.efi"
            IMAGE_NAME "${target}")
    add_dependencies("${target}-esp" ${target})
    cmx_add_iso_image("${target}-iso"
            ESP_IMAGE "${target}.img"
            ESP_IMAGE_NAME "esp"
            IMAGE_NAME "${target}")
    add_dependencies("${target}-iso" "${target}-esp")
endmacro()

if (EFITEST_BUILD_TESTS)
    efitest_add_tests(efitest-test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/test")
endif ()