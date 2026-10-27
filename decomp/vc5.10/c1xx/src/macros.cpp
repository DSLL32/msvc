/* Definition parsing keeps the shared replacement buffer and signed
 * parameter-count selector. Macro expansion remains separately outlined. */
#include <string.h>
#include "frontend-init.h"
#include "parser-state.h"
#include "preprocessor.h"
#include <recon.h>
#pragma intrinsic(strcmp)
#pragma intrinsic(strlen, memcpy)

extern "C" {
int __cdecl scan_identifier(unsigned);
unsigned __cdecl read_directive_character(unsigned);
void __cdecl discard_input_character(void);
unsigned __cdecl process_special_character(void);
unsigned __cdecl resolve_special_character(unsigned);
int __cdecl skip_line_comment(void);
void __cdecl skip_macro_block_comment(void);
void __cdecl skip_preserved_macro_comment(void);
void __cdecl parse_macro_parameters(void);
int __cdecl find_macro_parameter(const char *);
int __cdecl read_source_chunk(void);
int __cdecl splice_input_line(void);
void __cdecl report_macro_input_end(int);
unsigned __cdecl read_macro_replacement(void);
void __cdecl grow_work_buffer(BYTE *, BYTE *, BYTE **);
BYTE *__cdecl copy_macro_quoted_text(BYTE *, BYTE **, unsigned);
BYTE __cdecl advance_multibyte_source(void);
void __cdecl define_preprocessor_macro(const char *, unsigned, int);
int __cdecl macro_is_disabled(CompilerMacro *); /*ec99*/
void __cdecl push_macro_expansion(CompilerMacro *); /*ecd0*/
void __cdecl select_macro_replacement(void); /*ed41*/
int __cdecl pop_macro_expansion(void); /*f09f*/
void __cdecl substitute_macro_arguments(CompilerMacro *, int); /*19de3*/
void __cdecl append_macro_text(const char *); /*2c407*/
char *__cdecl macro_argument_text(void); /*4716c*/
void __cdecl append_macro_quoted(unsigned, const char *); /*471b3*/
char *__cdecl format_macro_line_number(void); /*a16a7*/
BYTE *__cdecl copy_quoted_filename(BYTE *, const char *); /*80776*/
long __cdecl get_macro_file_time(void); /*8066f*/
char *__cdecl format_predefined_time(long); /*3b453*/
int __cdecl skip_conditional_whitespace(void); /*f154*/
int __cdecl scan_macro_identifier(unsigned); /*ed78*/
int __cdecl evaluate_conditional_identifier(CompilerNameEntry *); /*41d87*/
}

