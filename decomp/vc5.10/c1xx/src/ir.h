#ifndef C1XX_IR_H
#define C1XX_IR_H
#include "frontend-types.h"
#include "names.h"
#include <recon.h>

struct CompilerExpression;
struct CompilerString;
struct CompilerDeclarationMetadata;
struct CompilerFunctionContext;
struct CompilerClassBinding;
/* The recycled-list pointer addresses original +8 within the record. */
struct CompilerSymbolLink {
    CompilerNameEntry *name;
    CompilerSymbolLink *next;
    void RECON_THISCALL SetClassContext(CompilerClassBinding *); /*8653f, receiver is symbol+8*/
};
/* The common BYTE header applies to short namespace records too. The
 * natural alignment gap before the name is not a modeled field. */
struct CompilerSymbol {
    int RECON_THISCALL IsFunction(void); /*143d*/
    CompilerNameEntry *RECON_THISCALL Name(void); /*21427*/
    void RECON_THISCALL SetName(CompilerNameEntry *); /*374b7*/
    void RECON_THISCALL SetKind(BYTE); /*fbed*/
    CompilerSymbol *RECON_THISCALL CopyFrom(CompilerSymbol *, unsigned); /*4606e*/
    CompilerSymbol *RECON_THISCALL Alias(void); /*86549*/
    void RECON_THISCALL SetAlias(CompilerSymbol *); /*20e2c*/
    int RECON_THISCALL SuppressBrowser(void); /*86533*/
    short RECON_THISCALL DeclarationQualifier(void); /*8652e*/
    void RECON_THISCALL SetDeclarationQualifier(WORD); /*20e83*/
    void RECON_THISCALL SetDeclarationOwner(CompilerSymbol *); /*2b760*/
    void RECON_THISCALL MarkDefined(void); /*8653a*/
    void RECON_THISCALL MarkParameter(void); /*465c9*/
    BYTE kind;                         /* +0 */
    BYTE type_flags;                   /* +1, finalizer's low2 mode bits */
    BYTE definition_flags;             /* +2, namespace definition bit2 */
    BYTE scope_flags;                  /* +3, runtime helper sets bit0 */
    BYTE attributes;                   /* +4, encountered bit6 */
    BYTE qualifiers;                   /* +5, declaration qualifier bits */
    WORD declaration_qualifiers;       /* +6, index consumed by declaration lookup */
    CompilerSymbol *RECON_THISCALL Initialize(unsigned, CompilerNameEntry *, unsigned);
    void RECON_THISCALL MarkReferenced(void);
    void RECON_THISCALL SetDeclarationMetadata(CompilerDeclarationMetadata *);
    void RECON_THISCALL SetDeclarationScopeNumber(BYTE); /*25571, RET4*/
    void RECON_THISCALL SetFunctionContext(CompilerFunctionContext *);
};
struct CompilerNamedSymbol : CompilerSymbol {
    CompilerSymbolLink link;           /* original +8 name / +c next */
};
/* Distinct constructor variants: only the observed DWORD zero stores are
 * modeled. Their later interpretations remain behind pending consumers. */
struct CompilerSmallSymbol : CompilerNamedSymbol {
    union {
        DWORD initial_payload;         /* original +10, kinds16/17 */
        CompilerSymbolLink *target;     /* kind17 prepared-function alias */
        CompilerSymbolLink *variants;   /* same +10 cell, overload-chain head */
    };
};
struct CompilerClassBinding {
    unsigned reset_a;
    union { unsigned reset_b; CompilerSymbolLink *members; };
    union { unsigned reset_c; CompilerSymbolLink *last_member; };
};
/* Kind24 borrows this owner. Only the named pointer, SHORT index and DWORD
 * selection flag are consumed; gaps and allocation extent stay unmodeled. */
