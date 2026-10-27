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
/* Encountered cells of BYTE column63c78. Gaps are representation-only and
 * must not be indexed before their own semantic relation is established. */
typedef union CompilerExpressionClasses {
    struct {
        BYTE unmodeled_00[6], symbol;
        BYTE string; /* kind7, RVA63c7f ->7 */
        BYTE unmodeled_08[37], end_of_line; /* kind45, RVA63ca5 ->1 */
        BYTE unmodeled_2e[4], logical_and; /* kind50, RVA63caa ->50 */
        BYTE unmodeled_33[1], simple_assignment; /* kind52, RVA63cac ->52 */
        BYTE unmodeled_35[6], equality; /* kind59, RVA63cb3 ->110 */
        BYTE logical_not, indirection; /* kind60, RVA63cb4 ->60 */
        BYTE greater, unmodeled_3f[2], subscript; /* kind65, RVA63cb9 ->65 */
        BYTE unmodeled_42[1], shift_left, less; /* kinds67/68 ->128/124 */
        BYTE unmodeled_45[1], subtraction, unmodeled_47[3], multiplication;
        BYTE unmodeled_4b[4], logical_or, unmodeled_50[1], addition, unmodeled_52[1], member_address; /* kind79 ->79; kind83 ->131 */
        BYTE unmodeled_54[17], integer;
        BYTE unmodeled_66[10], call_argument;
        BYTE unmodeled_71[3], assignment; /* kind116, RVA63cec ->116 */
        BYTE unmodeled_75[1], left_parenthesis; /* kind118 ->118 */
        BYTE unmodeled_77[7], right_parenthesis; /* kind126 ->126 */
    } known;
    BYTE indexed[1];
} CompilerExpressionClasses;
#ifdef __cplusplus
typedef char ExpressionSubscriptClassOffset[offsetof(CompilerExpressionClasses, known.subscript) == 65 ? 1 : -1];
typedef char ExpressionShiftClassOffset[offsetof(CompilerExpressionClasses, known.shift_left) == 67 ? 1 : -1];
typedef char ExpressionSubtractClassOffset[offsetof(CompilerExpressionClasses, known.subtraction) == 70 ? 1 : -1];
typedef char ExpressionMultiplyClassOffset[offsetof(CompilerExpressionClasses, known.multiplication) == 74 ? 1 : -1];
typedef char ExpressionAdditionClassOffset[offsetof(CompilerExpressionClasses, known.addition) == 81 ? 1 : -1];
typedef char ExpressionMemberAddressOffset[offsetof(CompilerExpressionClasses, known.member_address) == 83 ? 1 : -1];
typedef char ExpressionRightParenClassOffset[offsetof(CompilerExpressionClasses, known.right_parenthesis) == 126 ? 1 : -1];
#endif
#ifdef __cplusplus
extern "C" {
#endif
extern CompilerExpressionClasses c1_expression_classes;
#ifdef __cplusplus
}
#endif
/* Selected spelling relations from pointer column638a0. Only the named
 * punctuation cells have semantic values; all gaps are unread representation.
 * This prefix view does not establish the original column's capacity. */
typedef union CompilerTokenSpellings {
    struct {
        const char *unmodeled_0[52], *assignment;
        const char *unmodeled_53[2], *comma;
        const char *unmodeled_56[5], *star;
        const char *unmodeled_62[12], *indirection;
        const char *unmodeled_75[6], *plus;
        const char *unmodeled_82[35], *left_brace, *left_parenthesis;
        const char *unmodeled_119[4], *right_brace;
        const char *unmodeled_124[2], *right_parenthesis, *semicolon;
    } known;
    const char *indexed[128];
} CompilerTokenSpellings;
#ifdef __cplusplus
typedef char TokenSpellingParenOffset[offsetof(CompilerTokenSpellings, known.left_parenthesis) == 118 * sizeof(char *) ? 1 : -1];
#endif
#ifdef __cplusplus
extern "C" {
#endif
extern const CompilerTokenSpellings c1_token_spellings;
#ifdef __cplusplus
}
#endif
#define C1_HAS_TOKEN_SPELLING(k) ((k) == 52 || (k) == 55 || (k) == 61 || (k) == 74 || (k) == 81 || \
    (k) == 117 || (k) == 118 || (k) == 123 || (k) == 126 || (k) == 127)
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
    struct CompilerType *RECON_THISCALL WithoutTopQualifiers(void);
    struct CompilerType *RECON_THISCALL WithAttributes(unsigned);
    struct CompilerType *RECON_THISCALL WithShape(struct CompilerTypeShape *);
    struct CompilerType *RECON_THISCALL FunctionResult(void);
    struct CompilerType *RECON_THISCALL DecayFunction(void);
    struct CompilerType *RECON_THISCALL DecayArray(void);
    struct CompilerType *RECON_THISCALL AddressType(unsigned);
    struct CompilerType *RECON_THISCALL WithShapeFlags(unsigned);
    struct CompilerType *RECON_THISCALL WithFunctionConvention(unsigned);
    struct CompilerType *RECON_THISCALL WithoutOuterShape(void);
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
    union { CompilerType *type; unsigned qualifier_flags;
            BYTE label_flags; /* Kind9 lowBYTE+4; assembly-definition bit0x10. */ };
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
        CompilerSymbolBinding *previous_parameter; /* Parameter Link stores the preceding embedded binding. */
        struct { WORD intrinsic_opcode, intrinsic_traits; BYTE unmodeled_24[12]; };
        /* Source-only tagged views for global-record operands. Native symbol
         * consumers remain guarded; these aliases assign no new defaults. */
        struct {
            union { struct CompilerNameEntry *alternate_name, *segment_class; };
            union {
                int integer_value;
                int enum_base_width;
                struct { union { BYTE parameter_kind, bit_offset, segment_kind; };
                         BYTE bit_width, unmodeled_26[2]; };
            };
            union { unsigned alternate_size; struct CompilerIrSymbol *initializer_symbol; };
            union {
                struct { BYTE unmodeled_2c, unmodeled_2d_flags, unmodeled_2e[2]; };
                struct { WORD tag_definition_flags; BYTE unmodeled_tag_2e[2]; };
            };
        };
    };
    void *kind4_context; /* Constructor clears this pointer only for kind4. */