extern "C" int __cdecl expand_macro(CompilerMacro *macro)
{
    int expanded = 0, kind;
    BYTE character, next;
    unsigned special, category;
    const char *text;
    if (macro_is_disabled(macro)) return 0;
    frontend_bss.macro_input_mode = 0;
    frontend_bss.saved_macro_text_cursor = frontend_bss.macro_text_cursor;
    frontend_bss.macro_source_position = frontend_bss.saved_context_stack.position;
begin_expansion:
    frontend_bss.macro_token_prefix = 0;
    frontend_bss.saved_macro_token_cursor = frontend_bss.macro_token_cursor;
    kind = macro->body->kind;
    if (kind < -3) {
        if (frontend_bss.browser_filename) frontend_data.backend->Slot24(macro);
        push_macro_expansion(macro);
        if (frontend_bss.macro_expansion_depth < 0 || frontend_bss.macro_expansion_depth >= 256) {
            __asm { int 3 }
        }
        frontend_bss.macro_expansion_frames[frontend_bss.macro_expansion_depth].disabled = 1;
        switch (kind) {
        case -11: case -10: case -8:
            append_macro_text("1");
            break;
        case -9:
            if (!frontend_bss.debug_module->Slot50())
                report_compiler_diagnostic(73, ".\\p0macros.c", 0x3b8);
            if (frontend_data.options.reproducible) {
                frontend_bss.macro_timestamp[0] = '1';
                frontend_bss.macro_timestamp[1] = 0;
            } else {
                text = format_predefined_time(get_macro_file_time());
                memcpy(frontend_bss.macro_timestamp, text, 25);
                memcpy(frontend_bss.macro_timestamp + 11, text + 16, 4);
                memcpy(frontend_bss.macro_timestamp + 15, text + 20, 4);
                memcpy(frontend_bss.macro_timestamp + 19, text + 10, 4);
                frontend_bss.macro_timestamp[23] = text[14];
                frontend_bss.macro_timestamp[24] = 0;
            }
            append_macro_quoted('"', frontend_bss.macro_timestamp);
            break;
        case -7:
            if (!frontend_bss.debug_module->Slot50())
                report_compiler_diagnostic(73, ".\\p0macros.c", 0x3a6);
            append_macro_quoted('"', frontend_bss.macro_time);
            break;
        case -6:
            if (!frontend_bss.debug_module->Slot50())
                report_compiler_diagnostic(73, ".\\p0macros.c", 0x3a1);
            append_macro_quoted('"', frontend_bss.macro_date);
            break;
        case -5:
            text = frontend_data.options.reproducible ? "Brepro.cxx" :
                (char *)frontend_bss.saved_context_stack.FileName();
            frontend_bss.macro_text_cursor = copy_quoted_filename(frontend_bss.macro_text_cursor, text);
            break;
        case -4:
            append_macro_text(format_macro_line_number());
            break;
        }
        if (pop_macro_expansion()) return 1;
        goto resume_text;
    }
    if (!kind) {
        expanded = 1;
        if (frontend_bss.browser_filename) frontend_data.backend->Slot24(macro);
        if (macro->body->payload[0]) {
            push_macro_expansion(macro);
            select_macro_replacement();
            goto resume_text;
        }
        frontend_bss.macro_text_cursor = frontend_bss.saved_macro_text_cursor;
        goto collect_text;
    }
    if (kind == -1) kind = 0;
    if (!skip_conditional_whitespace()) goto resume_text;
check_parenthesis:
    if (*frontend_bss.source_cursor == '/') {
        ++frontend_bss.source_cursor;
        next = *frontend_bss.source_cursor++;
        if (frontend_data.scanner.classes[next] == SC_SPECIAL)
            next = (BYTE)resolve_special_character(next);
        if (next == '/') {
            frontend_data.compiler_flags |= 0x800;
            skip_line_comment();
            goto comment_finished;
        }
        --frontend_bss.source_cursor;
        next = *frontend_bss.source_cursor++;
        if (frontend_data.scanner.classes[next] == SC_SPECIAL)
            next = (BYTE)resolve_special_character(next);
        if (next == '*') {
            frontend_data.compiler_flags |= 0x800;
            if (frontend_bss.options.preserve_comments && !(frontend_data.compiler_flags & 0x8000))
                skip_preserved_macro_comment();
            else skip_macro_block_comment();
        comment_finished:
            frontend_data.compiler_flags &= ~0x800U;
            if (skip_conditional_whitespace()) goto check_parenthesis;
            goto resume_text;
        }
        frontend_bss.source_cursor -= 2;
    }
    if (*frontend_bss.source_cursor == '(') {
        ++frontend_bss.source_cursor;
        frontend_data.compiler_flags |= 0x1000;
        expanded = 1;
        substitute_macro_arguments(macro, kind);
        frontend_data.compiler_flags &= ~0x1000U;
        if (frontend_bss.browser_filename) frontend_data.backend->Slot24(macro);
        goto resume_text;
    }
    push_macro_expansion(macro);
    select_macro_replacement();
    frontend_bss.saved_macro_text_cursor = frontend_bss.macro_text_cursor;
    append_macro_text(macro->identifier->name);
    if (frontend_data.compiler_flags & 0x400000) {
        frontend_data.compiler_flags &= ~0x400000U;
        append_macro_text("\n");
        --frontend_bss.saved_context_stack.position;
    } else if (frontend_bss.macro_whitespace_pending) append_macro_text(" ");
    frontend_bss.source_cursor = frontend_bss.saved_macro_text_cursor;
    if (pop_macro_expansion()) return 1;
resume_text:
collect_text:
    while (frontend_bss.macro_input_mode) {
        if (frontend_bss.macro_text_cursor >= frontend_bss.work_buffers[1].limit)
            grow_work_buffer(frontend_bss.macro_text_cursor, frontend_bss.work_buffers[1].data,
                &frontend_bss.work_buffers[1].limit);
        character = *frontend_bss.source_cursor++;
        category = frontend_data.scanner.classes[character];
        switch (category) {
        case SC_IDENTIFIER: case SC_LETTER_L: case SC_MARKER1:
            frontend_bss.saved_macro_text_cursor = frontend_bss.macro_text_cursor;
            if (scan_macro_identifier(character)) {
                macro = frontend_data.current_identifier->macro;
                if (macro) {
                    if (!macro_is_disabled(macro)) goto begin_expansion;
                    *frontend_bss.macro_text_cursor++ = 4;
                    *frontend_bss.macro_text_cursor++ = (BYTE)strlen(frontend_data.current_identifier->name);
                }
            }
            if ((frontend_data.compiler_flags & 0x4000) &&
                frontend_data.current_identifier == frontend_data.builtin_names.defined_operator)
                evaluate_conditional_identifier(frontend_data.current_identifier);
            break;
        case SC_DOT:
            if (frontend_data.scanner.classes[*frontend_bss.source_cursor] != SC_DIGIT) {
                *frontend_bss.macro_text_cursor++ = '.';
                break;
            }
            /* A leading decimal point shares the numeric-spelling loop. */
        case SC_DIGIT:
            for (;;) {
                *frontend_bss.macro_text_cursor++ = character;
                if (frontend_bss.macro_text_cursor >= frontend_bss.work_buffers[1].limit)
                    grow_work_buffer(frontend_bss.macro_text_cursor, frontend_bss.work_buffers[1].data,
                        &frontend_bss.work_buffers[1].limit);
                character = *frontend_bss.source_cursor++;
                if (frontend_data.scanner.flags[character] & 4) continue;
                if ((character == '+' || character == '-') &&
                    (*frontend_bss.macro_text_cursor == 'e' || *frontend_bss.macro_text_cursor == 'E'))
                    continue;
                --frontend_bss.source_cursor;
                break;
            }
            break;
        case SC_MARKER2:
            append_macro_quoted('"', macro_argument_text());
            break;
        case SC_MARKER3:
            append_macro_quoted('\'', macro_argument_text());
            break;
        case SC_DQUOTE: case SC_SQUOTE:
            frontend_bss.macro_text_cursor = copy_macro_quoted_text(frontend_bss.macro_text_cursor,
                &frontend_bss.work_buffers[1].limit, character);
            break;
        case SC_SPACE:
            do { next = *frontend_bss.source_cursor++; }
            while (frontend_data.scanner.classes[next] == SC_SPACE);
            --frontend_bss.source_cursor;
            *frontend_bss.macro_text_cursor++ = ' ';
            break;
        case SC_SPECIAL:
            special = process_special_character();
            if (special & 0x50) *frontend_bss.macro_text_cursor++ = character;
            break;
        default:
            *frontend_bss.macro_text_cursor++ = character;
            break;
        }
    }
    return expanded;
}