struct CompilerAliasTarget {
    CompilerSymbol *symbol;
    BYTE unmodeled_04_07[4];
    short owner_index;
    BYTE unmodeled_0a_0b[2];
    unsigned selected;
};
struct CompilerAliasSymbol : CompilerNamedSymbol { CompilerAliasTarget *target; };
/* Alternate member-group variant: the +10 payload is a borrowed link chain. */
typedef CompilerSmallSymbol CompilerMemberGroupSymbol;
/* Kind25's borrowed overload chain. The middle link word is not consumed. */
struct CompilerOverloadLink {
    CompilerSymbol *symbol;
    BYTE unmodeled_04_07[4];
    CompilerOverloadLink *next;
};
struct CompilerOverloadGroup { CompilerOverloadLink *head; };
struct CompilerOverloadSymbol : CompilerNamedSymbol { CompilerOverloadGroup *group; };
/* One-slot iterator over variants represented by a member link. */
struct CompilerMemberVariantIterator {
    CompilerSymbolLink *chain, *current, *source;
    RECON_FORCEINLINE CompilerMemberVariantIterator() : chain(NULL), current(NULL), source(NULL) {}
    RECON_THISCALL CompilerMemberVariantIterator(CompilerSymbolLink *); /*1da40*/
    void RECON_THISCALL SetLink(CompilerSymbolLink *); /*1db44*/
    int RECON_THISCALL Advance(void); /*1da7f*/
    int RECON_THISCALL IsPrimary(void); /*29364*/
    CompilerSymbol *RECON_THISCALL GetSymbol(void); /*2937d*/
    virtual int RECON_THISCALL Next(void); /*27797, c92f4 slot0*/
};
typedef char CompilerMemberVariantIterator_size[sizeof(CompilerMemberVariantIterator) == 16 ? 1 : -1];
struct CompilerClassMemberIterator {
    int valid;
    CompilerSymbolLink *chain;
    union { unsigned reset_value; CompilerSymbolLink *entry; }; /* +8: iterator result */
    CompilerSymbolLink *current, *owner;
    unsigned mode;
    CompilerClassMemberIterator *RECON_THISCALL Initialize(CompilerSymbolLink *, unsigned);
    CompilerClassMemberIterator *RECON_THISCALL InitializeHead(CompilerSymbolLink *); /*7add*/
    CompilerClassMemberIterator *RECON_THISCALL InitializeBinding(CompilerClassBinding *); /*29475*/
    int RECON_THISCALL Next(void); /*1d9ac*/
};
/* Kind15 owns24 bytes. Only the completion consumer's fields are named;
 * the final byte has no initialization or equality claim. */
struct CompilerClassMemberSymbol : CompilerNamedSymbol {
    CompilerSymbol *resolved;
    short owner_index;
    BYTE flags, unmodeled_17;
};
typedef char CompilerClassMembers_layout[(sizeof(CompilerClassBinding) == 12 &&
    sizeof(CompilerClassMemberIterator) == 24 &&
    sizeof(CompilerClassMemberSymbol) == 24 &&
    offsetof(CompilerClassMemberSymbol, resolved) == 0x10 &&
    offsetof(CompilerClassMemberSymbol, owner_index) == 0x14 &&
    offsetof(CompilerClassMemberSymbol, flags) == 0x16) ? 1 : -1];
struct CompilerKind4Symbol : CompilerNamedSymbol {
    BYTE unmodeled_10_23[20];
    union { DWORD initial_context; CompilerClassBinding *class_binding; }; /* same original+24 */
};
typedef char CompilerKind4Symbol_layout[(sizeof(CompilerKind4Symbol) == 40 &&
    offsetof(CompilerKind4Symbol, initial_context) == 0x24) ? 1 : -1];
struct CompilerAbstractSymbol : CompilerNamedSymbol {
    struct CompilerAbstractType *type; /* original +10; actual16-byte payload */
};
/* Extended members are accessed only for the larger IR symbol variants.
 * They must never enlarge the actual 20-byte namespace allocation. */
struct CompilerIrSymbol : CompilerNamedSymbol {
    union {
        struct CompilerSymbolPayload *payload;
        CompilerType *type;
    };                                 /* +10, one shared typed pointer cell */
    unsigned emission_state;           /* +14, low2 reference count, bit2 context */
    unsigned encoded_index;            /* +18 */
    union {
        unsigned declaration_state;   /* original whole DWORD+1c */
        struct {
            BYTE storage_class, function_flags, member_flags, reference_flags;
        };                            /* original +1c..1f */
    };
};
/* The function variant exposes only its encountered builtin classification. */
struct CompilerRuntimeFunction : CompilerIrSymbol {
    unsigned member_offset; /* original +20 */
    BYTE unmodeled_24_25[2];
    WORD member_scope_number; /* original +26 */
    BYTE unmodeled_28_29[2];
    BYTE builtin_operation; /* original +2a */
    BYTE function_class; /* original +2b, lookup initializes1 */
    BYTE builtin_flags;     /* original +2c, builtin test bit3 */
    BYTE linkage_flags, queue_flags; /* original+2d/+2e */
};
typedef char CompilerIrSymbol_prefix[sizeof(CompilerIrSymbol) == 32 ? 1 : -1];
typedef char CompilerRuntimeFunction_offsets[(
    offsetof(CompilerRuntimeFunction, member_offset) == 0x20 &&
    offsetof(CompilerRuntimeFunction, member_scope_number) == 0x26 &&
    offsetof(CompilerRuntimeFunction, builtin_operation) == 0x2a &&
    offsetof(CompilerRuntimeFunction, function_class) == 0x2b &&
    offsetof(CompilerRuntimeFunction, builtin_flags) == 0x2c &&
    offsetof(CompilerRuntimeFunction, linkage_flags) == 0x2d &&
    offsetof(CompilerRuntimeFunction, queue_flags) == 0x2e) ? 1 : -1];
