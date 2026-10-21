#ifndef OPTIONS_H
#define OPTIONS_H
#include "c1xx.h"
#include "configuration.h"
typedef struct Vector {
    short count, capacity;
    int arena;
    void **items;
} Vector;
typedef struct ZRule {
    unsigned letter, action;
    int *target;
} ZRule;
typedef struct OptionRule {
    char *pattern;
    void *target;
    signed char result;
    unsigned char action;
    unsigned short reserved;
} OptionRule;
typedef struct CompilerOptions {
    CompilerConfiguration configuration;
    int option_nologo;
    int option_FAT;
    char *path_characters;
    int option_C;
    Vector *defines;
    int option_E;
    Vector *include_directories;
    Vector *forced_includes;
    char *option_Fi;
    char *source_file;
    int option_WX;
    int option_W;
    int option_Zp;
    char *error_file;
    char *intermediate_prefix;
    int option_xc;
    int option_m;
    char *option_V;
    char *option_NV;
    char *option_Fr;
    char *option_FR;
    char *option_A;
    char *option_Q;
    char *database_file;
    char *option_Fo;
    int option_Fs;
    int option_Gf;
    int option_Os;
    int option_Oc;
    int option_Og;
    int option_Oe;
    int option_Or;
    int option_Oo;
    int option_Ov;
    int option_Of;
    char *pch_use_file;
    int pch_use;
    char *pch_create_file;
    int pch_create;
    char *option_Fp;
    char *pch_auto_file;
    int pch_auto;
    char *pch_link_name;
    char *unrecognized_argument;
    int debug_old_format;
    int z_n;
    int z_f;
    int syntax_only;
    int z_I;
    int z_r;
} CompilerOptions;
typedef struct PackingEntry {
    struct PackingEntry *next;
    int alignment, field08;
} PackingEntry;
typedef struct PackingStack {
    PackingEntry *active, *free;
} PackingStack;
extern PackingStack *packing_stack;
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
extern OptionRule option_rules[];
extern unsigned char warning_levels[800];
extern unsigned lowest_warning_700, highest_warning_700;
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
typedef struct ArenaBlock ArenaBlock;
struct ArenaBlock {
    ArenaBlock *next;
    unsigned bytes;
    unsigned char data[1];
};
typedef struct ArenaPool {
    ArenaBlock *first, *last;
    unsigned char *next;
    int remaining;
    unsigned field10;
} ArenaPool;
typedef struct Arena {
    unsigned block_bytes;
    ArenaBlock *free_blocks;
    ArenaPool pools[6];
} Arena;
extern Arena compiler_arena, diagnostic_arena;
void __stdcall refill_arena(Arena *arena, ArenaPool *pool);
Vector *__stdcall new_vector(short capacity, int arena);
int match_option(OptionRule *rules, char *argument, char *(*next)(void), int copy);
char *next_argument(void);
extern char *current_option_argument;
int prefix_character(char *characters, char c);
int match_option_suffix(char *pattern, char *argument, unsigned action);
void apply_option(OptionRule *rule, char *value, int copy);
int parse_option_number(char *value);
void __stdcall append_vector(Vector *vector, void *item);
void __stdcall grow_vector(Vector *vector);
void initialize_japanese_characters(void);
void finalize_options(void);
void fatal_error(unsigned code, ...);
void compiler_warning(int level, unsigned code, ...);
#endif
