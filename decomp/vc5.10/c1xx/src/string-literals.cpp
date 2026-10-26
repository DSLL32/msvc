#include <string.h>
#include <stdlib.h>
#include "parser-state.h"
#include "scanner.h"
#include <recon.h>
#pragma intrinsic(memcpy)

extern "C" {
BYTE *__cdecl ensure_quoted_cursor(BYTE *);
BYTE *__cdecl convert_quoted_chunk(BYTE *, int, unsigned);
void __cdecl normalize_quoted_literal(CompilerQuotedValue *);
WORD __cdecl read_escaped_character(unsigned, int);
unsigned __cdecl process_special_character(void);
unsigned __cdecl resolve_special_character(unsigned);
BYTE __cdecl advance_multibyte_source(void);
void __cdecl copy_numeric_spelling(ParserSemanticView *, const char *, unsigned, unsigned);
}

/* The quoted arm shares the actual BA8C8 saved-value owner. The WORD fields
 * at+4/+6 are distinct from the numeric literal arm. CD564 points into the
 * existing2048-byte token-text workspace; no second spelling buffer is owned. */
extern "C" void __cdecl scan_quoted_literal(int wide, unsigned quote)
{
    CompilerQuotedValue *literal = &frontend_bss.saved_token_value.known.quoted;
    BYTE *out = (BYTE *)frontend_data.token_text_cursor, *chunk;
    WORD character;
    unsigned category, special, next, arena_kind;
    int escaped_characters = 0;
    literal->text = out;
    literal->flags = (WORD)((literal->flags ^ ((wide ^ literal->flags) & 1)) & ~2);
    if (frontend_bss.quoted_skip_mode) wide = 0;
    for (;;) {
        chunk = out;
        character = *frontend_bss.source_cursor++;
        category = frontend_data.scanner.classes[character & 0xff];
        switch (category) {
        case SC_SPECIAL:
            special = process_special_character();
            if (special & ~0x50U) continue;
            if (!(special & 0x10) || (frontend_data.compiler_flags & 0x2000) ||
                frontend_bss.quoted_skip_mode) break;
            next = *frontend_bss.source_cursor++;
            if (!frontend_data.scanner.classes[next])
                next = resolve_special_character(next) & 0xff;
            character = read_escaped_character(next, wide);
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
            --frontend_bss.source_cursor;
            if (!frontend_bss.quoted_skip_mode) compiler_warning(1);
            /* The preceding byte is outside the known owner when an empty
             * unterminated literal begins at the workspace base. */
            if (out == (BYTE *)compiler_token_text) { __asm { int 3 } }
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
                literal->length = (WORD)(out - literal->text);
                if (literal->flags & 2) normalize_quoted_literal(literal);
                arena_kind = frontend_bss.symbol_arena_kind;
                if (arena_kind == 5 || (frontend_data.compiler_mode_flags & 0x02000000))
                    arena_kind = 0;
                copy_numeric_spelling(&saved_parser_value, (const char *)literal->text,
                    literal->length, arena_kind);
                frontend_data.token_text_cursor = compiler_token_text;
                return;
            }
            break;
        case SC_LEAD:
            out = ensure_quoted_cursor(out);
            *out++ = (BYTE)character;
            character = advance_multibyte_source();
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
            memmove(frontend_data.token_text_cursor + 1, frontend_data.token_text_cursor,
                out - (BYTE *)frontend_data.token_text_cursor);
            ++out;
            *frontend_data.token_text_cursor = (BYTE)character;
        }
    }
}

extern "C" BYTE *__cdecl ensure_quoted_cursor(BYTE *cursor)
{
    if ((unsigned)cursor >= (unsigned)(compiler_token_text + 2048)) {
        compiler_warning(26);
        return (BYTE *)compiler_token_text;
    }
    return cursor;
}
extern "C" void __cdecl copy_numeric_spelling(ParserSemanticView *value,
    const char *text, unsigned count, unsigned arena_index)
{
    unsigned length = (WORD)count;
    if (arena_index >= 9) { __asm { int 3 } }
    CompilerArena *arena = &frontend_bss.arenas[arena_index];
    BYTE *memory = arena->cursor;
    arena->cursor += (length + 3) & ~3U;
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    memcpy(memory, text, length);
    value->quoted.length = (WORD)count;
    value->quoted.text = memory;
}
extern "C" BYTE *__cdecl convert_quoted_chunk(BYTE *chunk, int wide, unsigned count)
{
    if (!wide) return chunk + count;
    BYTE *temporary = ensure_quoted_cursor((BYTE *)(((unsigned)chunk + count + 4) & ~3U));
    if (mbtowc((WCHAR *)temporary, (const char *)chunk, count) < 0)
        compiler_warning(2);
    chunk[0] = temporary[0];
    chunk[1] = temporary[1];
    return chunk + 2;
}
extern "C" void __cdecl normalize_quoted_literal(CompilerQuotedValue *) { __asm { int 3 } }
extern "C" WORD __cdecl read_escaped_character(unsigned, int) { __asm { int 3 } return 0; }
