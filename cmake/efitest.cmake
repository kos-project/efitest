include_guard()

macro(efitest_add_tests target access)
    # Search for source files to transform/copy
    foreach (directory IN ITEMS ${ARGN})
        file(GLOB_RECURSE source_files "${directory}/*.c*")
        list(APPEND all_source_files ${source_files})
    endforeach ()
    string(REPLACE ";" "," file_flags "${all_source_files}")
    # Set up directories
    set(generated_dir "${EFITEST_BINARY_DIR}/efitest-generated")
    if (NOT EXISTS ${generated_dir})
        file(MAKE_DIRECTORY ${generated_dir})
    endif ()
    # Discover the tests while configuring
    execute_process(COMMAND "${EFITEST_BINARY_DIR}/efitest-prebuild/efitest-discoverer"
            -o ${generated_dir}
            -f ${file_flags}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
    # Define a dummy target for IDE integration
    add_library("${target}-dummy" STATIC ${all_source_files})
    target_link_libraries("${target}-dummy" PRIVATE efitest)
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

macro(efitest_include_directories target access)
    target_include_directories(${target} ${access} ${ARGN})
    target_include_directories("${target}-dummy" ${access} ${ARGN})
endmacro()

macro(efitest_link_libraries target access)
    target_link_libraries(${target} ${access} ${ARGN})
    target_link_libraries("${target}-dummy" ${access} ${ARGN})
endmacro()

macro(efitest_compile_options target access)
    target_compile_options(${target} ${access} ${ARGN})
    target_compile_options("${target}-dummy" ${access} ${ARGN})
endmacro()

macro(efitest_compile_definitions target access)
    target_compile_definitions(${target} ${access} ${ARGN})
    target_compile_definitions("${target}-dummy" ${access} ${ARGN})
endmacro()