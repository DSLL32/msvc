#ifndef C1_COMPILER_STATE_H
#define C1_COMPILER_STATE_H
#include <stdio.h>
#include "c1.h"
#include "command-line.h"
#include "scanner.h"
#include "frontend-state.h"
#include "preprocessor.h"
#ifdef __cplusplus
#include "declarator.h"
#endif
#include <recon.h>

/* Only recovered semantic fields live here. Other historical bytes are
 * UNMODELED; neither zero defaults nor byte equivalence are assumed. */
/* Original RVA5f400 has26 DWORD slots. Only kinds1/6/9/13 are known.
 * Gap storage supplies layout, never defaults, snapshot bytes or read access. */
typedef struct CompilerRecordSizeFields {
        unsigned unmodeled_00[1];
        unsigned declaration_symbol_bytes; /* slot1, RVA5f404, default52 */
        unsigned unmodeled_02[4];
        unsigned intrinsic_symbol_bytes; /* slot6, RVA5f418, default52 */
        unsigned unmodeled_07[2];
        unsigned global_symbol_bytes; /* slot9, RVA5f424, default52 */
        unsigned unmodeled_0a[3];
        unsigned abstract_symbol_bytes; /* slot13, RVA5f434, default52 */
        unsigned unmodeled_0e[12];
} CompilerRecordSizeFields;
typedef union CompilerRecordSizes {
    CompilerRecordSizeFields known;
    unsigned indexed[26];
} CompilerRecordSizes;
typedef char CompilerRecordSizes_extent[sizeof(CompilerRecordSizes) == 104 ? 1 : -1];
typedef char CompilerRecordSizes_kind1[offsetof(CompilerRecordSizeFields, declaration_symbol_bytes) == 4 ? 1 : -1];
typedef char CompilerRecordSizes_kind6[offsetof(CompilerRecordSizeFields, intrinsic_symbol_bytes) == 24 ? 1 : -1];
typedef char CompilerRecordSizes_kind9[offsetof(CompilerRecordSizeFields, global_symbol_bytes) == 36 ? 1 : -1];
typedef char CompilerRecordSizes_kind13[offsetof(CompilerRecordSizeFields, abstract_symbol_bytes) == 52 ? 1 : -1];
/* Only head9 of the source's indexed cache is known. The open-ended array
 * spelling retains the original generic access, not an inferred table bound.
 * The preceding36 bytes supply storage identity only; they are never included
 * in defaults/snapshot transfer and native indexed reads are guarded. */
