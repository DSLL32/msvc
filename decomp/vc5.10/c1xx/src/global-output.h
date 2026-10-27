/* Source-only views refer to existing original owners, never new allocations. */
#ifndef C1XX_GLOBAL_OUTPUT_H
#define C1XX_GLOBAL_OUTPUT_H
#include "ir.h"
#include <recon.h>

struct CompilerGlobalLocation { unsigned first, second, third; };
struct CompilerGlobalOption {
    CompilerGlobalOption *next;
    unsigned kind;
    const BYTE *value;
};
/* The class-detail count and sorted link vector form this same8-byte
 * subobject. The lookup method receives its address, not the class header. */
struct CompilerMemberNameIndex {
    unsigned count;
    CompilerSymbolLink **items;
    CompilerSymbolLink *RECON_THISCALL Find(CompilerNameEntry *); /*1128b, RET4*/
};
struct InheritedMemberRecord {
    union {
        CompilerSymbolLink *member;
        CompilerVector *members; /* flags bit2 selects the vector form */
    };
    BYTE unmodeled_04_0b[8];
    BYTE flags;
    BYTE unmodeled_0d_0f[3];
};
struct ClassMemberFunctionTable {
    unsigned kind;
    CompilerRuntimeFunction *function;
    BYTE unmodeled_08_0f[8];
    int count;
    int capacity;
    InheritedMemberRecord *records;
    unsigned storage_bytes;
    BYTE unmodeled_20_2f[16];
    void *merge_value;
    CompilerVector *merge_targets;
    BYTE unmodeled_38_3b[4];
    CompilerRuntimeFunction *secondary_function;
    unsigned secondary_function_state;
};

/* Shared borrowed group view. The class-binding collector consumes only
 * its first symbol; the existing function grouping fields retain their roles. */
struct InheritedMemberGroup {
    CompilerSymbol *symbol;
    BYTE unmodeled_04_13[16];
    CompilerVector *functions[2];
    BYTE unmodeled_1c_1f[4];
    unsigned secondary_state;
};
/* Shared partial view of the actual class-detail owner. Only encountered
 * fields are named; opaque gaps retain storage without interpretation. This
 * is not its allocation extent, an initializer, or a whole-object contract. */
struct CompilerGlobalClassDetail {
    unsigned size, member_size; /* original+0/+4, size readers' two roles */
    CompilerVector *member_groups[2]; /* +8/+c, arena5 vector owners */
    CompilerVector *additional_members; /* +10 */
    BYTE unmodeled_14_1f[12];
    union {
        struct {
            unsigned member_count; /* +20, indexed member lookup owner */
            CompilerSymbolLink **member_index; /* +24, sorted borrowed member links */
        };
        CompilerMemberNameIndex member_names;
    };
    void *declaration_reference; /* +28 */
    WORD scope_number; /* +2c */
    BYTE unmodeled_2e_2f[2];
    DWORD member_list_state; /* +30, member-list initializer writes0 */
    unsigned debug_index; /* +34 */
    WORD nested_scope_number; /* +38 */
    BYTE member_phase_flags, context_flags; /* +3a/+3b */
    BYTE declaration_flags, inheritance_flags; /* +3c/+3d */
    BYTE completion_flags, template_flags; /* +3e/+3f */
    BYTE generated_member_flags; /* +40 */
    BYTE inherited_member_flags; /* +41, declaration checks bits1..4 */
    BYTE declared_member_flags; /* +42, special-member check consumes bit4 */
    BYTE accessible_member_flags, duplicate_member_flags; /* +43/+44 */
};
typedef char CompilerGlobalClassDetail_offsets[(
    offsetof(CompilerGlobalClassDetail, size) == 0 &&
    offsetof(CompilerGlobalClassDetail, member_size) == 4 &&
    offsetof(CompilerGlobalClassDetail, member_groups) == 8 &&
    offsetof(CompilerGlobalClassDetail, additional_members) == 0x10 &&
    offsetof(CompilerGlobalClassDetail, member_count) == 0x20 &&
    offsetof(CompilerGlobalClassDetail, member_index) == 0x24 &&
    offsetof(CompilerGlobalClassDetail, member_list_state) == 0x30 &&
    offsetof(CompilerGlobalClassDetail, scope_number) == 0x2c &&
    offsetof(CompilerGlobalClassDetail, debug_index) == 0x34 &&
    offsetof(CompilerGlobalClassDetail, context_flags) == 0x3b &&
    offsetof(CompilerGlobalClassDetail, inheritance_flags) == 0x3d &&
    offsetof(CompilerGlobalClassDetail, template_flags) == 0x3f) ? 1 : -1];
