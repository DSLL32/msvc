#ifndef C2_H
#define C2_H
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <float.h>
#include <math.h>
#include <excpt.h>
#include <io.h>
#include <stdarg.h>
/* This target imports the FILE array; the SDK accessor adds a graph vertex. */
#undef _iob
__declspec(dllimport) extern FILE _iob[];
#include <recon.h>

typedef void (__cdecl *C2Initializer)(void);
enum OptionAction {
    OPT_ENABLE = 1,
    OPT_DISABLE = 5,
    OPT_STRING = 34,
    OPT_SELECTOR = 35,
    OPT_INTEGER = 36,
    OPT_STRING_LIST = 38,
    OPT_STRING_ONCE = 39
};
typedef struct C2Option {
    char *pattern;
    void *value;
    unsigned char result, kind;
    unsigned short reserved;
} C2Option;
typedef struct C2Selector {
    int selector, kind;
    void *value;
} C2Selector;
extern C2Option c2_options[54];
extern char *c2_current_option, *c2_option_operand;
extern char *c2_unexpected_argument;
int __fastcall parse_option_integer(char *value);
struct _exception;
struct _startupinfo { int newmode; };

/* Internal MSVCRT declarations used by the original embedded startup. */
__declspec(dllimport) void __cdecl __set_app_type(int);
__declspec(dllimport) int *__cdecl __p__fmode(void);
__declspec(dllimport) int *__cdecl __p__commode(void);
__declspec(dllimport) int _adjust_fdiv;
__declspec(dllimport) void __cdecl __setusermatherr(int (__cdecl *)(struct _exception *));
void __cdecl _initterm(C2Initializer *, C2Initializer *);
__declspec(dllimport) void __cdecl __getmainargs(int *, char ***, char ***, int, struct _startupinfo *);
__declspec(dllimport) char ***__cdecl __p___initenv(void);
int __cdecl _XcptFilter(unsigned long, struct _EXCEPTION_POINTERS *);

extern C2Initializer *c2_onexit_begin, *c2_onexit_end;
extern int c2_fmode, c2_commode, c2_adjust_fdiv;
extern int c2_default_math_error, c2_new_mode, c2_wildcards;
extern C2Initializer c2_c_initializers[1], c2_cpp_initializers[1];

void mainCRTStartup(void);
void c2_set_argv(void);
void c2_set_default_precision(void);
int c2_math_error(struct _exception *);
int main(int, char **, char **);
int c2_register_exit(C2Initializer callback);
C2Initializer c2_register_onexit(C2Initializer callback);
void __fastcall c2_read_environment(char *flags);
char **c2_copy_arguments(void);
extern int c2_argc;
extern char **c2_argv;
void c2_initialize_options(void);
int __fastcall c2_wildcard_match(char *pattern, char *input, unsigned kind);
void __fastcall c2_apply_option(C2Option *option, char *value, int mode);
char *c2_next_argument(void);
int __fastcall c2_match_option(C2Option *table, char *input, char *(__cdecl *next)(void), int mode);
void __fastcall c2_validate_alignment(char *alignment);
char *__fastcall c2_get_diagnostic_string(int message);
char *__fastcall find_cached_message(int message);
void __fastcall c2_system_fatal(int message, char *prefix, char *filename);
void c2_print_banner(void);
FILE *__fastcall c2_open_intermediate(char *base, char *suffix, char *mode, char *description);
void c2_read_header(void);
void c2_reset_symbol_table(void);
void *c2_create_type_table(void);
void c2_initialize_constants(void);
void c2_initialize_opcodes(void);
void c2_initialize_machine(void);
void c2_initialize_trees(void);
void c2_initialize_codegen(void);
void c2_initialize_registers(void);
void c2_initialize_optimizer(void);
void c2_compile_unit(void);
void c2_finalize_output(void);
void c2_flush_debug(void);
extern int c2_cleanup_started, c2_output_incomplete;
extern FILE *c2_output_stream;
void c2_cleanup(void);
BOOL WINAPI c2_console_handler(DWORD control);
void __fastcall c2_report_internal(char *source, int line);
void c2_fatal(int message, ...);
char *__fastcall lookup_message(unsigned message, int allow_missing);
typedef struct MessageEntry { unsigned id; char *text; } MessageEntry;
extern MessageEntry messages[7];
extern int messages_initialized;
extern char message_buffer[384];
void initialize_messages(void);
char *__fastcall load_message(unsigned message, char *buffer, int capacity, int allow_missing);
char *__fastcall unescape_message(char *buffer);
void emit_diagnostic(int category, int message, char *format, va_list arguments);
unsigned __fastcall resolve_diagnostic_location(unsigned location, char **file);
void format_diagnostic(char *buffer, const char *format, ...);
void format_diagnostic_args(char *buffer, const char *format, va_list arguments);
int __fastcall format_integer(int value, char *buffer, unsigned base);
#endif