#ifdef __cplusplus
    CompilerIrSymbol *RECON_THISCALL CopyFrom(CompilerIrSymbol *, unsigned);
    void RECON_THISCALL CopyInitializerFrom(CompilerIrSymbol *, unsigned);
    CompilerIrSymbol *RECON_THISCALL Initialize(unsigned kind,
        struct CompilerNameEntry *name, unsigned arena);
    void RECON_THISCALL SetDeclarationContext(void *);
    int RECON_THISCALL IsNamed(void);
    void RECON_THISCALL MarkAssemblyReference(void);
#endif
} CompilerIrSymbol;
typedef char IrSymbol_label_flags_offset[offsetof(CompilerIrSymbol, label_flags) == 4 ? 1 : -1];
/* Initializer frames at57588 have stride20 and a producer depth bound32.
 * Only encountered frames0..5 have semantic support; other rows and the
 * named storage gaps remain unmodeled even within the reserved span. */
enum { C1_INITIALIZER_LAST = 5 };
typedef struct CompilerInitializerFrame {
    CompilerIrSymbol *symbol;
    BYTE unmodeled_04[4];
    CompilerType *type;
    unsigned offset;
    BYTE flags, reset_byte;
    BYTE unmodeled_12[2];
} CompilerInitializerFrame;
typedef char InitializerFrame_extent[sizeof(CompilerInitializerFrame) == 20 ? 1 : -1];
typedef char InitializerFrame_flags[offsetof(CompilerInitializerFrame, flags) == 16 ? 1 : -1];
/* Aggregate-definition state at original53be8, stride24. The initializer
 * establishes these fixed-width cells; their wider interpretations wait for
 * the actual member-layout consumers. Only flags bits0/2 are currently used. */
typedef struct CompilerRecordDefinition {
    struct CompilerIrSymbol *symbol;
    DWORD initialized_04;
    CompilerType *type;
    DWORD byte_size;
    WORD bitfield_type_flags, alignment;
    BYTE initialized_14, flags;
    BYTE unmodeled_16[2];
} CompilerRecordDefinition;
typedef char CompilerRecordDefinition_extent[sizeof(CompilerRecordDefinition) == 24 ? 1 : -1];
typedef char CompilerRecordDefinition_flags[offsetof(CompilerRecordDefinition, flags) == 21 ? 1 : -1];
/* Actual twelve-byte member-list owner. The first pointer is initialized but
 * its role is not yet recovered; binding consumers establish first/last. */
typedef struct CompilerRecordMembers {
    union { void *unmodeled_00; unsigned arena_kind; };
    CompilerSymbolBinding *first, *last;
} CompilerRecordMembers;
typedef char CompilerRecordMembers_extent[sizeof(CompilerRecordMembers) == 12 ? 1 : -1];
/* Generic24-byte key/payload map, used by enum names and static symbols.
 * Its separately allocated bucket vector and12-byte linked entries retain
 * their actual owners. Only the two constructor flag bits are interpreted. */
