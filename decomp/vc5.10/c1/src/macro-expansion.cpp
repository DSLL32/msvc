/* Macro replacement and rescanning. Encoded parameter markers belong to the
 * replacement language; ordinary punctuation remains literal source text. */
#include <string.h>
#include "frontend.h"
#include "scanner.h"
#include "keywords.h"
#pragma intrinsic(memcpy, strlen, strcmp)
#include <recon.h>

extern "C" {
extern char c1_numeric_token_text[128];
int __cdecl format_buffer(char *, const char *, unsigned, ...);
int __cdecl macro_is_active(CompilerMacro *);
void __cdecl push_macro_expansion(CompilerMacro *);
void __cdecl begin_macro_replacement(void);
int __cdecl finish_macro_replacement(void);
int __cdecl skip_macro_whitespace(void);
const char *__cdecl resolve_macro_argument(int *);
int __cdecl scan_macro_identifier(BYTE);
void __cdecl parse_macro_arguments(CompilerMacro *, int);
void __cdecl append_macro_text(const char *);
BYTE *__cdecl quote_macro_filename(BYTE *, const char *);
const char *__cdecl current_macro_line(void);
const char *__cdecl macro_argument_text(void);
void __cdecl append_macro_quoted(unsigned, const char *);
long __cdecl current_source_timestamp(void);
char *__cdecl format_predefined_time(long);
BYTE *__cdecl grow_macro_text(BYTE *, BYTE *, BYTE **);
BYTE *__cdecl copy_macro_quoted_text(BYTE *, BYTE **, unsigned);
unsigned __cdecl process_special_character(void);
unsigned __cdecl process_trigraph(void);
int __cdecl splice_input_line(void);
void __cdecl refill_lexer_source(void);
void __cdecl copy_macro_argument_range(BYTE *, BYTE *);
void __cdecl finish_macro_argument(void);
unsigned __cdecl read_unspliced_character(void);
unsigned __cdecl resolve_special_character(unsigned);
void __cdecl skip_line_comment(void);
int __cdecl skip_block_comment(void);
void __cdecl skip_preserved_macro_comment(void);
void __cdecl skip_macro_block_comment(void);
int __cdecl scan_identifier(unsigned);
int __cdecl expand_defined_operator(CompilerNameEntry *);
void __cdecl report_language_diagnostic(unsigned, unsigned, ...);
void __cdecl compiler_warning(int, ...);
}
#ifdef __linux__
#define MACRO_UNSUPPORTED(s) recon_unsupported(s)
#else
#define MACRO_UNSUPPORTED(s) __asm { int 3 }
#endif
enum { C1_MACRO_RESCAN_LAST = 4 }; /* Only records0..4 have encountered lifetimes. */
#define MACRO_ASSERT(test,line) do { if (!(test)) compiler_error(73,".\\p0macros.c",line); } while (0)

/* Original timestamp scratch at546a0. Its bytes are written by the timestamp
 * branch; there is no initialized-text or private CRT buffer identity claim. */
static char macro_timestamp[25];

