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
#include "efitest/efitest_init.h"

#define ETEST_CALL(f, ...) uefi_call_wrapper(f, 0, ##__VA_ARGS__)

// NOLINTBEGIN
static UINTN g_group_pass_count = 0;
static UINTN g_test_count = 0;
static UINTN g_test_pass_count = 0;
static EFITestRunCallback g_pre_run_callback = NULL;
static EFITestRunCallback g_post_run_callback = NULL;
static EFITestCallback g_pre_group_callback = NULL;
static EFITestCallback g_post_group_callback = NULL;
static EFITestCallback g_pre_test_callback = NULL;
static EFITestCallback g_post_test_callback = NULL;
// NOLINTEND

void reset_colors() {
    ETEST_CALL(ST->ConOut->SetAttribute, ST->ConOut, EFI_BACKGROUND_BLACK | EFI_LIGHTGRAY);
}

void set_colors(UINTN colors) {
    ETEST_CALL(ST->ConOut->SetAttribute, ST->ConOut, colors);
}

void yield_cpu() {
#if defined(CPU_X86)
    __asm__ __volatile__("hlt");
#elif defined(CPU_ARM)
    __asm__ __volatile__("wfi");
#elif defined(CPU_RISCV)
    __asm__ __volatile__("pause");
#else
    /* NOP */
#endif
}

_Noreturn void halt_cpu() {
    while(TRUE) {
        yield_cpu();
    }
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
    Print(L"%lu/%lu tests passed in total\n", g_test_pass_count, g_test_count);
}

EFI_STATUS EFIAPI efi_main(EFI_HANDLE image, EFI_SYSTEM_TABLE* sys_table) {
    InitializeLib(image, sys_table);
    InitializeUnicodeSupport((UINT8*) "en-US");
    ETEST_CALL(sys_table->BootServices->SetWatchdogTimer, 0, 0, 0, NULL);
    ETEST_CALL(sys_table->ConOut->ClearScreen, sys_table->ConOut);

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

    halt_cpu();
}

void efitest_on_pre_run_group(EFITestContext* context) {
    g_group_pass_count = 0;
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

    if(g_post_test_callback != NULL) {
        g_post_test_callback(context);
    }
}

void efitest_assert(BOOLEAN condition, EFITestContext* context) {
    context->failed = !condition;
}

void efitest_log_v(const UINT16* format, va_list args) {
    UINT16* message = VPoolPrint(format, args);
    Print(L"%s\n", message);
    FreePool(message);
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