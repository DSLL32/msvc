#ifndef C1_FRONTEND_STATE_H
#define C1_FRONTEND_STATE_H
#include <stddef.h>
#include "c1.h"
#include <recon.h>

struct CompilerBackend;
#if defined(__cplusplus) && defined(_MSC_VER) && !defined(RECON_NATIVE_TEST)
/* The original callback is a4-byte member pointer; this class has no bases.
 * Tell VC5 before the declaration alternative uses the incomplete class. */
struct __single_inheritance CompilerTranslationUnit;
#else
struct CompilerTranslationUnit;
#endif
struct CompilerDependency;
struct CompilerInput;
struct CompilerDebugModule;
struct CompilerTextInput;
struct CompilerLexer;
struct CompilerTokenStream;
struct CompilerTypeShape;
struct CompilerNameEntry;
/* Encoded two-DWORD declaration position; component meanings remain pending. */
typedef struct CompilerSourcePosition { DWORD first, second; } CompilerSourcePosition;

typedef struct CompilerBuiltinNames {
    struct CompilerNameEntry *defined_operator, *formal_parameter, *ellipsis;
    struct CompilerNameEntry *abstract_declarator, *main_entry, *winmain_entry;
    struct CompilerNameEntry *dllmain_entry, *alignment_member, *unnamed;
    struct CompilerNameEntry *dollar, *directive_section, *local_static_guard;
    struct CompilerNameEntry *setjmp_name, *catch_name, *local_size, *line_variable;
} CompilerBuiltinNames;

typedef struct CompilerType {
    WORD flags, attributes;
    struct CompilerTypeShape *shape;
    union {
        unsigned auxiliary;
        struct CompilerIrSymbol *symbol_owner; /* Tagged nonprimitive alternative. */
    };
    /* Type interning compares only the first twelve bytes. Debug builds
     * initialize this cache separately; the scratch descriptor leaves it. */
    unsigned debug_index;
#ifdef __cplusplus
    unsigned RECON_THISCALL GetCallingConvention(void);
    struct CompilerType *RECON_THISCALL WithAttributes(unsigned);
    struct CompilerType *RECON_THISCALL FunctionResult(void);
    struct CompilerType *RECON_THISCALL DecayFunction(void);
    struct CompilerType *RECON_THISCALL DecayArray(void);
    struct CompilerType *RECON_THISCALL WithShapeFlags(unsigned);
    struct CompilerType *RECON_THISCALL WithFunctionConvention(unsigned);
#endif
} CompilerType;
typedef char CompilerType_size[sizeof(CompilerType) == 16 ? 1 : -1];

typedef struct CompilerTypeEntry {
    struct CompilerTypeEntry *next;
    CompilerType type;
} CompilerTypeEntry;
typedef char CompilerTypeEntry_size[sizeof(CompilerTypeEntry) == 20 ? 1 : -1];

typedef struct CompilerTypeList {
    int count;
    BYTE unmodeled_04[4]; /* Transferred internally, not part of the lookup key. */
    CompilerType *types[1]; /* Variable count, allocated separately. */
} CompilerTypeList;
typedef struct CompilerTypeListEntry {
    struct CompilerTypeListEntry *next;
    CompilerTypeList list;
} CompilerTypeListEntry;

/* Integer-node projection. Original fields are kind0/flags1/type4/value16.
 * The authentic24-byte allocation is separate from the known fields;
 * original bytes2..3 and8..15 remain unmodeled and unwritten. */
struct CompilerIrSymbol;
typedef struct CompilerIntegerConstant {
    BYTE kind, flags;
    BYTE unmodeled_02[2];
    CompilerType *type;
    BYTE unmodeled_08[8];
    union {
        __int64 integer;
        struct { unsigned low, high; } words;
        struct CompilerIrSymbol *symbol;
        BYTE *extended;
        unsigned index;
    };
} CompilerIntegerConstant;
typedef char CompilerIntegerConstant_payload[offsetof(CompilerIntegerConstant, integer) == 16 ? 1 : -1];
typedef char CompilerIntegerConstant_extent[sizeof(CompilerIntegerConstant) == 24 ? 1 : -1];

/* Views used by the IL writer. Original symbol fields occur at3,4,20,24,29,31;
 * original type-shape flags/storage/base occur at0,2,4. Omitted storage is
 * unmodeled, and these types do not establish original allocation extents. */