typedef struct CompilerSymbolMapEntry {
    union { struct CompilerNameEntry *name; struct CompilerIrSymbol *source; void *key; };
    struct CompilerIrSymbol *symbol;
    struct CompilerSymbolMapEntry *next;
} CompilerSymbolMapEntry;
typedef char CompilerSymbolMapEntry_extent[sizeof(CompilerSymbolMapEntry) == 12 ? 1 : -1];
typedef struct CompilerSymbolMap {
    unsigned arena_kind;
    unsigned match_kind, hash_kind;
    WORD capacity;
    BYTE flags, padding_0f;
    CompilerSymbolMapEntry **symbols;
    unsigned count;
} CompilerSymbolMap;
typedef char CompilerSymbolMap_extent[sizeof(CompilerSymbolMap) == 24 ? 1 : -1];
typedef char CompilerSymbolMap_symbols[offsetof(CompilerSymbolMap, symbols) == 16 ? 1 : -1];
#ifdef __cplusplus
typedef int (RECON_THISCALL CompilerTypeShape::*CompilerTypeShapePredicate)(void);
#endif
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
        struct CompilerIrSymbol *symbol_owner; /* Source-only symbol-bearing shape. */
    };
#ifdef __cplusplus
    CompilerTypeShape *RECON_THISCALL FindMatching(CompilerTypeShapePredicate);
    int RECON_THISCALL IsNotArray(void);
    CompilerTypeShape *RECON_THISCALL Replace(CompilerTypeShape *, const CompilerTypeShape *);
    void RECON_THISCALL SetParameters(unsigned value, unsigned kind);
    CompilerTypeShape *RECON_THISCALL InternUntil(CompilerTypeShape *stop);
#endif
};

struct CompilerTypeShapeEntry {
    struct CompilerTypeShapeEntry *next;
    struct CompilerTypeShape shape;
};
typedef char CompilerTypeShapeEntry_size[sizeof(struct CompilerTypeShapeEntry) == 16 ? 1 : -1];

/* Diagnostic text handles address20-byte rows. Creation clears a new group
 * of64 rows, then reserves centered cursors in a zeroed allocation. */
