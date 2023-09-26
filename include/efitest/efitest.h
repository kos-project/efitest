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
#define ETEST_ASSERT(x) efitest_assert((x), context, __LINE__ - 4, #x)
#define ETEST_ASSERT_EQ(a, b) ETEST_ASSERT(a == b)
#define ETEST_ASSERT_NE(a, b) ETEST_ASSERT(a != b)
#define ETEST_ASSERT_LT(a, b) ETEST_ASSERT(a < b)
#define ETEST_ASSERT_LE(a, b) ETEST_ASSERT(a <= b)
#define ETEST_ASSERT_GT(a, b) ETEST_ASSERT(a > b)
#define ETEST_ASSERT_GE(a, b) ETEST_ASSERT(a >= b)

// Macros that can be used within the test
#define ETEST_TEST_NAME (context->test_name)
#define ETEST_FILE_NAME (context->file_name)
#define ETEST_FILE_PATH (context->file_path)
#define ETEST_GROUP_NAME (context->group_name)
#define ETEST_GROUP_SIZE (context->group_size)
#define ETEST_GROUP_INDEX (context->group_index)
#define ETEST_LINE_NUMBER (context->line_number)
#define ETEST_FAILED (context->failed)

#define ETEST_UUID_LENGTH 36

ETEST_API_BEGIN

typedef void (*EFITestCallback)(const EFITestContext* context);
typedef void (*EFITestRunCallback)();

void efitest_uuid_generate(EFITestUUID* value);
void efitest_uuid_to_string(const EFITestUUID* value, char* buffer);
BOOLEAN efitest_uuid_compare(const EFITestUUID* value1, const EFITestUUID* value2);

void efitest_on_pre_run_test(EFITestContext* context);
void efitest_on_post_run_test(EFITestContext* context);
void efitest_on_pre_run_group(EFITestContext* context);
void efitest_on_post_run_group(EFITestContext* context);

void efitest_assert(BOOLEAN condition, EFITestContext* context, UINTN line_number, const char* expression);
void efitest_log_v(const UINT16* format, va_list args);
void efitest_log(const UINT16* format, ...);

void efitest_set_pre_run_callback(EFITestRunCallback callback);
void efitest_set_post_run_callback(EFITestRunCallback callback);

void efitest_set_pre_group_callback(EFITestCallback callback);
void efitest_set_post_group_callback(EFITestCallback callback);

void efitest_set_pre_test_callback(EFITestCallback callback);
void efitest_set_post_test_callback(EFITestCallback callback);

void efitest_errors_add(const EFITestError* error);
const EFITestError* efitest_errors_get();
const EFITestError* efitest_errors_get_last();
UINTN efitest_errors_get_count();
void efitest_errors_clear();
BOOLEAN efitest_errors_compare(const EFITestError* error1, const EFITestError* error2);
BOOLEAN efitest_errors_get_index(const EFITestError* error, UINTN* index);

ETEST_API_END