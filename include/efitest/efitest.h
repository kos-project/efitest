// Copyright 2023 Karma Krafts & associates
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/**
 * @author Alexander Hinze
 * @since 24/09/2023
 */

#pragma once

#include "efitest_api.h"

typedef struct _EFITestUUID {
    UINT32 data[4];// 128 bits for a v4 UUID
} EFITestUUID;

typedef struct _EFITestContext {
    const char* test_name; // The name of the current test being run
    const char* file_path; // The absolute path to the source file the test is defined in
    const char* file_name; // The name of the file the test is defined in
    const char* group_name;// The name of the test group the current test is part of
    UINTN group_size;      // The total number of tests within the current group
    UINTN group_index;     // The index of the current test within the current group
    UINTN line_number;     // The line number where the function is defined
    BOOLEAN failed;        // Determines if the test has failed
} EFITestContext;

typedef struct _EFITestError {
    EFITestUUID uuid;      // UUID for comparing errors
    EFITestContext context;// Context captured in the moment of the error
    const char* expression;// The code snippet which caused the error
    UINTN line_number;     // The line number the assertion failed on
} EFITestError;

/*
 * Intrinsic macro recognized by the discoverer, don't change!
 * This macro defines a static function that is guaranteed to
 * be inlined. This is so the compiler can inline its code into
 * the generated trampoline function to prevent symbol pollution.
 */
#define ETEST_DEFINE_TEST(n) ETEST_INLINE static inline void n(EFITestContext* context)

// Assertions
/**
 * Assert the given statement inside of an EFITEST unit test
 * definition. This macro may not be used outside of said
 * definitions since it uses a hidden context variable.
 * @param x The expression to assert. True means success,
 *  false means the unit test will fail and an error will
 *  be generated (and added to the internal error list).
 */
#define ETEST_ASSERT(x) efitest_assert((x), context, __LINE__ - 4, #x)

/**
 * Assert that the two given values are equal.
 * @param a The first value to compare.
 * @param b The second value to compare.
 */
#define ETEST_ASSERT_EQ(a, b) ETEST_ASSERT(a == b)

/**
 * Assert that the two given values are not equal.
 * @param a The first value to compare.
 * @param b The second value to compare.
 */
#define ETEST_ASSERT_NE(a, b) ETEST_ASSERT(a != b)

/**
 * Assert that the first value is less than the second value.
 * @param a The first value to compare.
 * @param b The second value to compare.
 */
#define ETEST_ASSERT_LT(a, b) ETEST_ASSERT(a < b)

/**
 * Assert that the first value is less than or equal to the second value.
 * @param a The first value to compare.
 * @param b The second value to compare.
 */
#define ETEST_ASSERT_LE(a, b) ETEST_ASSERT(a <= b)

/**
 * Assert that the first value is greater than the second value.
 * @param a The first value to compare.
 * @param b The second value to compare.
 */
#define ETEST_ASSERT_GT(a, b) ETEST_ASSERT(a > b)

/**
 * Assert that the first value is greater than or equal to the second value.
 * @param a The first value to compare.
 * @param b The second value to compare.
 */
#define ETEST_ASSERT_GE(a, b) ETEST_ASSERT(a >= b)

/**
 * Expands to the current unit test name.
 * May only be used within a EFITEST test definitions.
 */
#define ETEST_TEST_NAME (context->test_name)

/**
 * Expands to the current file name.
 * May only be used within a EFITEST test definitions.
 */
#define ETEST_FILE_NAME (context->file_name)

/**
 * Expands to the current file path.
 * May only be used within a EFITEST test definitions.
 */
#define ETEST_FILE_PATH (context->file_path)

/**
 * Expands to the current test group name.
 * May only be used within a EFITEST test definitions.
 */
#define ETEST_GROUP_NAME (context->group_name)

/**
 * Expands to the current test group size.
 * May only be used within a EFITEST test definitions.
 */
#define ETEST_GROUP_SIZE (context->group_size)

/**
 * Expands to the current test group index.
 * May only be used within a EFITEST test definitions.
 */
#define ETEST_GROUP_INDEX (context->group_index)

/**
 * Expands to the line number at which the current
 * test is declared in the source file.
 * May only be used within a EFITEST test definitions.
 */
#define ETEST_LINE_NUMBER (context->line_number)

/**
 * Expands to a boolean that indicates whether the
 * current test has already failed or not.
 * May only be used within a EFITEST test definitions.
 */
#define ETEST_FAILED (context->failed)

#define ETEST_UUID_LENGTH 36
#define ETEST_SPACER "[------]"
#define ETEST_SPACER_OK "[--OK--]"
#define ETEST_SPACER_FAILED "[FAILED]"

ETEST_API_BEGIN

// Type definitions
typedef void (*EFITestCallback)(const EFITestContext* context);
typedef void (*EFITestRunCallback)();

/**
 * Generate a random version 4 UUID and store
 * the result into the value pointed to by the
 * given pointer.
 * @param value A pointer to the value to store the
 *  newly generated UUID into.
 */