struct CompilerSavedContextRecord {
    unsigned handle;
    void *allocation;
    unsigned prefix_offset, suffix_offset, capacity;
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

/* Declaration storage/qualifier flags. Only the encountered bits of the
 * extended WORD have meanings; setters retain all other bits. DWORD4 is also
 * the allocated-segment pointer when extended bit0x100 is present. */
/* The assembler reset owner establishes these scalar widths/zero writers.
 * Offset-named cells have no recovered meaning beyond that reset yet; later
 * parser readers must refine their roles before consuming them. */
/* Three actual16-byte operand records start at59fc8; their12-byte values
 * start at5a028 and end at the next reset cell5a04c. The statement producer
 * starts at0 and increments only below2. The reached IMUL path constructs
 * all three records with the same constructor and known fields;
 * wider indexes and unencountered transformations remain guarded.
 * The constructor clears12 value bytes. Only kind8/register10 are refined
 * by the reached register builder; the other semantic roles stay unmodeled. */
typedef struct CompilerAssemblyOperandValue {
    struct CompilerIrSymbol *symbol;
    union { unsigned unmodeled_04, displacement; };
    BYTE kind_flags, segment_and_scale, register_code;
    union { BYTE unmodeled_0b, index_register; };
} CompilerAssemblyOperandValue;
typedef union CompilerAssemblyValue {
    BYTE unmodeled[12];
    CompilerAssemblyOperandValue operand;
} CompilerAssemblyValue;
/* This builder writes only kind0 and register8. The other fields are not
 * initialized here; the payload4 alternative belongs to other builders. */
typedef struct CompilerAssemblyExpression {
    WORD kind;
    BYTE unmodeled_02[2];
    unsigned payload, register_code;
} CompilerAssemblyExpression;
typedef char AssemblyExpression_size[sizeof(CompilerAssemblyExpression) == 12 ? 1 : -1];
typedef char AssemblyExpression_register[offsetof(CompilerAssemblyExpression, register_code) == 8 ? 1 : -1];
typedef char AssemblyValue_kind[offsetof(CompilerAssemblyOperandValue, kind_flags) == 8 ? 1 : -1];
typedef char AssemblyValue_register[offsetof(CompilerAssemblyOperandValue, register_code) == 10 ? 1 : -1];
typedef char AssemblyValue_segment_scale[offsetof(CompilerAssemblyOperandValue, segment_and_scale) == 9 ? 1 : -1];
typedef struct CompilerAssemblyOperand {
    CompilerAssemblyValue *value;
    BYTE form, modifiers;
    BYTE unmodeled_06[2]; /* Constructor leaves these two bytes untouched. */
    union { DWORD reset_08; struct CompilerIrSymbol *source_symbol; };
    union { DWORD reset_0c; struct CompilerIrSymbol *symbol; };
} CompilerAssemblyOperand;
typedef char AssemblyOperand_size[sizeof(CompilerAssemblyOperand) == 16 ? 1 : -1];
typedef char AssemblyValue_size[sizeof(CompilerAssemblyValue) == 12 ? 1 : -1];
typedef char AssemblyOperand_reset_offset[offsetof(CompilerAssemblyOperand, reset_08) == 8 ? 1 : -1];
/* Counted record at54fd8: only the reached sixteen-byte prefix is modeled.
 * Its byte writers establish length/opcode/width plus thirteen operand bytes;
 * no total scratch-buffer capacity or untouched-tail default is claimed. */
typedef struct CompilerAssemblyRecord {
    BYTE length, opcode, width, operands[13];
} CompilerAssemblyRecord;
typedef char AssemblyRecord_prefix[sizeof(CompilerAssemblyRecord) == 16 ? 1 : -1];
typedef struct CompilerAssemblyResetState {
    unsigned brace_depth, reset_68, reset_04; /* RVA5a058/5a068/5a004; actual parser writes. */
    WORD reset_54, reset_5c; /* RVA5a054/5a05c */
    unsigned operand_index; /* RVA5a064, reset0; statement parser advances up to2. */
    unsigned reset_08, reset_60, reset_9fc4, reset_9ffc; /* RVA5a008/5a060/59fc4/59ffc */
    unsigned reset_4c; /* RVA5a04c, operand initializer clears it. */
    unsigned first_form; /* RVA59ff8, setter writes numeric instruction-form index. */
    struct CompilerAssemblyOperand *current_operand; /* RVA5a050, written from real initializer result. */
    CompilerAssemblyOperand operands[3]; /* RVA59fc8..59ff8; record2 remains unread. */
    CompilerAssemblyValue values[3]; /* RVA5a028..5a04c; record2 remains unread. */
    BYTE encoded_opcode, encoding_flags, implicit_encoding; /* BSS5a014/15/18; written by encoding selection. */
    CompilerAssemblyValue *output_values[3]; /* BSS5a01c..5a028, published by the authentic writer. */
    CompilerAssemblyOperand *output_operands[3]; /* BSS59fb8..59fc4, same three operand owners. */
} CompilerAssemblyResetState;

typedef struct CompilerDeclarationAttributes {
    WORD flags;
    WORD extended_flags;
    union { unsigned extra; struct CompilerIrSymbol *allocated_segment; };
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
    union { BYTE unmodeled_08[8]; struct { CompilerParserNode *argument_head; BYTE unmodeled_0c[4]; }; };
    union { const char *text; struct CompilerIrSymbol *symbol; CompilerParserNode *left; unsigned low; };
    union {
        CompilerParserNode *argument;
        CompilerParserNode *right;
        unsigned length, high;
        struct { WORD string_length, string_flags; }; /* String-node source view. */
    };
};
#endif
/* Original semantic values have24 bytes. Native transfers select only the
 * established fields for each encountered alternative: declaration state,
 * declarator pairs, or numeric type/value. Other bytes and alternatives
 * have no default, full-copy or equality claim. */
typedef struct CompilerStringValue {
    const char *text;
    WORD length, flags;
} CompilerStringValue;
typedef char CompilerStringValue_size[sizeof(CompilerStringValue) == 8 ? 1 : -1];
typedef char CompilerStringValue_flags[offsetof(CompilerStringValue, flags) == 6 ? 1 : -1];
/* Ten-byte x87 extended number followed by the compiler's precision tag.
 * This explicit representation is independent of host long-double padding. */
typedef struct InitializerExtendedReal {
    DWORD significand_low, significand_high;
    WORD exponent, precision;
} InitializerExtendedReal;
typedef char InitializerExtendedReal_size[sizeof(InitializerExtendedReal) == 12 ? 1 : -1];
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
    struct { BYTE unmodeled_00[8]; union { CompilerType *type; unsigned value; }; } assembly;
    BYTE unmodeled_00[24];
#ifdef __cplusplus
    CompilerParserDeclaration declaration;
    CompilerDeclarationAttributes attributes;
    struct { BYTE unmodeled_00[12]; struct CompilerIrSymbol *symbol;
             CompilerDeclarationApply apply; } tag_declaration;
    struct CompilerParameterDescription *parameters;
    CompilerParserNode *node;
    struct { struct CompilerDeclarator *declarator; struct CompilerIrSymbol *symbol; } declarator_symbol;
    struct { CompilerType *type; unsigned unmodeled_04; __int64 value; } typed_constant;
    struct { CompilerType *type; unsigned unmodeled_04; InitializerExtendedReal *value; } real_constant;
    struct { const char *text; unsigned length; } string;
    CompilerStringValue string_attributes; /* Quoted-token WORD fields at+4/+6. */
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