extern "C" int __cdecl expand_macro(CompilerMacro *macro)
{
    int expanded = 0, kind, comment;
    unsigned character, category;
    BYTE *out;
    const char *stamp;
    CompilerMacroExpansionFrame *frame;
    if (macro_is_active(macro)) return 0;
    c1_bss.macro_active_count = 0;
    c1_bss.saved_macro_text_cursor = c1_bss.macro_text_cursor;
    c1_bss.macro_line = c1_bss.current_context.value;
expand_identifier:
    c1_bss.macro_argument_count = 0;
    c1_bss.saved_macro_token_cursor = c1_bss.macro_token_cursor;
    kind = macro->body->kind;
    if (kind < -3) {
        if (c1_bss.browser_filename) c1_data.backend->Slot24(macro);
        push_macro_expansion(macro);
        if ((unsigned)c1_bss.lexical_skip_mode > C1_MACRO_RESCAN_LAST)
            MACRO_UNSUPPORTED("macro builtin rescan index outside encountered records");
        frame = &c1_bss.macro_expansions[c1_bss.lexical_skip_mode];
        frame->active = 1;
        switch (kind) {
        case -4: append_macro_text(current_macro_line()); break;
        case -6:
            MACRO_ASSERT(c1_bss.debug_module->Slot50(),0x3a1);
            append_macro_quoted('"', c1_bss.macro_date); break;
        case -5:
            c1_bss.macro_text_cursor = quote_macro_filename(c1_bss.macro_text_cursor,
                c1_data.options.reproducible ? "Brepro.c" : c1_bss.current_context.FileName());
            break;
        case -7:
            MACRO_ASSERT(c1_bss.debug_module->Slot50(),0x3a6);
            append_macro_quoted('"', c1_bss.macro_time); break;
        case -8: append_macro_text("1"); break;
        case -9:
            MACRO_ASSERT(c1_bss.debug_module->Slot50(),0x3b8);
            if (c1_data.options.reproducible) {
                macro_timestamp[0] = '1'; macro_timestamp[1] = 0;
            } else {
                stamp = format_predefined_time(current_source_timestamp());
                memcpy(macro_timestamp, stamp, 25);
                memcpy(macro_timestamp + 11, stamp + 16, 4);
                memcpy(macro_timestamp + 15, stamp + 20, 4);
                memcpy(macro_timestamp + 19, stamp + 10, 5);
                macro_timestamp[24] = 0;
            }
            append_macro_quoted('"', macro_timestamp); break;
        }
        if (finish_macro_replacement()) return 1;
        goto rescan;
    }
    if (!kind) {
        expanded = 1;
        if (c1_bss.browser_filename) c1_data.backend->Slot24(macro);
        if (macro->body->payload[0]) {
            push_macro_expansion(macro);
            begin_macro_replacement();
            goto rescan;
        }
        c1_bss.macro_text_cursor = c1_bss.saved_macro_text_cursor;
        out = c1_bss.macro_text_cursor;
        goto scan_next;
    }
    if (kind == -1) kind = 0;
find_arguments:
    if (!skip_macro_whitespace()) goto rescan;
    if (*c1_bss.source_cursor == '/') {
        ++c1_bss.source_cursor;
        if (!c1_data.options.language_extensions && !c1_data.options.preprocess_only) {
            character = (BYTE)read_unspliced_character();
            if (character == '/') {
                character = (BYTE)read_unspliced_character();
                if (character == '*') {
                    report_language_diagnostic(4,179);
                    c1_data.lexer_flags |= 0x800;
                    if (c1_bss.options.preserve_comments && !(c1_data.lexer_flags & 0x8000))
                        skip_preserved_macro_comment();
                    else skip_macro_block_comment();
                    c1_data.lexer_flags &= ~0x800U;
                    comment = 0;
                } else {
                    --c1_bss.source_cursor;
                    report_language_diagnostic(4,1);
                    c1_data.lexer_flags |= 0x800;
                    skip_line_comment();
                    c1_data.lexer_flags &= ~0x800U;
                    comment = 1;
                }
            } else {
                --c1_bss.source_cursor;
                character = (BYTE)read_unspliced_character();
                if (character == '*') {
                    c1_data.lexer_flags |= 0x800;
                    skip_block_comment();
                    c1_data.lexer_flags &= ~0x800U;
                    comment = 1;
                } else { --c1_bss.source_cursor; comment = 0; }
            }
        } else {
            character = *c1_bss.source_cursor++;
            if (c1_data.scanner.classes[character] == SC_SPECIAL)
                character = (BYTE)resolve_special_character(character);
            if (character == '/') {
                c1_data.lexer_flags |= 0x800;
                skip_line_comment();
                c1_data.lexer_flags &= ~0x800U;
                comment = 1;
            } else {
                --c1_bss.source_cursor;
                character = *c1_bss.source_cursor++;
                if (c1_data.scanner.classes[character] == SC_SPECIAL)
                    character = (BYTE)resolve_special_character(character);
                if (character == '*') {
                    c1_data.lexer_flags |= 0x800;
                    if (c1_bss.options.preserve_comments && !(c1_data.lexer_flags & 0x8000))
                        skip_preserved_macro_comment();
                    else skip_macro_block_comment();
                    c1_data.lexer_flags &= ~0x800U;
                    comment = 1;
                } else { --c1_bss.source_cursor; comment = 0; }
            }
        }
        if (comment) goto find_arguments;
        --c1_bss.source_cursor;
    }
    if (*c1_bss.source_cursor == '(') {
        ++c1_bss.source_cursor;
        c1_data.lexer_flags |= 0x1000;
        expanded = 1;
        parse_macro_arguments(macro,kind);
        c1_data.lexer_flags &= ~0x1000U;
        if (c1_bss.browser_filename) c1_data.backend->Slot24(macro);
        goto rescan;
    }
    push_macro_expansion(macro);
    begin_macro_replacement();
    c1_bss.saved_macro_text_cursor = c1_bss.macro_text_cursor;
    append_macro_text(macro->identifier->name);
    if (c1_data.lexer_flags & 0x400000) {
        c1_data.lexer_flags &= ~0x400000U;
        append_macro_text("\n");
        --c1_bss.current_context.value;
    }
    if (c1_bss.work_buffers[1].data) append_macro_text(" ");
    c1_bss.source_cursor = c1_bss.saved_macro_text_cursor;
    if (finish_macro_replacement()) return 1;
rescan:
    out = c1_bss.macro_text_cursor;
scan_next:
    if (!c1_bss.macro_active_count) return expanded;
    if (out >= c1_bss.work_buffers[1].limit) {
        out = grow_macro_text(out,c1_bss.work_buffers[1].data,&c1_bss.work_buffers[1].limit);
        c1_bss.macro_text_cursor = out;
    }
    character = *c1_bss.source_cursor++;
    category = c1_data.scanner.classes[character];
    switch (category) {
    case SC_SPECIAL:
        if (!(process_special_character() & 0x50)) goto rescan;
        goto append_character;
    case SC_SPACE:
        while (c1_data.scanner.classes[*c1_bss.source_cursor++] == SC_SPACE) {}
        --c1_bss.source_cursor;
        character = ' ';
        goto append_character;
    case SC_IDENTIFIER:
    case SC_LETTER_L:
    case 38: /* Encoded identifier rescan. */
        c1_bss.saved_macro_text_cursor = out;
        if (scan_macro_identifier((BYTE)character)) {
            macro = c1_data.current_identifier->macro;
            if (macro) {
                if (!macro_is_active(macro)) goto expand_identifier;
                character = 4;
                *c1_bss.macro_text_cursor++ = (BYTE)character;
                *c1_bss.macro_text_cursor++ = (BYTE)strlen(c1_data.current_identifier->name);
            }
        }
        out = c1_bss.macro_text_cursor;
        if ((c1_data.lexer_flags & 0x4000) &&
            c1_data.current_identifier == c1_data.builtin_names.defined_operator) {
            expand_defined_operator(c1_data.current_identifier);
            goto rescan;
        }
        goto scan_next;
    case SC_DOT:
        if (c1_data.scanner.classes[*c1_bss.source_cursor] != SC_DIGIT)
            goto append_character;
        goto numeric_character;
    case SC_DIGIT:
        for (;;) {
numeric_character:
            *c1_bss.macro_text_cursor++ = (BYTE)character;
            out = c1_bss.macro_text_cursor;
            if (out >= c1_bss.work_buffers[1].limit) {
                out = grow_macro_text(out,c1_bss.work_buffers[1].data,&c1_bss.work_buffers[1].limit);
                c1_bss.macro_text_cursor = out;
            }
            character = *c1_bss.source_cursor++;
            if (c1_data.scanner.flags[character] & 4) continue;
            if (character != '+' && character != '-') break;
            /* The original tests the unused output-cursor byte here. No
             * defining writer is established for that exponent domain. */
            MACRO_UNSUPPORTED("macro exponent continuation requires an unwritten output-byte contract");
            if (*c1_bss.macro_text_cursor != 'e' && *c1_bss.macro_text_cursor != 'E') break;
        }
        --c1_bss.source_cursor;
        goto scan_next;
    case SC_SQUOTE:
    case SC_DQUOTE:
        c1_bss.macro_text_cursor = copy_macro_quoted_text(out,&c1_bss.work_buffers[1].limit,character);
        goto scan_next;
    case 39:
        append_macro_quoted('"',macro_argument_text());
        goto rescan;
    case 40:
        append_macro_quoted('\'',macro_argument_text());
        goto rescan;
    default: goto append_character;
    }
append_character:
    *c1_bss.macro_text_cursor++ = (BYTE)character;
    out = c1_bss.macro_text_cursor;
    goto scan_next;
}

