#include <string.h>
#include <time.h>
#include <io.h>
#include <fcntl.h>
#include <share.h>
#include <errno.h>
#include <stdlib.h>
#include "frontend.h"
#include "keywords.h"
#include "global-record.h"
#pragma intrinsic(strcpy, memcpy, strlen)
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
int __cdecl open_source_file(const char *, unsigned, int, int, int);
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
    if (!c1_bss.options.preprocess_stdout && c1_data.c9_il &&
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
extern "C" void __cdecl record_macro_definition(const char *, const char *, int);
extern "C" int __cdecl compare_macro_text(const void *, const void *, unsigned);
extern "C" void __cdecl report_language_diagnostic(unsigned, unsigned, ...);
extern "C" void __cdecl define_preprocessor_macro(const char *text, unsigned length, int kind)
{
    CompilerNameEntry *identifier = c1_data.current_identifier;
    if (identifier == c1_data.builtin_names.defined_operator) {
        report_language_diagnostic(1, 117, identifier, "#define");
        return;
    }
    CompilerMacro *macro = identifier->macro;
    if (!c1_data.source_include_depth && c1_bss.forced_source_index) {
        record_macro_definition(identifier->name,
            length ? c1_bss.current_macro_text : "", kind);
        identifier = c1_data.current_identifier;
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
    CompilerPool *pool = &c1_bss.pools[5];
    if (pool->remaining < 8)
        pool->Refill(8);
    identifier = c1_data.current_identifier;
    macro = (CompilerMacro *)pool->cursor;
    pool->cursor += 8;
    pool->remaining -= 8;
    macro->identifier = identifier;
    CompilerArena *arena = &c1_bss.arenas[0];
    CompilerMacroBody *body = (CompilerMacroBody *)arena->cursor;
    arena->cursor += (length + 11) & ~3U;
    if (arena->cursor > arena->committed_limit)
        arena->CommitExtent();
    macro->body = body;
    if (length)
        memcpy(body->payload, text, length);
    identifier = c1_data.current_identifier;
    identifier->flags |= 0x80000;
    identifier->macro = macro;
    body->kind = (signed char)kind;
    c1_data.backend->Slot28(macro);
}
extern "C" void __cdecl record_macro_definition(const char *name, const char *text, int kind)
{ __asm { int 3 } }
extern "C" int __cdecl compare_macro_text(const void *first, const void *second, unsigned size)
{ __asm { int 3 } return 0; }
/* These alternate PCH inputs remain unmodeled until their actual producers
 * and consumers are reached. The ordinary source-file path does not read them. */
static void *unmodeled_pch_input_cursor; /* BSS RVA55924 */
static int unmodeled_source_descriptor; /* initialized RVA5e228; consumed by _read */
extern "C" void __cdecl select_pch_input(void *);
extern "C" int __cdecl open_source_descriptor(const char *);
extern "C" void __cdecl push_source_context(CompilerSourceFile *, int, int, int);
extern "C" void __cdecl report_include_file(int depth, const char *name);
extern "C" void __cdecl report_response_error(int, ...);
extern "C" int __cdecl open_source_file(const char *name, unsigned hash,
                                        int kind, int flags, int track)
{
    CompilerSourceFile *file = c1_data.translation_unit->FindSourceFile(name, hash);
    int descriptor;
    if (file->record_id & 0xff000000) return -3;
    if (kind == 2) {
#ifdef RECON_NATIVE_TEST
        recon_unsupported("open_source_file: unmodeled PCH source context");
#endif
        select_pch_input(unmodeled_pch_input_cursor);
        push_source_context(file, kind, unmodeled_source_descriptor, flags);
        return -2;
    }
    descriptor = name ? open_source_descriptor(name) : stdin->_file;
    if (descriptor == -1) {
        if (kind == 1) report_response_error(83, 341, name);
        return -1;
    }
    push_source_context(file, kind, descriptor, flags);
    if (name && track && c1_bss.options.show_includes)
        report_include_file(c1_data.source_include_depth, name);
    return descriptor;
}
extern "C" CompilerNameEntry *__cdecl intern_name_span(const char *, unsigned, unsigned, int *);
CompilerSourceFile *RECON_THISCALL CompilerTranslationUnit::LookupSourceFile(CompilerNameEntry *name)
{ __asm { int 3 } return NULL; }
CompilerSourceFile *RECON_THISCALL CompilerTranslationUnit::FindSourceFile(
    const char *name, unsigned hash)
{
    unsigned length;
    int created;
    CompilerNameEntry *identifier;
    CompilerSourceFile *file;
    CompilerArena *arena = &c1_bss.arenas[0];
    if (hash) length = strlen(name);
    else {
        const unsigned char *text = (const unsigned char *)name;
        while (*text) {
            hash = (hash >> 4) + hash * 4 + *text;
            ++text;
        }
        length = (unsigned)(text - (const unsigned char *)name);
    }
    if (length > 256) compiler_error(81, name, 256);
    identifier = intern_name_span(name, hash, length, &created);
    if (!created) {
        file = LookupSourceFile(identifier);
        if (file) return file;
    }
    file = (CompilerSourceFile *)arena->cursor;
    arena->cursor += 20;
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    if (file) {
        file->name = identifier;
        file->kind = 21;
    }
    if (source_files) file->next = source_files;
    source_files = file;
    return file;
}
extern "C" void __cdecl select_pch_input(void *cursor) { __asm { int 3 } }
extern "C" const char *__cdecl prepare_source_path(const char *);
extern "C" int __cdecl close_old_source_descriptor(void);
extern "C" int __cdecl open_source_descriptor(const char *name)
{
    const char *path = prepare_source_path(name);
    int descriptor = -1;
    int attempt = 1;
    while (attempt < 2) {
        descriptor = _sopen(path, _O_BINARY, _SH_DENYWR);
        if (descriptor != -1 || errno != EMFILE) break;
        close_old_source_descriptor();
        ++attempt;
    }
    return descriptor;
}
extern "C" char *__cdecl duplicate_argument(const char *, int);
extern "C" const char *__cdecl prepare_source_path(const char *name)
{
    if (c1_bss.options.fat) {
        char drive[4], directory[256], base[256], extension[256];
        char *path = duplicate_argument(name, 3);
        _splitpath(path, drive, directory, base, extension);
        base[8] = 0;
        extension[4] = 0;
        _makepath(path, drive, directory, base, extension);
        return path;
    }
    return name;
}
extern "C" int __cdecl close_old_source_descriptor(void) { __asm { int 3 } return 0; }
extern "C" void __cdecl push_source_context(CompilerSourceFile *file, int kind, int descriptor, int flags)
{ __asm { int 3 } }
extern "C" void __cdecl report_include_file(int depth, const char *name) { __asm { int 3 } }
extern "C" void __cdecl report_source_file_error(const char *name, int file)
{ __asm { int 3 } }
