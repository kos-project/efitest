include_guard()

add_subdirectory(${PARENT_SOURCE_DIR} "${PARENT_BINARY_DIR}/submake")

macro(efitest_include_directories target access)
    target_include_directories(${target} ${access} ${ARGN})
endmacro()

macro(efitest_link_libraries target access)
    target_link_libraries(${target} ${access} ${ARGN})
endmacro()

macro(efitest_compile_options target access)
    target_compile_options(${target} ${access} ${ARGN})
endmacro()

macro(efitest_compile_definitions target access)
    target_compile_definitions(${target} ${access} ${ARGN})
endmacro()

macro(efitest_set name)
    set(${name} ${ARGN})
endmacro()

macro(efitest_unset name)
    unset(${name})
endmacro()