typedef struct CompilerSymbolCachePrefix {
    BYTE unmodeled_00[36];
    CompilerSymbolBinding *head9;
} CompilerSymbolCachePrefix;
typedef union CompilerSymbolCacheHeads {
    CompilerSymbolCachePrefix known;
    CompilerSymbolBinding *indexed[1];
} CompilerSymbolCacheHeads;
typedef char CompilerSymbolCacheHeads_head9[offsetof(CompilerSymbolCachePrefix, head9) == 36 ? 1 : -1];
typedef struct CompilerInitializedState {
    int first_call;
    int compiler_phase;
    int c9_il;
    DWORD compiler_flags;
    SYSTEM_INFO system_info;
    int internal_bm_flags;
    int internal_zb_value;
    int memory_percent;
    CompilerOptionData options;
    CompilerScannerTables scanner;
    /* Encountered frontend state in .datafe, initially zero. */
    struct CompilerBackend *backend;                 /* RVA5e3cc */
    struct CompilerTranslationUnit *translation_unit; /* RVA5e4f4 */
    unsigned pch_option_flags;                       /* RVA5ef98 */
    int dependency_local;                            /* RVA5efa4 */
    int saved_pch_mode;                              /* RVA5f32c */
    struct CompilerDependency *dependency;           /* RVA5f3d4 */
    int report_statistics;                          /* RVA5f500 */
    CompilerBuiltinTypes builtin_types;             /* Encountered type globals */
    int normalized_pool_strings;                    /* RVA5ef74 */
    int normalized_readonly_strings;                /* RVA5ef7c */
    int browser_enabled, browser_full;              /* RVA5ef90/5ef94 */
    int normalized_stack_checks;                    /* RVA5efa8; default1 */
    int normalized_oq;                              /* RVA5f2d0 */
    unsigned browser_mode;                          /* RVA5e3c4 */
    struct CompilerBackend *named_browser;            /* RVA5e3d0 */
    struct CompilerBackend *initial_backend;          /* RVA5e3d4 */
    int pch_separate_debug;                         /* RVA5f338 */
    CompilerTokenPrototype *predefined_tokens[245]; /* RVA5e5c4..5e998 */
    struct CompilerNameEntry **name_buckets;          /* RVA5e408; 2048 buckets */
    struct CompilerNameEntry *current_identifier; /* RVA5e420, defaultNULL */
    unsigned expression_arena_kind; /* RVA5f5dc, canonical default3 */
    unsigned ir_reference_epoch; /* RVA5f96c, default1; lowbit in symbol flags */
    int ir_symbol_state_ready;   /* RVA5f370, default1 */
    int ir_symbol_state_changed; /* RVA5f374, default0 */
    int last_line_delta; /* RVA5f4d8, default0 */
    struct CompilerTypeEntry **type_buckets; /* RVA5e9b0, defaultNULL; 2048 */
    struct CompilerTypeShapeEntry **type_shape_buckets; /* RVA5e40c, defaultNULL; 1024 */
    struct CompilerTypeListEntry **type_list_buckets; /* RVA5e404, defaultNULL; 1024 */
    CompilerBuiltinNames builtin_names; /* Encountered builtin identifier slots; defaultNULL */
    unsigned abstract_symbol_cache_index; /* RVA5f49c, default9 */
    CompilerTypeList *empty_type_list; /* RVA5e580, defaultNULL */
    unsigned type_arena_kind; /* RVA5f5e0, canonical default3 */
    unsigned intrinsic_symbol_cache_index; /* RVA5f480, default9 */
    unsigned next_symbol_record_id; /* RVA5e9c0, default16 */
    unsigned global_symbol_cache_index; /* RVA5f48c, default9 */
    CompilerIrSymbol *anonymous_global_symbol; /* RVA5e3ec, defaultNULL */
    CompilerIrSymbol *local_size_symbol; /* RVA5e3f0, defaultNULL */
    /* RVA5e398: mutable ctime-derived text, including original extra NUL.
     * Date/time macros borrow slices of the reformatted text. */
    char predefined_time_text[26];
    unsigned macro_reset_value; /* RVA5e3fc, default0; reset by macro setup */
    int source_include_depth; /* RVA5e224, default-1; increments/decrements with input stack */
    int emit_global_locations; /* RVA5f340, default0; global writer location mode */
    CompilerSourceFrame *source_frames; /* RVA5e218, defaultNULL */
    CompilerSourceFrame *root_source_frame; /* RVA5e21c, defaultNULL */
    CompilerSourceFrame *free_source_frames; /* RVA5e220, defaultNULL */
    int source_descriptor; /* RVA5e228, default-1 */
    int conditional_depth; /* RVA5f350, default-1 */
    struct CompilerParser *active_parser; /* RVA5e3b4, defaultNULL */
    unsigned debug_module_kind; /* RVA5e3e0, canonical default2; module query overwrites */
    unsigned lexer_flags; /* RVA5eddc, canonical DWORD default0; scanner mode bits. */
    unsigned declaration_symbol_cache_index; /* RVA5f46c, canonical default9 */
    void *function_context; /* RVA5e598, initializedNULL; optional definition context */
    unsigned local_symbol_limit; /* RVA5e9c4, default0x3fffff */
    unsigned function_reference_epoch; /* RVA5f36c, default1; distinct from5f96c */
    int disable_inline_records; /* RVA5f378, default0 */
    unsigned parser_flags; /* RVA5ede0, canonical DWORD0 */
    CompilerType *function_return_type; /* RVA5e46c, defaultNULL */
    WORD function_label_depth; /* RVA5f388, default0 */
    int function_label_state_first, function_label_state_second; /* RVA5f4dc/5f4e0, default-1; reset together */
    unsigned function_record_flags; /* RVA5ede4, DWORD default0 */
    int line_range_warning_enabled; /* RVA5f4e4, default1; one diagnostic per long-line interval */
    CompilerIrSymbol *label_symbols; /* RVA5f3dc, defaultNULL; list uses embedded binding links */
    int try_depth, finally_depth, exception_filter_depth; /* RVA5f39c/a0/a4, DWORD0 */
    struct CompilerFunctionDirectoryBlock *function_directory_head; /* RVA5e3f4, NULL */
    struct CompilerFunctionDirectoryBlock *function_directory_current; /* RVA5e3f8, NULL */
    unsigned function_directory_index; /* RVA5e9b8, default19 */
    unsigned function_debug_handle; /* RVA5f954, default0; debug-provider handle */
    unsigned function_cleanup_flags; /* RVA5f348, DWORD0; reset on function exit */
    unsigned debug_symbol_index; /* RVA5f984, DWORD0; function exit resets it */
    void *function_scope_cache; /* RVA5f3fc, NULL; lazy table is cleared on function exit */
    int source_exhausted; /* RVA5e214, DWORD0; lexer EOF latch */
    int final_input_token_emitted; /* RVA5e210, DWORD0; class0 EOF handoff occurs once */
    int omit_external_names; /* RVA5f4d0, DWORD0; scoped by global binding writer */
    unsigned pch_type_record_value; /* RVA5e9e0, DWORD0; PCH type-record chain value */
    const char *pch_record_name; /* RVA5f320, NULL; set by PCH stop processing */
    struct CompilerGlobalOption *global_options; /* RVA5e578, NULL; serialized option list */
    int pch_expression_stream_open; /* RVA5f330, DWORD0 */
    int pch_primary_stream_open, pch_secondary_stream_open; /* RVA5f5c8/5f5cc, DWORD0 */
    int pch_cleanup_done; /* RVA5f5d0, DWORD0; once-only cleanup marker */
    struct CompilerSavedContextRecord *saved_context_records; /* RVA5f4f4, NULL */
    int saved_context_count; /* RVA5f4f8, DWORD0 */
    /* From here, snapshot transfer selects only the known cells. */
    CompilerRecordSizes record_sizes;
    CompilerSymbolCacheHeads symbol_cache; /* base5e4f8; only head9/5e51c defaultNULL */
} CompilerInitializedState;
#define abstract_symbol_free symbol_cache.known.head9
/* A context saves its current object/value together with a linked stack of
 * prior contexts. The domain of the value is not yet recovered. */
