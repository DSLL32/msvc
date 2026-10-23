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
#include <ctype.h>
#include <errno.h>
#include <stddef.h>
/* This target imports the FILE array; the SDK accessor adds a graph vertex. */
#undef _iob
__declspec(dllimport) extern FILE _iob[];
/* Its character macros also import data directly rather than SDK accessors. */
#undef _pctype
__declspec(dllimport) extern unsigned short *_pctype;
#undef MB_CUR_MAX
#define MB_CUR_MAX __mb_cur_max
__declspec(dllimport) extern int __mb_cur_max;
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
extern FILE *c2_input, *c2_globals_input, *c2_current_il;
extern int c2_il_extended_header, c2_il_source_table;
extern int c2_il_symbol_count, c2_il_initial_symbol_count;
extern int c2_il_target_size, c2_il_eight_byte_target;
int c2_read_il_byte(void);
unsigned short c2_read_il_word(void);
long c2_read_il_long(void);
int __fastcall c2_open_pdb(char *name, char *object_name);
void c2_reset_symbol_table(void);
typedef struct C2Symbol C2Symbol;
typedef struct C2ScopeHash C2ScopeHash;
extern C2ScopeHash global_scope;
void *c2_create_type_table(void);
void *__fastcall c2_allocate_node(signed char kind, int pool);
unsigned __fastcall c2_node_size(int kind);
void *__fastcall c2_pool_allocate(int pool, unsigned bytes);
extern unsigned long c2_node_bytes[4];
typedef struct C2PoolBlock {
    struct C2PoolBlock *next;
    int capacity;
    unsigned char data[1];
} C2PoolBlock;
typedef struct C2Pool {
    C2PoolBlock *blocks;
    unsigned char *cursor;
    int remaining;
} C2Pool;
/* Pool4 is used directly for retained source-file names, not node accounting. */
extern C2Pool c2_pools[5];
extern C2PoolBlock *c2_free_blocks;
extern void *c2_types;
typedef struct C2Real {
    unsigned long significand_low, significand_high;
    unsigned short exponent, kind;
} C2Real;
extern C2Real c2_real_zero, c2_real_one, c2_real_two;
void c2_real_from_int(int *input, C2Real *output);
void store_extended(void); /* Private ABI: ESI destination, incoming ST(0). */
void c2_initialize_constants(void);
#define C2_INTRINSIC_COUNT 344
typedef struct C2Intrinsic {
    unsigned short result_type, reserved_type;
    char *commands, *runtime_name;
    unsigned char flags, argument_signature;
    unsigned short reserved_flags;
} C2Intrinsic;
typedef struct C2IntrinsicInitializer {
    short number;
    unsigned short result_type;
    char *commands, *runtime_name;
    unsigned char flags, argument_signature;
} C2IntrinsicInitializer;
typedef char C2IntrinsicSizeCheck[sizeof(C2Intrinsic) == 16 ? 1 : -1];
typedef char C2IntrinsicInitializerSizeCheck[sizeof(C2IntrinsicInitializer) == 16 ? 1 : -1];
extern C2Intrinsic intrinsics[C2_INTRINSIC_COUNT];
void initialize_intrinsics(void);
extern unsigned selected_save_register_mask, call_clobbered_register_mask;
extern int stack_alignment, c2_cpu_generation;
void initialize_machine_registers(void);
typedef struct C2SectionContext C2SectionContext;
typedef struct C2SectionGroup C2SectionGroup;
typedef struct C2SymbolScope C2SymbolScope;

#pragma pack(2)
struct C2Symbol {
    C2Symbol *hash_next;
    signed char kind;
    unsigned char pending05[7];
    long parameter_offset, value;
    unsigned flags;
    char *name;
    unsigned char pending1c[8];
    union {
        C2SectionContext *context;
        C2SectionGroup *group;
        unsigned section_serial;
    } association;
    unsigned serial;
    long source_line;
    unsigned char source_kind, pending31;
};
struct C2ScopeHash {
    C2ScopeHash *previous;
    C2Symbol *buckets[1024];
};
typedef char C2ScopeHashSizeCheck[sizeof(C2ScopeHash) == 4100 ? 1 : -1];
typedef char C2ScopeHashBucketsCheck[offsetof(C2ScopeHash, buckets) == 4 ? 1 : -1];

