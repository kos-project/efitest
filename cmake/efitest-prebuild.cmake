string(TOLOWER "${CMAKE_BUILD_TYPE}" build_type)
set(efitest_build_dir "efitest-${build_type}")
set(efitest_build_dir_path "${PARENT_BINARY_DIR}/efitest-prebuild")

# Pre-build discoverer on-demand using a nested CMake process
message(STATUS "Configuring EFITEST tools..")
execute_process(COMMAND ${CMAKE_COMMAND} -E env CC=${CC} env CXX=${CXX} ${CMAKE_COMMAND}
        -S .
        -B "${efitest_build_dir_path}"
        -G "Unix Makefiles"
        -DCMAKE_BUILD_TYPE=Release
        -DEFITEST_SUB_BUILD=ON
        RESULT_VARIABLE exit_code
        ERROR_VARIABLE process_error
        OUTPUT_QUIET
        WORKING_DIRECTORY ${PARENT_SOURCE_DIR})
if (NOT ${exit_code} EQUAL 0)
    message(FATAL_ERROR "Could not configure EFITEST tools: ${process_error}")
endif ()

message(STATUS "Building EFITEST discoverer..")
execute_process(COMMAND ${CMAKE_COMMAND}
        --build "${efitest_build_dir_path}"
        --target efitest-discoverer
        RESULT_VARIABLE exit_code
        ERROR_VARIABLE process_error
        OUTPUT_QUIET
        WORKING_DIRECTORY ${PARENT_SOURCE_DIR})
if (NOT ${exit_code} EQUAL 0)
    message(FATAL_ERROR "Could not build EFITEST discoverer: ${process_error}")
endif ()