/* The original symbol owner has multiple tagged forms. This source-only
 * view spells the writer's accesses; it must not replace or enlarge an
 * accepted compact native allocation. Entry guards precede all its reads. */
struct CompilerGlobalSymbolView {
    BYTE kind, type_flags, definition_flags, scope_flags; /* original0..3 */
    BYTE attributes, qualifiers; /* original4/5 */
    WORD declaration_qualifiers; /* original6/7, source-only finalizer path */
    CompilerNameEntry *name; /* original8 */
    CompilerGlobalSymbolView *next; /* original+c */
    CompilerType *type; /* original+10 */
    unsigned references, record_index; /* +14/+18 */
    BYTE storage_class, symbol_flags, member_flags, reference_flags; /*1c..1f*/
    union { /* original+20, selected by the actual record kind */
        CompilerNameEntry *alternate_name;
        CompilerGlobalClassDetail *class_detail;
        unsigned symbol_value;
        int member_offset; /* same+20, constructor member-layout consumer */
    };
    union {
        int integer_value;
        struct CompilerHashTable *lookup_table; /* same+24, class-name lookup variant */
        BYTE parameter_kind;
        WORD declaration_scope_number;
        struct { BYTE scope_depth, depth, unmodeled_26_27[2]; } reference_position;
        struct { BYTE first, second, unmodeled_26_27[2]; } member_selection; /* kind2 inputs to node36 */
    }; /* same original+24 owner; label/reference variants consume BYTE+24/+25 */
    union {
        unsigned alternate_size; /* original+28 */
        CompilerSymbol *pending_initializer; /* same+28, translation-unit pending-symbol variant */
        CompilerExpression *reference_expression; /* same+28, kind22 expression consumer11df9 */
        struct { WORD alternate_low; BYTE builtin_operation, function_class; };
    };
    BYTE builtin_flags; /* original+2c, function variant */
    union { BYTE template_flags, linkage_flags; }; /* +2d, tagged variants */
    union { BYTE extended_flags, queue_flags; };   /* +2e, tagged variants */
    BYTE unmodeled_2f;
};
/* Existing saved_context_stack.origin (BA8F0) supplies this actual owner. */
struct CompilerSourceFile {
    CompilerNameEntry *name; /* original0, genuine identifier owner */
    BYTE kind; /* original4, constructor stores21 */
    BYTE unmodeled_05[3]; /* retained storage; no field/default interpretation */
    unsigned reference_flags; /* original+8; non-IL reference index in high29 bits */
    unsigned record_index; /* original+c */
    CompilerSourceFile *next; /* original+10 */
    unsigned RECON_THISCALL OutputIndex(void);
    unsigned RECON_THISCALL AssignReference(void); /*9b15e*/
};
typedef char CompilerSourceFile_size[(sizeof(CompilerSourceFile) == 20 &&
    offsetof(CompilerSourceFile, reference_flags) == 8 &&
    offsetof(CompilerSourceFile, record_index) == 12) ? 1 : -1];
typedef char CompilerConstructorMemberOffset[offsetof(CompilerGlobalSymbolView, member_offset) == 0x20 ? 1 : -1];
typedef char CompilerReferencePosition_offset[(
    offsetof(CompilerGlobalSymbolView, reference_position.depth) == 0x25) ? 1 : -1];
#endif
