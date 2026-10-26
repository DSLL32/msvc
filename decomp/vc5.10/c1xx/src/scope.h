#ifndef C1XX_SCOPE_H
#define C1XX_SCOPE_H
#include "ir.h"
#include <recon.h>

struct CompilerScopeLookup;
struct CompilerScopeMatch;
struct CompilerScopeContext;
/* Existing +1c owner, distinct from a virtual scope-table object. Its
 * representation remains behind the pending list methods. */
struct CompilerSecondaryScopes {
    void RECON_THISCALL AppendMatches(CompilerNameEntry *, CompilerScopeLookup *);
    void RECON_THISCALL AppendLookup(CompilerNameEntry *, unsigned, unsigned *, CompilerScopeLookup *);
};
/* Exact eleven-slot interface at original c92c8. Pending operations expose
 * only proven argument widths; their bodies remain behind named traps. */
struct CompilerScopeTable {
    CompilerScopeTable *next;             /* +4 */
    BYTE level;                           /* +8, opaque alignment +9..b */
    CompilerScopeTable *outer;            /* +c */
    CompilerScopeContext *context;        /* +10 */
    RECON_FORCEINLINE CompilerScopeTable(unsigned nesting) {
        next = NULL; level = (BYTE)nesting; outer = NULL; context = NULL;
    }
    CompilerSymbol *RECON_THISCALL PrepareInsert(CompilerSymbol *);
    void RECON_THISCALL RestoreContext(void); /*99cf9*/
    virtual int RECON_THISCALL HasBuckets(void) = 0;
    virtual int RECON_THISCALL HasSecondary(void) = 0;
    virtual void RECON_THISCALL Insert(CompilerSymbol *) = 0;
    virtual CompilerScopeLookup *RECON_THISCALL Find(CompilerNameEntry *, unsigned) = 0;
    virtual CompilerScopeMatch *RECON_THISCALL FindOrdinary(CompilerNameEntry *) = 0;
    virtual CompilerScopeLookup *RECON_THISCALL Lookup(CompilerNameEntry *, unsigned, unsigned *) = 0;
    virtual int RECON_THISCALL FindNested(CompilerNameEntry *, CompilerSymbol **) = 0;
    virtual CompilerSymbolLink **RECON_THISCALL FindLink(CompilerSymbolLink *) = 0;
    virtual void RECON_THISCALL Serialize(void) = 0;
    virtual void RECON_THISCALL Check(void) = 0;
    virtual void RECON_THISCALL Link(CompilerSymbol *) = 0;
};
typedef char CompilerScopeTable_size[sizeof(CompilerScopeTable) == 20 ? 1 : -1];
struct CompilerScope : CompilerScopeTable {
    CompilerSymbolLink **buckets;         /* +14 */
    unsigned bucket_mask;                /* +18 */
    CompilerSecondaryScopes *secondary;   /* +1c, separate secondary-scope list */
    RECON_FORCEINLINE CompilerScope() : CompilerScopeTable(0) {
        bucket_mask = 0x3ff; secondary = NULL;
    }
    int RECON_THISCALL HasBuckets(void);
    int RECON_THISCALL HasSecondary(void);
    void RECON_THISCALL Insert(CompilerSymbol *);
    CompilerScopeLookup *RECON_THISCALL Find(CompilerNameEntry *, unsigned);
    CompilerScopeMatch *RECON_THISCALL FindOrdinary(CompilerNameEntry *);
    CompilerScopeLookup *RECON_THISCALL Lookup(CompilerNameEntry *, unsigned, unsigned *);
    int RECON_THISCALL FindNested(CompilerNameEntry *, CompilerSymbol **);
    CompilerSymbolLink **RECON_THISCALL FindLink(CompilerSymbolLink *);
    void RECON_THISCALL Serialize(void);
    void RECON_THISCALL Check(void);
    void RECON_THISCALL Link(CompilerSymbol *);
    void RECON_THISCALL EnsureBuckets(void); /*99d12*/
    CompilerScopeMatch *RECON_THISCALL MatchTag(CompilerSymbol *, unsigned); /*4d632*/
};
typedef char CompilerScope_size[sizeof(CompilerScope) == 32 ? 1 : -1];

/* Factory-owned class and auxiliary scope variants. Their only common
 * storage is the actual20-byte interface/header, not root hash-table fields. */
