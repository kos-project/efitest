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

#include <efitest/efitest.h>
#include <efitest/efitest_utils.h>

ETEST_DEFINE_TEST(test_success) {
    ETEST_ASSERT(true);
}

ETEST_DEFINE_TEST(test_failure) {
    ETEST_ASSERT(false);
}

ETEST_DEFINE_TEST(test_compare_success) {
    ETEST_ASSERT_EQ(0x04, (sizeof(UINT16) << (UINT32) 0x01));
}

ETEST_DEFINE_TEST(test_compare_failure) {
    ETEST_ASSERT_EQ(3.444F, (sizeof(UINTN) << (UINT32) 0x02));
}

ETEST_DEFINE_TEST(test_string_compare_success) {
    ETEST_ASSERT_NE("HELLO \"WORLD\"!", NULL);
}

ETEST_DEFINE_TEST(test_string_compare_failure) {
    ETEST_ASSERT_EQ("HELLO \"WORLD\"!", NULL);
}

ETEST_DEFINE_TEST(test_wstring_compare_success) {
    ETEST_ASSERT_NE(L"HELLO \"WORLD\"!", NULL);
}

ETEST_DEFINE_TEST(test_wstring_compare_failure) {
    ETEST_ASSERT_EQ(L"HELLO \"WORLD\"!", NULL);
}

ETEST_DEFINE_TEST(test_ustring_compare_success) {
    ETEST_ASSERT_NE(u8"HELLO \"WORLD\"!", NULL);
}

ETEST_DEFINE_TEST(test_ustring_compare_failure) {
    ETEST_ASSERT_EQ(u8"HELLO \"WORLD\"!", NULL);
}