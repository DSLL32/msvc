#include "compiler-state.h"
#include "scanner.h"
#include <recon.h>

extern "C" {
unsigned __cdecl parse_directive_character(unsigned);
unsigned __cdecl process_special_character(void);
unsigned __cdecl resolve_special_character(unsigned);
unsigned __cdecl read_unspliced_character(void);
unsigned char __cdecl read_multibyte_character(void);
void __cdecl skip_line_comment(void);
void __cdecl skip_macro_block_comment(void);
void __cdecl skip_preserved_macro_comment(void);
void __cdecl compiler_warning(int, ...);
void __cdecl report_language_diagnostic(unsigned, unsigned, ...);
BYTE *__cdecl grow_macro_text(BYTE *, BYTE *, BYTE **);
BYTE *__cdecl copy_macro_quoted_text(BYTE *, BYTE **, unsigned);
int __cdecl find_macro_parameter(const char *);
}

/* Replacement lists retain ordinary text and encode parameter references as
 * a two-byte operation/index. Token pasting removes the surrounding space;
 * stringizing and Microsoft's characterizing operator select other opcodes. */
extern "C" unsigned __cdecl read_macro_replacement(void)
{
    BYTE *out = (BYTE *)c1_bss.current_macro_text;
    BYTE *identifier;
    unsigned character, next, category, special;
    int stringize = 0, characterize = 0, parameter, comment;
    character = *c1_bss.source_cursor++;
    if (!(c1_data.scanner.flags[character] & 0x20)) {
        while (c1_data.scanner.classes[character] == SC_SPACE)
            character = *c1_bss.source_cursor++;
        if (!(c1_data.scanner.flags[character] & 0x20))
            character = (BYTE)parse_directive_character(character);
    }
dispatch:
    if (out >= c1_bss.work_buffers[1].limit)
        out = grow_macro_text(out, c1_bss.work_buffers[1].data, &c1_bss.work_buffers[1].limit);
    category = c1_data.scanner.classes[character];
    switch (category) {
    case SC_NEWLINE:
        --c1_bss.source_cursor;
        if (out == (c1_data.options.language_extensions ? (BYTE *)c1_bss.current_macro_text :
                    c1_bss.work_buffers[1].data)) return 0;
        if (out >= c1_bss.work_buffers[1].limit)
            out = grow_macro_text(out, c1_bss.work_buffers[1].data, &c1_bss.work_buffers[1].limit);
        *out++ = 0;
        *out++ = 0x44; /* End of an encoded replacement list. */
        return (unsigned)(out - (BYTE *)c1_bss.current_macro_text);
    case SC_SPECIAL:
        special = process_special_character();
        if (special & 0x50) {
            if (special & 0x10) goto escaped_character;
            goto append_character;
        }
        goto next_character;
    case SC_SPACE:
    skip_spaces:
        character = *c1_bss.source_cursor++;
        if (!(c1_data.scanner.flags[character] & 0x20)) {
            while (c1_data.scanner.classes[character] == SC_SPACE)
                character = *c1_bss.source_cursor++;
            if (!(c1_data.scanner.flags[character] & 0x20))
                character = (BYTE)parse_directive_character(character);
        }
        if (character != '\n') *out++ = ' ';
        goto dispatch;
    case SC_SQUOTE:
    case SC_DQUOTE:
        out = copy_macro_quoted_text(out, &c1_bss.work_buffers[1].limit, character);
        goto next_character;
    case SC_HASH:
        for (;;) {
            next = *c1_bss.source_cursor++;
            if (c1_data.scanner.classes[next] != SC_SPECIAL) break;
            if (!(process_special_character() & ~0x50U)) break;
        }
        if (c1_data.scanner.classes[next] == SC_HASH) {
            if (out == (BYTE *)c1_bss.current_macro_text) {
                compiler_warning(160);
                goto dispatch;
            }
            if (out[-1] == ' ') --out;
            character = *c1_bss.source_cursor++;
            if (!(c1_data.scanner.flags[character] & 0x20)) {
                while (c1_data.scanner.classes[character] == SC_SPACE)
                    character = *c1_bss.source_cursor++;
                if (!(c1_data.scanner.flags[character] & 0x20))
                    character = (BYTE)parse_directive_character(character);
            }
            if (character == '\n') {
                --c1_bss.source_cursor;
                compiler_warning(161);
            }
            goto dispatch;
        }
        if (c1_bss.macro_parameter_count > 0) {
            if (c1_data.scanner.classes[next] == SC_AT && c1_data.options.language_extensions)
                characterize = 1;
            else { --c1_bss.source_cursor; stringize = 1; }
            character = *c1_bss.source_cursor++;
            if (!(c1_data.scanner.flags[character] & 0x20)) {
                while (c1_data.scanner.classes[character] == SC_SPACE)
                    character = *c1_bss.source_cursor++;
                if (!(c1_data.scanner.flags[character] & 0x20))
                    character = (BYTE)parse_directive_character(character);
            }
            if (c1_data.scanner.flags[character] & 1) goto identifier_start;
            stringize = characterize = 0;
            compiler_warning(162);
            goto dispatch;
        }
        *out++ = '#';
        character = c1_bss.source_cursor[-1];
        goto dispatch;
    case SC_IDENTIFIER:
    case SC_LETTER_L:
    identifier_start:
        identifier = out;
    identifier_character:
        if (out >= c1_bss.work_buffers[1].limit)
            out = grow_macro_text(out, c1_bss.work_buffers[1].data, &c1_bss.work_buffers[1].limit);
        *out++ = (BYTE)character;
        character = *c1_bss.source_cursor++;
        if (c1_data.scanner.flags[character] & 3) goto identifier_character;
    identifier_end:
        if (c1_data.scanner.classes[character] == SC_SPECIAL) {
            if (process_special_character() & ~0x50U) {
                character = *c1_bss.source_cursor++;
                if (c1_data.scanner.flags[character] & 3) goto identifier_character;
                goto identifier_end;
            }
        }
        if (c1_bss.macro_parameter_count > 0) {
            *out = 0;
            parameter = find_macro_parameter((const char *)identifier);
            if (parameter >= 1) {
                out = identifier;
                *out++ = stringize ? 2 : characterize ? 3 : 1;
                *out++ = (BYTE)parameter;
            } else if (stringize || characterize) compiler_warning(162);
            stringize = characterize = 0;
        }
        goto dispatch;
    case SC_SLASH:
        comment = 0;
        if (c1_data.options.language_extensions || c1_data.options.preprocess_only) {
            next = *c1_bss.source_cursor++;
            if (c1_data.scanner.classes[next] == SC_SPECIAL)
                next = (BYTE)resolve_special_character(next);
            if (next == '/') {
                c1_data.lexer_flags |= 0x800;
                skip_line_comment();
                goto line_comment_finished;
            }
            --c1_bss.source_cursor;
            next = *c1_bss.source_cursor++;
            if (c1_data.scanner.classes[next] == SC_SPECIAL)
                next = (BYTE)resolve_special_character(next);
        } else {
            next = (BYTE)read_unspliced_character();
            if (next == '/') {
                next = (BYTE)read_unspliced_character();
                if (next == '*') {
                    report_language_diagnostic(4, 179);
                    goto rejected_nested_comment;
                }
                --c1_bss.source_cursor;
                report_language_diagnostic(4, 1);
                c1_data.lexer_flags |= 0x800;
                skip_line_comment();
                goto line_comment_finished;
            }
            --c1_bss.source_cursor;
            next = (BYTE)read_unspliced_character();
        }
        if (next == '*') {
            c1_data.lexer_flags |= 0x800;
            if (c1_bss.options.preserve_comments && !(c1_data.lexer_flags & 0x8000))
                skip_preserved_macro_comment();
            else skip_macro_block_comment();
            c1_data.lexer_flags &= ~0x800U;
            comment = 1;
        } else --c1_bss.source_cursor;
        if (comment) goto skip_spaces;
        goto append_character;
    rejected_nested_comment:
        c1_data.lexer_flags |= 0x800;
        if (c1_bss.options.preserve_comments && !(c1_data.lexer_flags & 0x8000))
            skip_preserved_macro_comment();
        else skip_macro_block_comment();
        c1_data.lexer_flags &= ~0x800U;
        goto append_character;
    line_comment_finished:
        c1_data.lexer_flags &= ~0x800U;
        goto skip_spaces;
    case SC_LEAD:
        *out++ = (BYTE)character;
        character = read_multibyte_character();
        goto append_character;
    case SC_INVALID:
        compiler_warning(18, character);
        goto next_character;
    }
append_character:
    *out++ = (BYTE)character;
next_character:
    character = *c1_bss.source_cursor++;
    goto dispatch;
escaped_character:
    *out++ = (BYTE)character;
    character = *c1_bss.source_cursor++;
    if (c1_data.scanner.classes[character] == SC_SPECIAL)
        character = (BYTE)resolve_special_character(character);
    if (c1_data.scanner.classes[character] >= SC_SQUOTE &&
        c1_data.scanner.classes[character] <= SC_DQUOTE) {
        --out;
        compiler_warning(295, character);
        goto next_character;
    }
    goto append_character;
}
extern "C" char c1_numeric_token_text[128];
extern "C" BYTE *__cdecl grow_macro_text(BYTE *cursor, BYTE *base, BYTE **limit)
{
    unsigned span;
    do {
        span = (unsigned)(*limit - base);
        if (span >= 0x500000) {
            if (!c1_bss.lexical_skip_mode)
                compiler_error(11, c1_numeric_token_text, 0x500000);
            else compiler_error(56, 0x500000, c1_bss.macro_expansions[1].macro->identifier);
        } else {
            if (!VirtualAlloc(base + span + 5, span, MEM_COMMIT, PAGE_READWRITE))
                compiler_fatal(60);
            *limit = base + span * 2 + 5;
        }
    } while (cursor >= *limit - 5);
    return cursor;
}
/* RVAe32c. Preserve quoted replacement text, including encoded escapes;
 * a newline warns and supplies the closing delimiter without consuming it. */
