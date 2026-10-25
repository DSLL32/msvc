#ifndef C1XX_FRONTEND_H
#define C1XX_FRONTEND_H
#include "c1xx.h"
#include "output.h"
#include "options.h"
#include "scanner.h"
#include "frontend-driver.h"
#include "frontend-types.h"
#include "names.h"
#include <recon.h>

enum { FRONTEND_STATE_BYTES = 8520, FRONTEND_INITIALIZERS = 43 };

/* Only encountered fields are modeled. Unmodeled original snapshot bytes
 * have no native representation and must not reach a semantic consumer. */
typedef struct TypeRankMapping {
    int code;
    unsigned char rank;
} TypeRankMapping;

typedef struct FrontendData {
    int first_invocation;
    TypeRankMapping primitive_type_ranks[8];
    TypeRankMapping complex_type_ranks[9];
    int compiler_phase;
    int option_c9il;
    unsigned compiler_flags;
    SYSTEM_INFO system_info;
    int option_zb;
    int option_bm;
    int option_zm_percent;
    CompilerOptionData options;
    int language_context_depth;
    CompilerScannerTables scanner;
    /* Original datafe RVA cd514: signed nesting counter, default zero. */
    int recursion_depth;
    struct CompilerBackend *backend; /* datafe cc3e0, initially NULL */
    struct CompilerTranslationUnit *translation_unit; /* cc528, NULL */
    unsigned pch_option_flags; /* cd130, zero */
    int saved_pch_mode; /* cd4dc, zero */
    struct CompilerDependency *dependency; /* cd5bc, NULL */
    int report_statistics; /* cd7b0, zero */
    CompilerBuiltinTypes builtin_types; /* cc4a8..cc524, explicit type pointers */
    int normalized_stack_checks; /* cd150, default1 */
    int normalized_oq; /* cd478, default0 */
    int normalized_pool_strings, normalized_readonly_strings; /* cd10c/cd114 */
    int browser_enabled, browser_full; /* cd128/cd12c */
    int pch_separate_debug; /* cd4e8 */
    int current_vtordisp; /* ccb80, zero */
    CompilerNameEntry *bool_identifier; /* cc698, NULL; interned builtin "bool" */
    int browser_mode; /* initialized cc3d8, zero */
    struct CompilerBackend *named_browser, *initial_backend; /* cc3e4/cc3e8, NULL */
    struct CompilerTokenRecord *canonical_token_records[244]; /* cc704..ccad4, NULL */
    struct CompilerKeywordTable *keyword_table; /* cc430, NULL; pointee not yet read */
    CompilerNameEntry *current_identifier; /* cc450, shared generic/keyword header */
    struct CompilerPointerArray *frontend_pointer_table; /* cc5b4, NULL */
    struct CompilerPointerArray *empty_pointer_array; /* cc6b0, NULL; temporary guard1 */
    int pch_expression_file; /* cd4e0, zero */
    unsigned constant_arena_index; /* cd88c, original initialized5 */
    int force_type_serialization; /* cd550, zero */
    int type_debug_mode; /* cd54c, default1 */
    int reference_context; /* cdc1c, default1 */
    unsigned last_ir_line_delta; /* cd6f0, zero */
    CompilerTypeTables type_tables; /* four original initialized NULL heads */
    CompilerBuiltinNames builtin_names; /* own named initializedNULL slots; boolaliasescc698 above */
    unsigned namespace_symbol_bytes; /* cd674, actual initialized20 */
} FrontendData;

/* A linked context stack saves the first two values in each stack frame. */
typedef struct FrontendContextStack {
    void *origin;
    unsigned position;
    void *top;
#ifdef __cplusplus
    void RECON_THISCALL Push(void *, unsigned, int, int, FrontendContextStack *);
#endif
} FrontendContextStack;

typedef struct FrontendSourcePosition {
    void *origin;
    unsigned position;
} FrontendSourcePosition;

/* Frame contents remain unknown until a semantic consumer is reached. */
typedef struct DeferredWorkFrame DeferredWorkFrame;

