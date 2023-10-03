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

#include "efitest/efitest.h"
#include "code_renderer.h"
#include "efitest/efitest_init.h"
#include "efitest/efitest_utils.h"

// NOLINTBEGIN
static const char* g_hex_chars = "0123456789ABCDEF";// Used for UUID string conversion
static UINTN g_group_pass_count = 0;
static UINTN g_group_error_count = 0;
static UINTN g_test_count = 0;
static UINTN g_test_pass_count = 0;
static EFITestRunCallback g_pre_run_callback = NULL;
static EFITestRunCallback g_post_run_callback = NULL;
static EFITestCallback g_pre_group_callback = NULL;
static EFITestCallback g_post_group_callback = NULL;
static EFITestCallback g_pre_test_callback = NULL;
static EFITestCallback g_post_test_callback = NULL;
static EFITestError* g_errors = NULL;
static UINTN g_error_count = 0;
// RNG statae
static UINT64 g_rand_z = 362436069;// Value suggested by author
static UINT64 g_rand_w = 521288629;// Value suggested by author
// NOLINTEND

/*
 * Implementation based on MWC generator described in
 * http://www.cse.yorku.ca/~oz/marsaglia-rng.html
 */
UINT32 rand() {// clang-format off
    return (
        ((g_rand_z = 36969 * (g_rand_z & UINT16_MAX) + (g_rand_z >> UINT16_WIDTH)) << UINT16_WIDTH)
        + (g_rand_w = 18000 * (g_rand_w & UINT16_MAX) + (g_rand_w >> UINT16_WIDTH))
    );
}// clang-format on

_Noreturn void shutdown() {
    UEFI_CALL(ST->RuntimeServices->ResetSystem, EfiResetShutdown, EFI_SUCCESS, 0, NULL);
    __builtin_unreachable();
}

void print_test_result(const EFITestContext* context) {
    if(context->failed) {
        set_colors(EFI_RED);
        Print(L"[FAILED] ");
    }
    else {
        set_colors(EFI_GREEN);
        Print(L"[--OK--] ");
    }
    set_colors(EFI_WHITE);
    Print(L"%a\n", context->test_name);
    reset_colors();
}

void print_error(const EFITestError* error) {
    render_code(error->expression, error->line_number);
    Print(L"\n");
}

void print_test_results() {
    Print(L"[------] Test run finished!\n");
    if(g_test_pass_count < g_test_count) {
        set_colors(g_test_pass_count <= (g_test_count >> 1) ? EFI_RED : EFI_YELLOW);
        Print(L"[FAILED] ");
    }
    else {
        set_colors(EFI_GREEN);
        Print(L"[--OK--] ");
    }
    reset_colors();
    Print(L"%lu/%lu tests passed in total\n\n", g_test_pass_count, g_test_count);
}

__attribute__((unused)) EFI_STATUS EFIAPI efi_main(EFI_HANDLE image, EFI_SYSTEM_TABLE* sys_table) {
    InitializeLib(image, sys_table);
    InitializeUnicodeSupport((UINT8*) "en-US");

    UEFI_CALL(sys_table->BootServices->SetWatchdogTimer, 0, 0, 0, NULL);
    UEFI_CALL(sys_table->ConOut->ClearScreen, sys_table->ConOut);

    set_colors(EFI_BACKGROUND_BLUE | EFI_WHITE);
    Print(L"== EFITEST Integrated Testing Environment ==\n");
    Print(L"Copyright (C) 2023 Karma Krafts & associates\n");
    reset_colors();
    Print(L"\n");

    if(g_pre_run_callback != NULL) {
        g_pre_run_callback();
    }

    EFITestContext context;
    efitest_run_tests(&context);
    print_test_results();

    if(g_post_run_callback != NULL) {
        g_post_run_callback();
    }

    free(g_errors);
    shutdown();
}

/*
 * Implementation based on the procedure described in
 * https://www.cryptosys.net/pki/uuid-rfc4122.html
 */
void efitest_uuid_generate(EFITestUUID* value) {
    for(UINT32 index = 0; index < 4; ++index) {
        UINT32* data = &(value->data[index]);
        *data = rand();// Generate 4 bytes at a time
        switch(index) {
            case 1: {
                UINT8* byte_ptr = ((UINT8*) data) + 2;
                // Replace high nibble with '4'
                *byte_ptr = (*byte_ptr & 0b00001111) | 0b01000000;
                break;
            }
            case 2: {
                UINT8* byte_ptr = ((UINT8*) data);
                // Replace two MSB with '10' so byte is '8', '9', 'A', 'B'
                *byte_ptr = (*byte_ptr & 0b00111111) | 0b10000000;
                break;
            }
        }
    }
}

void efitest_uuid_to_string(const EFITestUUID* value, char* buffer) {
    UINT8* data = (UINT8*) value->data;
    for(UINT32 index = 0; index < 16; ++index) {
        const UINT8 byte = data[index];
        *(buffer++) = g_hex_chars[(byte & 0xFF) >> 4];
        *(buffer++) = g_hex_chars[byte & 0x0F];
        if(index == 3 || index == 5 || index == 7 || index == 9) {
            *(buffer++) = '-';
        }
    }
}

