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
static const char* g_string_prefixes[] = {"L\"", "u\"", "U\"", "u8\"", "u16\"", "u32\"", "\""};
// clang-format off
static const char* g_keywords[] = {
        // Shared keywords
        "void",
        "char",
        "short",
        "int",
        "long",
        "unsigned"
        "float",
        "double",
        "true",
        "false",
        "nullptr",
        "bool",
        "sizeof",
        "if",
        "elseif",
        "else",
        "for",
        "while",
        "do",
        "goto",
        "struct",
        // C keywords
        "_Atomic",
        "_Thread_local",
        "_Noreturn",
        "_Bool",
        "_Generic",
        // C++ keywords
        "template",
        "typename",
        "using",
        "friend",
        "class"
};
static const char* g_operators[] = {
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
// NOLINTEND

static inline void render_gutter(UINTN line_number) {
    set_colors(EFI_BACKGROUND_LIGHTGRAY | EFI_BLACK);
    Print(L"%-6lu", line_number);
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

void update_colors() {
    if(g_string_prefix_count > 0) {
        set_colors(EFI_BACKGROUND_BLACK | EFI_LIGHTRED);
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

static inline BOOLEAN is_one_of(const char* chars, UINTN num_chars, char value) {
    for(UINTN index = 0; index < num_chars; ++index) {
        if(chars[index] != value) {
            continue;
        }
        return TRUE;
    }
    return FALSE;
}

static inline BOOLEAN is_dec_digit(char value) {
    return is_one_of(g_dec_digits, strlen(g_dec_digits), value);
}

static inline BOOLEAN is_hex_digit(char value) {
    return is_one_of(g_hex_digits, strlen(g_hex_digits), value);
}

static inline BOOLEAN is_bin_digit(char value) {
    return value == '0' || value == '1';
}

void handle_number_state(const char* current) {
    if(g_string_count == 0 && g_number_count == 0) {
        char curr_char = *current;
        if(curr_char == '-' || is_dec_digit(curr_char)) {
            const char* lookahead = current;
            BOOLEAN prefixed = FALSE;

            while(*lookahead != '\0') {
                curr_char = *lookahead;
                const char next_char = *(lookahead + 1);
                if(curr_char == '0' && is_one_of("xXbB", 4, next_char)) {}
                if(!is_dec_digit(curr_char)) {}
                ++lookahead;
            }
        }
    }
}

void handle_operator_state(const char* current) {
    if(g_string_count == 0 && g_operator_count == 0) {
        for(UINTN index = 0; index < arraylen(g_operators); ++index) {
            const char* operator= g_operators[index];
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

void pre_update_states(const char* current) {
    handle_string_state(current);
    handle_keyword_state(current);
    handle_number_state(current);
    handle_operator_state(current);
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
}

void render_code(const char* buffer, UINTN line_number) {// NOLINT
    const UINTN lines = count_lines(buffer);
    char* current = (char*) buffer;// Cast away const for a local copy
    UINTN line_index = 0;

    render_gutter(line_number - 1);
    Print(L"...\n");
    render_gutter(line_number);

    while(*current != '\0') {
        pre_update_states(current);

        // Handle new-lines and render additional gutters as needed
        if(*current == '\n') {
            ++line_index;
            render_gutter(line_number + line_index);
        }

        update_colors();
        Print(L"%c", *current);
        post_update_states();

        ++current;
    }

    Print(L"\n");
    render_gutter(line_number + lines);
    Print(L"...\n\n");
}