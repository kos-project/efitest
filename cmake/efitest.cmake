include_guard()

macro(efitest_add_tests target access)
    find_program(GREP "grep")
    if (NOT GREP)
        message(FATAL_ERROR "Could not find grep, please make sure to install it")
    endif ()

    # Find out number of cores in the machine
    execute_process(COMMAND ${GREP} -c ^processor /proc/cpuinfo
            OUTPUT_VARIABLE num_threads)
    string(REGEX REPLACE "\n+$" "" num_threads "${num_threads}")

    # Search for test source files recursively
    foreach (directory IN ITEMS ${ARGN})
        file(GLOB_RECURSE source_files "${directory}/*.c*")
        list(APPEND all_source_files ${source_files})
    endforeach ()

    string(REPLACE ";" "," file_flags "${all_source_files}")
    string(TOLOWER "${CMAKE_BUILD_TYPE}" build_type)
    set(build_dir "efitest-${build_type}")
    set(build_dir_path "${CMAKE_CURRENT_SOURCE_DIR}/${build_dir}")
    set(generated_dir_path "${build_dir_path}/generated")

    # Prepare build directory
    if (NOT EXISTS "${generated_dir_path}")
        file(MAKE_DIRECTORY "${generated_dir_path}")
    endif ()

    # Pre-build discoverer if required
    add_custom_target("${target}-efitest-prebuild" ALL
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
    add_custom_command(TARGET "${target}-efitest-prebuild"
            COMMAND ${CMAKE_COMMAND}
            -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
            -DCMAKE_CURRENT_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}
            -DCMAKE_CURRENT_BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}
            -P "${EFITEST_SOURCE_DIR}/cmake/efitest-prebuild.cmake"
            BYPRODUCTS "${build_dir}/efitest-discoverer"
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

    # Set up script injector target
    add_custom_target("${target}-efitest-inject" ALL
            WORKING_DIRECTORY ${build_dir_path})
    add_custom_command(TARGET "${target}-efitest-inject"
            COMMAND efitest-script-injector
            -s "${EFITEST_SOURCE_DIR}/CMakeLists.txt"
            -i "${EFITEST_SOURCE_DIR}/cmake/efitest-build.cmake"
            -o "${generated_dir_path}/CMakeLists.txt"
            WORKING_DIRECTORY ${generated_dir_path})
    add_dependencies("${target}-efitest-inject" "${target}-efitest-prebuild")

    # Set up discovery target
    add_custom_target("${target}-efitest-discovery" ALL
            WORKING_DIRECTORY ${build_dir_path})
    add_custom_command(TARGET "${target}-efitest-discovery"
            COMMAND efitest-discoverer
            -o "${generated_dir_path}/src"
            -f ${file_flags}
            WORKING_DIRECTORY ${build_dir_path})
    add_dependencies("${target}-efitest-discovery" "${target}-efitest-prebuild")

    # Set up wrapped build target
    add_custom_target(${target} ALL
            WORKING_DIRECTORY ${generated_dir_path})
    add_custom_command(TARGET ${target}
            COMMAND ${CMAKE_COMMAND} -E env CC=$ENV{CC} CXX=$ENV{CXX} ${CMAKE_COMMAND} -S . -B "build" -G "Unix Makefiles"
            -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
            -DTARGET_NAME="${target}"
            -DPARENT_SOURCE_DIR=${EFITEST_SOURCE_DIR}
            -DPARENT_BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}
            WORKING_DIRECTORY ${generated_dir_path})
    add_custom_command(TARGET ${target}
            COMMAND ${CMAKE_COMMAND} --build "build" --target "${target}-iso" -- -j ${num_threads}
            WORKING_DIRECTORY ${generated_dir_path})
    add_dependencies(${target} "${target}-efitest-inject" "${target}-efitest-discovery")

    # Set up dummy target so we get syntax highlighting and code completion
    set(${target}_source_files)
    foreach (arg IN ITEMS ${ARGN})
        file(GLOB_RECURSE sources "${arg}/*.c*")
        list(APPEND ${target}_source_files ${sources})
        file(GLOB_RECURSE headers "${arg}/*.h*")
        list(APPEND ${target}_source_files ${headers})
    endforeach ()
    add_library("${target}-efitest-dummy" STATIC ${${target}_source_files})
    target_link_libraries("${target}-efitest-dummy" PRIVATE efitest)
endmacro()

macro(efitest_include_directories target access)
    # Dummy used by script injector
endmacro()

macro(efitest_link_libraries target access)
    # Dummy used by script injector
endmacro()

macro(efitest_compile_options target access)
    # Dummy used by script injector
endmacro()

macro(efitest_compile_definitions target access)
    # Dummy used by script injector
endmacro()

macro(efitest_set)
    # Dummy used by script injector
endmacro()

macro(efitest_unset)
    # Dummy used by script injector
endmacro()