/* Parameter records share the named header and declaration-state DWORD
 * at original+1c. The link tail at+20 also belongs to the kind12 formal
 * sentinel when it is inserted into a parameter set. */
struct CompilerParameterSymbol : CompilerIrSymbol {
    CompilerSymbolLink *previous_parameter; /* original+20, link-relative+18 */
};
/* The const-qualified value variant reads its stored payload at+28.
 * It shares the common32-byte symbol prefix; +20..27 stay uninterpreted. */
struct CompilerStoredConstantSymbol : CompilerIrSymbol {
    BYTE unmodeled_20_27[8];
    union { __int64 integer; DWORD pointer_bits; } value;
};
typedef char CompilerStoredConstantSymbol_value_offset[
    offsetof(CompilerStoredConstantSymbol, value) == 0x28 ? 1 : -1];
struct CompilerTranslationSymbol : CompilerIrSymbol {
    struct CompilerSymbolState *state; /* original +20, first record clears it */
};
typedef char CompilerIrVariant_offsets[(
    offsetof(CompilerParameterSymbol, previous_parameter) == 0x20 &&
    offsetof(CompilerTranslationSymbol, state) == 0x20) ? 1 : -1];
/* Resolve's indexed class/member path reads only these two fields. The
 * original context's other bytes and the symbol tail remain unmodeled. */
struct CompilerMemberContext { BYTE flags; /* original +3b, bit7 */ };
struct CompilerMemberSymbol : CompilerNamedSymbol {
    CompilerMemberContext *context; /* original +20 */
    BYTE declaration_flags; /* original +5, encountered bit0 */
};
/* The declarator lookup follows these pointer owners. Only the named
 * pointers are modeled; no intervening bytes or other payload are copied. */
struct CompilerSymbolBindingScope {
    void *entries; /* original +0, qualified-owner presence test */
    BYTE unmodeled_04_07[4];
    CompilerSymbol *symbol; /* original +8 */
};
struct CompilerSymbolBindingOwner {
    union { CompilerSymbolBindingScope *scope; CompilerSymbolBindingOwner *inner; }; /* original+0 */
    CompilerSymbolBindingOwner *next; /* original+4, qualified lookup alternatives */
    union { CompilerNamedSymbol *named_scope; CompilerSymbol *symbol; }; /* original+8 */
};
struct CompilerSymbolBinding {
    CompilerSymbolBindingOwner *owner; /* original +0 */
    CompilerSymbolBindingOwner *last;  /* original +4, same alternative-list owner */
};
typedef char CompilerSymbolBinding_offsets[(
    offsetof(CompilerSymbolBindingScope, symbol) == 8 &&
    offsetof(CompilerSymbolBindingOwner, scope) == 0 &&
    offsetof(CompilerSymbolBindingOwner, named_scope) == 8) ? 1 : -1];
struct CompilerBoundSymbol : CompilerNamedSymbol {
    CompilerSymbolBinding *binding; /* original +10 */
};
struct CompilerIndexedTypeSymbol : CompilerNamedSymbol {
    short type_index; /* original +14; interpreted only for non4/non23 kind */
};
struct CompilerExpression {
    BYTE kind, flags;                  /* original +0/+1 */
    BYTE constant_flags;               /* +2, typed-constant zero/one bit0 */
    CompilerType *type;                /* original +4 */
    BYTE unmodeled_08_0f[8];
    union Value {
        __int64 integer;              /* original +10..17 */
        struct Words { DWORD low, high; } words;
        struct Operands { CompilerExpression *first, *second; } operands;
        CompilerSymbol *symbol;
        CompilerString *string;
        const BYTE *bytes;
        void *pointer; /*source-only original+10 variant, no extra cell*/
        CompilerQuotedValue quoted; /* same +10 pointer/+14 length/+16 flags */
    } value;
    /* The natural+3 gap and opaque+8..f bytes remain uninterpreted. */
};
typedef char CompilerExpression_offsets[(offsetof(CompilerExpression, type) == 4 && offsetof(CompilerExpression, value) == 16 && sizeof(CompilerExpression) == 24) ? 1 : -1];
struct CompilerString {
    void RECON_THISCALL GetData(unsigned *, const void **, unsigned *);
};
#endif