struct CompilerAbstractType;
typedef struct CompilerSymbolBinding {
    struct CompilerNameEntry *name;
    struct CompilerSymbolBinding *next;
} CompilerSymbolBinding;
typedef struct CompilerIrSymbol {
    BYTE kind;
    BYTE flags, scope_bits;
    BYTE declaration_flags;
    union { CompilerType *type; unsigned qualifier_flags; };
    CompilerSymbolBinding binding;
    union {
        struct CompilerAbstractType *abstract_type;
        CompilerType *parameter_type;
        CompilerType *function_type;
        CompilerType *value_type;
    };
    unsigned reference_flags;
    unsigned record_id;
    union {
        struct { BYTE intrinsic_low_flags, scope_flags, unmodeled_1e, identifier_flags; };
        unsigned intrinsic_flags;
        struct { BYTE storage_class, symbol_qualifiers, linkage_flags, function_flags; };
    };
    union {
        BYTE unmodeled_20[16];
        struct { BYTE unmodeled_label_20[4], label_nesting, label_exception_depth, unmodeled_label_26[10]; };
        unsigned symbol_value; /* Translation-state initializer clears DWORD+32. */
        struct { WORD intrinsic_opcode, intrinsic_traits; BYTE unmodeled_24[12]; };
        /* Source-only tagged views for global-record operands. Native symbol
         * consumers remain guarded; these aliases assign no new defaults. */
        struct {
            struct CompilerNameEntry *alternate_name;
            union {
                int integer_value;
                struct { BYTE parameter_kind, bit_width, unmodeled_26[2]; };
            };
            union { unsigned alternate_size; struct CompilerIrSymbol *initializer_symbol; };
            BYTE unmodeled_2c, unmodeled_2d_flags, unmodeled_2e[2];
        };
    };
    void *kind4_context; /* Constructor clears this pointer only for kind4. */
#ifdef __cplusplus
    CompilerIrSymbol *RECON_THISCALL Initialize(unsigned kind,
        struct CompilerNameEntry *name, unsigned arena);
    void RECON_THISCALL SetDeclarationContext(void *);
    int RECON_THISCALL IsNamed(void);
#endif
} CompilerIrSymbol;
struct CompilerTypeShape {
    WORD flags;
    union {
        WORD attributes;
        struct { BYTE storage, attribute_high; };
    };
    struct CompilerTypeShape *base;
    union {
        unsigned value;
        struct CompilerTypeList *parameters;
    };
#ifdef __cplusplus
    void RECON_THISCALL SetParameters(unsigned value, unsigned kind);
    CompilerTypeShape *RECON_THISCALL InternUntil(CompilerTypeShape *stop);
#endif
};

struct CompilerTypeShapeEntry {
    struct CompilerTypeShapeEntry *next;
    struct CompilerTypeShape shape;
};
typedef char CompilerTypeShapeEntry_size[sizeof(struct CompilerTypeShapeEntry) == 16 ? 1 : -1];

/* The cleanup owner establishes a20-byte row and only its+4 allocation.
 * No other field or row default is inferred from the empty current table. */
struct CompilerSavedContextRecord {
    BYTE unmodeled_00[4];
    void *allocation;
    BYTE unmodeled_08[12];
};
typedef char CompilerSavedContextRecord_size[sizeof(struct CompilerSavedContextRecord) == 20 ? 1 : -1];
struct CompilerFunctionDirectoryRecord {
    CompilerSymbolBinding *binding;
    long expression_position, symbol_position;
    unsigned debug_handle, ir_records, parameter_symbols, context_index, flags;
};
struct CompilerFunctionDirectoryBlock {
    struct CompilerFunctionDirectoryBlock *next;
    struct CompilerFunctionDirectoryRecord records[20];
};
typedef char FunctionDirectoryRecord_size[sizeof(struct CompilerFunctionDirectoryRecord) == 32 ? 1 : -1];
typedef char FunctionDirectoryBlock_size[sizeof(struct CompilerFunctionDirectoryBlock) == 644 ? 1 : -1];

typedef struct CompilerBuiltinTypes {
    CompilerType *type_0000, *type_4041_first, *type_0063, *type_4063_first;
    CompilerType *type_0041, *type_4041_second, *type_0042, *type_4042_first;
    CompilerType *type_0043, *type_4043, *type_004c, *type_404c, *type_0040;
    CompilerType *type_4063_second, *type_0042_second, *type_4042_second;
    CompilerType *type_0044, *type_4044, *type_0106, *type_0107, *type_0108;
    CompilerType *type_0802, *type_0080;
    CompilerType *type_0041_q0100, *type_4041_q0100, *type_0063_q0100;
    CompilerType *implicit_int_function, *tagged_0080;
} CompilerBuiltinTypes;