struct CompilerContextFrame;
typedef struct CompilerContextState {
    void *object;
    unsigned value;
    struct CompilerContextFrame *saved_frame;
#ifdef __cplusplus
    const char *RECON_THISCALL FileName(void);
    struct CompilerParserToken *RECON_THISCALL ReadToken(void);
    void RECON_THISCALL Push(struct CompilerLexer *, unsigned, unsigned,
                            unsigned, const struct CompilerContextState *);
#endif
} CompilerContextState;
typedef void (__cdecl *C1BufferRelocator)(void *, unsigned char *, unsigned char *);
typedef struct CompilerOutputBuffer {
    unsigned char *data;
    unsigned char *cursor;
    int capacity; /* Grow uses signed division/comparison for its increment. */
    C1BufferRelocator relocated;
    void *relocation_context;
    FILE *stream;
    int good;
    /* Original+0x1c begins a260-byte filename, followed by fd at+0x120.
     * Constructor writes only filename[0]; BSS reset supplies static zeros. */
    char filename[MAX_PATH];
    int file_descriptor;
    BYTE unmodeled_124[8]; /* Later debug-interface state is not interpreted here. */
    char *owned_debug_name; /* Original+0x12c, BSSNULL; debug path joins two names. */
#ifdef __cplusplus
    /* Explicit method spelling keeps the containing BSS a trivial C owner. */
    void RECON_THISCALL Destroy(void);
    int RECON_THISCALL Close(void);
    int RECON_THISCALL CloseStream(void);
    int RECON_THISCALL Grow(int);
    void RECON_THISCALL Flush(void);
    void RECON_THISCALL FlushSource(void);
    int RECON_THISCALL Ensure(int);
    int RECON_THISCALL Reserve(int);
    void RECON_THISCALL WriteShort(short);
    void RECON_THISCALL WriteIndex(unsigned);
    int RECON_THISCALL WriteBytes(const void *, unsigned);
    void RECON_THISCALL MarkLocation(unsigned, unsigned);
    void RECON_THISCALL MarkGlobal(unsigned);
    int RECON_THISCALL WriteSmall(unsigned);
    void RECON_THISCALL WriteFunctionIndex(unsigned);
    void RECON_THISCALL AppendTo(struct CompilerOutputBuffer *);
    int RECON_THISCALL Open(const char *, const char *, const char *);
#endif
} CompilerOutputBuffer;
typedef char CompilerOutputBuffer_filename_offset[offsetof(CompilerOutputBuffer, filename) == 28 ? 1 : -1];
typedef char CompilerOutputBuffer_descriptor_offset[offsetof(CompilerOutputBuffer, file_descriptor) == 288 ? 1 : -1];
typedef char CompilerOutputBuffer_owned_name_offset[offsetof(CompilerOutputBuffer, owned_debug_name) == 300 ? 1 : -1];
typedef char CompilerOutputBuffer_owned_extent[sizeof(CompilerOutputBuffer) == 304 ? 1 : -1];
/* Numeric and declaration-prefix alternatives share the authentic value
 * owner. Declaration consumers currently use only flags/value/type; the final
 * twelve bytes retain no native semantic/default/equality claim. */
