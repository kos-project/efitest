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

// NOLINTBEGIN
static const char* g_dec_digits = "0123456789";
static const char* g_hex_digits = "0123456789aAbBcCdDeEfF";
static const char* g_number_chars = "xXbBuUlLfF.";
static const char* g_alpha_chars = "aAbBcCdDeEfFgGhHiIjJkKlLmMnNoOpPqQrRsStTuUvVwWxXyYzZ";
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
static UINTN g_keyword_count = 0;
static UINTN g_string_prefix_count = 0;
static UINTN g_string_count = 0;
static UINTN g_number_count = 0;
static UINTN g_operator_count = 0;
static UINTN g_identifier_count = 0;
// NOLINTEND

static inline void render_gutter(UINTN line_number) {
    set_colors(EFI_BACKGROUND_LIGHTGRAY | EFI_BLACK);
    Print(L"%-8lu", line_number);
    reset_colors();
    Print(L" ");
}

static inline UINTN count_lines(const char* buffer) {
    UINTN result = 0;
    char* current = (char*) buffer;
    while(*current != '\0') {
        if(*current == '\n') {
            ++result;
        }
        ++current;
    }
    return ++result;
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
    return is_one_of(g_alpha_chars, value);
}

void update_colors() {
    if(g_string_prefix_count > 0) {
        set_colors(EFI_BACKGROUND_BLACK | EFI_LIGHTMAGENTA);// Same as keywords
        return;
    }
    if(g_string_count > 0) {
        set_colors(EFI_BACKGROUND_BLACK | EFI_LIGHTGREEN);
        return;
    }
    if(g_keyword_count > 0) {
        set_colors(EFI_BACKGROUND_BLACK | EFI_LIGHTMAGENTA);
        return;
    }
    if(g_number_count > 0) {
        set_colors(EFI_BACKGROUND_BLACK | EFI_LIGHTCYAN);
        return;
    }
    if(g_operator_count > 0) {
        set_colors(EFI_BACKGROUND_BLACK | EFI_WHITE);
        return;
    }
    if(g_identifier_count > 0) {
        set_colors(EFI_BACKGROUND_BLACK | EFI_YELLOW);
        return;
    }
    reset_colors();
}

void handle_string_state(const char* current) {
    if(g_string_count == 0) {
        for(UINTN index = 0; index < arraylen(g_string_prefixes); ++index) {
            const char* prefix = g_string_prefixes[index];
            const UINTN prefix_length = strlen(prefix);
            if(strlen(current) < prefix_length) {
                continue;
            }
            if(memcmp(prefix, current, prefix_length) == 0) {
                const char* lookahead = current + prefix_length;
                while(*lookahead != '\0') {
                    if(*lookahead == '"' && *(lookahead - 1) != '\\') {
                        const UINTN text_length = ((UINTN) lookahead) - ((UINTN) current);
                        if(prefix_length > 1) {
                            g_string_prefix_count = prefix_length - 1;// Highlight prefixes in keyword color
                        }
                        g_string_count = text_length + 1;// One extra for the closing "
                        return;
                    }
                    ++lookahead;
                }
            }
        }
    }
}

void handle_keyword_state(const char* current) {
    if(g_string_count == 0 && g_keyword_count == 0) {
        for(UINTN index = 0; index < arraylen(g_keywords); ++index) {
            const char* keyword = g_keywords[index];
            const UINTN kw_length = strlen(keyword);
            if(strlen(current) < kw_length) {
                continue;
            }
            if(memcmp(keyword, current, kw_length) == 0) {
                const char* lookahead = current + kw_length;
                while(*lookahead != '\0') {
                    if(*lookahead == ' ') {
                        g_keyword_count = kw_length;
                        return;
                    }
                    ++lookahead;
                }
            }
        }
    }
}

static inline void handle_number_state_internal(const char* current) {
    const char* lookahead = current;
    while(*lookahead != '\0') {
        const char curr_char = *lookahead;
        if(!is_digit(curr_char) && !is_one_of(g_number_chars, curr_char)) {
            g_number_count = ((UINTN) lookahead) - ((UINTN) current);
            return;
        }
        ++lookahead;
    }
}

void handle_number_state(const char* current) {
    if(g_string_count == 0 && g_identifier_count == 0 && g_number_count == 0) {
        if(*current == '-') {
            handle_number_state_internal(current + 1);
            return;
        }
        if(is_dec_digit(*current)) {
            handle_number_state_internal(current);
        }
    }
}

void handle_operator_state(const char* current) {
    if(g_string_count == 0 && g_operator_count == 0) {
        for(UINTN index = 0; index < arraylen(g_operators); ++index) {
            // clang-format off
            const char* operator = g_operators[index]; // Clang format bug..
            // clang-format on
            const UINTN op_length = strlen(operator);
            if(strlen(current) < op_length) {
                continue;
            }
            if(memcmp(operator, current, op_length) == 0) {
                g_operator_count = op_length;
                return;
            }
        }
    }
}

void handle_identifier_state(const char* current) {
    if(g_string_count == 0 && g_identifier_count == 0) {
        const char* lookahead = current;
        BOOLEAN is_first = TRUE;
        while(*lookahead != '\0') {
            if(!is_alpha(*lookahead) && *lookahead != '_' && !(!is_first && is_dec_digit(*lookahead))) {
                g_identifier_count = ((UINTN) lookahead) - ((UINTN) current);
                return;
            }
            ++lookahead;
            is_first = FALSE;
        }
    }
}

void pre_update_states(const char* current) {
    handle_string_state(current);
    handle_keyword_state(current);
    handle_number_state(current);
    handle_operator_state(current);
    handle_identifier_state(current);
}

void post_update_states() {
    if(g_string_prefix_count > 0) {
        --g_string_prefix_count;
    }
    if(g_string_count > 0) {
        --g_string_count;
    }
    if(g_keyword_count > 0) {
        --g_keyword_count;
    }
    if(g_number_count > 0) {
        --g_number_count;
    }
    if(g_operator_count > 0) {
        --g_operator_count;
    }
    if(g_identifier_count > 0) {
        --g_identifier_count;
    }
}

void render_code(const char* buffer, UINTN line_number) {// NOLINT
    const UINTN lines = count_lines(buffer);
    char* current = (char*) buffer;// Cast away const for a local copy
    UINTN line_index = 0;
    UINTN max_width = 0;
    UINTN width = 0;

    render_gutter(line_number);

    while(*current != '\0') {
        pre_update_states(current);

        // Handle new-lines and render additional gutters as needed
        if(*current == '\n' || *(current + 1) == '\0') {
            if(max_width < width) {
                max_width = width;
            }
            width = 0;
        }
        if(*current == '\n') {
            ++line_index;
            render_gutter(line_number + line_index);
        }

        update_colors();
        Print(L"%c", *current);

        post_update_states();

        ++current;
        ++width;
    }
    ++max_width;

    Print(L"\n");

    Print(L"         ");
    set_colors(EFI_BACKGROUND_BLACK | EFI_RED);
    for(UINTN index = 0; index < max_width; ++index) {
        Print(L"^");
    }
    reset_colors();
    Print(L"\n");
}