struct CompilerClassScope : CompilerScopeTable {
    CompilerSymbol *symbol;               /* +14 */
    CompilerScopeTable *auxiliary;         /* +18 */
    unsigned reset_value;                 /* +1c */
    RECON_FORCEINLINE CompilerClassScope(unsigned nesting, CompilerSymbol *owner)
        : CompilerScopeTable(nesting), symbol(owner), auxiliary(NULL), reset_value(0) {}
    inline int RECON_THISCALL HasBuckets(void); /* canonical TRUE11ab */
    inline int RECON_THISCALL HasSecondary(void); /* canonical FALSE5adc */
    void RECON_THISCALL Insert(CompilerSymbol *);
    CompilerScopeLookup *RECON_THISCALL Find(CompilerNameEntry *, unsigned);
    CompilerScopeMatch *RECON_THISCALL FindOrdinary(CompilerNameEntry *);
    CompilerScopeLookup *RECON_THISCALL Lookup(CompilerNameEntry *, unsigned, unsigned *);
    int RECON_THISCALL FindNested(CompilerNameEntry *, CompilerSymbol **);
    CompilerSymbolLink **RECON_THISCALL FindLink(CompilerSymbolLink *);
    void RECON_THISCALL Serialize(void);
    void RECON_THISCALL Check(void);
    void RECON_THISCALL Link(CompilerSymbol *);
};
struct CompilerAuxiliaryScope : CompilerScopeTable {
    CompilerParameterSet *RECON_THISCALL ParameterTable(void); /*fdd6*/
    union {
        void *value; /* +14, borrowed class-template owner */
        struct CompilerParameterSet *parameters; /* same +14, parameter-scope role */
    };
    RECON_FORCEINLINE CompilerAuxiliaryScope(void *owner, unsigned nesting = 0xfa)
        : CompilerScopeTable(nesting), value(owner) {}
    int RECON_THISCALL HasBuckets(void);
    inline int RECON_THISCALL HasSecondary(void); /* canonical FALSE5adc */
    void RECON_THISCALL Insert(CompilerSymbol *);
    CompilerScopeLookup *RECON_THISCALL Find(CompilerNameEntry *, unsigned);
    CompilerScopeMatch *RECON_THISCALL FindOrdinary(CompilerNameEntry *);
    CompilerScopeLookup *RECON_THISCALL Lookup(CompilerNameEntry *, unsigned, unsigned *);
    int RECON_THISCALL FindNested(CompilerNameEntry *, CompilerSymbol **);
    CompilerSymbolLink **RECON_THISCALL FindLink(CompilerSymbolLink *);
    void RECON_THISCALL Serialize(void);
    void RECON_THISCALL Check(void);
    void RECON_THISCALL Link(CompilerSymbol *);
};
typedef char CompilerClassScope_layout[(sizeof(CompilerClassScope) == 32 && offsetof(CompilerClassScope, symbol) == 20 && offsetof(CompilerClassScope, auxiliary) == 24) ? 1 : -1];
typedef char CompilerAuxiliaryScope_layout[(sizeof(CompilerAuxiliaryScope) == 24 && offsetof(CompilerAuxiliaryScope, value) == 20) ? 1 : -1];

/* Find builds an8-byte list owner and12-byte symbol/scope/next nodes.
 * These are distinct allocations and identities, not interchangeable views. */
struct CompilerScopeMatch {
    CompilerSymbol *RECON_THISCALL Reference(void); /*1d81a physical alias*/
    CompilerScopeMatch *RECON_THISCALL Initialize(CompilerSymbol *, CompilerScopeTable *); /*88b40*/
    CompilerSymbol *symbol;
    CompilerScopeTable *scope;
    CompilerScopeMatch *next;
};
struct CompilerScopeLookup {
    CompilerScopeMatch *head, *tail;
    CompilerScopeMatch *RECON_THISCALL Select(unsigned);
    CompilerScopeMatch *RECON_THISCALL MergeFunctionMatches(void); /*9a26c*/
};
typedef char CompilerScopeMatch_size[sizeof(CompilerScopeMatch) == 12 ? 1 : -1];
typedef char CompilerScopeLookup_size[sizeof(CompilerScopeLookup) == 8 ? 1 : -1];

struct CompilerNamespaceData {
    CompilerScopeTable *scope; /* first DWORD of actual 8-byte payload */
    union { WORD type_index, scope_number; }; /* same original+4 SHORT/WORD cell */
    BYTE unmodeled_06_07[2];
};
struct CompilerNamespaceSymbol : CompilerNamedSymbol {
    CompilerNamespaceData *data; /* original +10, actual 20-byte symbol record */
};
struct CompilerSavedLookup;
struct CompilerTranslationRecord;
#endif