extern "C" void __cdecl process_define_directive(int notify_debug)
{
    BYTE character, next;
    unsigned category, length;
    int index;
    if (notify_debug) {
        frontend_bss.debug_module->Slot1c();
        if (frontend_data.options.incremental_rebuild &&
            ((ClassDependencyRecordsView *)frontend_data.dependency->records)->record_members) {
            index = frontend_data.declaration_context_index;
            if (index >= 16) { __asm { int 3 } }
            while (index >= 0) {
                frontend_data.dependency->RecordMacroScope(frontend_bss.class_scope_rows[index].symbol);
                --index;
            }
        }
    }
    character = *frontend_bss.source_cursor++;
    if (!(frontend_data.scanner.flags[character] & 0x20)) {
        while (frontend_data.scanner.classes[character] == SC_SPACE)
            character = *frontend_bss.source_cursor++;
        if (!(frontend_data.scanner.flags[character] & 0x20))
            character = (BYTE)read_directive_character(character);
    }
    if (!(frontend_data.scanner.flags[character] & 1)) {
        compiler_warning(7);
        discard_input_character();
        return;
    }
    scan_identifier(character);
    frontend_bss.current_macro_text = (char *)frontend_bss.work_buffers[1].data;
    frontend_bss.macro_parameter_count = 0;
read_character:
    character = *frontend_bss.source_cursor++;
    category = frontend_data.scanner.classes[character];
    switch (category) {
    case SC_SPECIAL:
        if (process_special_character() & ~0x50U) goto read_character;
        break;
    case SC_SPACE:
        if (character == '\r') goto read_character;
        goto replacement;
    case SC_SLASH:
        next = *frontend_bss.source_cursor++;
        if (frontend_data.scanner.classes[next] == SC_SPECIAL)
            next = (BYTE)resolve_special_character(next);
        if (next == '/') {
            frontend_data.compiler_flags |= 0x800;
            skip_line_comment();
            frontend_data.compiler_flags &= ~0x800U;
            goto replacement;
        }
        --frontend_bss.source_cursor;
        next = *frontend_bss.source_cursor++;
        if (frontend_data.scanner.classes[next] == SC_SPECIAL)
            next = (BYTE)resolve_special_character(next);
        if (next == '*') {
            frontend_data.compiler_flags |= 0x800;
            if (frontend_bss.options.preserve_comments && !(frontend_data.compiler_flags & 0x8000))
                skip_preserved_macro_comment();
            else skip_macro_block_comment();
            frontend_data.compiler_flags &= ~0x800U;
            goto replacement;
        }
        --frontend_bss.source_cursor;
        character = '/';
        break;
    case SC_LPAREN:
        parse_macro_parameters();
        if (!frontend_bss.macro_parameter_count) frontend_bss.macro_parameter_count = -1;
        goto replacement;
    case SC_NEWLINE:
        --frontend_bss.source_cursor;
        define_preprocessor_macro(NULL, 0, 0);
        return;
    }
    compiler_warning(8, character);
replacement:
    length = read_macro_replacement();
    if (!frontend_data.options.language_extensions) {
        length += (unsigned)(frontend_bss.current_macro_text - (char *)frontend_bss.work_buffers[1].data);
        frontend_bss.current_macro_text = (char *)frontend_bss.work_buffers[1].data;
    }
    define_preprocessor_macro(frontend_bss.current_macro_text, length, frontend_bss.macro_parameter_count);
}

