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

#include "code_renderer.h"
#include "efitest/efitest_utils.h"

#define STACK_BUFFER_SIZE 1024

// NOLINTBEGIN
static const char* g_dec_digits = "0123456789";
static const char* g_hex_digits = "0123456789aAbBcCdDeEfF";
static const char* g_number_chars = "xXbBuUlLfF.";
static const char* g_string_prefixes[] = {"L\"", "u\"", "U\"", "u8\"", "u16\"", "u32\"", "\""};
// clang-format off
static const char* g_keywords[] = {
        // Shared keywords
        "void",
        "char", "short", "int", "long",
        "unsigned", "signed",
        "float", "double",
        "true", "false", "nullptr",
        "bool",
        "sizeof", "alignas", "alignof",
        "if", "elseif", "else",
        "for", "while", "do",
        "goto", "continue",
        "switch", "break", "case", "default",
        "inline", "static", "volatile", "extern", "register",
        "static_assert",
        "thread_local",
        "typedef",
        "typeof", "typeof_unqual",
        "const", "constexpr",
        "struct", "union", "enum",
        // C keywords
        "restrict",
        "_Atomic", "_Thread_local",
        "_Noreturn",
        "_Bool",
        "_Alignas", "_Alignof",
        "_Complex", "_Imaginary", "_BitInt",
        "_Decimal128", "_Decimal64", "_Decimal32",
        "_Static_assert",
        "_Pragma", "_Generic",
        // C++ keywords
        "concept", "requires",
        "template", "typename", "decltype",
        "public", "protected", "private",
        "using",
        "friend", "noexcept", "explicit", "mutable",
        "virtual", "final", "override",
        "class",
        "asm",
        "and", "and_eq", "bitand", "bitor", "compl", "not", "not_eq", "xor", "xor_eq",
        "atomic_cancel", "atomic_commit", "atomic_noexcept",
        "auto",
        "try", "catch", "throw",
        "char8_t", "char16_t", "char32_t",
        "consteval", "constinit",
        "co_await", "co_return", "co_yield",
        "new", "delete",
        "dynamic_cast", "const_cast", "reinterpret_cast", "static_cast",
        "export", "import", "module",
        "namespace",
        "reflexpr",
        "this",
        "typeid",
        "transaction_safe", "transaction_safe_dynamic", "synchronized",
        // GCC/Clang extensions
        "__asm__", "__volatile__", "__attribute__",
        // MSVC extensions
        "__asm", "__volatile", "__forceinline", "__declspec",
        // Pseudo keywords (standard types)
        "int8_t", "int16_t", "int32_t", "int64_t",
        "uint8_t", "uint16_t", "uint32_t", "uint64_t",
        "size_t", "ptrdiff_t",
        "intptr_t", "uintptr_t",
        "wchar_t"
};
static const char* g_operators[] = {
        "...",
        "<<=", ">>=", "|=", "&=", "^=",
        "<<", ">>", "||", "|", "&&", "&", "^",
        "++", "--",
        "+=", "-=", "*=", "/=", "%=",
        "==", "!=",
        "+", "-", "*", "/", "%", "~", "!"
};
// clang-format on
// NOLINTEND

static inline void render_gutter(UINTN line_number) {
    set_colors(EFI_BACKGROUND_LIGHTGRAY | EFI_BLACK);
    Print(L"%-8lu", line_number);
    reset_colors();
    Print(L" ");
}

static inline BOOLEAN is_one_of(const char* chars, char value) {
    const UINTN length = strlen(chars);
    for(UINTN index = 0; index < length; ++index) {
        if(chars[index] != value) {
            continue;
        }
        return TRUE;
    }
    return FALSE;
}

static inline BOOLEAN is_dec_digit(char value) {
    return is_one_of(g_dec_digits, value);
}

static inline BOOLEAN is_hex_digit(char value) {
    return is_one_of(g_hex_digits, value);
}

static inline BOOLEAN is_bin_digit(char value) {
    return value == '0' || value == '1';
}

static inline BOOLEAN is_digit(char value) {
    return is_dec_digit(value) | is_hex_digit(value) | is_bin_digit(value);
}

static inline BOOLEAN is_alpha(char value) {
    return (value >= 'A' && value <= 'Z') || (value >= 'a' && value <= 'z');
}

BOOLEAN handle_string_state(const char* current, UINTN* advance) {
    for(UINTN index = 0; index < arraylen(g_string_prefixes); ++index) {
        const char* prefix = g_string_prefixes[index];
        const UINTN prefix_length = strlen(prefix);
        if(strlen(current) < prefix_length) {
            continue;
        }
        if(memcmp(prefix, current, prefix_length) == 0) {
            const char* lookahead = current + prefix_length;
            do {
                const char prev_char = *(lookahead - 1);
                const char curr_char = *(lookahead++);
                if(curr_char == '"' && prev_char != '\\') {
                    const UINTN text_length = ((UINTN) lookahead) - ((UINTN) current);
                    *advance = (text_length + 1);
                    set_colors(EFI_LIGHTGREEN);
                    return TRUE;
                }
            } while(*lookahead != '\0');
        }
    }
    return FALSE;
}

