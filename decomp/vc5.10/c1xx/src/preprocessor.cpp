#include <string.h>
#include <time.h>
#include "frontend-init.h"
#include "preprocessor.h"
#pragma intrinsic(strcpy, memcpy)
#include <recon.h>

extern "C" {
long __cdecl get_compilation_time(long *);
char *__cdecl format_predefined_time(long);
void __cdecl initialize_macro_buffers(void);
void __cdecl define_preprocessor_macro(const char *, unsigned, int);
void __cdecl define_command_line_macro(char *);
CompilerNameEntry *__cdecl find_or_create_identifier(const char *);
void __cdecl write_global_header(void);
int __cdecl open_source_file(const char *, int, int, int, int);
void __cdecl report_source_file_error(const char *, int);
}

extern "C" void __cdecl define_command_line_macros(CompilerVector *definitions)
{
    long stamp;
    short index;
    int file;
    frontend_data.scanner.classes[1] = SC_MARKER1;
    frontend_data.scanner.classes[2] = SC_MARKER2;
    frontend_data.scanner.classes[3] = SC_MARKER3;
    frontend_data.scanner.classes[4] = SC_MARKER4;
    if (frontend_data.options.language_extensions) {
        frontend_data.scanner.classes['$'] = SC_IDENTIFIER;
        frontend_data.scanner.flags['$'] |= 1;
    }
    if (frontend_data.options.reproducible) {
        frontend_bss.macro_date_time[0] = '1';
        frontend_bss.macro_date_time[1] = 0;
        frontend_bss.macro_date = frontend_bss.macro_time = frontend_bss.macro_date_time;
    } else {
        get_compilation_time(&stamp);
        strcpy(frontend_bss.macro_date_time, format_predefined_time(stamp));
        frontend_bss.macro_date = frontend_bss.macro_date_time + 4;
        frontend_bss.macro_date_time[15] = 0;
        frontend_bss.macro_time = frontend_bss.macro_date_time + 16;
        frontend_bss.macro_date_time[24] = 0;
    }
    initialize_macro_buffers();
    find_or_create_identifier("__LINE__");
    define_preprocessor_macro(NULL, 0, -4);
    find_or_create_identifier("__FILE__");
    define_preprocessor_macro(NULL, 0, -5);
    find_or_create_identifier("__DATE__");
    define_preprocessor_macro(NULL, 0, -6);
    find_or_create_identifier("__TIME__");
    define_preprocessor_macro(NULL, 0, -7);
    find_or_create_identifier("__TIMESTAMP__");
    define_preprocessor_macro(NULL, 0, -9);
    find_or_create_identifier("__cplusplus");
    define_preprocessor_macro(NULL, 0, -10);
    if (!frontend_data.options.no_bool) {
        find_or_create_identifier("__BOOL_DEFINED");
        define_preprocessor_macro(NULL, 0, -11);
    }
    for (index = 0; definitions && index < definitions->count; ++index)
        define_command_line_macro((char *)definitions->items[index]);
    if (!frontend_bss.options.preprocess_stdout && frontend_data.option_c9il &&
        !frontend_bss.options.prototypes_only)
        write_global_header();
    file = open_source_file(frontend_bss.options.source_file, 0, 1, 1, 0);
    if (file > -1 && frontend_bss.pch_file_failed)
        report_source_file_error(frontend_bss.options.source_file, file);
    frontend_bss.pending_forced_includes = frontend_bss.options.forced_includes;
    frontend_bss.forced_include_index = 0;
}

extern "C" long __cdecl get_compilation_time(long *stamp)
{
    return (long)time((time_t *)stamp);
}
extern "C" char *__cdecl format_predefined_time(long stamp)
{
    stamp &= 0x7fffffff;
    char *text = ctime((time_t *)&stamp);
    if (text) {
        /* ctime supplies 25 bytes including its newline. The initialized
         * trailing NUL belongs to this mutable buffer and is retained. */
        memcpy(frontend_data.predefined_time_text, text, 25);
        if (frontend_data.predefined_time_text[8] == '0')
            frontend_data.predefined_time_text[8] = ' ';
        memcpy(frontend_data.predefined_time_text + 11, text + 20, 4);
        memcpy(frontend_data.predefined_time_text + 16, text + 11, 4);
        memcpy(frontend_data.predefined_time_text + 20, text + 15, 4);
    }
    return frontend_data.predefined_time_text;
}
extern "C" void __cdecl initialize_macro_buffers(void)
{
    frontend_data.macro_reset_value = 0;
    frontend_bss.macro_token_cursor = frontend_bss.work_buffers[0].data;
    frontend_bss.macro_text_cursor = frontend_bss.work_buffers[1].data;
    frontend_bss.saved_macro_text_cursor = frontend_bss.work_buffers[1].data;
}
extern "C" void __cdecl record_macro_definition(const char *, const char *, int);
extern "C" int __cdecl compare_macro_text(const void *, const void *, unsigned);
extern "C" void __cdecl report_language_diagnostic(unsigned, unsigned, ...);
extern "C" void __cdecl define_preprocessor_macro(const char *text, unsigned length, int kind)
{
    if (!length)
        text = NULL;
    CompilerNameEntry *identifier = frontend_data.current_identifier;
    if (identifier == frontend_data.builtin_names.defined_operator) {
        report_language_diagnostic(1, 117, identifier, "#define");
        return;
    }
    CompilerMacro *macro = identifier->macro;
    if (!frontend_data.source_include_depth && frontend_bss.forced_source_index) {
        record_macro_definition(identifier->name,
            length ? frontend_bss.current_macro_text : "", kind);
        identifier = frontend_data.current_identifier;
    }
    if (macro) {
        int previous_kind = macro->body->kind;
        if (previous_kind < -3) {
            report_language_diagnostic(1, 117, identifier, "#define");
            return;
        }
        int changed;
        if (kind != -3 && kind != previous_kind)
            changed = 1;
        else if (length)
            changed = compare_macro_text(text, macro->body->payload, length - 1);
        else
            changed = macro->body->payload[0] != 0;
        if (!changed)
            return;
        report_language_diagnostic(1, 5, identifier);
    }
    CompilerPool *pool = &frontend_bss.pools[6]; /* actual c04a8 */
    if (pool->remaining < 8)
        pool->Refill(8);
    identifier = frontend_data.current_identifier;
    macro = (CompilerMacro *)pool->cursor;
    pool->cursor += 8;
    pool->remaining -= 8;
    macro->identifier = identifier;
    CompilerArena *arena = &frontend_bss.arenas[0];
    CompilerMacroBody *body = (CompilerMacroBody *)arena->cursor;
    arena->cursor += (length + 11) & ~3U;
    if (arena->cursor > arena->committed_limit)
        arena->CommitExtent();
    macro->body = body;
    if (length)
        memcpy(body->payload, text, length);
    identifier = frontend_data.current_identifier;
    identifier->flags |= 0x80000;
    identifier->macro = macro;
    body->kind = (signed char)kind;
    frontend_data.backend->Slot28(macro);
}
extern "C" void __cdecl record_macro_definition(const char *name, const char *text, int kind)
{ __asm { int 3 } }
extern "C" int __cdecl compare_macro_text(const void *left, const void *right, unsigned bytes)
{ __asm { int 3 } return 0; }
extern "C" void __cdecl define_command_line_macro(char *definition) { __asm { int 3 } }
extern "C" int __cdecl open_source_file(const char *name, int kind, int report, int mode, int track) { __asm { int 3 } return -1; }
extern "C" void __cdecl report_source_file_error(const char *name, int file) { __asm { int 3 } }
