#include <string.h>
#include <stdlib.h>
#include "frontend.h"
#include <recon.h>
#pragma intrinsic(memcpy)

extern "C" {
BYTE *__cdecl ensure_quoted_cursor(BYTE *);
BYTE *__cdecl convert_quoted_chunk(BYTE *, int, unsigned);
void __cdecl copy_numeric_spelling(CompilerValue *, const char *, unsigned, unsigned);
void __cdecl normalize_quoted_literal(CompilerStringValue *);
unsigned __cdecl process_special_character(void);
unsigned __cdecl resolve_special_character(unsigned);
unsigned __cdecl read_escaped_character(unsigned, int);
unsigned char __cdecl read_multibyte_character(void);
void __cdecl compiler_warning(int, ...);
void __cdecl report_language_diagnostic(unsigned, unsigned, ...);
}

/* RVA1fa2b. The incoming length is consumed as a WORD; ordinary multi-byte
 * character constants collect up to four bytes in source order. */
extern "C" CompilerParserToken *__cdecl make_character_constant(unsigned wide,
    const void *text, unsigned count)
{
    WORD length = (WORD)count;
    const BYTE *bytes = (const BYTE *)text;
    unsigned __int64 value = 0;
    CompilerParserToken *token;
    if (wide) {
        value = *(const WORD *)text;
        if (length > 4) report_language_diagnostic(3, 66);
    } else if (!length) compiler_warning(137);
    else if (length > 4) compiler_warning(15);
    else {
        for (unsigned index = 0; index < length; ++index)
            value = (value << 8) | bytes[index];
        if (!c1_data.options.unsigned_char && length == 1)
            value = (__int64)(signed char)(BYTE)value;
    }
    c1_bss.literal.typed_constant.type = wide ? c1_data.builtin_types.type_4042_first :
        c1_data.builtin_types.type_0063;
    c1_bss.literal.typed_constant.value = value;
    if (c1_bss.symbol_arena_kind) {
        if (c1_bss.symbol_arena_kind >= 6) {
#ifdef __linux__
            recon_unsupported("character-token arena index is not modeled");
#else
            __asm { int 3 }
#endif
        }
        CompilerArena *arena = &c1_bss.arenas[c1_bss.symbol_arena_kind];
        token = (CompilerParserToken *)arena->cursor;
        arena->cursor += 32;
        if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    } else {
        CompilerPool *pool = &c1_bss.pools[6];
        if (pool->remaining < 32) pool->Refill(32);
        token = (CompilerParserToken *)pool->cursor;
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
    return token;
}

/* Literal spelling uses the real shared2048-byte BSS workspace. Each ordinary
 * chunk is converted at its original call site; no host string conversion is
 * substituted for either pending converter. Flags retain their other bits. */
extern "C" void __cdecl scan_quoted_literal(int wide, unsigned quote)
{
    CompilerStringValue *literal = &c1_bss.literal.string_attributes;
    BYTE *out = c1_data.quoted_cursor, *chunk;
    WORD character;
    unsigned category, special, next;
    int escaped_characters = 0;
    literal->text = (const char *)out;
    literal->flags = (WORD)((literal->flags ^ ((literal->flags ^ wide) & 1)) & ~2);
    if (c1_bss.lexical_skip_mode) wide = 0;
    for (;;) {
        chunk = out;
        character = *c1_bss.source_cursor++;
        category = c1_data.scanner.classes[character & 0xff];
        switch (category) {
        case SC_SPECIAL:
            special = process_special_character();
            if (special & ~0x50U) continue;
            if (!(special & 0x10) || (c1_data.lexer_flags & 0x2000) ||
                c1_bss.lexical_skip_mode) break;
            next = *c1_bss.source_cursor++;
            if (c1_data.scanner.classes[next] == SC_SPECIAL)
                next = resolve_special_character(next) & 0xff;
            character = (WORD)read_escaped_character(next, wide);
            if (wide) {
                if (character != (BYTE)character) {
                    out = ensure_quoted_cursor(out);
                    *out++ = (BYTE)character;
                    *out++ = (BYTE)(character >> 8);
                    continue;
                }
            } else if ((BYTE)quote == '\'') ++escaped_characters;
            break;
        case SC_NEWLINE:
            --c1_bss.source_cursor;
            if (!c1_bss.lexical_skip_mode) compiler_warning(1);
            if (out == (BYTE *)literal->text) {
#ifdef __linux__
                recon_unsupported("empty unterminated literal preceding byte is not modeled");
#else
                __asm { int 3 }
#endif
            }
            if (out[-1] == '\r') --out;
            character = (WORD)(signed char)quote;
            goto delimiter;
        case SC_SQUOTE:
        case SC_DQUOTE:
        delimiter:
            if ((int)character == (signed char)quote) {
                out = ensure_quoted_cursor(out);
                *out = 0;
                out = convert_quoted_chunk(chunk, wide, (unsigned)(out - chunk) + 1);
                literal->length = (WORD)(out - (const BYTE *)literal->text);
                if (literal->flags & 2) normalize_quoted_literal(literal);
                copy_numeric_spelling(&c1_bss.literal, literal->text, literal->length,
                    c1_bss.symbol_arena_kind == 3 ? 0 : c1_bss.symbol_arena_kind);
                c1_data.quoted_cursor = c1_bss.quoted_workspace;
                return;
            }
            break;
        case SC_LEAD:
            out = ensure_quoted_cursor(out);
            *out++ = (BYTE)character;
            character = read_multibyte_character();
            break;
        }
        out = ensure_quoted_cursor(out);
        if (!escaped_characters) {
            *out++ = (BYTE)character;
            *out = 0;
            out = convert_quoted_chunk(chunk, wide, (unsigned)(out - chunk));
        } else if (escaped_characters == 1) {
            *out++ = (BYTE)character;
        } else {
            memmove(c1_data.quoted_cursor + 1, c1_data.quoted_cursor,
                    out - c1_data.quoted_cursor);
            ++out;
            *c1_data.quoted_cursor = (BYTE)character;
        }
    }
}

extern "C" BYTE *__cdecl ensure_quoted_cursor(BYTE *cursor)
{
    if ((unsigned)cursor >= (unsigned)(c1_bss.quoted_workspace + 2048)) {
        compiler_warning(26);
        return c1_bss.quoted_workspace;
    }
    return cursor;
}
extern "C" BYTE *__cdecl convert_quoted_chunk(BYTE *chunk, int wide, unsigned count)
{
    BYTE *temporary;
    if (!wide) return chunk + count;
    temporary = ensure_quoted_cursor((BYTE *)(((unsigned)chunk + count + 4) & ~3U));
    if (mbtowc((WCHAR *)temporary, (const char *)chunk, count) < 0)
        compiler_warning(2);
    chunk[0] = temporary[0];
    chunk[1] = temporary[1];
    return chunk + 2;
}
extern "C" void __cdecl normalize_quoted_literal(CompilerStringValue *) { __asm { int 3 } }
extern "C" unsigned __cdecl read_escaped_character(unsigned, int) { __asm { int 3 } return 0; }
