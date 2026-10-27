#ifndef C1XX_FRONTEND_TYPES_H
#define C1XX_FRONTEND_TYPES_H
#include <stddef.h>
#include "c1xx.h"
#include <recon.h>
/* Two actual callers use this12-byte cursor with distinct record domains.
 * Initializer/advance own the first eight bytes; current is a borrowed owner. */
typedef struct PendingDeclarationCursor {
    BYTE unmodeled_00_07[8];
    void *current;
} PendingDeclarationCursor;
struct CompilerQuotedValue {
    BYTE *text;
    WORD length, flags;
};
/* The operator initializer replaces each nonzero WORD type code by the
 * returned type pointer in the same original DWORD. No integer/pointer
 * cross-state equality is claimed. Original row+c and padding stay unmodeled. */
typedef union CompilerOperatorOperand {
    WORD code;
    struct CompilerType *type;
} CompilerOperatorOperand;
typedef struct CompilerPredefinedOperator {
    BYTE token; /* original row+0 */
    CompilerOperatorOperand operands[2]; /* original row+4/+8 */
    struct CompilerNameEntry *identifier; /* original row+10 */
} CompilerPredefinedOperator;
struct CompilerTypeShape;
struct CompilerFunctionType;
struct CompilerInternedType;
struct CompilerMappedType;
struct CompilerCompositeType;
struct CompilerTypeSequence;
/* The head counts follow the actual hash masks in each lookup consumer.
 * Entry payloads stay opaque until those functions are reconstructed. */
