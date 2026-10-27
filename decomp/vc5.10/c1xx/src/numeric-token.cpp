/* Numeric token spelling and type selection. This source uses the own C++
 * type-code domains and the authentic spelling-reader boundary.
 * Wider semantic-record transfers remain guarded before unknown storage. */
#include <string.h>
#include "parser-state.h"
#include "lexer.h"
#include <recon.h>
#pragma intrinsic(memcpy, strcmp)

#define LEXER_FLAGS frontend_data.compiler_flags
#define literal_text frontend_bss.numeric_literal_text
#ifdef RECON_NATIVE_TEST
#define NUMERIC_LITERAL_VALUE frontend_bss.saved_token_value.literal
#else
#define NUMERIC_LITERAL_VALUE saved_parser_value.literal
#endif
#ifdef RECON_NATIVE_TEST
#define NUMERIC_STORE(ch) do { \
    if (cursor >= literal_text + 128) \
        recon_unsupported("numeric spelling: write crosses bounded scratch owner"); \
    *cursor++ = (char)(ch); \
} while (0)
#else
#define NUMERIC_STORE(ch) (*cursor++ = (char)(ch))
#endif

extern "C" {
unsigned __cdecl process_special_character(void); /*ef91*/
unsigned __cdecl resolve_special_character(unsigned); /*48a42, consumes low byte*/

BYTE __cdecl read_numeric_character(void)
{
    BYTE character = *frontend_bss.source_cursor++;
    if (character) {
        --frontend_bss.source_cursor;
        character = *frontend_bss.source_cursor++;
        if (!frontend_data.scanner.classes[character])
            character = (BYTE)resolve_special_character(character);
        return character;
    }
    if (process_special_character() == 0x20) {
        ++frontend_bss.source_cursor;
        return 0;
    }
    return *frontend_bss.source_cursor++;
}
int __cdecl numeric_digit_value(unsigned character)
{
#ifdef RECON_NATIVE_TEST
    if (character > 255)
        recon_unsupported("numeric digit: unsigned byte domain only");
#endif
    if (frontend_data.scanner.classes[character & 255] == SC_DIGIT)
        return character - '0';
    return (character & 0xdf) - ('A' - 10);
}
unsigned __int64 __cdecl parse_integer_digits(const char *text, int length, int base)
{
    int configured_bytes = frontend_data.option_zb;
    int bits = configured_bytes * 8;
    int short_limit = bits / 4;
    unsigned __int64 value = 0;
#ifdef RECON_NATIVE_TEST
    if (configured_bytes != 4 && configured_bytes != 8 && configured_bytes != 16)
        recon_unsupported("integer spelling: unsupported configured width");
#endif
    if (length > short_limit) {
        while (*text == '0') { ++text; --length; }
        if (!length) return 0;
        if (length > short_limit) {
            int too_large = 0;
            if (base == 16) too_large = 1;
            else if (base == 10) {
                int digits = configured_bytes == 8 ? 20 : configured_bytes == 16 ? 39 : 10;
                if (length >= digits) {
                    const char *maximum = configured_bytes == 8 ? "18446744073709551615" :
                        configured_bytes == 16 ? "340282366920938463463374607431768211455" : "4294967295";
                    too_large = length != digits || strcmp(text, maximum) > 0;
                }
            } else if (base == 8) {
                int digits = bits / 3 + 1;
                if (length >= digits)
                    too_large = length != digits || (BYTE)*text > (bits % 3 == 2 ? '3' : '1');
            }
            if (too_large) { compiler_warning(177); return 0; }
        }
    }
    for (;;) {
        int digit = numeric_digit_value((BYTE)*text);
        if (digit >= base) compiler_warning(41, (BYTE)*text, base);
        value += digit;
        if (!*++text) return value;
        value *= base;
    }
}
CompilerType *__cdecl select_extended_decimal_type(unsigned __int64 *); /*824d3*/
CompilerType *__cdecl select_decimal_literal_type(unsigned __int64 *value)
{
    if (*value <= 0x7fffffff) return frontend_data.builtin_types.type_0063;
    if (frontend_data.option_bm)
        return select_extended_decimal_type(value);
    if (*value > 0xffffffff && frontend_data.option_zb > 4)
        return select_extended_decimal_type(value);
    return frontend_data.builtin_types.type_4044;
}
CompilerType *__cdecl select_based_literal_type(unsigned __int64 *value)
{
    if (*value <= 0x7fffffff) return frontend_data.builtin_types.type_0063;
    if (*value <= 0xffffffff) return frontend_data.builtin_types.type_4063_first;
    if (!frontend_data.options.option_zbe && frontend_data.option_zb <= 4)
        return frontend_data.builtin_types.type_4044;
    return (signed __int64)*value < 0 ? frontend_data.builtin_types.type_404c :
        frontend_data.builtin_types.type_004c;
}
CompilerType *__cdecl select_signed_wide_literal_type(unsigned __int64 *); /*3ed88*/
CompilerType *__cdecl select_unsigned_literal_type(unsigned __int64 *); /*50889*/
CompilerType *__cdecl select_unsigned_wide_literal_type(unsigned __int64 *); /*4fdf3*/
CompilerType *__cdecl select_sized_literal_type(int); /*8240f*/
void __cdecl copy_numeric_spelling(ParserSemanticView *, const char *, unsigned, unsigned); /*1008f*/
unsigned __cdecl collect_floating_token(char *); /*82384, existing*/
ParserTokenView *__cdecl scan_floating_token(char *); /*500ad, existing*/

ParserTokenView *__cdecl scan_numeric_token(unsigned input, int alternate_base)
{
    char *cursor = literal_text;
    BYTE character = (BYTE)input;
    unsigned base, length, arena_kind;
    unsigned __int64 value;
    CompilerType *type = frontend_data.builtin_types.type_0063;
    ParserTokenView *token;
    CompilerArena *arena;
    CompilerPool *pool;
    if ((frontend_bss.options.preprocess_stdout || (LEXER_FLAGS & 0x800000)) &&
        !(LEXER_FLAGS & 0x4000)) {
        literal_text[0] = character;
        length = collect_floating_token(literal_text + 1) + 1;
        if (length >= 128) report_compiler_error(64);
        arena_kind = frontend_bss.symbol_arena_kind;
        if (arena_kind == 5 || (frontend_data.compiler_mode_flags & 0x02000000)) arena_kind = 0;
        copy_numeric_spelling(&saved_parser_value, literal_text, length, arena_kind);
        goto allocate_token;
    }
    if (character == '0') {
        character = read_numeric_character();
        if ((character & 0xdf) == 'X') {
            base = 16;
            character = read_numeric_character();
            while (frontend_data.scanner.flags[character] & 2) {
                NUMERIC_STORE(character);
                character = read_numeric_character();
            }
            if (cursor == literal_text) compiler_warning(153);
            goto scan_suffix;
        }
        base = 8;
        NUMERIC_STORE('0');
    } else base = 10;
    while (alternate_base ? (frontend_data.scanner.flags[character] & 2) :
                           frontend_data.scanner.classes[character] == SC_DIGIT) {
        NUMERIC_STORE(character);
        character = read_numeric_character();
        /* Preserve the original diagnostic point after fetching the next
         * character. Native checks before every write and terminating NUL. */
        if (cursor > literal_text + 128) report_compiler_error(64);
    }
    if (!alternate_base) {
        if (character == '.' || (character & 0xdf) == 'E') {
            --frontend_bss.source_cursor;
            return scan_floating_token(cursor);
        }
    } else {
        BYTE suffix = (BYTE)cursor[-1];
        if ((frontend_data.scanner.flags[suffix] & 0x10) && (character & 0xdf) != 'H')
            --cursor;
        else {
            suffix = character;
            if (!(frontend_data.scanner.flags[suffix] & 0x10)) goto scan_suffix;
            character = read_numeric_character();
        }
        switch (suffix & 0xdf) {
        case 'B': base = 2; break;
        case 'D': base = 10; break;
        case 'H': base = 16; break;
        case 'O': case 'Q': base = 8; break;
        }
    }
scan_suffix:
    character &= 0xdf;
    if (character == 'L') {
        character = read_numeric_character() & 0xdf;
        if (character == 'U') type = frontend_data.builtin_types.type_4044;
        else {
            --frontend_bss.source_cursor;
            type = frontend_data.builtin_types.type_0044;
        }
    } else if (frontend_data.option_zb > 0 && character == 'I') {
        type = select_sized_literal_type(0);
    } else if (character == 'U') {
        character = read_numeric_character() & 0xdf;
        if (character == 'L') type = frontend_data.builtin_types.type_4044;
        else if (frontend_data.option_zb > 0 && character == 'I')
            type = select_sized_literal_type(1);
        else {
            --frontend_bss.source_cursor;
            type = frontend_data.builtin_types.type_4063_first;
        }
    } else --frontend_bss.source_cursor;
    if (frontend_data.scanner.flags[*frontend_bss.source_cursor] & 1)
        compiler_warning(59, 77, 306);
    NUMERIC_STORE(0);
    length = (unsigned)(cursor - literal_text);
    value = length <= 1 ? 0 : parse_integer_digits(literal_text, length - 1, base);
    switch (type->flags) {
    case 0x63:
        type = base == 10 ? select_decimal_literal_type(&value) : select_based_literal_type(&value);
        break;
    case 0x41: value = (signed __int64)(value << 56) >> 56; break;
    case 0x42: value = (signed __int64)(value << 48) >> 48; break;
    case 0x43: value = (signed __int64)(value << 32) >> 32; break;
    case 0x44: type = select_signed_wide_literal_type(&value); break;
    case 0x4d:
        /* Own original CL=c0 for both compiler shift helpers; each clamps
         * counts>=64, producing zero. Avoid an undefined oversized C shift. */
        value = 0;
        break;
    case 0x4041: value &= 0xff; break;
    case 0x4042: value &= 0xffff; break;
    case 0x4043: value &= 0xffffffff; break;
    case 0x4044: type = select_unsigned_wide_literal_type(&value); break;
    case 0x404d: value = 0; break;
    case 0x4063: type = select_unsigned_literal_type(&value); break;
    }
    NUMERIC_LITERAL_VALUE.type = type;
    NUMERIC_LITERAL_VALUE.value = value;
allocate_token:
    arena_kind = frontend_bss.symbol_arena_kind;
    if (arena_kind) {
#ifdef RECON_NATIVE_TEST
        if (arena_kind >= 9) recon_unsupported("numeric token: unsupported arena index");
#endif
        arena = &frontend_bss.arenas[arena_kind];
        token = (ParserTokenView *)arena->cursor;
        arena->cursor += 48;
        if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    } else {
        pool = &frontend_bss.pools[7];
        if (pool->remaining < 48) pool->Refill(48);
        token = (ParserTokenView *)pool->cursor;
        pool->cursor += 48; pool->remaining -= 48;
    }
    if (token) {
        token->kind = 101;
        /* Native saved_parser_value is held before this wider40B transfer.
         * The genuine original source retains its complete record copy. */
#ifdef RECON_NATIVE_TEST
        memcpy(&token->value, &frontend_bss.saved_token_value, 40);
#else
        memcpy(&token->value, &saved_parser_value, 40);
#endif
    }
    return token;
}
}
