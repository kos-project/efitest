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

#include "efitest/efitest.h"
#include "efitest/efitest_init.h"

#define ETEST_CALL(f, ...) uefi_call_wrapper(f, 0, ##__VA_ARGS__)

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

EFI_STATUS EFIAPI efi_main(EFI_HANDLE image, EFI_SYSTEM_TABLE* sys_table) {
    InitializeLib(image, sys_table);
    InitializeUnicodeSupport((UINT8*) "en-US");
    ETEST_CALL(sys_table->ConOut->ClearScreen, sys_table->ConOut);

    EFITestContext context;
    efitest_run_tests(&context);

    halt_cpu();
}

void efitest_pre_run_test(EFITestContext* context) {
}

void efitest_post_run_test(EFITestContext* context) {
}

void efitest_assert(BOOLEAN condition, EFITestContext* context) {
}

void efitest_log_v(const UINT16* format, va_list args) {
    UINT16* message = VPoolPrint(format, args);
    Print(L"[EFITEST] %s\n", message);
    FreePool(message);
}

void efitest_log(const UINT16* format, ...) {
    va_list args;
    va_start(args, format);
    efitest_log_v(format, args);
    va_end(args);
}