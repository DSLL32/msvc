#ifndef OPTIONS_H
#define OPTIONS_H
#include "c1xx.h"
typedef struct Vector {
    short count, capacity;
    int arena;
    void **items;
} Vector;
typedef struct OptionRule {
    char *pattern;
    void *target;
    signed char result;
    unsigned char action;
    unsigned short reserved;
} OptionRule;
typedef struct CompilerOptions {
    int japanese_identifiers;
    int debug_old_format, debug_info, line_records;
    char *database_file, *pch_link_name, *unrecognized_argument, *source_file;
    int syntax_only;
    unsigned char pch_auto;
    char *pch_auto_file;
    unsigned char pch_create;
    char *pch_create_file;
    unsigned char pch_use;
    char *pch_use_file;
    Vector *defines, *include_directories, *forced_includes;
} CompilerOptions;
typedef struct PackingEntry {
    struct PackingEntry *next;
    int alignment, field08;
} PackingEntry;
typedef struct PackingStack {
    PackingEntry *active, *free;
} PackingStack;
extern PackingStack *packing_stack;
extern int packing_alignment;
void *__stdcall allocate_permanent(unsigned bytes);
void *new_permanent_page(void);
typedef struct BitSet {
    unsigned bit_count;
    unsigned char bits[1];
} BitSet;
BitSet *__stdcall new_bitset(unsigned bit_count);
PackingStack *new_packing_stack(void);
PackingEntry *push_packing(PackingStack *stack);
void clear_packing(PackingEntry *entry);
extern CompilerOptions options;
extern OptionRule *option_rules;
extern unsigned char warning_levels[800];
extern unsigned lowest_warning_700, highest_warning_700;
extern char database_path[256], pch_object_name[256];
extern char **argument_vector_allocation;
typedef struct Scope Scope;
void __stdcall push_scope(Scope *scope);
void initialize_packing(void);
void initialize_checksums(void);
void initialize_lead_bytes(void);
void mark_lead_byte_ranges(unsigned char *ranges);
void read_env_arguments(char *arguments);
char **extend_argument_vector(void);
void *__stdcall checked_malloc(unsigned bytes);
Vector *__stdcall new_vector(int capacity, int arena);
int match_option(OptionRule *rules, char *argument, char *(*next)(void), int copy);
char *next_argument(void);
void initialize_japanese_characters(void);
void finalize_options(void);
void fatal_error(unsigned code, ...);
#endif