extern "C" BYTE *__cdecl copy_macro_quoted_text(BYTE *out, BYTE **limit, unsigned quote)
{
    BYTE character;
    unsigned special;
    *out++ = (BYTE)quote;
    for (;;) {
        if (out >= *limit)
            grow_macro_text(out, c1_bss.work_buffers[1].data, limit);
        character = *c1_bss.source_cursor++;
        switch (c1_data.scanner.classes[character]) {
        case SC_NEWLINE:
            compiler_warning(1);
            --c1_bss.source_cursor;
            character = (BYTE)quote;
            /* FALL THROUGH */
        case SC_SQUOTE: case SC_DQUOTE:
            if (character == (BYTE)quote) {
                *out++ = character;
                return out;
            }
            break;
        case SC_SPECIAL:
            special = process_special_character();
            if (!(special & 0x50)) continue;
            if (special & 0x10) {
                *out++ = c1_bss.source_cursor[-1];
                character = *c1_bss.source_cursor++;
                if (c1_data.scanner.classes[character] == SC_SPECIAL)
                    character = (BYTE)resolve_special_character(character);
            }
            break;
        case SC_LEAD:
            *out++ = character;
            character = read_multibyte_character();
            break;
        }
        *out++ = character;
    }
}
extern "C" int __cdecl find_macro_parameter(const char *name)
{
    const BYTE *record = c1_bss.work_buffers[1].data;
    const BYTE *parameter, *text;
    int index;
    for (index = 1; index <= c1_bss.macro_parameter_count; ++index) {
        parameter = record + 1;
        text = (const BYTE *)name;
        while (*text == *parameter && *text) { ++text; ++parameter; }
        if (*text == *parameter) return index;
        record += *record;
    }
    return -1;
}

