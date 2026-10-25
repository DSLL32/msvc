#include <string.h>
#include <time.h>
#include "compiler-state.h"
#pragma intrinsic(strcpy, memcpy)
#include <recon.h>

/* One boolean flag selects which encountered token IDs receive a shared
 * prototype. Dynamic lexer records have their own later constructors.
 * Original table RVA63d70; only the used ID0..244 domain is modeled here. */
static const BYTE token_has_prototype[245] = {
    0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 0, 0,
    0, 0, 0, 0, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1,
    1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0,
    0, 0, 1, 1, 0, 0, 1, 1, 0, 1, 1, 1, 0, 1, 0, 1,
    0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 1, 0, 1, 1, 1,
    0, 1, 1, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0,
    1, 0, 1, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 1, 0, 1, 1,
    1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 0, 0, 0, 1, 1,
    1, 1, 0, 1, 1, 1, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0,
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 0,
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1,
    0, 0, 0, 1, 1,
};

extern "C" void __cdecl initialize_predefined_tokens(void)
{
    unsigned kind;
    CompilerPool *pool = &c1_bss.pools[6];
    CompilerTokenPrototype *token;
    for (kind = 1; kind < 245; ++kind) {
        if (token_has_prototype[kind]) {
            if (pool->remaining < 32)
                pool->Refill(32);
            token = (CompilerTokenPrototype *)pool->cursor;
            pool->cursor += 32;
            pool->remaining -= 32;
            if (token) {
                token->kind = (BYTE)kind;
                token->tag = (BYTE)kind;
            }
            c1_data.predefined_tokens[kind] = token;
        } else {
            c1_data.predefined_tokens[kind] = NULL;
        }
    }
}

/* The hash consumers mask to 0x7ff and index DWORD bucket heads. The
 * initializer only reserves their storage; it does not initialize or inspect
 * any table byte. Entry layout is pending until its actual constructors. */
extern "C" void __cdecl initialize_keyword_table(void)
{
    if (!c1_data.name_buckets) {
        CompilerArena *arena = &c1_bss.arenas[0];
        BYTE *memory = arena->cursor;
        arena->cursor += 8192;
        if (arena->cursor > arena->committed_limit)
            arena->CommitExtent();
        c1_data.name_buckets = (CompilerNameEntry **)memory;
    }
}


extern "C" {
long __cdecl get_compilation_time(long *);
char *__cdecl format_predefined_time(long);
void __cdecl initialize_macro_buffers(void);
void __cdecl define_command_line_macro(char *);
void __cdecl define_preprocessor_macro(const char *, unsigned, int);
struct CompilerNameEntry *__cdecl find_or_create_identifier(const char *);
void __cdecl write_global_header(void);
int __cdecl open_source_file(const char *, int, int, int, int);
void __cdecl report_source_file_error(const char *, int);
}
extern "C" void __cdecl define_command_line_macros(CompilerVector *definitions)
{
    long stamp;
    short index;
    int file;
    /* These dispatch classes are used for the four internal marker bytes. */
    c1_data.scanner.classes[1] = 38;
    c1_data.scanner.classes[2] = 39;
    c1_data.scanner.classes[3] = 40;
    c1_data.scanner.classes[4] = 41;
    if (c1_data.options.language_extensions) {
        c1_data.scanner.classes['$'] = SC_IDENTIFIER;
        c1_data.scanner.flags['$'] |= 1;
    }
    if (c1_data.options.reproducible) {
        c1_bss.macro_date_time[0] = '1';
        c1_bss.macro_date_time[1] = 0;
        c1_bss.macro_date = c1_bss.macro_time = c1_bss.macro_date_time;
    } else {
        get_compilation_time(&stamp);
        strcpy(c1_bss.macro_date_time, format_predefined_time(stamp));
        c1_bss.macro_date = c1_bss.macro_date_time + 4;
        c1_bss.macro_date_time[15] = 0;
        c1_bss.macro_time = c1_bss.macro_date_time + 16;
        c1_bss.macro_date_time[24] = 0;
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
    if (!c1_data.options.language_extensions) {
        find_or_create_identifier("__STDC__");
        define_preprocessor_macro(NULL, 0, -8);
    }
    for (index = 0; definitions && index < definitions->count; ++index)
        define_command_line_macro((char *)definitions->items[index]);
    if (!c1_bss.preprocessor_header_ready && c1_data.c9_il &&
        !c1_bss.options.prototypes_only)
        write_global_header();
    file = open_source_file(c1_bss.options.source_file, 0, 1, 1, 0);
    if (file > -1 && c1_bss.pch_file_failed)
        report_source_file_error(c1_bss.options.source_file, file);
    c1_bss.forced_include_index = 0;
    c1_bss.pending_forced_includes = c1_bss.options.forced_includes;
}
extern "C" long __cdecl get_compilation_time(long *result)
{ return time(result); }
extern "C" char *__cdecl format_predefined_time(long stamp)
{
    stamp &= 0x7fffffff;
    char *text = ctime(&stamp);
    if (text) {
        /* The first25 bytes retain the original newline. The separate
         * initialized trailing NUL is not overwritten by this copy. */
        memcpy(c1_data.predefined_time_text, text, 25);
        if (c1_data.predefined_time_text[8] == '0')
            c1_data.predefined_time_text[8] = ' ';
        memcpy(c1_data.predefined_time_text + 11, text + 20, 4);
        memcpy(c1_data.predefined_time_text + 16, text + 11, 4);
        memcpy(c1_data.predefined_time_text + 20, text + 15, 4);
    }
    return c1_data.predefined_time_text;
}
extern "C" void __cdecl initialize_macro_buffers(void)
{
    c1_data.macro_reset_value = 0;
    c1_bss.macro_token_cursor = c1_bss.work_buffers[0].data;
    c1_bss.macro_text_cursor = c1_bss.work_buffers[1].data;
    c1_bss.saved_macro_text_cursor = c1_bss.work_buffers[1].data;
}
extern "C" void __cdecl define_command_line_macro(char *definition)
{ __asm { int 3 } }
extern "C" void __cdecl define_preprocessor_macro(const char *text, unsigned length, int kind)
{ __asm { int 3 } }
extern "C" int __cdecl open_source_file(const char *name, int kind, int report, int mode, int track)
{ __asm { int 3 } return -1; }
extern "C" void __cdecl report_source_file_error(const char *name, int file)
{ __asm { int 3 } }