/* Encountered semantic variants only; the original 32-byte tail is unmodeled. */
typedef union TokenValue {
    __int64 integer;
    void *pointer;
} TokenValue;

typedef struct SentinelToken {
    unsigned char kind;
    TokenValue value;
} SentinelToken;

struct CompilerArena;
struct PchImage;
struct PendingGroup;
struct DeferredAction;

/* Genuine thiscall POD: no implicit constructors, vtable or unknown fields. */
typedef struct CompilerPool {
    unsigned char *cursor;
    unsigned remaining;
    unsigned minimum_pages;
    struct CompilerArena *arena;
#ifdef __cplusplus
    CompilerPool *RECON_THISCALL Initialize(unsigned minimum_pages);
    void RECON_THISCALL BindArena(unsigned arena_index);
    unsigned char *RECON_THISCALL Refill(unsigned request_bytes);
#endif
} CompilerPool;

typedef struct PchContext {
    int image_count;
    struct PchImage *root_image;
    struct PchImage **images;
    DWORD stored_file_position;
    const char *file_name;
    LONG file_offset;
    HANDLE file;
    HANDLE mapping_handles[6];
    int loaded_image_count;
    struct CompilerArena *scratch_arena;
    struct PchImage **original_pages;
    struct PchImage **mapped_pages;
#ifdef __cplusplus
    PchContext *RECON_THISCALL Initialize(void);
#endif
} PchContext;

typedef struct CompilerWorkBuffer {
    BYTE *data;
    BYTE *limit;
} CompilerWorkBuffer;

/* These nine descriptors are encountered by the arena-initialization loop.
 * The reset member proves only zero for reset_value; later uses stay pending. */
typedef struct ArenaDescriptor {
    unsigned kind;
    int reserve_bytes;
    unsigned commit_unit;
    unsigned flags;
    unsigned alignment;
} ArenaDescriptor;

typedef struct CompilerArena {
    BYTE *cursor;
    BYTE *committed_limit;
    BYTE *base;
    unsigned committed_bytes;
    const ArenaDescriptor *descriptor;
    unsigned reserve_bytes;
    unsigned reset_value;
    int mapped_view;
#ifdef __cplusplus
    int RECON_THISCALL Reserve(const ArenaDescriptor *descriptor, BYTE *address);
    void RECON_THISCALL Reset(void);
    unsigned char *RECON_THISCALL AllocatePages(unsigned pages, unsigned *remaining_bytes);
    void RECON_THISCALL CommitExtent(void);
    BYTE *RECON_THISCALL Allocate(unsigned bytes);
#endif
} CompilerArena;

/* The initializer and 16-bit checksum consumer use all three byte tables. */
typedef struct CompilerChecksumTables {
    unsigned char update[256];
    unsigned char high[256];
    unsigned char low[256];
} CompilerChecksumTables;

/* Original c6b40 is the existing pending-group head, not a second object.
 * The tail slot at+4 is not modeled until the genuine Add member is reached. */
struct CompilerSymbol;
typedef struct PendingGroupList {
    struct PendingGroup *head;
#ifdef __cplusplus
    struct CompilerSymbol *RECON_THISCALL Add(struct CompilerSymbol *);
#endif
} PendingGroupList;