typedef union CompilerValue {
    __int64 integer;
    struct { CompilerType *type; unsigned unmodeled_04; __int64 value; } typed_constant;
    struct {
        unsigned flags, value;
        CompilerType *type;
        void *unmodeled_0c;
        DWORD unmodeled_10;
    } declaration;
    struct CompilerNameEntry *identifier;
    struct CompilerIrSymbol *symbol; /* Resolved declaration-name token. */
    struct CompilerTokenStream *recorded_stream;
    struct { const void *text; unsigned length; } string; /* Source-only literal view. */
    struct { void *object; unsigned value; } context; /* Source-only location alternative. */
    /* Authentic24-byte owner. Only the numeric8-byte alternative currently
     * has a native state contract; Run's declaration overlay is guarded. */
    BYTE unmodeled_00[24];
} CompilerValue;
/* Six encountered reservation descriptors. Alignment is also used by the
 * PCH serializer; flags select early initialization and reservation policy. */
typedef struct CompilerArenaDescriptor {
    unsigned kind;
    int reserve_bytes;
    unsigned commit_unit;
    DWORD flags;
    unsigned alignment;
} CompilerArenaDescriptor;
typedef struct CompilerArena {
    BYTE *cursor;
    BYTE *committed_limit;
    BYTE *base;
    unsigned committed_bytes;
    CompilerArenaDescriptor *descriptor;
    unsigned reserve_bytes;
    /* Reset writes zero here. Its broader role remains unrecovered. */
    unsigned reset_value;
    int mapped_view;
#ifdef __cplusplus
    int RECON_THISCALL Initialize(CompilerArenaDescriptor *, void *);
    void RECON_THISCALL Reset(void);
    unsigned char *RECON_THISCALL AllocatePages(unsigned, unsigned *);
    void RECON_THISCALL CommitExtent(void);
    void RECON_THISCALL ClearUsed(void);
    void RECON_THISCALL Release(void);
    BYTE *RECON_THISCALL AllocateBytes(unsigned);
#endif
} CompilerArena;
/* Genuine thiscall POD. Only four encountered fields are modeled; no
 * implicit construction/destruction, vtable, RTTI or exception machinery. */
