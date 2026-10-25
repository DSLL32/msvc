#include <string.h>
#include <time.h>
#include "frontend.h"
#pragma intrinsic(strcpy)
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
extern "C" char *__cdecl format_predefined_time(long stamp) { __asm { int 3 } return NULL; }
extern "C" void __cdecl initialize_macro_buffers(void) { __asm { int 3 } }
extern "C" void __cdecl define_preprocessor_macro(const char *text, unsigned length, int kind) { __asm { int 3 } }
extern "C" void __cdecl define_command_line_macro(char *definition) { __asm { int 3 } }
extern "C" int __cdecl open_source_file(const char *name, int kind, int report, int mode, int track) { __asm { int 3 } return -1; }
extern "C" void __cdecl report_source_file_error(const char *name, int file) { __asm { int 3 } }