void efitest_uuid_generate(EFITestUUID* value);

/**
 * Convert the given version 4 UUID to a string
 * and store the resulting characters into the
 * given buffer.
 * @param value The v4 UUID to convert to a string.
 * @param buffer A buffer large enough to hold a v4 UUID
 *  as described by the ETEST_UUID_LENGTH macro constant.
 */
void efitest_uuid_to_string(const EFITestUUID* value, char* buffer);

/**
 * Check equality between two version 4 UUIDs.
 * @param value1 A pointer to the first UUID value to compare.
 * @param value2 A pointer to the second UUID value to compare.
 * @return True if both of the given UUIDs are equal.
 */
BOOLEAN efitest_uuid_compare(const EFITestUUID* value1, const EFITestUUID* value2);

/**
 * Print a formatted string to the UEFI serial console.
 * @param format The format of the string to print. GNU-EFU PrintLib spec applies.
 * @param args A va_list of formatting parameters.
 */
void efitest_logf_v(const UINT16* format, va_list args);

/**
 * Print a formatted string to the UEFI serial console.
 * @param format The format of the string to print. GNU-EFU PrintLib spec applies.
 * @param ... A variable number of formatting parameters (at least 1).
 */
void efitest_logf(const UINT16* format, ...);

/**
 * Print a formatted string to the UEFI serial console and jump to a new line.
 * @param format The format of the string to print. GNU-EFU PrintLib spec applies.
 * @param args A variable number of formatting parameters (at least 1).
 */
void efitest_loglnf_v(const UINT16* format, va_list args);

/**
 * Print a formatted string to the UEFI serial console and jump to a new line.
 * @param format The format of the string to print. GNU-EFU PrintLib spec applies.
 * @param ... A variable number of formatting parameters (at least 1).
 */
void efitest_loglnf(const UINT16* format, ...);

/**
 * Print a string to the UEFI serial console.
 * @param message A null-terminated string to print.
 */
void efitest_log(const UINT16* message);

/**
 * Print a string to the UEFI serial console and jump to a new line.
 * @param message A null-terminated string to print.
 */
void efitest_logln(const UINT16* message);

/**
 * Set a callback function to be called before running all unit tests.
 * @param callback A pointer to a callback function to be called
 *  before all unit tests.
 */
void efitest_set_pre_run_callback(EFITestRunCallback callback);

/**
 * Set a callback function to be called after running all unit tests.
 * @param callback A pointer to a callback function to be called
 *  after all unit tests.
 */
void efitest_set_post_run_callback(EFITestRunCallback callback);

/**
 * Set a callback function to be called before running each test group.
 * @param callback A pointer to a callback function to be called
 *  before running each test group.
 */
void efitest_set_pre_group_callback(EFITestCallback callback);

/**
 * Set a callback function to be called after running each test group.
 * @param callback A pointer to a callback function to be called
 *  after running each test group.
 */
void efitest_set_post_group_callback(EFITestCallback callback);

/**
 * Set a callback function to be called before running each test.
 * @param callback A pointer to a callback function to be called
 *  before running each test.
 */
void efitest_set_pre_test_callback(EFITestCallback callback);

/**
 * Set a callback function to be called after running each test.
 * @param callback A pointer to a callback function to be called
 *  after running each test.
 */
void efitest_set_post_test_callback(EFITestCallback callback);

/**
 * Allocate a new entry in the global error list and copy the given
 * error into the newly created entry.
 * @param error A pointer to an error to be added to the global error list.
 */
void efitest_errors_add(const EFITestError* error);

/**
 * @return A pointer to the global error list.
 */
const EFITestError* efitest_errors_get();

/**
 * @return A pointer to the last error that was added
 *  to the global error list.
 */
const EFITestError* efitest_errors_get_last();

/**
 * @return The number of entries in the global error list.
 */
UINTN efitest_errors_get_count();

/**
 * Clear the global error list.
 */
void efitest_errors_clear();

/**
 * Compare the given errors using their UUIDs.
 * See efitest_uuid_compare for more information.
 * @param error1 A pointer to the first error to compare.
 * @param error2 A pointer to the second error to compare.
 * @return True if the UUIDs of both errors are equal.
 */
BOOLEAN efitest_errors_compare(const EFITestError* error1, const EFITestError* error2);

/**
 * Finds the index of the given error within the
 * global error list.
 * @param error The error to search for.
 * @param index A pointer to the index of the entry which is to be found.
 * @return True if the error was found, false otherwise.
 */
BOOLEAN efitest_errors_get_index(const EFITestError* error, UINTN* index);

/* INTERNAL FUNCTIONS USED BY INJECTED CODE AND MACROS */
void efitest_assert(BOOLEAN condition, EFITestContext* context, UINTN line_number, const char* expression);
void efitest_on_pre_run_test(EFITestContext* context);
void efitest_on_post_run_test(EFITestContext* context);
void efitest_on_pre_run_group(EFITestContext* context);
void efitest_on_post_run_group(EFITestContext* context);

ETEST_API_END