typedef struct CompilerPool {
    unsigned char *cursor;
    unsigned remaining;
    unsigned minimum_pages;
    struct CompilerArena *arena;
#ifdef __cplusplus
    CompilerPool *RECON_THISCALL Initialize(unsigned);
    void RECON_THISCALL SetArena(unsigned);
    unsigned char *RECON_THISCALL Refill(unsigned);
    unsigned char *RECON_THISCALL AllocateBytes(unsigned);
#endif
} CompilerPool;
/* Pointer-relocation/file context: fields are established by its own load,
 * map and cleanup methods; pointed image/arena layouts remain separate. */
typedef struct CompilerPchState {
    int image_count;
    struct CompilerPchImage *root_image;
    struct CompilerPchImage **images;
    DWORD stored_file_position;
    const char *file_name;
    DWORD file_offset;
    HANDLE file;
    HANDLE mapping_handles[6];
    int loaded_image_count;
    struct CompilerArena *scratch_arena;
    struct CompilerPchImage **original_pages;
    struct CompilerPchImage **mapped_pages;
#ifdef __cplusplus
    CompilerPchState *RECON_THISCALL Initialize(void);
    void RECON_THISCALL Release(void);
#endif
} CompilerPchState;

typedef struct CompilerWorkBuffer {
    BYTE *data;
    BYTE *limit;
} CompilerWorkBuffer;

typedef struct CompilerChecksumTables {
    BYTE transform[256];
    BYTE high[256];
    BYTE low[256];
} CompilerChecksumTables;