extern "C" int __cdecl read_source_chunk(void);
extern "C" int __cdecl splice_input_line(void);
extern "C" void __cdecl report_macro_input_end(int);

/* Parameter names are byte-length records in the same work area. Each
 * record includes its length byte and terminating NUL; the replacement
 * text starts immediately after the final completed record. */
extern "C" void __cdecl parse_macro_parameters(void)
{
    BYTE *out = (BYTE *)c1_bss.current_macro_text + 1;
    BYTE *name = out;
    unsigned character, category;
    int previous;
    for (;;) {
        character = *c1_bss.source_cursor++;
        if (!(c1_data.scanner.flags[character] & 0x20)) {
            while (c1_data.scanner.classes[character] == SC_SPACE)
                character = *c1_bss.source_cursor++;
            if (!(c1_data.scanner.flags[character] & 0x20))
                character = (BYTE)parse_directive_character(character);
        }
        category = c1_data.scanner.classes[character];
        if (category == SC_IDENTIFIER || category == SC_LETTER_L) {
            if (out != name) compiler_warning(10, character);
            *out++ = (BYTE)character;
        identifier_character:
            character = *c1_bss.source_cursor++;
            if (c1_data.scanner.flags[character] & 3) {
                *out++ = (BYTE)character;
                goto identifier_character;
            }
            if (!character) {
                if (read_source_chunk()) report_macro_input_end(4);
                goto identifier_character;
            }
            if (character == '\\' && splice_input_line()) goto identifier_character;
            --c1_bss.source_cursor;
            *out++ = 0;
            continue;
        }
        if (category == SC_COMMA || category == SC_RPAREN) {
            if (out > name) {
                previous = find_macro_parameter((const char *)name);
                if (previous >= 1) compiler_warning(9, name);
                else {
                    *(BYTE *)c1_bss.current_macro_text = (BYTE)(out - (BYTE *)c1_bss.current_macro_text);
                    c1_bss.current_macro_text = (const char *)out;
                    ++c1_bss.macro_parameter_count;
                }
            } else if (character == ',' || c1_bss.macro_parameter_count > 0)
                compiler_warning(10, character);
            if (character == ')') return;
            out = (BYTE *)c1_bss.current_macro_text + 1;
            name = out;
            continue;
        }
        compiler_warning(10, character);
    }
}
extern "C" void __cdecl report_macro_input_end(int) { __asm { int 3 } }