extern "C" int __cdecl macro_is_active(CompilerMacro *macro)
{
    int index;
    for (index = c1_bss.lexical_skip_mode; index >= 0; --index) {
        if ((unsigned)index > C1_MACRO_RESCAN_LAST)
            MACRO_UNSUPPORTED("macro recursion lookup outside encountered records");
        if (c1_bss.macro_expansions[index].macro == macro &&
            c1_bss.macro_expansions[index].active) return 1;
    }
    return 0;
}
extern "C" void __cdecl push_macro_expansion(CompilerMacro *macro)
{
    CompilerMacroExpansionFrame *frame;
    c1_bss.lexical_skip_mode = (int)((unsigned)c1_bss.lexical_skip_mode + 1);
    if ((int)c1_bss.lexical_skip_mode >= 256) compiler_fatal(9);
    if ((unsigned)c1_bss.lexical_skip_mode > C1_MACRO_RESCAN_LAST)
        MACRO_UNSUPPORTED("macro push outside encountered rescan records");
    ++c1_bss.macro_active_count;
    frame = &c1_bss.macro_expansions[c1_bss.lexical_skip_mode];
    frame->macro = macro;
    frame->arguments = c1_bss.saved_macro_token_cursor;
    frame->argument_count = (signed char)c1_bss.macro_argument_count;
    frame->source = c1_bss.source_cursor;
    frame->argument_index = 0;
    c1_bss.macro_text_cursor = c1_bss.saved_macro_text_cursor;
    frame->replacement = c1_bss.macro_text_cursor;
}
extern "C" void __cdecl begin_macro_replacement(void)
{
    CompilerMacroExpansionFrame *frame;
    CompilerMacroBody *body;
    int parameters;
    if ((unsigned)c1_bss.lexical_skip_mode > C1_MACRO_RESCAN_LAST)
        MACRO_UNSUPPORTED("macro replacement outside encountered rescan records");
    frame = &c1_bss.macro_expansions[c1_bss.lexical_skip_mode];
    body = frame->macro->body;
    c1_bss.source_cursor = body->payload;
    if (!c1_data.options.language_extensions) {
        parameters = body->kind;
        while (parameters-- > 0) c1_bss.source_cursor += *c1_bss.source_cursor;
    }
    frame->active = 1;
}
extern "C" int __cdecl finish_macro_replacement(void)
{
    CompilerMacroExpansionFrame *frame;
    BYTE *out;
    if ((unsigned)c1_bss.lexical_skip_mode > C1_MACRO_RESCAN_LAST)
        MACRO_UNSUPPORTED("macro finish outside encountered rescan records");
    frame = &c1_bss.macro_expansions[c1_bss.lexical_skip_mode];
    --c1_bss.macro_active_count;
    if (c1_bss.macro_active_count) {
        --c1_bss.lexical_skip_mode;
        c1_bss.source_cursor = frame->source;
        frame->active = 0;
        c1_bss.macro_token_cursor = frame->arguments;
        return 0;
    }
    out = c1_bss.macro_text_cursor;
    if (out >= c1_bss.work_buffers[1].limit) {
        out = grow_macro_text(out,c1_bss.work_buffers[1].data,&c1_bss.work_buffers[1].limit);
        c1_bss.macro_text_cursor = out;
    }
    *c1_bss.macro_text_cursor++ = 0;
    *c1_bss.macro_text_cursor++ = 'R';
    c1_bss.source_cursor = frame->replacement;
    c1_bss.macro_rescan_output_pending = 1;
    return 1;
}
extern "C" int __cdecl skip_macro_whitespace(void)
{
    unsigned character, special;
    int ready = 0;
    c1_bss.macro_whitespace_seen = 0;
    c1_data.lexer_flags &= ~0x400000U;
    for (;;) {
        character = *c1_bss.source_cursor++;
        switch (c1_data.scanner.classes[character]) {
        case SC_SPACE:
            c1_bss.macro_whitespace_seen = 1;
            continue;
        case SC_NEWLINE:
            if (c1_data.lexer_flags & 0x8000) {
                --c1_bss.source_cursor;
                goto finish;
            }
            ++c1_bss.current_context.value;
            if (c1_bss.options.preprocess_stdout)
                fputc(c1_bss.source_cursor[-1], c1_bss.preprocessor_output);
            c1_data.lexer_flags |= 0x400000;
            c1_bss.macro_whitespace_seen = 1;
            continue;
        case SC_SPECIAL:
            special = process_special_character();
            if (special & 0xa) goto finish;
            if (special & ~0x50U) continue;
            /* The ordinary source byte remains for the macro caller. */
        default:
            --c1_bss.source_cursor;
            ready = 1;
            goto finish;
        }
    }
finish:
    if (c1_bss.macro_whitespace_seen) {
        if (c1_bss.macro_text_cursor >= c1_bss.work_buffers[1].limit)
            grow_macro_text(c1_bss.macro_text_cursor, c1_bss.work_buffers[1].data,
                &c1_bss.work_buffers[1].limit);
        if (c1_bss.macro_text_cursor > c1_bss.work_buffers[1].data &&
            c1_bss.macro_text_cursor[-1] != ' ')
            *c1_bss.macro_text_cursor++ = ' ';
    }
    return ready;
}
extern "C" CompilerNameEntry *__cdecl intern_name_span(const char *, unsigned, unsigned, int *);
extern "C" int __cdecl scan_macro_identifier(BYTE character)
{
    BYTE *first = c1_bss.macro_text_cursor;
    const BYTE *parameter;
    unsigned hash = 0, length = 0;
    int parameter_mode = 0, identifier = 1, suppress_length = -1, created;
restart:
    if (c1_data.scanner.classes[character] == 38) {
        parameter = (const BYTE *)resolve_macro_argument(&parameter_mode);
        if (parameter && *parameter) {
            do {
                if (!(c1_data.scanner.flags[*parameter] & 3)) identifier = 0;
                if (c1_bss.macro_text_cursor >= c1_bss.work_buffers[1].limit)
                    grow_macro_text(c1_bss.macro_text_cursor, c1_bss.work_buffers[1].data,
                        &c1_bss.work_buffers[1].limit);
                *c1_bss.macro_text_cursor++ = *parameter;
                hash = (hash >> 4) + hash * 4 + *parameter++;
                ++length;
            } while (*parameter);
        }
    } else {
        *c1_bss.macro_text_cursor++ = character;
        hash = (hash >> 4) + hash * 4 + character;
        ++length;
    }
next_character:
    character = *c1_bss.source_cursor++;
    while (c1_data.scanner.flags[character] & 3) {
        if (c1_bss.macro_text_cursor >= c1_bss.work_buffers[1].limit)
            grow_macro_text(c1_bss.macro_text_cursor, c1_bss.work_buffers[1].data,
                &c1_bss.work_buffers[1].limit);
        *c1_bss.macro_text_cursor++ = character;
        hash = (hash >> 4) + hash * 4 + character;
        ++length;
        character = *c1_bss.source_cursor++;
    }
    if (c1_data.scanner.classes[character] == 41) {
        suppress_length = *c1_bss.source_cursor++;
        goto next_character;
    }
    if (c1_data.scanner.classes[character] == 38) {
        parameter_mode = 1;
        goto restart;
    }
    --c1_bss.source_cursor;
    if (!identifier) return 0;
    if (suppress_length == c1_bss.macro_text_cursor - first && suppress_length != -1) {
        *c1_bss.macro_text_cursor++ = 4;
        *c1_bss.macro_text_cursor++ = (BYTE)suppress_length;
        return 0;
    }
    *c1_bss.macro_text_cursor = 0;
    c1_data.current_identifier = intern_name_span((const char *)first, hash, length, &created);
    return !created;
}
extern "C" void __cdecl parse_macro_arguments(CompilerMacro *macro, int expected)
{
    int depth = 0, comment, parameter_mode;
    unsigned character, next, special;
    BYTE *argument_start, *saved_tokens = c1_bss.macro_token_cursor;
    BYTE *saved_text, *saved_parameters;
    *c1_bss.macro_text_cursor++ = c1_bss.source_cursor[-1];
next_argument:
    if (c1_bss.macro_text_cursor >= c1_bss.work_buffers[1].limit)
        grow_macro_text(c1_bss.macro_text_cursor, c1_bss.work_buffers[1].data,
            &c1_bss.work_buffers[1].limit);
skip_leading_space:
    if (!skip_macro_whitespace()) goto finish;
    if (*c1_bss.source_cursor == '/') {
        ++c1_bss.source_cursor;
        if (!c1_data.options.language_extensions && !c1_data.options.preprocess_only) {
            next = (BYTE)read_unspliced_character();
            if (next == '/') {
                next = (BYTE)read_unspliced_character();
                if (next == '*') {
                    report_language_diagnostic(4, 179);
                    c1_data.lexer_flags |= 0x800;
                    skip_block_comment();
                    c1_data.lexer_flags &= ~0x800U;
                    comment = 0;
                } else {
                    --c1_bss.source_cursor;
                    report_language_diagnostic(4, 1);
                    c1_data.lexer_flags |= 0x800;
                    skip_line_comment();
                    c1_data.lexer_flags &= ~0x800U;
                    comment = 1;
                }
            } else {
                --c1_bss.source_cursor;
                next = (BYTE)read_unspliced_character();
                if (next == '*') {
                    c1_data.lexer_flags |= 0x800;
                    skip_block_comment();
                    c1_data.lexer_flags &= ~0x800U;
                    comment = 1;
                } else { --c1_bss.source_cursor; comment = 0; }
            }
        } else {
            next = *c1_bss.source_cursor++;
            if (c1_data.scanner.classes[next] == SC_SPECIAL)
                next = (BYTE)resolve_special_character(next);
            if (next == '/') {
                c1_data.lexer_flags |= 0x800;
                skip_line_comment();
                c1_data.lexer_flags &= ~0x800U;
                comment = 1;
            } else {
                --c1_bss.source_cursor;
                next = *c1_bss.source_cursor++;
                if (c1_data.scanner.classes[next] == SC_SPECIAL)
                    next = (BYTE)resolve_special_character(next);
                if (next == '*') {
                    c1_data.lexer_flags |= 0x800;
                    if (c1_bss.options.preserve_comments && !(c1_data.lexer_flags & 0x8000))
                        skip_preserved_macro_comment();
                    else skip_macro_block_comment();
                    c1_data.lexer_flags &= ~0x800U;
                    comment = 1;
                } else { --c1_bss.source_cursor; comment = 0; }
            }
        }
        if (comment) goto skip_leading_space;
        argument_start = c1_bss.macro_text_cursor;
        *c1_bss.macro_text_cursor++ = '/';
    } else argument_start = c1_bss.macro_text_cursor;
    parameter_mode = 0;
scan:
    character = *c1_bss.source_cursor++;
    switch (c1_data.scanner.classes[character]) {
    case SC_RPAREN:
        if (--depth < 0) goto end_argument;
        break;
    case SC_COMMA:
        if (!depth) goto end_argument;
        break;
    case SC_LPAREN:
        ++depth;
        break;
    case SC_SPACE:
    case SC_NEWLINE:
        --c1_bss.source_cursor;
        if (skip_macro_whitespace()) goto scan;
        goto end_argument;
    case SC_SQUOTE:
    case SC_DQUOTE:
        c1_bss.macro_text_cursor = copy_macro_quoted_text(c1_bss.macro_text_cursor,
            &c1_bss.work_buffers[1].limit, character);
        goto scan;
    case SC_IDENTIFIER:
    case SC_LETTER_L:
        *c1_bss.macro_text_cursor++ = (BYTE)character;
        character = *c1_bss.source_cursor++;
        while (c1_data.scanner.flags[character] & 3) {
            if (c1_bss.macro_text_cursor >= c1_bss.work_buffers[1].limit)
                grow_macro_text(c1_bss.macro_text_cursor, c1_bss.work_buffers[1].data,
                    &c1_bss.work_buffers[1].limit);
            *c1_bss.macro_text_cursor++ = (BYTE)character;
            character = *c1_bss.source_cursor++;
        }
        if (c1_data.scanner.classes[character] != 38) {
            --c1_bss.source_cursor;
            goto scan;
        }
        parameter_mode = 1;
        /* Encoded argument marker follows the identifier prefix. */
    case 38:
        append_macro_text(resolve_macro_argument(&parameter_mode));
        goto scan;
    case 39:
        append_macro_quoted('"', macro_argument_text());
        goto scan;
    case 40:
        append_macro_quoted('\'', macro_argument_text());
        goto scan;
    case SC_SPECIAL:
        saved_text = c1_bss.macro_text_cursor;
        saved_parameters = c1_bss.macro_token_cursor;
        special = process_special_character();
        if (special & 0xa) return;
        c1_bss.macro_token_cursor = saved_parameters;
        c1_bss.macro_text_cursor = saved_text;
        if (!(special & 0x50)) goto scan;
        break;
    case SC_SLASH:
        if (!c1_data.options.language_extensions && !c1_data.options.preprocess_only) {
            next = (BYTE)read_unspliced_character();
            if (next == '/') {
                next = (BYTE)read_unspliced_character();
                if (next != '*') {
                    --c1_bss.source_cursor;
                    report_language_diagnostic(4, 1);
                    c1_data.lexer_flags |= 0x800;
                    skip_line_comment();
                    c1_data.lexer_flags &= ~0x800U;
                    comment = 1;
                } else {
                    report_language_diagnostic(4, 179);
                    c1_data.lexer_flags |= 0x800;
                    if (c1_bss.options.preserve_comments && !(c1_data.lexer_flags & 0x8000))
                        skip_preserved_macro_comment();
                    else skip_macro_block_comment();
                    c1_data.lexer_flags &= ~0x800U;
                    comment = 0;
                }
            } else {
                --c1_bss.source_cursor;
                next = (BYTE)read_unspliced_character();
                if (next == '*') {
                    c1_data.lexer_flags |= 0x800;
                    skip_block_comment();
                    c1_data.lexer_flags &= ~0x800U;
                    comment = 1;
                } else { --c1_bss.source_cursor; comment = 0; }
            }
        } else {
            next = *c1_bss.source_cursor++;
            if (c1_data.scanner.classes[next] == SC_SPECIAL)
                next = (BYTE)resolve_special_character(next);
            if (next == '/') {
                c1_data.lexer_flags |= 0x800;
                skip_line_comment();
                c1_data.lexer_flags &= ~0x800U;
                comment = 1;
            } else {
                --c1_bss.source_cursor;
                next = *c1_bss.source_cursor++;
                if (c1_data.scanner.classes[next] == SC_SPECIAL)
                    next = (BYTE)resolve_special_character(next);
                if (next == '*') {
                    c1_data.lexer_flags |= 0x800;
                    if (c1_bss.options.preserve_comments && !(c1_data.lexer_flags & 0x8000))
                        skip_preserved_macro_comment();
                    else skip_macro_block_comment();
                    c1_data.lexer_flags &= ~0x800U;
                    comment = 1;
                } else { --c1_bss.source_cursor; comment = 0; }
            }
        }
        if (!comment) break;
        if (c1_bss.macro_text_cursor > c1_bss.work_buffers[1].data &&
            c1_bss.macro_text_cursor[-1] == ' ') goto scan;
        character = ' ';
        break;
    }
    *c1_bss.macro_text_cursor++ = (BYTE)character;
    goto scan;
end_argument:
    if (c1_bss.macro_text_cursor - 2 >= c1_bss.work_buffers[1].data &&
        c1_data.scanner.classes[c1_bss.macro_text_cursor[-1]] == SC_SPACE &&
        c1_data.scanner.classes[c1_bss.macro_text_cursor[-2]] != 41)
        --c1_bss.macro_text_cursor;
    if (c1_bss.macro_text_cursor > argument_start) {
        ++c1_bss.macro_argument_count;
        copy_macro_argument_range(argument_start, c1_bss.macro_text_cursor);
    }
    *c1_bss.macro_text_cursor++ = (BYTE)character;
    if (depth >= 0) goto next_argument;
finish:
    c1_bss.saved_macro_token_cursor = saved_tokens;
    if (expected < (int)c1_bss.macro_argument_count)
        report_language_diagnostic(1, 2, c1_data.current_identifier);
    else if (expected > (int)c1_bss.macro_argument_count)
        report_language_diagnostic(1, 3, c1_data.current_identifier);
    if (macro->body->payload[0]) {
        push_macro_expansion(macro);
        finish_macro_argument();
    } else {
        c1_bss.macro_token_cursor = c1_bss.saved_macro_token_cursor;
        c1_bss.macro_text_cursor = c1_bss.saved_macro_text_cursor;
    }
}
extern "C" void __cdecl append_macro_text(const char *text)
{
    if (!text) return;
    while (*text) {
        if (c1_bss.macro_text_cursor >= c1_bss.work_buffers[1].limit)
            grow_macro_text(c1_bss.macro_text_cursor,
                c1_bss.work_buffers[1].data, &c1_bss.work_buffers[1].limit);
        *c1_bss.macro_text_cursor++ = (BYTE)*text++;
    }
}
extern "C" BYTE *__cdecl quote_macro_filename(BYTE *out, const char *filename)
{
    const BYTE *source = (const BYTE *)filename;
    unsigned category;
    *out++ = '"';
    while ((*out = *source) != 0) {
        category = c1_data.scanner.classes[*source];
        if (category == SC_SPECIAL) *++out = '\\';
        else if (category == SC_LEAD) *++out = *++source;
        ++out;
        ++source;
    }
    *out++ = '"';
    return out;
}
/* Source-only expression context at RVA5e9a0. Its pointer and preceding
 * field have no default, reset or native semantic contract. */