typedef struct CompilerDeclarationControl {
    BYTE flags, unmodeled_01;
} CompilerDeclarationControl;
typedef struct CompilerBss {
    CompilerInitializedState *saved_data;
    int cleanup_count;
    C1Initializer cleanup_callbacks[32];
    CompilerContextState context;
    CompilerContextState current_context;
    CompilerContextState contexts[6];
    CompilerOutputBuffer output_buffers[8];
    CompilerValue literal;
    CompilerValue saved_literal;
    unsigned char saved_literal_available;
    unsigned long pdb_signature;
    CompilerPool pools[9];
    /* Fixed-width all-zero prototype copied into later records; their layouts
     * are separate. No five logical fields are inferred from scalar stores. */
    BYTE empty_record_prototype[20];
    CompilerPchState pch_state;
    int debug_break;
    int command_argc;
    char **command_argv;
    CompilerWorkBuffer work_buffers[2];
    CompilerArena arenas[6];
    CompilerOptionBss options;
    CompilerChecksumTables checksums;
    /* Byte-indexed case folding for path comparison. A lead byte is 0xffff;
     * ordinary entries hold the converted character and '/' maps to '\\'. */
    WORD path_fold[256];
    /* Borrowed current argument, retained for option-error diagnostics. */
    char *current_argument;
    /* Diagnostic total, shared by guarded compilation and its reporting path. */
    int error_count;
    unsigned initial_option_flags;                  /* RVA57f40 */
    int global_expression_scope_active;             /* RVA53d74 */
    unsigned forced_source_index;                   /* RVA57e34 */
    CompilerPchSettings saved_options;               /* RVA57a30 projection */
    struct CompilerInput *input;                    /* RVA54fb8 */
    int pch_input_ready;                             /* RVA54fc0 */
    const char *pch_input_file;                      /* RVA54fcc */
    struct CompilerDebugModule *debug_module;       /* RVA550cc */
    FILE *preprocessed_stream;                      /* RVA55060 */
    struct CompilerTextInput *text_input;           /* RVA58f48 */
    CompilerLexerBackend *lexer_backend;            /* RVA518e0 */
    char *browser_filename;                         /* RVA54ddc */
    int pch_replayed;                               /* RVA53d7c */
    int pch_file_failed;                            /* RVA54f20 */
    unsigned processor_instruction_mask;             /* RVA55098; opcode-table mask */
    FILE *preprocessor_output;                      /* RVA54dbc */
    /* Six output-token prefixes used by the non-IL path. */
    BYTE output_kinds[6];
    int expression_seek_status; /* RVA57e38, actual fseek/Close result; BSS */
    unsigned ir_record_count; /* RVA55058 */
    unsigned symbol_arena_kind; /* RVA518d8; symbols allocate from this arena. */
    struct CompilerSymbolArenaFrame *saved_symbol_arenas; /* RVA518dc; linked saved indices */
    CompilerIrSymbol *current_function; /* RVA54c90 */
    CompilerIrSymbol *current_scope_symbol; /* RVA54dd4 */
    int pending_line_delta; /* RVA5780c; current line minus saved line origin */
    CompilerSourcePosition declaration_position; /* RVA57810..57818; BSS */
    int declaration_nesting; /* RVA54db8; nested declarations select arena2 */
    BYTE declaration_scope_active; /* RVA54c7c, BSS; participates in symbol scope bits */
    char macro_date_time[26]; /* RVA550d0; ctime text, split into date/time strings */
    char *macro_date; /* RVA54fbc */
    char *macro_time; /* RVA54db4 */
    CompilerVector *pending_forced_includes; /* RVA54da8 */
    short forced_include_index; /* RVA54dac */
    BYTE *macro_token_cursor; /* RVA5003c; first work-buffer cursor */
    BYTE *macro_text_cursor; /* RVA50040; second work-buffer cursor */
    BYTE *saved_macro_text_cursor; /* RVA50034 */
    const char *current_macro_text; /* RVA50028; encoded command-line/directive text */
    BYTE *source_cursor; /* RVA54f30, BSS; current byte input position */
    char incremental_database_path[256]; /* RVA578a8, BSS; cached .idb path */
    CompilerParserValue parser_value; /* RVA53bb8; only firstDWORD modeled at runtime */
    WORD browser_context_value; /* RVA53be4 low WORD, BSS; upper WORD remains unmodeled */
    struct CompilerParserNode *case_label; /* RVA53ff8, BSS; reset on statement entry */
    struct CompilerPendingScopeSymbol *pending_scope_symbols; /* RVA54b30, BSSNULL */
    int function_stack_check_reported; /* RVA53d68, BSS; one-time diagnostic */
    int parser_early_result; /* RVA55068, BSS; parser early exit */
    struct CompilerParserNode *function_entry_label; /* RVA54fc8, BSS; label builder result */
    long function_expression_offset; /* RVA550fc, BSS; ftell of expression stream */
    long function_symbol_offset; /* RVA54f34, BSS; ftell of symbol stream */
    unsigned function_output_mode; /* RVA54c88, BSS; current consumers test only truth */
    unsigned parameter_symbol_count; /* RVA54fd0, BSS; writer consumes signed low WORD */
    unsigned lexical_skip_mode; /* RVA54da0, BSS; debugger function input */
    unsigned parameter_scope_value; /* RVA5401c, BSS; current parameter scope value */
    unsigned function_storage_class; /* RVA55088, BSS; captured parameter kind */
    unsigned function_reset_value; /* RVA54dd8, BSS; reset on function entry */
    unsigned nonil_type_index; /* RVA54c80, BSS; current context value at function entry */
    void *function_cleanup_context; /* RVA550c4, BSSNULL; function exit clears it */
    int token_input_finished; /* RVA54f24, BSS0; final token consumer writes1 */
    unsigned parameter_storage_class; /* RVA53d78, BSS0; function declarator copies parameter kind */
    unsigned parameter_scope_initial_value; /* RVA5509c, BSS0; function declarator copies parameter value */
    int type_description_depth; /* RVA54fc4, BSS initial0; nested scratch index */
    /* Prefix of original54a70. Only slot1 is encountered and initialized by
     * Normalize. Slot0 and the wider original stack remain unmodeled; native
     * code guards the index before access. Two is not the original capacity. */
    CompilerType type_descriptions[2];
    /* Actual declaration-entry writer clears context[1] on this path. Other
     * slots remain unsupported before reads, despite native reset storage. */
    void *declaration_contexts[60]; /* RVA54ca8, source indexed bound60 */
    int unevaluated_expression_depth; /* RVA53bd4, BSS0; parser context nesting */
    BYTE parameter_nesting; /* RVA5508c, BSS0; selects parameter allocation arena2 */
    int declaration_depth; /* RVA5416c, BSS initial0 */
    /* Only record0/1.flags are encountered. The real source has60 records;
     * native code must guard before any other record or unknown byte read. */
    CompilerDeclarationControl declaration_control[60];
    /* Original541f0+20*depth, checked depth<60; ends546a0 inside the
     * .bssfe reset owner. Only record0 is interpreted at this frontier.
     * C sees storage solely for the shared BSS reset; C++ uses real member
     * pointers (4B VC5,8B native i386), so its native record is24B. */
#ifdef __cplusplus
    CompilerParserDeclaration declaration_specs[60];
#elif defined(RECON_NATIVE_TEST)
    BYTE declaration_specs[60][24];
#else
    BYTE declaration_specs[60][20];
#endif
} CompilerBss;
#ifdef RECON_NATIVE_TEST
#define C1_DECLARATION_STORAGE_STRIDE 24
#else
#define C1_DECLARATION_STORAGE_STRIDE 20
#endif
typedef char CompilerBss_declaration_stride[
    sizeof(((CompilerBss *)0)->declaration_specs[0]) == C1_DECLARATION_STORAGE_STRIDE ? 1 : -1];
