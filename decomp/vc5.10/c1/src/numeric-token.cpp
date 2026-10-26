#include <string.h>
#include "frontend.h"
#include "scanner.h"
#pragma intrinsic(memcpy, strcmp)
#include <recon.h>

/* One numeric-spelling workspace at RVA54f38. The producer's bound is128;
 * this declaration does not give the bytes a semantic initial value. */
extern "C" char c1_numeric_token_text[128];
char c1_numeric_token_text[128];
struct CompilerNumericToken {
    BYTE kind;
    BYTE unmodeled_01[7];
    CompilerParserValue value;
};
typedef char NumericToken_size[sizeof(CompilerNumericToken) == 32 ? 1 : -1];
extern "C" {
void __cdecl compiler_warning(int, ...);
unsigned char __cdecl read_numeric_character(void);
unsigned __cdecl process_special_character(void);
unsigned __cdecl resolve_special_character(unsigned);
unsigned __int64 __cdecl parse_integer_digits(const char *, int, int);
int __cdecl numeric_digit_value(unsigned);
CompilerType *__cdecl select_decimal_literal_type(unsigned __int64 *);
CompilerType *__cdecl select_extended_integer_type(unsigned __int64 *);
CompilerType *__cdecl select_based_literal_type(unsigned __int64 *);
CompilerType *__cdecl select_signed_wide_literal_type(unsigned __int64 *);
CompilerType *__cdecl select_unsigned_literal_type(unsigned __int64 *);
CompilerType *__cdecl select_unsigned_wide_literal_type(unsigned __int64 *);
CompilerType *__cdecl select_sized_literal_type(int);
unsigned __cdecl collect_floating_token(char *);
struct CompilerParserToken *__cdecl scan_floating_token(char *);
void __cdecl copy_numeric_spelling(CompilerValue *, const char *, unsigned, unsigned);

CompilerParserToken *__cdecl scan_numeric_token(unsigned input, int alternate_base)
{
    char *cursor = c1_numeric_token_text;
    BYTE character = (BYTE)input;
    unsigned base;
    unsigned length;
    unsigned __int64 value;
    CompilerType *type = c1_data.builtin_types.type_0063;
    CompilerNumericToken *token;
    if (input < '0' || input > '9' || alternate_base || c1_bss.options.preprocess_stdout ||
        (c1_data.lexer_flags & 0x800000)) {
#ifdef __linux__
        recon_unsupported("numeric token mode is outside ordinary integer literals");
#else
        __asm { int 3 }
#endif
    }
    if ((c1_bss.options.preprocess_stdout || (c1_data.lexer_flags & 0x800000)) &&
        !(c1_data.lexer_flags & 0x4000)) {
        *cursor = character;
        length = collect_floating_token(cursor + 1) + 1;
        if (length >= 128) compiler_fatal(64);
        unsigned arena_kind = c1_bss.symbol_arena_kind;
        copy_numeric_spelling(&c1_bss.literal, cursor, length, arena_kind == 3 ? 0 : arena_kind);
        goto allocate_token;
    }
    if (character == '0') {
        character = read_numeric_character();
        if ((character & 0xdf) == 'X') {
            base = 16;
            character = read_numeric_character();
            while (c1_data.scanner.flags[character] & 2) {
                *cursor++ = character;
                character = read_numeric_character();
            }
            if (cursor == c1_numeric_token_text) compiler_warning(153);
            goto scan_suffix;
        }
        base = 8;
        *cursor++ = '0';
    } else base = 10;
    while (alternate_base ? (c1_data.scanner.flags[character] & 2) :
                           c1_data.scanner.classes[character] == SC_DIGIT) {
        *cursor++ = character;
        character = read_numeric_character();
        if (cursor > c1_numeric_token_text + 128) compiler_fatal(64);
    }
    if (!alternate_base) {
        if (character == '.' || (character & 0xdf) == 'E') {
            --c1_bss.source_cursor;
            return scan_floating_token(cursor);
        }
    } else {
        BYTE suffix = (BYTE)cursor[-1];
        if ((c1_data.scanner.flags[suffix] & 0x10) && (character & 0xdf) != 'H')
            --cursor;
        else {
            suffix = character;
            if (!(c1_data.scanner.flags[suffix] & 0x10)) goto scan_suffix;
            character = read_numeric_character();
        }
        switch (suffix & 0xdf) {
        case 'B': base = 2; break;
        case 'D': base = 10; break;
        case 'H': base = 16; break;
        case 'O':
        case 'Q': base = 8; break;
        }
    }
scan_suffix:
    character &= 0xdf;
    if (character == 'L') {
        character = read_numeric_character() & 0xdf;
        if (character == 'U') type = c1_data.builtin_types.type_4044;
        else {
            --c1_bss.source_cursor;
            type = c1_data.builtin_types.type_0044;
        }
    } else if (c1_data.internal_zb_value > 0 && character == 'I') {
        type = select_sized_literal_type(0);
    } else if (character == 'U') {
        character = read_numeric_character() & 0xdf;
        if (character == 'L') type = c1_data.builtin_types.type_4044;
        else if (c1_data.internal_zb_value > 0 && character == 'I')
            type = select_sized_literal_type(1);
        else {
            --c1_bss.source_cursor;
            type = c1_data.builtin_types.type_4063_first;
        }
    } else --c1_bss.source_cursor;
    if (c1_data.scanner.flags[*c1_bss.source_cursor] & 1)
        compiler_warning(59, 77, 306);
    *cursor++ = 0;
    length = (unsigned)(cursor - c1_numeric_token_text);
    value = length <= 1 ? 0 : parse_integer_digits(c1_numeric_token_text, length - 1, base);
    switch (type->flags) {
    case 0x63:
        type = base == 10 ? select_decimal_literal_type(&value) : select_based_literal_type(&value);
        break;
    case 0x41: value = (signed __int64)(value << 56) >> 56; break;
    case 0x42: value = (signed __int64)(value << 48) >> 48; break;
    case 0x43: value = (signed __int64)(value << 32) >> 32; break;
    case 0x44: type = select_signed_wide_literal_type(&value); break;
    case 0x4c:
        /* The original compiler's two shift helpers receive CL=c0. Both
         * clamp counts>=64; this produces zero without a C oversized shift. */
        value = 0;
        break;
    case 0x4041: value &= 0xff; break;
    case 0x4042: value &= 0xffff; break;
    case 0x4043: value &= 0xffffffff; break;
    case 0x4044: type = select_unsigned_wide_literal_type(&value); break;
    case 0x404c: value = 0; break;
    case 0x4063: type = select_unsigned_literal_type(&value); break;
    }
    c1_bss.literal.typed_constant.type = type;
    c1_bss.literal.typed_constant.value = value;
allocate_token:
    if (c1_bss.symbol_arena_kind) {
#ifdef RECON_NATIVE_TEST
        if (c1_bss.symbol_arena_kind >= 6)
            recon_unsupported("numeric-token arena index is not modeled");
#endif
        CompilerArena *arena = &c1_bss.arenas[c1_bss.symbol_arena_kind];
        token = (CompilerNumericToken *)arena->cursor;
        arena->cursor += 32;
        if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    } else {
        CompilerPool *pool = &c1_bss.pools[6];
        if (pool->remaining < 32) pool->Refill(32);
        token = (CompilerNumericToken *)pool->cursor;
        pool->cursor += 32;
        pool->remaining -= 32;
    }
    if (token) {
        token->kind = 101;
#ifdef RECON_NATIVE_TEST
        token->value.typed_constant.type = c1_bss.literal.typed_constant.type;
        token->value.typed_constant.value = c1_bss.literal.typed_constant.value;
#else
        memcpy(&token->value, &c1_bss.literal, 24);
#endif
    }
    return (CompilerParserToken *)token;
}
unsigned char __cdecl read_numeric_character(void)
{
    BYTE character = *c1_bss.source_cursor++;
    if (!character) {
        if (process_special_character() != 0x20)
            return *c1_bss.source_cursor++;
        ++c1_bss.source_cursor;
        return 0;
    }
    --c1_bss.source_cursor;
    character = *c1_bss.source_cursor++;
    if (c1_data.scanner.classes[character]) return character;
    return (BYTE)resolve_special_character(character);
}
unsigned __int64 __cdecl parse_integer_digits(const char *text, int length, int base)
{
    if (length < 1 || length > 10 || text[length] ||
        (base != 8 && base != 10) || c1_data.internal_zb_value != 4) {
#ifdef __linux__
        recon_unsupported("integer spelling is outside the recovered ordinary-width domain");
#else
        __asm { int 3 }
#endif
    }
    for (int checked = 0; checked < length; ++checked) {
        if (text[checked] < '0' || text[checked] >= '0' + base) {
#ifdef __linux__
            recon_unsupported("integer digit spelling is outside the recovered radix domain");
#else
            __asm { int 3 }
#endif
        }
    }
    int configured_bytes = c1_data.internal_zb_value;
    int bits = configured_bytes * 8;
    int short_limit = bits / 4;
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
    unsigned __int64 value = 0;
    for (;;) {
        int digit = numeric_digit_value((BYTE)*text);
        if (digit >= base) compiler_warning(41, (BYTE)*text, base);
        value += digit;
        if (!*++text) return value;
        value *= base;
    }
}
int __cdecl numeric_digit_value(unsigned character)
{
#ifdef RECON_NATIVE_TEST
    if (character > 255) recon_unsupported("numeric digit input is outside the encountered byte domain");
#endif
    if (c1_data.scanner.classes[character & 255] == SC_DIGIT)
        return character - '0';
    return (character & 0xdf) - ('A' - 10);
}
CompilerType *__cdecl select_decimal_literal_type(unsigned __int64 *value)
{
    if (*value <= 0x7fffffff) return c1_data.builtin_types.type_0063;
    if (c1_data.internal_bm_flags || (*value > 0xffffffff && c1_data.internal_zb_value > 4))
        return select_extended_integer_type(value);
    return c1_data.builtin_types.type_4044;
}
CompilerType *__cdecl select_extended_integer_type(unsigned __int64 *) { __asm { int 3 } return NULL; }
CompilerType *__cdecl select_based_literal_type(unsigned __int64 *value)
{
    if (*value <= 0x7fffffff) return c1_data.builtin_types.type_0063;
    if (*value <= 0xffffffff) return c1_data.builtin_types.type_4063_first;
    if (c1_data.internal_bm_flags || c1_data.internal_zb_value > 4)
        return ((signed __int64)*value < 0) ? c1_data.builtin_types.type_404c :
                                            c1_data.builtin_types.type_004c;
    return c1_data.builtin_types.type_4044;
}
CompilerType *__cdecl select_signed_wide_literal_type(unsigned __int64 *) { __asm { int 3 } return NULL; }
CompilerType *__cdecl select_unsigned_literal_type(unsigned __int64 *) { __asm { int 3 } return NULL; }
CompilerType *__cdecl select_unsigned_wide_literal_type(unsigned __int64 *) { __asm { int 3 } return NULL; }
CompilerType *__cdecl select_sized_literal_type(int) { __asm { int 3 } return NULL; }
/* Both numeric and quoted literals use the same spelling allocation. The
 * incoming length is truncated to a WORD before allocating/copying; flags
 * and the rest of the caller-owned literal value are not written. */
void __cdecl copy_numeric_spelling(CompilerValue *literal, const char *text,
                                   unsigned length, unsigned arena_kind)
{
    CompilerArena *arena = &c1_bss.arenas[arena_kind];
    unsigned bytes = (WORD)length;
    BYTE *copy = arena->cursor;
    arena->cursor += (bytes + 3) & ~3U;
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    memcpy(copy, text, bytes);
    literal->string_attributes.text = (const char *)copy;
    literal->string_attributes.length = (WORD)length;
}
}