struct CompilerMacroLineContext {
    BYTE unmodeled_00[4];
    const char *expression;
};
extern "C" CompilerMacroLineContext *c1_unmodeled_macro_line_context;
CompilerMacroLineContext *c1_unmodeled_macro_line_context;
extern "C" void __cdecl prepare_macro_line_expression(void) { __asm { int 3 } }
extern "C" const char *__cdecl current_macro_line(void)
{
    CompilerIrSymbol *symbol;
    if (c1_bss.debug_module->Slot7c() && !(c1_data.lexer_flags & 0x8000)) {
        /* The contextual expression producer has not been reached. */
        if (!(c1_data.lexer_flags & 0x8000))
            MACRO_UNSUPPORTED("debug macro line expression is not modeled");
        symbol = c1_bss.current_scope_symbol;
        if (!symbol || !(symbol->symbol_qualifiers & 1) || (symbol->storage_class & 7) == 3) {
            prepare_macro_line_expression();
            format_buffer(c1_numeric_token_text, "(%s+%d)", 128,
                c1_unmodeled_macro_line_context->expression,
                c1_bss.current_context.value - c1_bss.nonil_type_index);
            return c1_numeric_token_text;
        }
    }
    if (!c1_bss.debug_module->Slot4c())
        compiler_error(73, ".\\iccLINE.cpp", 114);
    format_buffer(c1_numeric_token_text, "%d", 128, c1_bss.current_context.value);
    return c1_numeric_token_text;
}
extern "C" const char *__cdecl macro_argument_text(void)
{
    CompilerMacroExpansionFrame *frame;
    BYTE *argument;
    int index;
    if ((unsigned)c1_bss.lexical_skip_mode > C1_MACRO_RESCAN_LAST)
        MACRO_UNSUPPORTED("macro stringification outside encountered rescan records");
    frame = &c1_bss.macro_expansions[c1_bss.lexical_skip_mode];
    index = (signed char)*c1_bss.source_cursor++;
    argument = frame->arguments;
    if (index > frame->argument_count) return NULL;
    if (index <= 0)
        MACRO_UNSUPPORTED("nonpositive stringification parameter is not supported");
    while (--index) argument += *(short *)argument;
    return (const char *)argument + 2;
}
extern "C" void __cdecl append_macro_quoted(unsigned quote, const char *text)
{
    int in_literal = 0;
    unsigned literal_quote = quote, character, category;
    if (!text) return;
    *c1_bss.macro_text_cursor++ = (BYTE)quote;
    for (;;) {
        if (c1_bss.macro_text_cursor >= c1_bss.work_buffers[1].limit)
            grow_macro_text(c1_bss.macro_text_cursor,
                c1_bss.work_buffers[1].data, &c1_bss.work_buffers[1].limit);
        character = (BYTE)*text;
        category = c1_data.scanner.classes[character];
        if (category == SC_SPECIAL) {
            if (!character) break;
            if (character == '\\' && in_literal)
                *c1_bss.macro_text_cursor++ = (BYTE)character;
        } else if (category == SC_SQUOTE || category == SC_DQUOTE) {
            if (category == SC_DQUOTE && (BYTE)quote == '"')
                *c1_bss.macro_text_cursor++ = '\\';
            if ((BYTE)quote != '\'') {
                if (!in_literal) {
                    in_literal = 1;
                    literal_quote = character;
                } else if (character == literal_quote) in_literal = 0;
            }
        } else if (category == SC_LEAD) {
            *c1_bss.macro_text_cursor++ = (BYTE)*text++;
        } else if (category == 41) {
            text += 2;
            continue;
        }
        *c1_bss.macro_text_cursor++ = (BYTE)*text++;
    }
    *c1_bss.macro_text_cursor++ = (BYTE)quote;
}
extern "C" long __cdecl current_source_timestamp(void) { __asm { int 3 } return 0; }

