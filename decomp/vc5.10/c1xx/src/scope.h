#ifndef C1XX_SCOPE_H
#define C1XX_SCOPE_H
#include "ir.h"
#include <recon.h>

struct CompilerScopeLookup;
struct CompilerScopeContext;
/* Exact eleven-slot interface at original c92c8. Pending operations expose
 * only proven argument widths; their bodies remain behind named traps. */
struct CompilerScope {
    CompilerScope *next;                 /* +4 */
    BYTE level;                          /* +8, natural padding +9..b */
    CompilerScope *outer;                /* +c */
    CompilerScopeContext *context;       /* +10 */
    CompilerSymbolLink **buckets;        /* +14 */
    unsigned bucket_mask;                /* +18 */
    CompilerScope *secondary;            /* +1c */
    RECON_FORCEINLINE CompilerScope() {
        next = NULL; level = 0; outer = NULL; context = NULL;
        bucket_mask = 0x3ff; secondary = NULL;
    }
    virtual int RECON_THISCALL HasBuckets(void);
    virtual int RECON_THISCALL HasSecondary(void);
    virtual void RECON_THISCALL Insert(CompilerSymbol *);
    virtual CompilerScopeLookup *RECON_THISCALL Find(CompilerNameEntry *, unsigned);
    virtual CompilerSymbol *RECON_THISCALL FindOrdinary(CompilerNameEntry *);
    virtual CompilerSymbol *RECON_THISCALL Lookup(CompilerNameEntry *, unsigned, unsigned *);
    virtual int RECON_THISCALL FindNested(CompilerNameEntry *, CompilerSymbol **);
    virtual CompilerSymbolLink **RECON_THISCALL FindLink(CompilerSymbolLink *);
    virtual void RECON_THISCALL Serialize(void);
    virtual void RECON_THISCALL Check(void);
    virtual void RECON_THISCALL Link(CompilerSymbol *);
};
typedef char CompilerScope_size[sizeof(CompilerScope) == 32 ? 1 : -1];

struct CompilerNamespaceData {
    CompilerScope *scope; /* first DWORD of actual 8-byte payload; rest unmodeled */
};
struct CompilerNamespaceSymbol : CompilerNamedSymbol {
    CompilerNamespaceData *data; /* original +10, actual 20-byte symbol record */
};
struct CompilerSavedLookup;
struct CompilerTranslationRecord;
#endif