typedef struct FrontendBss {
    FrontendData *saved_state;
    int cleanup_count;
    Initializer cleanup_callbacks[32];
    FrontendContextStack context_stack;
    FrontendContextStack saved_context_stack;
    FrontendContextStack nested_context_stacks[9];
    FrontendSourcePosition source_position;
    OutputBuffer output_buffers[FRONTEND_OUTPUT_BUFFERS];
    DeferredWorkFrame *deferred_work;
    TokenValue token_value;
    SentinelToken sentinel_token;
    unsigned long source_signature;
    CompilerPool pools[10];
    int type_ranks_initialized;
    unsigned char primitive_type_rank[65];
    unsigned char complex_type_rank[69];
    unsigned char empty_metadata_prototype[20];
    PchContext pch_context;
    PendingGroupList pending_groups;
    struct PendingGroup *active_group;
    int com_initialized;
    /* The encountered 73-character value and NUL, not the original capacity. */
    char generated_comment[74];
    unsigned comment_value_offset;
    unsigned generated_comment_length;
    struct DeferredAction *deferred_action_head;
    struct DeferredAction *deferred_action_tail;
    int deferred_action_depth;
    int break_into_debugger;
    int command_argc;
    char **command_argv;
    CompilerWorkBuffer work_buffers[2];
    CompilerArena arenas[9];
    CompilerOptionBss options;
    int language_contexts[10];
    CompilerChecksumTables checksum_tables;
    /* Filename canonicalization: FFFF preserves a multibyte lead byte. */
    WORD filename_case_map[256];
    /* Borrowed option text retained for diagnostics while matching. */
    char *current_argument;
    /* Original bssfe RVA c0abc: diagnostics count. */
    int error_count;
    unsigned initial_option_flags; /* bssfe c5490 */
    int pch_input_ready; /* c0a14 */
    char *pch_input_file; /* c0a20 */
    int global_expression_scope_active; /* bf204 */
    CompilerPchSettings saved_options; /* known c4f68..c5384 settings */
    unsigned forced_source_index; /* c5384 */
    struct CompilerDebugModule *debug_module; /* c0b48 */
    struct CompilerInput *input; /* c0a0c */
    FILE *preprocessed_stream; /* c0ab4 */
    struct CompilerTextInput *text_input; /* c6498 */
    char *browser_filename; /* c082c */
    int pch_replayed, pch_file_failed; /* bf20c/c0970 */
    unsigned char output_kinds[6]; /* scattered BYTE destinations */
    FILE *preprocessor_output; /* c080c */
    CompilerLexerBackend *lexer_backend; /* ba918 */
    unsigned ir_record_count; /* c0aac, zero */
    unsigned ir_line_delta; /* c4c68, zero; current source line minus file origin */
    struct CompilerSymbol *current_ir_symbol; /* c06e0, NULL */
    struct CompilerSymbol *current_function; /* c0824, NULL */
    int expression_seek_result; /* c5388, zero */
    unsigned symbol_arena_kind; /* ba910, BSS0; global initialization selects4 */
    struct CompilerSymbolArenaFrame *saved_symbol_arenas; /* ba914, NULL */
    unsigned processor_instruction_mask; /* c0aec, opcode-table mask */
} FrontendBss;

#ifdef __cplusplus
extern "C" {
#endif
extern FrontendData frontend_data;
extern FrontendBss frontend_bss;
extern const ArenaDescriptor arena_descriptors[9];
extern int restore_mode;
extern Initializer frontend_initializers[FRONTEND_INITIALIZERS];
void *checked_malloc(unsigned bytes);
int report_compiler_error(int number);
int report_compiler_diagnostic(int number, ...);
int register_cleanup(Initializer cleanup);
void initialize_work_buffers(void);
BYTE *reserve_work_buffer(unsigned reserve_bytes, unsigned commit_bytes);
void scan_internal_switches(char **argv);
void initialize_frontend_hook(void);
int process_early_option(char *argument, int state);
char *read_command_argument(char *source, char *argument, unsigned capacity);
void initialize_compiler_environment(void);
void initialize_default_arena(void);
unsigned calculate_arena_reservation(const ArenaDescriptor *descriptor);
void initialize_pool_arenas(void);
void process_compiler_options(int argc, char **argv);
int compile_translation_unit(void);
int run_compiler(void);
int compiler_exception_filter(EXCEPTION_POINTERS *exception);
void cleanup_main(void);
void initialize_deferred_work(void);
void destroy_deferred_work(void);
void initialize_generated_comment(void);
void destroy_generated_comment(void);
void initialize_frontend_24(void);
void initialize_frontend_25(void);
void initialize_frontend_26(void);
void initialize_frontend_27(void);
void initialize_frontend_28(void);
void initialize_frontend_29(void);
void initialize_frontend_30(void);
void initialize_frontend_31(void);
void initialize_frontend_32(void);
void initialize_frontend_33(void);
int initialize_type_ranks(void);
void initialize_frontend_37(void);
#ifdef __cplusplus
}
#endif
#endif
