#ifndef OPTIONS_H
#define OPTIONS_H
#include <stddef.h>
#include "c1.h"
#include <recon.h>

typedef struct Vector {
    short count, capacity;
    int arena;
    void **items;
} Vector;
typedef struct VectorIterator {
    Vector *vector;
    short index;
} VectorIterator;
typedef char VectorIterator_size[(sizeof(VectorIterator) == 8) ? 1 : -1];
typedef char VectorIterator_index[(offsetof(VectorIterator,index) == 4) ? 1 : -1];
typedef char Vector_size[(sizeof(Vector) == 12) ? 1 : -1];
typedef char Vector_capacity[(offsetof(Vector,capacity) == 2) ? 1 : -1];
typedef char Vector_arena[(offsetof(Vector,arena) == 4) ? 1 : -1];
typedef char Vector_items[(offsetof(Vector,items) == 8) ? 1 : -1];

typedef struct ZRule {
    unsigned letter;
    unsigned action;
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
    int option_nologo; /* original 0x5b584 */
    int option_FAT; /* original 0x5fd0c */
    char *path_characters; /* original 0x6c010 */
    int option_pf; /* original 0x6bbd4 */
    int option_C; /* original 0x5b1dc */
    Vector *defines; /* original 0x5b57c */
    int option_E; /* original 0x5b008 */
    Vector *include_directories; /* original 0x5b578 */
    Vector *forced_includes; /* original 0x60494 */
    int option_EP; /* original 0x6bbd0 */
    char *option_Fi; /* original 0x5d5ac */
    char *source_file; /* original 0x5f4bc */
    int option_J; /* original 0x6bb64 */
    int japanese_identifiers; /* original 0x6bb60 */
    int option_Bu; /* original 0x6bbdc */
    int option_Brepro; /* original 0x6bbe0 */
    int option_ide; /* original 0x6bbe4 */
    int option_MT; /* original 0x6bb88 */
    int option_MD; /* original 0x6bb80 */
    int option_M; /* original 0x6bb84 */
    int option_WX; /* original 0x5b22c */
    int option_W; /* original 0x6bf9c */
    int option_Zp; /* original 0x6c028 */
    int option_Oi; /* original 0x6bbe8 */
    int option_Ol; /* original 0x6bbec */
    int option_Oq; /* original 0x6bbf0 */
    int option_Gn; /* original 0x6bbc0 */
    char *error_file; /* original 0x6c01c */
    char *intermediate_prefix; /* original 0x6c024 */
    int option_xc; /* original 0x60490 */
    int option_m; /* original 0x5b00c */
    char *option_V; /* original 0x5f504 */
    int option_Gs; /* original 0x6bbf4 */
    int option_Gc; /* original 0x6bb8c */
    int option_Gr; /* original 0x6bb58 */
    int option_Gd; /* original 0x6bc00 */
    int option_Gz; /* original 0x6bc04 */
    char *option_NV; /* original 0x6bf68 */
    char *option_Fr; /* original 0x5b59c */
    char *option_FR; /* original 0x5d5b8 */
    char *option_A; /* original 0x6048c */
    char *option_Q; /* original 0x60488 */
    char *database_file; /* original 0x6bf88 */
    char *option_Fo; /* original 0x6bf78 */
    int option_Fs; /* original 0x5b234 */
    int option_PLMN; /* original 0x6bb94 */
    int option_PLMF; /* original 0x6bb90 */
    int option_GA; /* original 0x6bba0 */
    int option_GD; /* original 0x6bba4 */
    int option_Gw; /* original 0x6bbb4 */
    int option_Gq; /* original 0x6bbac */
    int option_GW; /* original 0x6bbb0 */
    int option_GEf; /* original 0x6bba8 */
    int option_Gf; /* original 0x60484 */
    int option_G1; /* original 0x6bc0c */
    int option_G2; /* original 0x6bc10 */
    int option_G3; /* original 0x6bc14 */
    int option_G4; /* original 0x6bc18 */
    int option_qc; /* original 0x6bc1c */
    int option_Oa; /* original 0x6bc2c */
    int option_Ow; /* original 0x6bc30 */
    int option_Os; /* original 0x6c030 */
    int option_Oc; /* original 0x6c03c */
    int option_Og; /* original 0x6c040 */
    int option_Oe; /* original 0x6c044 */
    int option_Op; /* original 0x6bc20 */
    int option_Oz; /* original 0x6bc34 */
    int option_Or; /* original 0x6c050 */
    int option_Oo; /* original 0x6c054 */
    int option_Ov; /* original 0x6c058 */
    int option_Of; /* original 0x6c05c */
    int option_Oy; /* original 0x6bc28 */
    int option_Ob; /* original 0x6bc24 */
    int option_NoComdat; /* original 0x6bb9c */
    char *pch_use_file; /* original 0x6be4c */
    int pch_use; /* original 0x6be54 */
    char *pch_create_file; /* original 0x6be50 */
    int pch_create; /* original 0x6be58 */
    int option_Yd; /* original 0x6bbb8 */
    char *option_Fp; /* original 0x6be68 */
    char *pch_auto_file; /* original 0x6be5c */
    int pch_auto; /* original 0x6be60 */
    char *pch_link_name; /* original 0x6be64 */
    char *unrecognized_argument; /* original 0x6c104 */
    int extensions_enabled; /* original 0x6bb54 */
    int z_E; /* original 0x6bb50 */
    int debug_info; /* original 0x6bb98 */
    int debug_old_format; /* original 0x6c108 */
    int z_n; /* original 0x6049c */
    int z_f; /* original 0x5f4c0 */
    int prototypes_only; /* original 0x5b244 */
    int z_I; /* original 0x60498 */
    int z_r; /* original 0x5f4c8 */
    int line_records; /* original 0x6bc48 */
} CompilerOptions;

