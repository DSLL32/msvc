#ifndef CL510_H
#define CL510_H
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <io.h>
#include <process.h>
#include <errno.h>
#include <ctype.h>
#include <mbstring.h>
#include <mbctype.h>
#include <recon.h>

typedef struct Option Option;
typedef struct OptionRule {
    char *pattern, *add, *conflict, *replace;
    void *extra;
} OptionRule;
typedef struct SpecialOption { char *name; char *pattern; } SpecialOption;
typedef struct Input Input;
typedef struct CompilerPass CompilerPass;
typedef struct Job Job;
typedef struct DeferredInput {
    struct DeferredInput *next;
    BOOL is_cpp;
    char *source_name, *object_name;
    unsigned timestamp;
} DeferredInput;
/* The remaining allocation tail stays undefined until its consumers are reached. */
struct Job {
    Job *next;
    Input *input;
    unsigned status;
    DeferredInput deferred;
    unsigned unrecovered;
    char *browser_name;
};
typedef struct PendingWork {
    DeferredInput *outdated, *sources;
    DeferredInput *compiled;
    DeferredInput *recycled;
    DeferredInput *failed;
} PendingWork;
struct Option {
    Option *next;
    unsigned flags;
    char *matching;
    char *name;
    char *value;
    Option *origin;
};
struct Input {
    Input *next;
    char *name;
    int kind;
    unsigned short explicit_kind;
    short sequence;
};
typedef struct Driver {
    Option *options, *other_options;
    CompilerPass *current_pass;
    Input *inputs;
    Job *current_jobs;
    DeferredInput *deferred_inputs;
    DeferredInput **deferred_tail;
    Input *current_input;
    char *temporary, *include_path, *directory;
} Driver;
typedef int (__stdcall *InvokeCompilerPass)(int, char **, BOOL);
typedef void (__stdcall *AbortCompilerPass)(int);
struct CompilerPass {
    char *name, *path, *environment;
    char option_prefix[4];
    unsigned flags;
    HMODULE module;
    InvokeCompilerPass invoke;
    AbortCompilerPass abort;
    char *options;
};
typedef struct InputKind {
    char *extension;
    int category;
    int *passes;
} InputKind;
typedef struct TemporarySuffix { char *suffix; int per_input; } TemporarySuffix;
extern TemporarySuffix temporary_suffixes[8];
extern Driver driver_storage;
extern Driver *driver;
extern Option *unrecognized_options, *free_options;
extern int source_count;
extern int options_after_link, link_warning;
extern int no_logo, echo_response, help_requested, show_commands;
extern int response_depth, response_first_line;
extern int default_macros, dll_options_enabled;
extern int debug_exceptions, error_count, output_produced, default_c_language;
extern char *source_language, *image_name;
extern int is_dll;
extern int keep_preprocessor_output;
extern int cleanup_temporaries;
extern int bk_requested;
extern int minimal_rebuild, reproducible, first_dependency_scan;
extern int allow_pch, first_stage_notice;
extern int use_standard_include, bz_requested, timing_requested;
extern char *object_name, *pdb_name, *browser_name;
extern CompilerPass compiler_passes[5];
extern InputKind input_kinds[11];
extern int interrupted, pass_running;
extern CompilerPass *active_pass;

int main(int, char **, char **);
BOOL install_control_handler(void);
BOOL WINAPI console_control_handler(DWORD);
char **split_arguments(char *, int);
char *skip_space(char *);
int count_arguments(char **);
char *program_name(char *);
char *program_directory(char *);
char *duplicate(char *);
void initialize_driver(char *, Driver *);
void show_usage(void);
int has_nologo(char **, int);
void show_banner(int);
void parse_arguments(char **, int);
void read_response(char *);
int classify_input(char *);
Input *create_input(char *, int);
void add_input(char *, int, int);
SpecialOption *find_special(char *);
int has_prefix(char *, char *);
void apply_special(SpecialOption *, char *);
int match_option(char **, int *);
extern OptionRule option_rules[];
OptionRule *find_rule(char **, int);
int match_pattern(OptionRule *, char **, int);
Option *define_macro(char **, int *);
void undefine_macro(Option *);
Option *link_arguments(char **, int *);
Option *c_input(char **, int *);
Option *cpp_input(char **, int *);
Option *object_input(char **, int *);
void typed_input(const char *, char **, int *, int);
Option *read_option(OptionRule *, char **, int *);
void validate_value(char *, char *, char *);
Option *new_option(char *, char *, char *, unsigned);
void append_option(Option **, Option *);
Option *expand_options(char *, Option *);
char *expand_word(char *, char *, char *);
char *expand_alternative(char *, char *, char *, char **);
char *append_literal(char *, char *);
char *append_escaped(char *, char *);
char *apply_extension(char *, char *, char *);
char *find_last_separator(char *, char *);
Option *detach_options(Option **, Option *, Option **);
int options_match(Option *, Option *);
Option *copy_option(Option *);
Option *unlink_option(Option **, Option *);
void delete_options(Option *);
void delete_option(Option *);
void option_conflict(unsigned, Option *, Option *);
int write_message(int, const char *, ...);
int vwrite_message(int, const char *, va_list);
char *load_string(unsigned);
char *format_message(unsigned *, const char *, va_list);
int format_number(unsigned, char *, unsigned);
void finish_options(void);
int has_any_option(Option *, Option *);
void prepare_inputs(Driver *);
int option_for_pass(Option *, CompilerPass *);
char *expand_filename(char *, char *, Driver *);
char *file_basename(char *, char *);
char *file_directory(char *, char *);
char *make_map_name(char *, char *);
void configure_assembly(Option *);
void enable_pass(Option *);
void toggle_pass(Option *);
void disable_pass(Option *);
void set_output_option(Option *);
void set_pass_suffix(Option *);
void set_pass_name(Option *);
void undefine_defaults(Option *);
void enable_dll(Option *);

void prepare_environment(Driver *);
char *join_text(char *, char *);
void show_help(void);
int console_rows(void);
void *allocate(unsigned);
char *append_text(char *, const char *);
void warning(unsigned, ...);
void release(void *);
void fatal(unsigned, ...);
void prepare_execution(void);
void run_driver(Driver *);
Job *source_jobs(Driver *);
Job *create_job(Input *);
unsigned file_timestamp(Driver *);
Job *jobs_from_deferred(DeferredInput *);
Job *prepare_jobs(Driver *, Job *, PendingWork *);
Job *next_jobs(Driver *, PendingWork *);
DeferredInput *deferred_from_jobs(Job *);
int run_jobs(Driver *, Job *);
Job *advance_jobs(Job *);
void configure_pass(Driver *);
char *find_file(char *, char *, int *);
int search_path(char *, char *, int);
void cleanup_files(int);
void delete_temporary(int);
int run_passes(int *, Driver *, int (*)(Driver *));
int link_pass(Driver *);
int final_pass(Driver *);
int quoted_length(const char *);
int expanded_length(char *, Driver *);
char *quote_argument(char *, const char *);
void start_timer(void);
void stop_timer(char *);
int execute_pass(int, CompilerPass *, char *, char *, char **);
void unload_pass(CompilerPass *);
char *file_name(char *, char *);
void driver_exit(int);
#endif