extern "C" int __cdecl expand_defined_operator(CompilerNameEntry *identifier)
{
    int result = 0, parentheses = 0;
    unsigned character, category;
    if (strcmp(identifier->name, "defined")) return 0;
    if (!skip_macro_whitespace() && !c1_bss.macro_active_count) return 0;
    if (*c1_bss.source_cursor == '(') {
        ++c1_bss.source_cursor;
        parentheses = 1;
        if (!skip_macro_whitespace() && !c1_bss.macro_active_count) return 0;
    }
    character = *c1_bss.source_cursor;
    if ((int)c1_bss.macro_active_count > 0) {
        if (c1_data.scanner.classes[character] == 38 ||
            (c1_data.scanner.flags[character] & 1)) {
            ++c1_bss.source_cursor;
            scan_macro_identifier((BYTE)character);
            result = c1_data.current_identifier->macro != NULL;
        }
    } else if (c1_data.scanner.flags[character] & 1) {
        ++c1_bss.source_cursor;
        scan_identifier(character);
        result = c1_data.current_identifier->macro != NULL;
    } else compiler_warning(parentheses ? 4 : 3);

    category = c1_data.scanner.classes[*c1_bss.source_cursor];
    if ((category == SC_SPACE || category == SC_SPECIAL) && !skip_macro_whitespace())
        return result;
    if (parentheses) {
        if (*c1_bss.source_cursor == ')') {
            ++c1_bss.source_cursor;
            --parentheses;
        }
        if (parentheses > 0 && !c1_bss.macro_active_count)
            report_language_diagnostic(1, 4);
    }
    return result;
}