typedef struct CompilerTypeHash { struct CompilerInternedType *buckets[2048]; } CompilerTypeHash;
typedef struct CompilerTypeMap { struct CompilerMappedType *buckets[1024]; } CompilerTypeMap;
typedef struct CompilerCompositeTypeHash { struct CompilerCompositeType *buckets[1024]; } CompilerCompositeTypeHash;
typedef struct CompilerTypeSequenceHash { struct CompilerTypeSequence *buckets[512]; } CompilerTypeSequenceHash;
typedef struct CompilerTypeTables {
    CompilerTypeHash *types; /* original ccb38 */
    CompilerTypeMap *mapped; /* cc434 */
    CompilerCompositeTypeHash *composite; /* cc42c */
    CompilerTypeSequenceHash *sequences; /* cc6bc */
} CompilerTypeTables;
typedef struct CompilerType {
#ifdef __cplusplus
    struct CompilerSymbol *RECON_THISCALL OwnerSymbol(void); /*36f41*/
#endif
    WORD flags, attributes;
    struct CompilerTypeShape *shape;
    WORD auxiliary;
    /* Original +a..b remain unmodeled natural padding, never a comparison
     * field. The conditional debug-info path explicitly zeroes +c. */
    unsigned debug_cache;
#ifdef __cplusplus
    unsigned RECON_THISCALL GetCallingConvention(void);
    unsigned RECON_THISCALL ConversionRank(void); /*1137e, NULL ECX is rank0*/
    CompilerType *RECON_THISCALL Substitute(CompilerType *);
    CompilerType *RECON_THISCALL WithCallingConvention(unsigned);
    CompilerType *RECON_THISCALL ApplyTypeQualifiers(WORD); /*1e083*/
    CompilerType *RECON_THISCALL StripOuterShape(void);
    CompilerType *RECON_THISCALL ConvertArray(unsigned);
    CompilerType *RECON_THISCALL RemoveFunction(void);
    CompilerType *RECON_THISCALL RemoveFunctionQualifier(void); /*40334*/
    CompilerType *RECON_THISCALL AddressType(unsigned); /*14e84 RET4*/
    CompilerType *RECON_THISCALL BindMemberScope(struct CompilerSymbol *); /*149c7 RET4*/
    CompilerType *RECON_THISCALL StripArrayDimensions(void);
    CompilerType *RECON_THISCALL StripQualifiers(void);
    CompilerType *RECON_THISCALL WithShape(CompilerTypeShape *); /*2335f*/
    int RECON_THISCALL Matches(CompilerType *, unsigned); /*35b3c*/
    CompilerType *RECON_THISCALL RemoveOuterShape(void); /*7b7b*/
#endif
} CompilerType;
typedef char CompilerType_size[sizeof(CompilerType) == 16 ? 1 : -1];
struct CompilerParameterSet;
struct CompilerParameterLink;
struct CompilerSymbol;
/* Interned shape header, distinct from the28-byte parser parameter object. */
struct CompilerTypeShape {
    WORD flags, attributes;            /* original +0/+2 */
    struct CompilerTypeShape *base;     /* +4 */
    union {
        CompilerType *type;
        struct CompilerSymbol *symbol;
        struct CompilerTypeList *parameters;
        struct CompilerFunctionType *function;
        unsigned value;
    } payload;                         /* +8, interpreted by shape flags */
#ifdef __cplusplus
    CompilerTypeShape *RECON_THISCALL Find(int (RECON_THISCALL CompilerTypeShape::*)(void));
    int RECON_THISCALL IsNotArray(void);
    CompilerTypeShape *RECON_THISCALL Replace(CompilerTypeShape *, CompilerTypeShape *);
    CompilerTypeShape *RECON_THISCALL InternChain(CompilerTypeShape *);
    void RECON_THISCALL SetType(CompilerType *);
    void RECON_THISCALL SetParameters(struct CompilerParameterSet *,
        struct CompilerParameterLink *, CompilerType *);
#endif
};
struct CompilerMappedType {
    struct CompilerMappedType *next;
    struct CompilerTypeShape shape;
};
typedef char CompilerMappedType_size[sizeof(struct CompilerMappedType) == 16 ? 1 : -1];
typedef struct CompilerInternedType {
    struct CompilerInternedType *next;
    CompilerType type;
} CompilerInternedType;
typedef char CompilerInternedType_size[sizeof(CompilerInternedType) == 20 ? 1 : -1];
typedef struct CompilerBuiltinTypes {
    CompilerType *type_0000, *type_4041_first, *type_0063, *type_4063_first; /* cc514/cc508/cc4c0/cc50c */
    CompilerType *type_0041, *type_4041_second, *type_0042, *type_4042_first; /* cc4c4/cc4c8/cc4cc/cc4d0 */
    CompilerType *type_0043, *type_4043, *type_004c, *type_404c; /* cc4d4/cc4d8/cc4dc/cc4e0 */
    CompilerType *type_0060, *type_0013; /* cc4ec/cc4f0 */
    CompilerType *type_4063_second, *type_0042_second, *type_4042_second; /* cc510/cc500/cc504 */
    CompilerType *type_0044, *type_4044, *type_0106, *type_0107, *type_0108; /* cc4f4/cc4f8/cc4b4/cc4b8/cc4bc */
    CompilerType *type_0802, *type_0080; /* cc4fc/cc518; both shape=NULL initially */
    CompilerType *type_0041_q0100, *type_4041_q0100, *type_0063_q0100; /* cc4a8/cc4b0/cc4ac */
    CompilerType *implicit_int_function, *tagged_0080; /* cc51c/cc520; latter has shape */
} CompilerBuiltinTypes;
typedef struct CompilerLexerBackend {
    int enabled;
    BYTE unmodeled_04_07[4]; /* original+4 is untouched by the initializer */
    struct CompilerBackend *backend; /* original+8, initial/saved backend */
} CompilerLexerBackend;
typedef char CompilerLexerBackend_extent[(sizeof(CompilerLexerBackend) == 12 && offsetof(CompilerLexerBackend, backend) == 8) ? 1 : -1];
/* The actual BEFD8 class-context rows have32-byte stride and a16-row
 * limit. The writer assigns only these fields; +c and+1e remain opaque. */
typedef struct DeclarationClassScope {
    struct CompilerGlobalSymbolView *symbol;
    void *reset_04;
    struct CompilerType *type;
    BYTE unmodeled_0c[4];
    unsigned reset_10;
    union { unsigned reset_14, member_bytes; }; /* same original+14 layout accumulator */
    WORD reset_18;
    union { WORD state, alignment; }; /* same original+1a, max capped alignment */
    BYTE reset_1c, access_flags;
    BYTE unmodeled_1e[2];
} DeclarationClassScope;
typedef char DeclarationClassScope_stride[(sizeof(DeclarationClassScope) == 32 && offsetof(DeclarationClassScope, access_flags) == 29) ? 1 : -1];
struct CompilerTextInput;
#ifdef __cplusplus
extern "C" {
#endif
extern const signed char compiler_primitive_type_bytes[16];
#ifdef __cplusplus
}
#endif
#endif
