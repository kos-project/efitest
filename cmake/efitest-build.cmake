cmake_minimum_required(VERSION 3.20)
project(${TARGET_NAME} LANGUAGES C CXX)

set(CMAKE_C_STANDARD 23)
set(CMAKE_CXX_STANDARD 23)

# Override current directory variables & append to module path to use parent's CMX installation
set(CMAKE_CURRENT_BINARY_DIR ${PARENT_BINARY_DIR}) # Output to parent binary dir
list(APPEND CMAKE_MODULE_PATH "${PARENT_SOURCE_DIR}/cmake")

include(cmx-bootstrap)
include(cmx-efi)

file(GLOB_RECURSE EFITEST_SOURCE_FILES "${PARENT_SOURCE_DIR}/src/*.c")
add_library(efitest STATIC ${EFITEST_SOURCE_FILES})
cmx_include_efi(efitest PUBLIC)
target_include_directories(efitest PUBLIC "${PARENT_SOURCE_DIR}/include")
if ((CMX_COMPILER_GCC OR CMX_COMPILER_CLANG) AND CMX_CPU_X86 AND CMX_CPU_64_BIT)
    target_compile_options(efitest PRIVATE -march=x86-64) # Disable SSE/AVX
endif ()

cmx_add_efi_executable(${TARGET_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(${TARGET_NAME} PRIVATE efitest)
add_dependencies(${TARGET_NAME} efitest)

cmx_add_esp_image("${TARGET_NAME}-esp"
        BOOT_FILE "${TARGET_NAME}.efi"
        IMAGE_NAME "${TARGET_NAME}")
add_dependencies("${TARGET_NAME}-esp" ${TARGET_NAME})
cmx_add_iso_image("${TARGET_NAME}-iso"
        ESP_IMAGE "${TARGET_NAME}.img"
        ESP_IMAGE_NAME "esp"
        IMAGE_NAME "${TARGET_NAME}")
add_dependencies("${TARGET_NAME}-iso" "${TARGET_NAME}-esp")