/* Zero introduces a replacement-stream control byte. The trailing R resumes
 * the genuine source pointer saved by the matching expansion frame. */
/* RVA196a8. This byte-returning endpoint crosses source refills and line
 * splices; it leaves encoded macro NULs for their active replacement owner. */
extern "C" unsigned __cdecl resolve_special_character(unsigned input)
{
    BYTE character = (BYTE)input;
    if (c1_data.scanner.classes[character] == SC_COLON) return character;
    for (;;) {
        while (!character && (int)c1_bss.macro_active_count <= 0) {
            process_special_character();
            character = *c1_bss.source_cursor++;
        }
        if (c1_data.scanner.classes[character] != SC_SPECIAL) return character;
        if (character == '?' && (process_trigraph() & ~0x40U)) {
            character = *c1_bss.source_cursor++;
            continue;
        }
        if (character == '\\' && splice_input_line()) {
            character = *c1_bss.source_cursor++;
            continue;
        }
        return character;
    }
}

extern "C" unsigned __cdecl process_special_character(void)
{
    unsigned character = c1_bss.source_cursor[-1];
    unsigned operation;
    CompilerMacroExpansionFrame *frame;
    if (character == '?') return process_trigraph();
    if (character == '\\') {
        if (!c1_bss.lexical_skip_mode && splice_input_line()) return 1;
        return 0x10;
    }
    if (!c1_bss.lexical_skip_mode) {
        refill_lexer_source();
        return 1;
    }
    operation = *c1_bss.source_cursor++;
    if (operation == 'A') {
        if (c1_bss.lexical_skip_mode > C1_MACRO_RESCAN_LAST)
            MACRO_UNSUPPORTED("macro argument replay outside encountered rescan records");
        frame = &c1_bss.macro_expansions[c1_bss.lexical_skip_mode];
        copy_macro_argument_range(frame->replacement,c1_bss.macro_text_cursor);
        c1_bss.macro_text_cursor = frame->replacement;
        finish_macro_argument();
        return 2;
    }
    if (operation == 'D') return finish_macro_replacement() ? 8 : 4;
    if (operation != 'R') return 0;
    if (c1_bss.lexical_skip_mode > C1_MACRO_RESCAN_LAST)
        MACRO_UNSUPPORTED("macro source replay outside encountered rescan records");
    frame = &c1_bss.macro_expansions[c1_bss.lexical_skip_mode];
    c1_bss.source_cursor = frame->source;
    frame->active = 0;
    if (c1_bss.lexical_skip_mode == 1) {
        c1_bss.macro_token_cursor = c1_bss.work_buffers[0].data;
        c1_bss.macro_text_cursor = c1_bss.work_buffers[1].data;
    }
    if (!c1_bss.macro_rescan_output_pending && c1_bss.browser_filename)
        c1_data.backend->Slot2c();
    c1_bss.macro_rescan_output_pending = 0;
    --c1_bss.lexical_skip_mode;
    return 0x20;
}
extern "C" void __cdecl refill_lexer_source(void) { __asm { int 3 } }
extern "C" void __cdecl copy_macro_argument_range(BYTE *first, BYTE *last)
{
    unsigned length = (unsigned)(last - first);
    BYTE *output;
    if (c1_bss.macro_token_cursor + length >= c1_bss.work_buffers[0].limit)
        grow_macro_text(c1_bss.macro_token_cursor + length,
            c1_bss.work_buffers[0].data, &c1_bss.work_buffers[0].limit);
    output = c1_bss.macro_token_cursor;
    memcpy(output + 2, first, length);
    output[length + 2] = 0;
    output[length + 3] = 'A';
    *(WORD *)output = (WORD)(length + 4);
    c1_bss.macro_token_cursor = output + length + 4;
}
extern "C" void __cdecl finish_macro_argument(void)
{
    CompilerMacroExpansionFrame *frame;
    BYTE *argument;
    unsigned index;
    if ((unsigned)c1_bss.lexical_skip_mode > C1_MACRO_RESCAN_LAST)
        MACRO_UNSUPPORTED("macro argument frame outside the encountered owners");
    frame = &c1_bss.macro_expansions[c1_bss.lexical_skip_mode];
    if (frame->argument_count <= frame->argument_index) {
        begin_macro_replacement();
        return;
    }
    ++frame->argument_index;
    argument = frame->arguments;
    index = (BYTE)(frame->argument_index - 1);
    while (index--) argument += *(short *)argument;
    c1_bss.source_cursor = argument + 2;
}