typedef struct C2Section {
    C2Symbol symbol;
    struct C2Section *next;
    char *class_name;
    unsigned pending3a;
    unsigned alignment;
    unsigned char attribute, combine, section_kind;
    unsigned char reserved45, pending46[4];
    struct C2Section *parent;
    union { unsigned order; C2Symbol *owner; } link;
    unsigned char selection, associative;
    unsigned short pending54;
} C2Section;
struct C2SectionGroup { unsigned pending00, count, pending08; };
typedef struct C2FunctionSymbol {
    C2Symbol symbol;
    unsigned char pending32[4];
    unsigned attributes;
    unsigned char pending3a[5];
    signed char scope_line;
    unsigned char pending40[22];
    long input_order;
    unsigned source_file;
    unsigned char pending5e[8];
    struct C2FunctionSymbol *pending_next;
    unsigned pending6a;
} C2FunctionSymbol;
/* The decoder's common variable/function record. Its kind selects overlapping
 * fields: local lifetime at3c, an alias target at3e, or flags/scope at3e/3f. */
typedef struct C2InputSymbol {
    C2Symbol symbol;
    unsigned pending32, attributes;
    union {
        long frame_size;
        struct { unsigned short pending3a; signed char lifetime; unsigned char pending3d; } local;
    } storage;
    union {
        struct {
            unsigned char flags;
            signed char scope_line;
            long auxiliary;
            unsigned char pending44[18];
        } variable;
        struct { C2Symbol *target; unsigned char pending42[20]; } alias;
    } detail;
    long input_order;
    unsigned source_file, pending5e;
    unsigned short field62, field64;
    C2FunctionSymbol *pending_next;
    unsigned additional_flags;
} C2InputSymbol;
typedef struct C2BlockSymbol {
    C2Symbol symbol;
    unsigned char pending32[12];
    unsigned char flags;
    signed char scope_line;
} C2BlockSymbol;
typedef struct C2DebugLink {
    struct C2DebugLink *next;
    C2Symbol *symbol;
} C2DebugLink;
typedef struct C2Filename {
    unsigned serial;
    char *name;
    struct C2Filename *next;
} C2Filename;
typedef struct C2DebugScope {
    short line;
    unsigned short pending02;
    unsigned start, end;
    C2DebugLink *records;
    struct C2DebugScope *parent, *next;
} C2DebugScope;
typedef struct C2SymbolListScope {
    C2DebugLink *records;
    struct C2SymbolListScope *child, *parent;
} C2SymbolListScope;
struct C2SymbolScope {
    C2FunctionSymbol *symbol;
    unsigned char pending04[12];
    C2ScopeHash *symbols;
    unsigned char pending14[12];
    C2DebugScope *debug_scope;
};
#pragma pack()
typedef char C2FunctionSymbolSizeCheck[sizeof(C2FunctionSymbol) == 110 ? 1 : -1];
typedef char C2InputSymbolSizeCheck[sizeof(C2InputSymbol) == 110 ? 1 : -1];
typedef char C2InputSymbolDetailCheck[offsetof(C2InputSymbol, detail) == 0x3e ? 1 : -1];
typedef char C2InputSymbolExtraCheck[offsetof(C2InputSymbol, additional_flags) == 0x6a ? 1 : -1];
typedef char C2BlockSymbolSizeCheck[sizeof(C2BlockSymbol) == 64 ? 1 : -1];
typedef char C2FunctionSymbolOrderCheck[offsetof(C2FunctionSymbol, input_order) == 0x56 ? 1 : -1];
typedef char C2FunctionSymbolNextCheck[offsetof(C2FunctionSymbol, pending_next) == 0x66 ? 1 : -1];
typedef char C2DebugScopeSizeCheck[sizeof(C2DebugScope) == 24 ? 1 : -1];
typedef char C2FilenameSizeCheck[sizeof(C2Filename) == 12 ? 1 : -1];
typedef char C2SymbolScopeHashCheck[offsetof(C2SymbolScope, symbols) == 16 ? 1 : -1];
typedef char C2SymbolScopeDebugCheck[offsetof(C2SymbolScope, debug_scope) == 32 ? 1 : -1];
typedef char C2SectionSizeCheck[sizeof(C2Section) == 86 ? 1 : -1];
typedef char C2SectionAlignmentCheck[offsetof(C2Section, alignment) == 0x3e ? 1 : -1];
typedef char C2SectionOrderCheck[offsetof(C2Section, link) == 0x4e ? 1 : -1];
extern C2Section *code_section, *data_section, *const_section, *bss_section;
extern C2Section *debug_symbols_section, *debug_types_section, *tls_section;
extern char *code_section_name, *data_section_name, *const_section_name;
extern char *bss_section_name, *tls_section_name;
extern C2SectionContext *section_context;
extern C2Section *section_head, *section_tail;
extern int c2_codeview_types;
void initialize_sections(void);
C2Section *__fastcall create_section(char *name, char *class_name, unsigned serial);
C2Section *__fastcall initialize_section_defaults(C2Section *section);
extern unsigned default_section_alignment;
void __fastcall insert_symbol(C2Symbol *symbol, int local);
void __fastcall append_section(C2Section *section);
void read_global_symbols(void);
void __fastcall read_symbols(C2SymbolScope *scope);
int __fastcall function_retains_symbols(C2FunctionSymbol *symbol, int conditional);
C2Symbol *__fastcall read_symbol(int retain_locals);
long input_compact_signed(void);
short input_compact_short(void);
unsigned input_varuint(void);
unsigned short input_type(void);
unsigned input_extended_flags(void);
void __fastcall input_string(char *buffer, unsigned capacity);
void __fastcall copy_symbol_type(C2Symbol *symbol);
char *__fastcall intern_symbol_name(C2Symbol *symbol, char *name, int pool);
char *__fastcall arena_strdup(char *name, int pool);
void __fastcall uppercase_symbol_name(char *name);
extern C2ScopeHash *local_symbol_table;
C2Symbol *__fastcall lookup_global_symbol(unsigned serial);
void __fastcall clone_alias_symbol(C2Symbol *symbol);
void __fastcall read_qualified_symbol(C2Symbol *symbol, int pool);
void compiler_warning(int level, int code, ...);
void __fastcall read_module_directive(int kind);
int __fastcall register_source_file(char *name);
char *__fastcall lookup_source_file(char *name, unsigned *bytes);
C2Symbol *__fastcall lookup_symbol(C2ScopeHash *scope, unsigned serial);
C2Symbol *__fastcall put_symbol(C2ScopeHash *scope, C2Symbol *symbol);
void __fastcall queue_symbol_debug(C2Symbol *symbol);
extern int c2_internal_listing;
extern C2Symbol *current_symbol;
extern C2Section *default_storage_section;
unsigned next_symbol_serial(void);
int __fastcall is_constant_symbol(C2Symbol *symbol);
int __fastcall is_known_symbol(unsigned serial);
C2Section *__fastcall select_symbol_section(C2FunctionSymbol *function, C2Symbol *symbol);
void __fastcall update_symbol_storage(C2Symbol *symbol);
unsigned __fastcall reserve_symbol_storage(C2Symbol *symbol);
void __fastcall record_filename(C2Symbol *symbol);
extern C2Filename *filenames;
void read_initializers(void);
/* Root initializer records are 28 bytes. Leaf variants reuse the payload;
 * these names describe the root form encountered by the IL dispatcher. */
typedef struct C2InitRecord {
    int kind;
    struct C2InitRecord *next;
    struct C2InitRecord *list_next;
    long offset;
    unsigned symbol_serial;
    int emitted;
    C2Symbol *symbol;
} C2InitRecord;
typedef char C2InitRecordSizeCheck[sizeof(C2InitRecord) == 28 ? 1 : -1];
typedef char C2InitRecordOffsetCheck[offsetof(C2InitRecord, offset) == 12 ? 1 : -1];
typedef char C2InitRecordSerialCheck[offsetof(C2InitRecord, symbol_serial) == 16 ? 1 : -1];
C2InitRecord *__fastcall new_initializer(int kind);
void __fastcall read_initializer_values(C2InitRecord *record);
C2InitRecord *__fastcall queue_initializer(C2InitRecord *record);
void __fastcall emit_initializer(C2InitRecord *record);
void __fastcall free_initializer(C2InitRecord *record);
extern char symbol_name[2058];
void __fastcall read_initializer_records(FILE *input);
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