typedef char CompilerBss_declaration_boundary[
    offsetof(CompilerBss, declaration_specs) ==
    (offsetof(CompilerBss, declaration_control) + sizeof(((CompilerBss *)0)->declaration_control) + 3) / 4 * 4 ? 1 : -1];
/* VC5 rounds the enclosing __int64-bearing BSS owner to8-byte alignment.
 * Its possible final4-byte padding has no semantic claim. */
typedef char CompilerBss_reset_extent[
    sizeof(CompilerBss) >= offsetof(CompilerBss, declaration_specs) +
        sizeof(((CompilerBss *)0)->declaration_specs) &&
    sizeof(CompilerBss) - offsetof(CompilerBss, declaration_specs) -
        sizeof(((CompilerBss *)0)->declaration_specs) <= 4 ? 1 : -1];
#undef C1_DECLARATION_STORAGE_STRIDE
/* These session fields are outside the datafe snapshot and BSS reset.
 * Original offset0x10 is unmodeled; no provider object is dereferenced here. */
typedef struct CompilerPdbContext {
    struct PDB *database;
    struct NameMap *names;
    struct TPI *types;
    unsigned retain_database; /* Original+12, initialized0; predicate-only owner */
    BYTE unmodeled_10[4]; /* No initializer or consumer is claimed here. */
    struct CompilerMreState *rebuild_state; /* Original+20, initializedNULL */
    int read_mode; /* Original+24, initialized0; suppresses commit when nonzero */
#ifdef __cplusplus
    void RECON_THISCALL Finish(void);
    int RECON_THISCALL Open(const char *, unsigned *, unsigned *, unsigned, const char *);
#endif
} CompilerPdbContext;
typedef char CompilerPdbContext_extent[sizeof(CompilerPdbContext) == 28 ? 1 : -1];
typedef char CompilerPdbContext_rebuild[offsetof(CompilerPdbContext, rebuild_state) == 20 ? 1 : -1];
typedef char CompilerPdbContext_mode[offsetof(CompilerPdbContext, read_mode) == 24 ? 1 : -1];