extern "C" int __cdecl macro_argument_continues(void);
extern "C" const char *__cdecl resolve_macro_argument(int *mode)
{
    CompilerMacroExpansionFrame *frame;
    BYTE *argument;
    int index, continues;
    if ((unsigned)c1_bss.lexical_skip_mode > C1_MACRO_RESCAN_LAST)
        MACRO_UNSUPPORTED("macro parameter frame outside the encountered owners");
    frame = &c1_bss.macro_expansions[c1_bss.lexical_skip_mode];
    index = (signed char)*c1_bss.source_cursor++;
    argument = frame->arguments;
    if (index > frame->argument_count) return NULL;
    continues = macro_argument_continues();
    if (!*mode && !continues) index = (signed char)(BYTE)(index + frame->argument_count);
    index = (signed char)(BYTE)(index - 1);
    *mode = continues;
    if (index < 0)
        MACRO_UNSUPPORTED("negative encoded macro parameter index is not supported");
    while (index--) argument += *(short *)argument;
    return (const char *)argument + 2;
}
extern "C" int __cdecl macro_argument_continues(void)
{
    unsigned character = *c1_bss.source_cursor++;
    int continues = c1_data.scanner.classes[character] == 38 ||
        (c1_data.scanner.flags[character] & 3) != 0;
    --c1_bss.source_cursor;
    return continues;
}