BOOLEAN efitest_uuid_compare(const EFITestUUID* value1, const EFITestUUID* value2) {
    const UINT32* data1 = value1->data;
    const UINT32* data2 = value2->data;
    for(UINT32 index = 0; index < 4; ++index) {
        if(data1[index] != data2[index]) {
            return FALSE;
        }
    }
    return TRUE;
}

void efitest_on_pre_run_group(EFITestContext* context) {
    g_group_pass_count = 0;
    g_group_error_count = 0;
    Print(L"[------] Running test group '%a'..\n", context->group_name);

    if(g_pre_group_callback != NULL) {
        g_pre_group_callback(context);
    }
}

void efitest_on_post_run_group(EFITestContext* context) {
    const UINTN group_size = context->group_size;

    if(g_group_pass_count < group_size) {
        set_colors(g_group_pass_count <= (group_size >> 1) ? EFI_RED : EFI_YELLOW);
        Print(L"[FAILED] ");
    }
    else {
        set_colors(EFI_GREEN);
        Print(L"[--OK--] ");
    }
    reset_colors();
    Print(L"%lu/%lu tests passed\n\n", g_group_pass_count, group_size);

    g_test_count += group_size;

    if(g_post_group_callback != NULL) {
        g_post_group_callback(context);
    }

    if(g_group_error_count > 0) {
        set_colors(EFI_BACKGROUND_BLACK | EFI_RED);
        Print(L"Assertion%a in ", g_group_error_count == 1 ? "" : "s");
        set_colors(EFI_BACKGROUND_BLACK | EFI_LIGHTRED);
        Print(L"%a ", context->file_name);
        set_colors(EFI_BACKGROUND_BLACK | EFI_RED);
        Print(L"%a failed:\n\n", g_group_error_count == 1 ? "has" : "have");
        reset_colors();

        for(UINTN index = 0; index < g_group_error_count; ++index) {
            print_error(efitest_errors_get_last() - ((g_group_error_count - 1) - index));
        }
    }
}

void efitest_on_pre_run_test(EFITestContext* context) {
    if(g_pre_test_callback != NULL) {
        g_pre_test_callback(context);
    }
}

void efitest_on_post_run_test(EFITestContext* context) {
    print_test_result(context);
    if(!context->failed) {
        ++g_group_pass_count;
        ++g_test_pass_count;
    }
    else {
        ++g_group_error_count;
    }

    if(g_post_test_callback != NULL) {
        g_post_test_callback(context);
    }
}

void efitest_assert(BOOLEAN condition, EFITestContext* context, UINTN line_number, const char* expression) {
    if((context->failed = !condition)) {
        EFITestError error;
        efitest_uuid_generate(&(error.uuid));
        error.context = *context;
        error.line_number = line_number;
        error.expression = expression;
        efitest_errors_add(&error);
    }
}

void efitest_logln_v(const UINT16* format, va_list args) {
    UINT16* message = VPoolPrint(format, args);
    Print(L"%s\n", message);
    FreePool(message);
}

void efitest_logln(const UINT16* format, ...) {
    va_list args;
    va_start(args, format);
    efitest_log_v(format, args);
    va_end(args);
}

void efitest_log_v(const UINT16* format, va_list args) {
    Print(format, args);
}

void efitest_log(const UINT16* format, ...) {
    va_list args;
    va_start(args, format);
    efitest_log_v(format, args);
    va_end(args);
}

void efitest_set_pre_run_callback(EFITestRunCallback callback) {
    g_pre_run_callback = callback;
}

void efitest_set_post_run_callback(EFITestRunCallback callback) {
    g_post_run_callback = callback;
}

void efitest_set_pre_group_callback(EFITestCallback callback) {
    g_pre_group_callback = callback;
}

void efitest_set_post_group_callback(EFITestCallback callback) {
    g_post_group_callback = callback;
}

void efitest_set_pre_test_callback(EFITestCallback callback) {
    g_pre_test_callback = callback;
}

void efitest_set_post_test_callback(EFITestCallback callback) {
    g_post_test_callback = callback;
}

void efitest_errors_add(const EFITestError* error) {
    const UINTN index = g_error_count++;
    g_errors = realloc(g_errors, g_error_count * sizeof(EFITestError));
    g_errors[index] = *error;
}

const EFITestError* efitest_errors_get() {
    return g_errors;
}

const EFITestError* efitest_errors_get_last() {
    if(g_errors == NULL) {
        return NULL;
    }
    return g_errors + (g_error_count - 1);
}

UINTN efitest_errors_get_count() {
    return g_error_count;
}

void efitest_errors_clear() {
    g_error_count = 0;
}

BOOLEAN efitest_errors_compare(const EFITestError* error1, const EFITestError* error2) {
    return efitest_uuid_compare(&(error1->uuid), &(error2->uuid));
}

BOOLEAN efitest_errors_get_index(const EFITestError* error, UINTN* index) {
    for(UINTN curr_index = 0; curr_index < g_error_count; ++curr_index) {
        EFITestError* current = &(g_errors[curr_index]);
        if(!efitest_errors_compare(current, error)) {
            continue;
        }
        *index = curr_index;
        return TRUE;
    }
    *index = 0;
    return FALSE;
}