extern "C" unsigned __cdecl read_macro_replacement(void)
{
    BYTE *out = (BYTE *)frontend_bss.current_macro_text;
    BYTE *identifier;
    unsigned character, next, category, special;
    int stringize = 0, characterize = 0, parameter;
    character = *frontend_bss.source_cursor++;
    if (!(frontend_data.scanner.flags[character] & 0x20)) {
        while (frontend_data.scanner.classes[character] == SC_SPACE)
            character = *frontend_bss.source_cursor++;
        if (!(frontend_data.scanner.flags[character] & 0x20))
            character = (BYTE)read_directive_character(character);
    }
dispatch:
    if (out >= frontend_bss.work_buffers[1].limit)
        grow_work_buffer(out, frontend_bss.work_buffers[1].data, &frontend_bss.work_buffers[1].limit);
    category = frontend_data.scanner.classes[character];
    switch (category) {
    case SC_NEWLINE:
        --frontend_bss.source_cursor;
        if (out == (frontend_data.options.language_extensions ? (BYTE *)frontend_bss.current_macro_text :
                    frontend_bss.work_buffers[1].data)) return 0;
        if (out >= frontend_bss.work_buffers[1].limit)
            grow_work_buffer(out, frontend_bss.work_buffers[1].data, &frontend_bss.work_buffers[1].limit);
        *out++ = 0;
        *out++ = 0x44; /* End of an encoded replacement list. */
        return (unsigned)(out - (BYTE *)frontend_bss.current_macro_text);
    case SC_SPECIAL:
        special = process_special_character();
        if (special & 0x50) {
            if (special & 0x10) goto escaped_character;
            goto append_character;
        }
        goto next_character;
    case SC_SPACE:
    skip_spaces:
        character = *frontend_bss.source_cursor++;
        if (!(frontend_data.scanner.flags[character] & 0x20)) {
            while (frontend_data.scanner.classes[character] == SC_SPACE)
                character = *frontend_bss.source_cursor++;
            if (!(frontend_data.scanner.flags[character] & 0x20))
                character = (BYTE)read_directive_character(character);
        }
        if (character != '\n') *out++ = ' ';
        goto dispatch;
    case SC_SQUOTE:
    case SC_DQUOTE:
        out = copy_macro_quoted_text(out, &frontend_bss.work_buffers[1].limit, character);
        goto next_character;
    case SC_HASH:
        for (;;) {
            next = *frontend_bss.source_cursor++;
            if (frontend_data.scanner.classes[next] != SC_SPECIAL) break;
            if (!(process_special_character() & ~0x50U)) break;
        }
        if (frontend_data.scanner.classes[next] == SC_HASH) {
            if (out == (BYTE *)frontend_bss.current_macro_text) {
                compiler_warning(160);
                goto dispatch;
            }
            if (out[-1] == ' ') --out;
            character = *frontend_bss.source_cursor++;
            if (!(frontend_data.scanner.flags[character] & 0x20)) {
                while (frontend_data.scanner.classes[character] == SC_SPACE)
                    character = *frontend_bss.source_cursor++;
                if (!(frontend_data.scanner.flags[character] & 0x20))
                    character = (BYTE)read_directive_character(character);
            }
            if (character == '\n') {
                --frontend_bss.source_cursor;
                compiler_warning(161);
            }
            goto dispatch;
        }
        if (frontend_data.scanner.classes[next] == SC_AT) {
            if (frontend_bss.macro_parameter_count <= 0 || !frontend_data.options.language_extensions)
                goto literal_hash;
            characterize = 1;
        } else if (frontend_bss.macro_parameter_count > 0) {
            --frontend_bss.source_cursor;
            stringize = 1;
        } else goto literal_hash;
        {
            character = *frontend_bss.source_cursor++;
            if (!(frontend_data.scanner.flags[character] & 0x20)) {
                while (frontend_data.scanner.classes[character] == SC_SPACE)
                    character = *frontend_bss.source_cursor++;
                if (!(frontend_data.scanner.flags[character] & 0x20))
                    character = (BYTE)read_directive_character(character);
            }
            if (frontend_data.scanner.flags[character] & 1) goto identifier_start;
            stringize = characterize = 0;
            compiler_warning(162);
            goto dispatch;
        }
literal_hash:
        *out++ = '#';
        character = frontend_bss.source_cursor[-1];
        goto dispatch;
    case SC_IDENTIFIER:
    case SC_LETTER_L:
    identifier_start:
        identifier = out;
    identifier_character:
        if (out >= frontend_bss.work_buffers[1].limit)
            grow_work_buffer(out, frontend_bss.work_buffers[1].data, &frontend_bss.work_buffers[1].limit);
        *out++ = (BYTE)character;
        character = *frontend_bss.source_cursor++;
        if (frontend_data.scanner.flags[character] & 3) goto identifier_character;
    identifier_end:
        if (frontend_data.scanner.classes[character] == SC_SPECIAL) {
            if (process_special_character() & ~0x50U) {
                character = *frontend_bss.source_cursor++;
                if (frontend_data.scanner.flags[character] & 3) goto identifier_character;
                goto identifier_end;
            }
        }
        if (frontend_bss.macro_parameter_count > 0) {
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
        next = *frontend_bss.source_cursor++;
        if (frontend_data.scanner.classes[next] == SC_SPECIAL)
            next = (BYTE)resolve_special_character(next);
        if (next == '/') {
            frontend_data.compiler_flags |= 0x800;
            skip_line_comment();
            frontend_data.compiler_flags &= ~0x800U;
            goto skip_spaces;
        }
        --frontend_bss.source_cursor;
        next = *frontend_bss.source_cursor++;
        if (frontend_data.scanner.classes[next] == SC_SPECIAL)
            next = (BYTE)resolve_special_character(next);
        if (next == '*') {
            frontend_data.compiler_flags |= 0x800;
            if (frontend_bss.options.preserve_comments && !(frontend_data.compiler_flags & 0x8000))
                skip_preserved_macro_comment();
            else skip_macro_block_comment();
            frontend_data.compiler_flags &= ~0x800U;
            goto skip_spaces;
        }
        --frontend_bss.source_cursor;
        goto append_character;
    case SC_LEAD:
        *out++ = (BYTE)character;
        character = advance_multibyte_source();
        goto append_character;
    case SC_INVALID:
        compiler_warning(18, character);
        goto next_character;
    }
append_character:
    *out++ = (BYTE)character;
next_character:
    character = *frontend_bss.source_cursor++;
    goto dispatch;
escaped_character:
    *out++ = (BYTE)character;
    next = *frontend_bss.source_cursor++;
    character = next;
    if (frontend_data.scanner.classes[next] == SC_SPECIAL)
        character = (BYTE)resolve_special_character(next);
    /* The target tests the original next byte's class, even when the
     * special-character helper supplied a different output byte. */
    if (frontend_data.scanner.classes[next] >= SC_SQUOTE &&
        frontend_data.scanner.classes[next] <= SC_DQUOTE) {
        --out;
        compiler_warning(295, next);
        character = *frontend_bss.source_cursor++;
        if (!(frontend_data.scanner.flags[character] & 0x20)) {
            while (frontend_data.scanner.classes[character] == SC_SPACE)
                character = *frontend_bss.source_cursor++;
            if (!(frontend_data.scanner.flags[character] & 0x20))
                character = (BYTE)read_directive_character(character);
        }
        goto dispatch;
    }
    goto append_character;
}

/* Parameters are length-linked names in the existing macro work buffer.
 * The current marker byte is published only after duplicate lookup. */
extern "C" void __cdecl parse_macro_parameters(void)
{
    BYTE *out = (BYTE *)frontend_bss.current_macro_text + 1, *name = out;
    BYTE character;
    unsigned category;
    int previous;
    for (;;) {
        character = *frontend_bss.source_cursor++;
        if (!(frontend_data.scanner.flags[character] & 0x20)) {
            while (frontend_data.scanner.classes[character] == SC_SPACE)
                character = *frontend_bss.source_cursor++;
            if (!(frontend_data.scanner.flags[character] & 0x20))
                character = (BYTE)read_directive_character(character);
        }
        category = frontend_data.scanner.classes[character];
        if (category == SC_IDENTIFIER || category == SC_LETTER_L) {
            if (out != name) compiler_warning(10, character);
            if (out >= frontend_bss.work_buffers[1].limit) { __asm { int 3 } }
            *out++ = character;
identifier_character:
            character = *frontend_bss.source_cursor++;
            if (frontend_data.scanner.flags[character] & 3) {
                if (out >= frontend_bss.work_buffers[1].limit) { __asm { int 3 } }
                *out++ = character;
                goto identifier_character;
            }
            if (!character) {
                if (read_source_chunk()) report_macro_input_end(4);
                goto identifier_character;
            }
            if (character == '\\' && splice_input_line()) goto identifier_character;
            --frontend_bss.source_cursor;
            if (out >= frontend_bss.work_buffers[1].limit) { __asm { int 3 } }
            *out++ = 0;
            continue;
        }
        if (category == SC_COMMA || category == SC_RPAREN) {
            if (out > name) {
                previous = find_macro_parameter((const char *)name);
                if (previous >= 1) compiler_warning(9, name);
                else {
                    *(BYTE *)frontend_bss.current_macro_text = (BYTE)(out - (BYTE *)frontend_bss.current_macro_text);
                    frontend_bss.current_macro_text = (char *)out;
                    ++frontend_bss.macro_parameter_count;
                }
            } else if (character == ',' || frontend_bss.macro_parameter_count > 0)
                compiler_warning(10, character);
            if (character == ')') return;
            out = (BYTE *)frontend_bss.current_macro_text + 1;
            name = out;
            continue;
        }
        compiler_warning(10, character);
    }
}
extern "C" void __cdecl skip_macro_block_comment(void) { __asm { int 3 } }
extern "C" void __cdecl skip_preserved_macro_comment(void) { __asm { int 3 } }
void RECON_THISCALL CompilerDependency::RecordMacroScope(CompilerGlobalSymbolView *) { __asm { int 3 } }

extern "C" int __cdecl find_macro_parameter(const char *name)
{
    const BYTE *record = frontend_bss.work_buffers[1].data;
    int index;
    for (index = 1; index <= frontend_bss.macro_parameter_count; ++index) {
        if (!strcmp(name, (const char *)record + 1)) return index;
        record += *record;
    }
    return -1;
}
extern "C" void __cdecl report_macro_input_end(int) { __asm { int 3 } }

extern "C" BYTE *__cdecl copy_macro_quoted_text(BYTE *, BYTE **, unsigned)
{ __asm { int 3 } return NULL; }
extern "C" int __cdecl macro_is_disabled(CompilerMacro *macro)
{
    int index;
    for (index = frontend_bss.macro_expansion_depth; index >= 0; --index) {
        if (index >= 256) { __asm { int 3 } }
        CompilerMacroExpansionFrame *frame = &frontend_bss.macro_expansion_frames[index];
        if (frame->macro == macro && frame->disabled) return 1;
    }
    return 0;
}
extern "C" void __cdecl push_macro_expansion(CompilerMacro *macro)
{
    int index = ++frontend_bss.macro_expansion_depth;
    if (index >= 256) report_compiler_error(9);
    if (index < 0 || index >= 256) { __asm { int 3 } }
    CompilerMacroExpansionFrame *frame = &frontend_bss.macro_expansion_frames[index];
    ++frontend_bss.macro_input_mode;
    frame->macro = macro;
    frame->tokens = frontend_bss.saved_macro_token_cursor;
    frame->token_prefix = (BYTE)frontend_bss.macro_token_prefix;
    frame->source = frontend_bss.source_cursor;
    frame->reset_byte = 0;
    frontend_bss.macro_text_cursor = frontend_bss.saved_macro_text_cursor;
    frame->text = frontend_bss.saved_macro_text_cursor;
}
extern "C" void __cdecl select_macro_replacement(void)
{
    int index = frontend_bss.macro_expansion_depth;
    if (index < 0 || index >= 256) { __asm { int 3 } }
    CompilerMacroExpansionFrame *frame = &frontend_bss.macro_expansion_frames[index];
    CompilerMacroBody *body = frame->macro->body;
    frontend_bss.source_cursor = (BYTE *)body->payload;
    if (!frontend_data.options.language_extensions) {
        int count = body->kind;
        while (count-- > 0)
            frontend_bss.source_cursor += *frontend_bss.source_cursor;
    }
    frame->disabled = 1;
}
extern "C" int __cdecl pop_macro_expansion(void)
{
    int index;
    --frontend_bss.macro_input_mode;
    if (!frontend_bss.macro_input_mode) {
        if (frontend_bss.macro_text_cursor >= frontend_bss.work_buffers[1].limit)
            grow_work_buffer(frontend_bss.macro_text_cursor, frontend_bss.work_buffers[1].data,
                &frontend_bss.work_buffers[1].limit);
        *frontend_bss.macro_text_cursor++ = 0;
        *frontend_bss.macro_text_cursor++ = 'R';
        index = frontend_bss.macro_expansion_depth;
        if (index < 0 || index >= 256) { __asm { int 3 } }
        frontend_bss.source_cursor = frontend_bss.macro_expansion_frames[index].text;
        frontend_bss.suppress_macro_end_notification = 1;
        return 1;
    }
    index = frontend_bss.macro_expansion_depth;
    if (index < 0 || index >= 256) { __asm { int 3 } }
    CompilerMacroExpansionFrame *frame = &frontend_bss.macro_expansion_frames[index];
    --frontend_bss.macro_expansion_depth;
    frontend_bss.source_cursor = frame->source;
    frame->disabled = 0;
    frontend_bss.macro_token_cursor = frame->tokens;
    return 0;
}
extern "C" void __cdecl substitute_macro_arguments(CompilerMacro *, int) { __asm { int 3 } }
extern "C" void __cdecl append_macro_text(const char *) { __asm { int 3 } }
extern "C" char *__cdecl macro_argument_text(void) { __asm { int 3 } return NULL; }
extern "C" void __cdecl append_macro_quoted(unsigned, const char *) { __asm { int 3 } }
extern "C" char *__cdecl format_macro_line_number(void) { __asm { int 3 } return NULL; }
extern "C" BYTE *__cdecl copy_quoted_filename(BYTE *, const char *) { __asm { int 3 } return NULL; }
extern "C" long __cdecl get_macro_file_time(void) { __asm { int 3 } return 0; }
