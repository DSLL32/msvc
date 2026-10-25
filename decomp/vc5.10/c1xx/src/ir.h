#ifndef C1XX_IR_H
#define C1XX_IR_H
#include "frontend-types.h"
#include "names.h"
#include <recon.h>

struct CompilerExpression;
struct CompilerString;
/* The recycled-list pointer addresses original +8 within the record. */
struct CompilerSymbolLink {
    CompilerNameEntry *name;
    CompilerSymbolLink *next;
};
/* The common header applies to the short namespace records too. Original
 * +1/+3 and other unrecovered bytes have no native placeholder. */
struct CompilerSymbol {
    BYTE kind;                         /* +0 */
    BYTE attributes;                   /* +4, encountered bit6 */
    BYTE definition_flags;             /* +2, namespace definition bit2 */
    CompilerSymbol *RECON_THISCALL Initialize(unsigned, CompilerNameEntry *, unsigned);
    void RECON_THISCALL EmitDeclaration(void);
};
struct CompilerNamedSymbol : CompilerSymbol {
    CompilerSymbolLink link;           /* original +8 name / +c next */
};
/* Distinct constructor variants: only the observed DWORD zero stores are
 * modeled. Their later interpretations remain behind pending consumers. */
struct CompilerSmallSymbol : CompilerNamedSymbol {
    DWORD initial_payload;             /* original +10, kinds16/17 */
};
struct CompilerKind4Symbol : CompilerNamedSymbol {
    DWORD initial_context;             /* original +24, kind4 */
};
struct CompilerAbstractSymbol : CompilerNamedSymbol {
    struct CompilerAbstractType *type; /* original +10; actual16-byte payload */
};
/* Extended members are accessed only for the larger IR symbol variants.
 * They must never enlarge the actual 20-byte namespace allocation. */
struct CompilerIrSymbol : CompilerNamedSymbol {
    struct CompilerSymbolPayload *payload; /* +10 */
    unsigned emission_state;           /* +14, low2 reference count, bit2 context */
    unsigned encoded_index;            /* +18 */
    BYTE function_flags;               /* +1d, encountered bit0 */
    BYTE reference_flags;              /* +1f, encountered bit0 */
    BYTE linkage_flags, queue_flags;   /* +2d bit1, +2e bit7 */
};
/* Resolve's indexed class/member path reads only these two fields. The
 * original context's other bytes and the symbol tail remain unmodeled. */
struct CompilerMemberContext { BYTE flags; /* original +3b, bit7 */ };
struct CompilerMemberSymbol : CompilerNamedSymbol {
    CompilerMemberContext *context; /* original +20 */
    BYTE declaration_flags; /* original +5, encountered bit0 */
};
struct CompilerExpression {
    BYTE kind, flags;                  /* original +0/+1 */
    CompilerType *type;                /* original +4 */
    union Value {
        __int64 integer;              /* original +10..17 */
        struct Words { DWORD low, high; } words;
        struct Operands { CompilerExpression *first, *second; } operands;
        CompilerSymbol *symbol;
        CompilerString *string;
        const BYTE *bytes;
    } value;
    /* Original +2..3 and +8..f remain unmodeled. The native view is compact. */
};
struct CompilerString {
    void RECON_THISCALL GetData(unsigned *, const void **, unsigned *);
};
#endif