#ifdef __cplusplus
extern "C" {
#endif
extern CompilerPdbContext c1_pdb_context;
extern CompilerPdbContext c1_output_pdb_context;
extern CompilerPdbContext *c1_current_pdb_context;
extern CompilerInitializedState c1_data;
extern CompilerBss c1_bss;
extern int c1_reuse_state;
extern C1Initializer const c1_initialization_callbacks[34];
void *__cdecl allocate_compiler_memory(unsigned);
void __cdecl compiler_fatal(int);
int __cdecl compiler_error(int, ...);
int __cdecl initialize_output_buffer_123ae(void);
int __cdecl initialize_output_buffer_12423(void);
void __cdecl destroy_output_buffer_23939(void);
void __cdecl destroy_output_buffer_23940(void);
int __cdecl register_compiler_cleanup(C1Initializer);
int __cdecl initialize_pdb_context_12676(void);
int __cdecl initialize_pdb_context_126b6(void);
void __cdecl destroy_pdb_context_238db(void);
void __cdecl destroy_pdb_context_238dc(void);
int __cdecl initialize_output_buffer_125fd(void);
void __cdecl destroy_output_buffer_238dd(void);
int __cdecl initialize_output_buffer_125ae(void);
void __cdecl destroy_output_buffer_23916(void);
int __cdecl initialize_output_buffer_1255f(void);
void __cdecl destroy_output_buffer_2391d(void);
int __cdecl initialize_output_buffer_12510(void);
void __cdecl destroy_output_buffer_23924(void);
int __cdecl initialize_output_buffer_124c1(void);
void __cdecl destroy_output_buffer_2392b(void);
int __cdecl initialize_output_buffer_12472(void);
void __cdecl destroy_output_buffer_23932(void);
void __cdecl initialize_state_12365(void);
void __cdecl initialize_state_12377(void);
void __cdecl initialize_state_12389(void);
void __cdecl initialize_state_123a4(void);
void __cdecl initialize_state_123a5(void);
void __cdecl initialize_state_123a6(void);
void __cdecl initialize_state_123a7(void);
void __cdecl initialize_state_1241c(void);
void __cdecl initialize_state_1246b(void);
void __cdecl initialize_state_124ba(void);
void __cdecl initialize_state_12509(void);
void __cdecl initialize_state_12558(void);
void __cdecl initialize_state_125a7(void);
void __cdecl initialize_state_125f6(void);
void __cdecl initialize_state_12645(void);
void __cdecl initialize_state_12646(void);
void __cdecl initialize_state_12663(void);
void __cdecl initialize_state_12664(void);
void __cdecl initialize_state_1266f(void);
void __cdecl initialize_state_126af(void);
void __cdecl initialize_state_126ef(void);
void __cdecl initialize_state_12712(void);
void __cdecl initialize_state_1271f(void);
void __cdecl initialize_state_1272c(void);
void __cdecl initialize_state_12739(void);
void __cdecl initialize_state_12746(void);
void __cdecl initialize_state_12753(void);
void __cdecl initialize_state_12760(void);
void __cdecl initialize_state_1276d(void);
void __cdecl initialize_state_1277a(void);
void __cdecl initialize_state_1277b(void);
void __cdecl initialize_state_1279c(void);
void __cdecl initialize_state_127a6(void);
void __cdecl initialize_work_buffers(void);
void *__cdecl reserve_work_buffer(unsigned, unsigned);
void __cdecl release_work_buffers_238da(void);
void __cdecl scan_internal_switches(char **);
unsigned __cdecl parse_internal_switch(const char *, unsigned);
const char *__cdecl next_internal_token(const char *, char *, unsigned);
void __cdecl initialize_state_12998(void);
void __cdecl initialize_compiler_arenas(void);
void __cdecl initialize_primary_arenas(void);
void __cdecl initialize_default_pools(void);
unsigned __cdecl arena_reservation_bytes(CompilerArenaDescriptor *);
extern CompilerArenaDescriptor c1_arena_descriptors[6];
void __cdecl parse_command_line(int, char **);
void __cdecl initialize_include_aliases(void);
void __cdecl initialize_checksums(void);
void __cdecl initialize_multibyte_scanner(void);
int __cdecl compile_translation_unit(void);
int __cdecl run_compiler(void);
int __cdecl compiler_exception_filter(EXCEPTION_POINTERS *exception);
#ifdef __cplusplus
}
#endif
#endif
