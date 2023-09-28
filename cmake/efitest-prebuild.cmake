string(TOLOWER "${CMAKE_BUILD_TYPE}" build_type)
set(efitest_build_dir "efitest-${build_type}")

if (NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${efitest_build_dir}")
    # Pre-build discoverer on-demand using a nested CMake process
    message(STATUS "Pre-building discoverer executable..")
    execute_process(COMMAND ${CMAKE_COMMAND}
            -S .
            -B "${efitest_build_dir}"
            -G "Unix Makefiles"
            -DCMAKE_BUILD_TYPE=Release
            RESULT_VARIABLE exit_code
            ERROR_VARIABLE process_error
            OUTPUT_QUIET
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
    if (NOT ${exit_code} EQUAL 0)
        message(FATAL_ERROR "Could not configure efitest discoverer: ${process_error}")
    endif ()
    execute_process(COMMAND ${CMAKE_COMMAND}
            --build "${efitest_build_dir}"
            --target efitest-discoverer
            RESULT_VARIABLE exit_code
            ERROR_VARIABLE process_error
            OUTPUT_QUIET
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
    if (NOT ${exit_code} EQUAL 0)
        message(FATAL_ERROR "Could not build efitest discoverer: ${process_error}")
    endif ()
endif ()