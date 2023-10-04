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
 * @since 26/09/2023
 */

#pragma once

#include "efitest.h"

#define UEFI_CALL(f, ...) uefi_call_wrapper(f, 0, ##__VA_ARGS__)

#define RETURN_IF_ERROR(x, ...)                                                                                        \
    do {                                                                                                               \
        if(x != EFI_SUCCESS) {                                                                                         \
            set_colors(EFI_RED);                                                                                       \
            efitest_logf(L"[ERROR-] %r", x);                                                                           \
            reset_colors();                                                                                            \
            return __VA_ARGS__;                                                                                        \
        }                                                                                                              \
    } while(0)

#define malloc(size) malloc_impl(size)
#define free(address) free_impl(address)
#define realloc(address, size) realloc_impl(address, size)
#define memcpy(dst, src, size) CopyMem(dst, src, size)
#define memset(address, value, size) SetMem(address, size, value)
#define memcmp(addr1, addr2, size) CompareMem(addr1, addr2, size)

#define usable_size(address) (*(const UINTN*) (((const UINT8*) (address)) - sizeof(UINTN)))

#define strlen(x) strlena((const UINT8*) (x))
#define strcmp(a, b) strcmpa((const UINT8*) a, (const UINT8*) b)
#define arraylen(x) (sizeof(x) / sizeof(*x))

static inline void reset_colors() {
    UEFI_CALL(ST->ConOut->SetAttribute, ST->ConOut, EFI_BACKGROUND_BLACK | EFI_LIGHTGRAY);
}

static inline void set_colors(UINTN colors) {
    UEFI_CALL(ST->ConOut->SetAttribute, ST->ConOut, colors);
}

static inline void* malloc_impl(UINTN size) {
    void* address = NULL;
    RETURN_IF_ERROR(UEFI_CALL(ST->BootServices->AllocatePool, EfiLoaderData, size + sizeof(UINTN), &address), NULL);
    memset(address, 0, size);
    *((UINTN*) address) = size;
    return ((UINT8*) address) + sizeof(UINTN);
}

static inline void free_impl(void* address) {
    if(address == NULL) {
        return;
    }
    UEFI_CALL(ST->BootServices->FreePool, ((UINT8*) address) - sizeof(UINTN));
}

static inline void* realloc_impl(void* address, UINTN size) {
    void* new_address = malloc(size);
    if(address != NULL) {
        const UINTN old_size = usable_size(address);
        if(size <= old_size) {
            return address;
        }
        memcpy(new_address, address, old_size);
        free(address);
    }
    return new_address;
}