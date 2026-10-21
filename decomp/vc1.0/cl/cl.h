#ifndef CL_H
#define CL_H
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <io.h>
#include <process.h>
#include <errno.h>

typedef struct Item {
    struct Item *next;
    unsigned flags;
    union { int kind; char *groups; } value;
    char *name, *extra;
    void *origin;
} Item;
typedef struct Input {
    struct Input *next;
    char *filename;
    int kind;
} Input;
typedef struct PassRecord PassRecord;
typedef struct Driver {
    Item *options, *other_options;
    int pass_index;
    PassRecord *current_pass;
    Input *inputs, *current_input;
    Input *first_link;
    char *temporary, *include_path, *directory, *error_file;
    char *programs[9][3];
    char *message_files[9][3];
    char *loaders[9][3];
} Driver;
extern Driver driver_storage, *driver;
extern int show_commands, quiet, errors, help_mode, preprocess;
extern Item *pending;
extern char *frontend_defaults[], *preprocessor_defaults[];
void install_control_handler(void);
char **split_arguments(char *);
int count_arguments(char **);
int has_nologo(char **, int);
void usage(void);
void banner(int);
char *program_name(char *);
Driver *initialize_driver(char *, Driver *);
void parse_arguments(char **, int);
void write_message(int, char *, ...);
char *join_path(char *, char *);
void load_messages(char *);
void *allocate(unsigned);
char *copy_string(char *);
void release(void *);
char *skip_space(char *);
void fatal(int, ...);
void warning(int, ...);
void driver_exit(int);
char *append_string(char *, char *);
void finish_options(Driver *);
void prepare_inputs(void);
void prepare_execution(void);
void run_driver(Driver *);
char *program_directory(char *);
typedef struct AttachedOption { int letter; char *patterns; } AttachedOption;
AttachedOption *scan_option_kind(char);
void parse_attached_option(AttachedOption *, char *);
int parse_option(char **, int *);
void read_response_file(char *);
int classify_input(char *);
void add_input(char *, int);
char *format_message(unsigned *, char *, va_list);
int unsigned_digits(unsigned, char *, unsigned);
void ide_message(int, ...);
BOOL WINAPI control_handler(DWORD);
typedef struct OptionRule {
    char *syntax, *expansion, *requires, *conflicts;
    void *special;
} OptionRule;
extern OptionRule option_rules[];
OptionRule *find_rule(char **, int);
int match_rule(OptionRule *, char **, int);
Item *read_option(OptionRule *, char **, int *);
Item *new_item(char *, char *, char *, unsigned);
void append_items(Item **, Item *);
void free_items(Item *);
Item *expand_options(char *, Item *);
Item *select_options(void *, Item *, Item **);
void warn_override(int, Item *, Item *);
void validate_value(char *, char *, char *);
char *expand_text(char *, char *, char *);
char *expand_choice(char *, char *, char *, char **);
char *expand_extension(char *, char *, char *);
char *append_optional(char *, char *);
extern char ide_buffer[2048];
int match_item(Item *, Item *);
Item *copy_item(Item *);
Item *detach_item(Item **, Item *);
void recycle_item(Item *);
int is_decimal(int);
char *last_of(char *, char *);
void remove_define(Item *);
typedef struct DriverSwitch { char *name; int kind; void *target; } DriverSwitch;
extern DriverSwitch driver_switches[];
extern char *object_file, *executable_file, *map_file;
extern int make_dll, coff_format;
int is_for_pass(Item *, void *);
char *expand_filename(char *, char *, Driver *);
char *filename_stem(char *, char *);
char *output_stem(char *, char *);
char *map_filename(char *, char *);
struct PassRecord {
    char *executable, *alternate, *parameter;
    int enabled;
    unsigned letter_prefix;
    void *field14;
};
typedef struct Profile { char *extension; int link; PassRecord *passes; } Profile;
extern Profile profiles[];
int console_rows(void);
Input *new_input(char *, int);
extern int input_count;
int any_match(Item *, Item *);
int run_passes(PassRecord *, Driver *, int (__cdecl *)(Driver *));
void initialize_pass(Driver *);
int compile_pass(Driver *);
int link_pass(Driver *);
void cleanup_files(void);
void initialize_ide(void);
extern int output_produced;
unsigned expanded_length(char *, Driver *);
unsigned nonblank_length(char *);
char *quote_if_space(char *, char *);
int spawn_pass(int, char *, char *, char **);
void start_timer(void);
void report_timer(char *);
int is_space(int);
void write_coff_response(Driver *);
void write_omf_response(Driver *);
void file_message(FILE *, char *, ...);
#endif