static inline BOOLEAN is_keyword_anchor(char value) {
    return !is_alpha(value) && !is_dec_digit(value) && value != '_';
}

BOOLEAN handle_keyword_state(const char* current, UINTN* advance) {
    if(!is_keyword_anchor(*(current - 1))) {
        return FALSE;
    }
    for(UINTN index = 0; index < arraylen(g_keywords); ++index) {
        const char* keyword = g_keywords[index];
        const UINTN kw_length = strlen(keyword);
        if(strlen(current) < kw_length) {
            continue;
        }
        if(memcmp(keyword, current, kw_length) == 0) {
            if(!is_keyword_anchor(*(current + kw_length))) {
                return FALSE;
            }

            *advance = kw_length;
            set_colors(EFI_LIGHTMAGENTA);
            return TRUE;
        }
    }
    return FALSE;
}

BOOLEAN handle_number_state(const char* current, UINTN* advance) {
    if(is_dec_digit(*current) || *current == '-') {
        const char* lookahead = current;
        do {
            const char curr_char = *(++lookahead);
            if(!is_digit(curr_char) && !is_one_of(g_number_chars, curr_char)) {
                *advance = ((UINTN) lookahead) - ((UINTN) current);
                set_colors(EFI_LIGHTCYAN);
                return TRUE;
            }
        } while(*lookahead != '\0');
    }
    return FALSE;
}

BOOLEAN handle_operator_state(const char* current, UINTN* advance) {
    for(UINTN index = 0; index < arraylen(g_operators); ++index) {
        // clang-format off
            const char* operator = g_operators[index]; // Clang format bug..
        // clang-format on
        const UINTN op_length = strlen(operator);
        if(strlen(current) < op_length) {
            continue;
        }
        if(memcmp(operator, current, op_length) == 0) {
            *advance = op_length;
            set_colors(EFI_WHITE);
            return TRUE;
        }
    }
    return FALSE;
}

BOOLEAN handle_identifier_state(const char* current, UINTN* advance) {
    if(!is_alpha(*current) && *current != '_') {
        return FALSE;
    }
    const char* lookahead = current;
    do {
        const char curr_char = *(++lookahead);
        if(!is_alpha(curr_char) && !is_dec_digit(curr_char) && curr_char != '_') {
            *advance = ((UINTN) lookahead) - ((UINTN) current);
            set_colors(EFI_YELLOW);
            return TRUE;
        }
    } while(*lookahead != '\0');

    return FALSE;
}

UINTN update_state(const char* current) {
    UINTN advance = 1;

    if(handle_string_state(current, &advance)) {
        return advance;
    }
    if(handle_keyword_state(current, &advance)) {
        return advance;
    }
    if(handle_number_state(current, &advance)) {
        return advance;
    }
    if(handle_operator_state(current, &advance)) {
        return advance;
    }
    handle_identifier_state(current, &advance);
    return advance;
}

void render_code(const char* buffer, UINTN line_number) {// NOLINT
    char* current = (char*) buffer;                      // Cast away const for a local copy
    UINTN line_index = 0;
    UINTN line_width = 0;
    UINTN max_width = 0;

    render_gutter(line_number);
    char stack_buffer[STACK_BUFFER_SIZE];
    void* heap_buffer = NULL;

    while(*current != '\0') {
        if(*current == '\n') {
            ++line_index;
            render_gutter(line_number + line_index);

            line_width = 0;
        }

        reset_colors();
        const UINTN advance = update_state(current);

        if(advance < STACK_BUFFER_SIZE) {
            memset(stack_buffer, '\0', advance + 1);
            memcpy(stack_buffer, current, advance);
            Print(L"%a", stack_buffer);
        }
        else {// Heap buffer for chunks > 1024 chars
            heap_buffer = realloc(heap_buffer, advance + 1);
            memset(heap_buffer, '\0', advance + 1);
            memcpy(heap_buffer, current, advance);
            Print(L"%a", heap_buffer);
        }

        current += advance;
        line_width += advance;
        if(max_width < line_width) {
            max_width = line_width;
        }
    }

    Print(L"\n");

    Print(L"         ");
    set_colors(EFI_BACKGROUND_BLACK | EFI_RED);
    for(UINTN index = 0; index < max_width; ++index) {
        Print(L"^");
    }
    reset_colors();
    Print(L"\n");

    free(heap_buffer);
}