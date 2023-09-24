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

#pragma once

#include "efitest_api.h"

typedef struct _EFITestContext {
    const char* test_name;// The name of the current test being run
    const char* file_name;// The name of the file the test is defined in
    UINTN line_number;    // The line number where the function is defined
} EFITestContext;

/*
 * Intrinsic macro recognized by the discoverer, don't change!
 * This macro defines a static function that is guaranteed to
 * be inlined. This is so the compiler can inline its code into
 * the generated trampoline function to prevent global scope pollution.
 */
#define ETEST_DEFINE_TEST(n) ETEST_INLINE static inline void n(EFITestContext* context)

// Assertions
#define ETEST_ASSERT(x) efitest_assert((x), context)
#define ETEST_ASSERT_EQ(a, b) efitest_assert((a) == (b), context)
#define ETEST_ASSERT_NE(a, b) efitest_assert((a) != (b), context)
#define ETEST_ASSERT_LT(a, b) efitest_assert((a) < (b), context)
#define ETEST_ASSERT_LE(a, b) efitest_assert((a) <= (b), context)
#define ETEST_ASSERT_GT(a, b) efitest_assert((a) > (b), context)
#define ETEST_ASSERT_GE(a, b) efitest_assert((a) >= (b), context)

// Utilities
#define ETEST_TEST_NAME context->test_name
#define ETEST_FILE_NAME context->file_name
#define ETEST_LINE_NUMBER context->line_number

ETEST_API_BEGIN

void efitest_pre_run_test(EFITestContext* context);
void efitest_post_run_test(EFITestContext* context);

void efitest_assert(BOOLEAN condition, EFITestContext* context);

void efitest_log_v(const UINT16* format, va_list args);
void efitest_log(const UINT16* format, ...);

ETEST_API_END