extern CompilerOptions options;
extern OptionRule option_rules[];
extern unsigned char warning_levels[800];
extern unsigned lowest_warning_700, highest_warning_700;
extern char database_path[256], pch_object_name[256];
extern char **argument_vector_allocation;
typedef struct PackingEntry {
    struct PackingEntry *next;
    int alignment;
    char *label;
} PackingEntry;
typedef struct PackingStack {
    PackingEntry *active, *free;
} PackingStack;
void *__stdcall allocate_permanent(unsigned bytes);
unsigned __stdcall smaller_size(unsigned left, unsigned right);
void *__stdcall copy_permanent_bytes(void *source, unsigned bytes, unsigned capacity);
void *__stdcall checked_malloc(unsigned bytes);
typedef struct BitSet {
    unsigned bit_count;
    unsigned char bits[1];
} BitSet;
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
BitSet *__stdcall new_bitset(unsigned bit_count);
BitSet *__stdcall clone_bitset(BitSet *source);
void __stdcall subtract_bitset(BitSet *target, BitSet *source);
void __stdcall free_bitset(BitSet *set);
void *new_permanent_page(void);
void __stdcall release_arena(int arena);
extern int permanent_page_count;
extern PackingStack *packing_stack;
PackingStack *new_packing_stack(void);
PackingEntry *push_packing(PackingStack *stack);
void pop_packing(PackingStack *stack);
unsigned __stdcall packing_padding(unsigned alignment, unsigned offset);
void clear_packing(PackingEntry *entry);

void initialize_packing(void);
void initialize_checksums(void);
void initialize_lead_bytes(void);
void mark_lead_byte_ranges(unsigned char *ranges);
void read_env_arguments(char *text);
char **extend_argument_vector(void);
void disable_multibyte_identifier_bytes(void);
Vector *__stdcall new_vector(short capacity, int arena);
char *next_argument(void);
int match_option(OptionRule *rules, char *argument, char *(*next)(void), int copy);
unsigned fatal_error(unsigned code, ...);
void finalize_options(void);
void compiler_warning(unsigned category, unsigned code, ...);
int effective_warning_level(unsigned level, unsigned code);
extern int option_Au, option_Aw;
extern char *current_option_argument;
int prefix_character(char *characters, char c);
int match_option_suffix(char *pattern, char *argument, unsigned action);
void apply_option(OptionRule *rule, char *value, int copy);
int parse_option_number(char *value);
void __stdcall append_vector(Vector *vector, void *item);
Vector *__stdcall grow_vector(Vector *vector);
Vector *__stdcall clone_vector(Vector *source, int arena);
Vector *__stdcall clone_vector_with_extra(Vector *source, short extra, int arena);
Vector *__stdcall clone_vector_with_slot(Vector *source, int arena);
int __stdcall vector_contains(Vector *vector, void *item);
void *__stdcall first_vector_item(Vector *vector);
int __stdcall vectors_equal(Vector *left, Vector *right);
int __stdcall valid_vector(Vector *vector);
Vector *__stdcall new_empty_vector(int arena);
void __stdcall store_vector_iterator(VectorIterator *iterator, void *item);
int __stdcall valid_vector_iterator(VectorIterator *iterator);
char *__stdcall join_truncated_names(char *prefix, char *suffix);
unsigned __stdcall vector_storage_bytes(Vector *vector);
#endif
