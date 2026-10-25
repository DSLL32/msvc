#ifndef C1XX_IR_H
#define C1XX_IR_H
#include "frontend-types.h"
#include <recon.h>

struct CompilerExpression;
struct CompilerString;
/* Encountered symbol members only; comments identify their original storage.
 * Unrelated bytes and the full symbol size remain unmodeled. */
struct CompilerSymbol {
    BYTE kind;                         /* +0 */
    BYTE attributes;                   /* +4, encountered bit6 */
    unsigned emission_state;           /* +14, low2 reference count, bit2 context */
    unsigned encoded_index;            /* +18 */
    BYTE function_flags;               /* +1d, encountered bit0 */
    BYTE reference_flags;              /* +1f, encountered bit0 */
    BYTE linkage_flags, queue_flags;   /* +2d bit1, +2e bit7 */
    void RECON_THISCALL EmitDeclaration(void);
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