typedef struct CompilerLexerBackend {
    int enabled;
    /* A later declaration callback sets this before consuming it. */
    int attribute_mode;
    struct CompilerBackend *backend;
} CompilerLexerBackend;

/* Parser declaration attributes use only WORD0 and DWORD4. The intervening
 * WORD has no established writer or semantic meaning. */
typedef struct CompilerDeclarationAttributes {
    WORD flags;
    WORD unmodeled_02;
    unsigned extra;
} CompilerDeclarationAttributes;
#ifdef __cplusplus
/* Run's declaration callback is a real single-inheritance member pointer.
 * VC5 stores it at+16 in the20-byte declaration alternative. The native
 * representation is wider and is not transferred or invoked in the current
 * first-DWORD semantic domain. */
typedef void (RECON_THISCALL CompilerTranslationUnit::*CompilerDeclarationApply)(void *);
struct CompilerParserDeclaration {
    unsigned flags, value;
    CompilerType *type;
    void *context;
    CompilerDeclarationApply apply;
};
#ifndef RECON_NATIVE_TEST
typedef char CompilerParserDeclaration_size[sizeof(CompilerParserDeclaration) == 20 ? 1 : -1];
#endif
/* Source view used by expression reductions. This records only accesses in
 * Run, not allocation defaults or the meaning of bytes2..3/8..15. */
struct CompilerParserNode {
    BYTE kind, flags;
    union {
        BYTE unmodeled_02[2];
        struct { BYTE classification_flags, unmodeled_03; }; /* Source-only float bit1 */
    };
    CompilerType *type;
    BYTE unmodeled_08[8];
    union { const char *text; struct CompilerIrSymbol *symbol; CompilerParserNode *left; unsigned low; };
    union { CompilerParserNode *argument; CompilerParserNode *right; unsigned length, high; };
};
#endif
/* Original semantic values have24 bytes. Native transfers select only the
 * established fields for each encountered alternative: declaration state,
 * declarator pairs, or numeric type/value. Other bytes and alternatives
 * have no default, full-copy or equality claim. */
typedef union CompilerParserValue {
    void *pointer; /* Pragma text/scalar union, original first DWORD. */
    struct CompilerDeclarator *declarator;
    unsigned declaration_flags;
    struct { void *object; unsigned value; } context; /* Location token first8 bytes */
    struct CompilerTokenStream *recorded_stream; /* Kind0xf2 uses only this DWORD */
    struct CompilerIrSymbol *symbol;
    struct CompilerNameEntry *name;
    CompilerType *type;
    __int64 integer;
    struct { unsigned low, high; } words;
    BYTE operation;
    BYTE unmodeled_00[24];
#ifdef __cplusplus
    CompilerParserDeclaration declaration;
    CompilerDeclarationAttributes attributes;
    struct CompilerParameterDescription *parameters;
    CompilerParserNode *node;
    struct { struct CompilerDeclarator *declarator; struct CompilerIrSymbol *symbol; } declarator_symbol;
    struct { CompilerType *type; unsigned unmodeled_04; __int64 value; } typed_constant;
    struct { const char *text; unsigned length; } string;
#endif
} CompilerParserValue;
typedef char CompilerParserValue_extent[sizeof(CompilerParserValue) == 24 ? 1 : -1];
struct CompilerParser;
struct CompilerParserToken;

/* Known settings within the original 1028-byte PCH option snapshot. These
 * fields are a typed projection; 316 original bytes remain UNMODELED. */
typedef struct CompilerPchSettings {
    int preprocess_only, language_extensions, option_zbe, internal_zb_value;
    int fastcall_calls, japanese_identifiers, unsigned_char;
    int dll_runtime, multithread_runtime, pascal_calls, debug_info, pch_debug;
    int preprocess_no_lines, option_pf, option_bu, reproducible;
    int intrinsics, stack_checks, cdecl_calls, stdcall_calls;
    int pool_strings, readonly_strings, code_based_strings, processor;
    int minimal_rebuild, inline_level;
    unsigned option_flags;
    int dependency_output, dependency_local;
    int optimize_size, assume_no_alias, assume_no_global_alias, global_optimize;
    int precise_float, omit_frame_pointer, line_records;
    char database_path[256], pch_link_path[256];
    int option_oq, option_ov, c9_il, incremental_compile, debug_types;
    int function_records, split_pdbs, no_eh_for_new;
    int normalized_pool_strings, normalized_readonly_strings;
    int browser_enabled, browser_full, normalized_stack_checks, normalized_oq;
} CompilerPchSettings;
#endif
