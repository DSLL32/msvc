#ifndef C1XX_FRONTEND_INIT_H
#define C1XX_FRONTEND_INIT_H
#include "frontend.h"
#include "input.h"
#include "scope.h"
#include "hash-table.h"
#include <recon.h>

/* Only the encountered virtual slots have semantic names. Unrecovered
 * preceding slots are declarations, never instantiated or given fake bodies. */
/* Browser interface:29 slots from the two original vtables. The earlier
 * source projection ended after+5c, before any tail consumer was reached.
 * Unknown operations retain neutral slot names and proven stack widths. */
struct CompilerBackend {
    RECON_FORCEINLINE CompilerBackend() {}
    virtual void RECON_THISCALL Finish(void) = 0;
    virtual void RECON_THISCALL Slot04(void) = 0;
    virtual void RECON_THISCALL Slot08(void) = 0;
    virtual void RECON_THISCALL Slot0c(void) = 0;
    virtual void RECON_THISCALL Slot10(void) = 0;
    virtual void RECON_THISCALL Slot14(unsigned) = 0;
    virtual void RECON_THISCALL Slot18(void) = 0;
    virtual void RECON_THISCALL Slot1c(unsigned) = 0;
    virtual void RECON_THISCALL Slot20(CompilerSymbol *) = 0;
    virtual void RECON_THISCALL Slot24(unsigned) = 0;
    virtual void RECON_THISCALL Slot28(struct CompilerMacro *) = 0;
    virtual void RECON_THISCALL Slot2c(void) = 0;
    virtual void RECON_THISCALL Slot30(unsigned) = 0;
    virtual void RECON_THISCALL Slot34(void) = 0;
    virtual void RECON_THISCALL Slot38(void) = 0;
    virtual void RECON_THISCALL Slot3c(unsigned) = 0;
    virtual void RECON_THISCALL Slot40(void) = 0;
    virtual void RECON_THISCALL Slot44(unsigned) = 0;
    virtual void RECON_THISCALL Slot48(void) = 0;
    virtual void RECON_THISCALL Slot4c(unsigned) = 0;
    virtual void RECON_THISCALL Slot50(unsigned) = 0;
    virtual int RECON_THISCALL Slot54(unsigned, unsigned) = 0;
    virtual void RECON_THISCALL ResetMode(void) = 0;
    virtual int RECON_THISCALL Slot5c(void) = 0;
    virtual void RECON_THISCALL Slot60(void) = 0;
    virtual unsigned RECON_THISCALL Slot64(CompilerSymbol *) = 0;
    virtual void RECON_THISCALL Slot68(void *, unsigned) = 0;
    virtual void RECON_THISCALL Slot6c(void) = 0;
    virtual void RECON_THISCALL Slot70(void) = 0;
    void RECON_THISCALL SetDebugMode(int);
};
struct CompilerDebugModule {
    RECON_FORCEINLINE CompilerDebugModule() {}
    virtual int RECON_THISCALL Finish(void) = 0;
    virtual int RECON_THISCALL Close(void) = 0;
    virtual int RECON_THISCALL Slot08(unsigned *) = 0;
    virtual int RECON_THISCALL Slot0c(unsigned *) = 0;
    virtual int RECON_THISCALL Slot10(void) = 0;
    virtual int RECON_THISCALL Slot14(unsigned, unsigned, unsigned, unsigned *, unsigned *, unsigned *, unsigned *) = 0;
    virtual int RECON_THISCALL Slot18(unsigned, unsigned, unsigned, unsigned *, unsigned *) = 0;
    virtual int RECON_THISCALL Slot1c(void) = 0;
    virtual int RECON_THISCALL Slot20(unsigned) = 0;
    virtual int RECON_THISCALL Slot24(unsigned) = 0;
    virtual int RECON_THISCALL Slot28(void) = 0;
    virtual int RECON_THISCALL Slot2c(void) = 0;
    virtual int RECON_THISCALL BeginSource(const char *, int) = 0;
    virtual int RECON_THISCALL Slot34(unsigned) = 0;
    virtual int RECON_THISCALL Slot38(unsigned, unsigned) = 0;
    virtual int RECON_THISCALL Slot3c(unsigned, unsigned, unsigned *) = 0;
    virtual int RECON_THISCALL Slot40(void) = 0;
    virtual int RECON_THISCALL Slot44(void) = 0;
    virtual int RECON_THISCALL Slot48(void) = 0;
    virtual int RECON_THISCALL Slot4c(void) = 0;
    virtual int RECON_THISCALL Slot50(void) = 0;
    virtual int RECON_THISCALL Slot54(unsigned, int *) = 0;
    virtual int RECON_THISCALL Slot58(unsigned *) = 0;
    virtual int RECON_THISCALL Slot5c(unsigned *) = 0;
    virtual void RECON_THISCALL Slot60(void) = 0;
    virtual int RECON_THISCALL Slot64(void) = 0;
    virtual int RECON_THISCALL Slot68(void) = 0;
    virtual int RECON_THISCALL Slot6c(void) = 0;
    virtual void RECON_THISCALL Slot70(void) = 0;
    virtual int RECON_THISCALL Slot74(const char *) = 0;
    virtual int RECON_THISCALL Slot78(unsigned) = 0;
    virtual int RECON_THISCALL Slot7c(void) = 0;
    virtual int RECON_THISCALL Slot80(void) = 0;
    virtual int RECON_THISCALL Slot84(void) = 0;
    virtual int RECON_THISCALL SetSourceDependency(void *) = 0;
    virtual int RECON_THISCALL Slot8c(unsigned, unsigned) = 0;
    virtual int RECON_THISCALL Slot90(unsigned, unsigned, unsigned *) = 0;
    virtual int RECON_THISCALL Slot94(unsigned, unsigned) = 0;
    virtual int RECON_THISCALL Slot98(void) = 0;
    virtual int RECON_THISCALL Slot9c(unsigned, unsigned) = 0;
    virtual int RECON_THISCALL Slota0(unsigned, unsigned) = 0;
    virtual int RECON_THISCALL Slota4(void) = 0;
    virtual int RECON_THISCALL Slota8(unsigned, unsigned) = 0;
    virtual int RECON_THISCALL Slotac(unsigned, unsigned) = 0;
    virtual int RECON_THISCALL Slotb0(void) = 0;
    virtual int RECON_THISCALL Slotb4(void) = 0;
    virtual int RECON_THISCALL Slotb8(void) = 0;
    virtual int RECON_THISCALL Slotbc(void) = 0;
    virtual int RECON_THISCALL Slotc0(unsigned, unsigned, unsigned) = 0;
    virtual int RECON_THISCALL Slotc4(unsigned, unsigned) = 0;
    virtual int RECON_THISCALL Slotc8(void) = 0;
    virtual int RECON_THISCALL Slotcc(unsigned, unsigned *) = 0;
    virtual int RECON_THISCALL Slotd0(void) = 0;
    virtual int RECON_THISCALL Slotd4(void) = 0;
    virtual void RECON_THISCALL Slotd8(unsigned) = 0;
    virtual int RECON_THISCALL Slotdc(void) = 0;
    virtual int RECON_THISCALL SlotE0(unsigned, unsigned) = 0;
    virtual int RECON_THISCALL SlotE4(void) = 0;
};

/* The null implementation owns only its vptr. Folded methods are code-free
 * aliases of the one physical registry body with the matching ABI. */
struct CompilerNullDebugModule : CompilerDebugModule {
    RECON_FORCEINLINE CompilerNullDebugModule() {}
    int RECON_THISCALL Close(void);
    int RECON_THISCALL Finish(void);
    int RECON_THISCALL Slot08(unsigned *);
    int RECON_THISCALL Slot0c(unsigned *);
    int RECON_THISCALL Slot10(void);
    int RECON_THISCALL Slot14(unsigned, unsigned, unsigned, unsigned *, unsigned *, unsigned *, unsigned *);
    int RECON_THISCALL Slot18(unsigned, unsigned, unsigned, unsigned *, unsigned *);
    int RECON_THISCALL Slot1c(void);
    int RECON_THISCALL Slot20(unsigned);
    int RECON_THISCALL Slot24(unsigned);
    int RECON_THISCALL Slot28(void);
    int RECON_THISCALL Slot2c(void);
    int RECON_THISCALL BeginSource(const char *, int);
    int RECON_THISCALL Slot34(unsigned);
    int RECON_THISCALL Slot38(unsigned, unsigned);
    int RECON_THISCALL Slot3c(unsigned, unsigned, unsigned *);
    int RECON_THISCALL Slot40(void);
    int RECON_THISCALL Slot44(void);
    int RECON_THISCALL Slot48(void);
    int RECON_THISCALL Slot4c(void);
    int RECON_THISCALL Slot50(void);
    int RECON_THISCALL Slot54(unsigned, int *);
    int RECON_THISCALL Slot58(unsigned *);
    int RECON_THISCALL Slot5c(unsigned *);
    void RECON_THISCALL Slot60(void);
    int RECON_THISCALL Slot64(void);
    int RECON_THISCALL Slot68(void);
    int RECON_THISCALL Slot6c(void);
    void RECON_THISCALL Slot70(void);
    int RECON_THISCALL Slot74(const char *);
    int RECON_THISCALL Slot78(unsigned);
    int RECON_THISCALL Slot7c(void);
    int RECON_THISCALL Slot80(void);
    int RECON_THISCALL Slot84(void);
    int RECON_THISCALL SetSourceDependency(void *);
    int RECON_THISCALL Slot8c(unsigned, unsigned);
    int RECON_THISCALL Slot90(unsigned, unsigned, unsigned *);
    int RECON_THISCALL Slot94(unsigned, unsigned);
    int RECON_THISCALL Slot98(void);
    int RECON_THISCALL Slot9c(unsigned, unsigned);
    int RECON_THISCALL Slota0(unsigned, unsigned);
    int RECON_THISCALL Slota4(void);
    int RECON_THISCALL Slota8(unsigned, unsigned);
    int RECON_THISCALL Slotac(unsigned, unsigned);
    int RECON_THISCALL Slotb0(void);
    int RECON_THISCALL Slotb4(void);
    int RECON_THISCALL Slotb8(void);
    int RECON_THISCALL Slotbc(void);
    int RECON_THISCALL Slotc0(unsigned, unsigned, unsigned);
    int RECON_THISCALL Slotc4(unsigned, unsigned);
    int RECON_THISCALL Slotc8(void);
    int RECON_THISCALL Slotcc(unsigned, unsigned *);
    int RECON_THISCALL Slotd0(void);
    int RECON_THISCALL Slotd4(void);
    void RECON_THISCALL Slotd8(unsigned);
    int RECON_THISCALL Slotdc(void);
    int RECON_THISCALL SlotE0(unsigned, unsigned);
    int RECON_THISCALL SlotE4(void);
};
/* External MSPDB interface, not a reconstruction of that dependency. */
struct PDB {
    virtual void RECON_THISCALL UnrecoveredSlot0(void) = 0;
    virtual void RECON_THISCALL UnrecoveredSlot1(void) = 0;
    virtual int RECON_THISCALL Slot08(unsigned *) = 0;
    virtual int RECON_THISCALL QueryLastError(char *text) = 0;
    virtual unsigned RECON_THISCALL QuerySignature(void) = 0;
    virtual unsigned RECON_THISCALL QueryAge(void) = 0;
};
struct CompilerHashTable;
struct CompilerTranslationUnit {
    CompilerScopeTable *RECON_THISCALL CurrentScope(void); /*e008 physical alias*/
    void RECON_THISCALL BeginParameterScope(CompilerParameterSet *); /*d7f3*/
    CompilerScopeTable *RECON_THISCALL SetEnclosingClass(CompilerSymbol *, unsigned); /*1defd RET8*/
    void RECON_THISCALL ActivateClassScope(CompilerScopeTable *); /*d858 RET4*/
    void RECON_THISCALL SelectScopeSymbol(CompilerSymbol *); /*1e6fb RET4*/
    void RECON_THISCALL InsertCurrentScope(CompilerSymbol *); /*244c0*/
    CompilerSymbol *RECON_THISCALL CreateFunctionGroup(CompilerSymbol *, CompilerSymbol *); /*2b4ef*/
    CompilerSymbolLink **RECON_THISCALL FindFunctionBindingSlot(CompilerSymbolLink *, unsigned); /*2b6bb*/
    void RECON_THISCALL InsertTagScope(CompilerSymbol *); /*40ed5*/
    void RECON_THISCALL InsertScopedTag(CompilerSymbol *); /*8063a*/
    CompilerScopeMatch *RECON_THISCALL FindTagName(CompilerNameEntry *, unsigned); /*25224*/
    CompilerScopeTable *RECON_THISCALL CreateClassScope(unsigned, unsigned, CompilerSymbol *); /*1df55 RET12*/
    CompilerScopeTable *RECON_THISCALL FileScope(void); /*7bea*/
    void RECON_THISCALL InsertDeferredTag(CompilerSymbol *); /*88b59*/
    void *RECON_THISCALL PrepareAllocation(unsigned, void *, unsigned); /*1b0cf*/
    CompilerScope *root; CompilerScopeTable *current;         /* original +0/+4 */
    CompilerScopeTable *scopes[128];            /* +8; only slot0 written here */
    BYTE depth;                            /* +208; d86e bounds it by 128 */
    CompilerSavedLookup *saved_lookup;    /* +20c */
    CompilerSymbol *retired_scopes;        /* +210, qualified lookup symbol, not a scope table */
    CompilerSymbolLink *deferred_symbols; /* +214 */
    struct CompilerTypeAlias *type_aliases; /* +218 */
    BYTE lookup_context;                   /* +21c: 1 initially, later diagnostic tag */
    int suppress_lookup_errors;           /* +220 */
    int lookup_override;                  /* +224 */
    DWORD opaque_lookup_state; /* original+228, nonzero selects qualified lookup; no initializer store */
    CompilerSymbol *global_namespace;     /* +22c */
    CompilerScopeTable *deferred_scopes;       /* +230 */
    CompilerSymbolLink *pending_symbols;  /* +234 */
    struct CompilerSourceFile *source_files;  /* +238, arena-owned source records */
    struct CompilerSourceFile *RECON_THISCALL LookupSourceFile(CompilerNameEntry *);
    struct CompilerSourceFile *RECON_THISCALL FindSourceFile(const char *, unsigned);
    CompilerTranslationUnit *RECON_THISCALL Initialize(void);
    CompilerParameterSet *RECON_THISCALL EndParameterScope(void);
    void RECON_THISCALL Finish(void);
    void RECON_THISCALL SetScopeMode(unsigned);
    void RECON_THISCALL MarkDefinitionScopes(void); /*21754*/
    void RECON_THISCALL PopCurrentScope(void); /*e028*/
    void RECON_THISCALL UnregisterCurrentScope(void); /*e049*/
    void RECON_THISCALL RestoreSavedLookup(unsigned); /*7bff*/
    void RECON_THISCALL PopSavedLookup(void); /*7c99*/
    void RECON_THISCALL RegisterScope(CompilerScopeTable *);
    void RECON_THISCALL PrepareScopeSymbol(CompilerSymbol *);
    CompilerSymbol *RECON_THISCALL CurrentDeclarationOwner(void);
    CompilerScopeMatch *RECON_THISCALL ProbeDeclaration(CompilerSymbol *);
    CompilerScopeTable *RECON_THISCALL DeclarationScope(unsigned);
    CompilerScopeMatch *RECON_THISCALL FindScopedDeclaration(CompilerSymbol *);
    CompilerScopeMatch *RECON_THISCALL LookupName(CompilerNameEntry *, unsigned);
    void RECON_THISCALL ApplyNamespaceAliases(CompilerSymbol *); /*15691 RET4*/
    CompilerSymbol *RECON_THISCALL LookupScopeName(CompilerSymbol *, CompilerNameEntry *, unsigned, unsigned); /*6e9b*/
    CompilerScopeMatch *RECON_THISCALL FindDirectName(CompilerNameEntry *); /*2142b*/
    CompilerScopeMatch *RECON_THISCALL FindHiddenName(CompilerNameEntry *); /*3252d*/
    CompilerScopeMatch *RECON_THISCALL FindContextName(CompilerNameEntry *); /*9a331*/
};
/* Partial view of the existing dependency-record owner; its first12 bytes
 * remain uninterpreted, with no allocation or default claim. */
struct ClassDependencyRecordsView {
    BYTE unmodeled_00_0b[12];
    DWORD record_members;
};
struct CompilerDependency {
    struct DependencyRecords *records;
    /* Original remaining 12 bytes stay unmodeled until their own consumer. */
    void RECON_THISCALL SetSourceName(const char *);
    int RECON_THISCALL Finish(int success);
    void RECON_THISCALL RecordSource(const char *, int, int);
    void RECON_THISCALL RecordClass(CompilerSymbol *); /*9b456 RET4*/
    int RECON_THISCALL CompleteClass(CompilerSymbol *); /*9b898 RET4*/
    int RECON_THISCALL QuerySource(void **);
    int RECON_THISCALL RestoreSource(void); /*28b48*/
    void RECON_THISCALL RecordMember(CompilerSymbol *); /*9b647 RET4*/
};


struct CompilerContextBuffer {
    DWORD *block;
    unsigned position, arena_kind;
    CompilerContextBuffer *RECON_THISCALL Initialize(unsigned);
};
struct CompilerTokenStream {
    RECON_FORCEINLINE CompilerTokenStream() {}
    virtual unsigned RECON_THISCALL Read(void) = 0;
    virtual unsigned RECON_THISCALL Peek(void) = 0;
    virtual void RECON_THISCALL Unread(unsigned) = 0;
    virtual void RECON_THISCALL Trace(unsigned) = 0;
    virtual int RECON_THISCALL Valid(void) = 0;
    virtual void RECON_THISCALL Reset(void) = 0;
    virtual void *RECON_THISCALL Destroy(unsigned) = 0;
};
struct CompilerLexer : CompilerTokenStream {
    void *initial_object;
    unsigned initial_value;
    void *current_object;
    unsigned current_value;
    CompilerContextBuffer buffer, read_buffer, start_buffer;
    unsigned arena_kind;
    unsigned char active : 1;
    unsigned char marker_emitted : 1; /* original+3c bit1, no default claim */
    unsigned unread_count; /* original+40, constructor0 */
    /* Only the first actual unread slot at+44 is encountered. This one-row
     * partial view is not an original capacity; +48..57 stay unmodeled.
     * Native guards before indexing a second row, with no slot default. */
    unsigned unread_tokens[1];
    RECON_FORCEINLINE CompilerLexer(void *object, unsigned value) {
        initial_object = object; initial_value = value;
        current_object = object; current_value = value;
        buffer.Initialize(1);
        read_buffer = buffer; start_buffer = buffer;
        arena_kind = 1; active = 1; unread_count = 0;
    }
    unsigned RECON_THISCALL Read(void);
    unsigned RECON_THISCALL Peek(void);
    void RECON_THISCALL Unread(unsigned);
    void RECON_THISCALL Trace(unsigned);
    int RECON_THISCALL Valid(void);
    void RECON_THISCALL Reset(void);
    void *RECON_THISCALL Destroy(unsigned);
};
/* The original40-byte declaration has a grounded28-byte context and type.
 * Its remaining8 bytes stay unmodeled before an actual consumer. */
struct CompilerDeclaration {
    CompilerDeclarationContext context; /* original+0..1b, same context type */
    CompilerType *type; /* original +1c */
};
/* Parsed parameter entries become a doubly linked argument chain. These
 * views expose only the actual pointer stores; other payload stays opaque. */
struct CompilerParameterLink {
    CompilerParameterLink *previous; /* original +8, first.previous is tail */
    CompilerParameterLink *next;     /* original +14 */
    void *value;                    /* original+10, only presence is tested */
};
struct CompilerParameterEntry {
    CompilerParameterEntry *next; /* original +4 */
    CompilerParameterLink *parameter; /* original +10 */
};
struct CompilerParameterContext {
    CompilerParameterEntry *head; /* original +0 */
};
struct CompilerParameterSet {
    CompilerSymbol *RECON_THISCALL Find(CompilerNameEntry *); /*fe0f*/
    unsigned arena_index; /* original+0, allocator domain */
    union {
        CompilerSymbolLink *head; /* original+4, borrowed symbol-link list */
        CompilerParameterSet *next_free; /* same+4 while on the free list */
    };
    CompilerSymbolLink *tail; /* original+8 */
};
typedef char CompilerParameterSet_size[sizeof(CompilerParameterSet) == 12 ? 1 : -1];
struct CompilerTypedSymbol : CompilerNamedSymbol {
    CompilerType *type; /* original +10 in this parameter-symbol variant */
};
/* Original +4 is unmodeled. The actual pointer tail begins at+8; the
 * native compact view starts it after count, without a placeholder field. */
struct CompilerParameterTypes {
    unsigned count;
    CompilerType *types[1];
};
struct CompilerTypeSequence {
    CompilerTypeSequence *next;
    CompilerParameterTypes parameters;
};
struct CompilerFunctionType {
    CompilerType *type; /* original +0 */
    BYTE symbol_count; /* +4; original+5..7 are representation bytes, not semantic fields */
    CompilerParameterLink *symbols; /* +8 */
    CompilerParameterTypes *parameters; /* +c */
};
/* Function-parameter description, distinct from an interned type shape.
 * Only the builder's stores are modeled; the full28-byte layout is unknown. */
struct CompilerParameterDescription {
    CompilerParameterDescription *RECON_THISCALL Initialize(unsigned, CompilerParameterSet *); /*fdda*/
    void RECON_THISCALL SetQualifiers(struct CompilerDeclaratorQualifiers *); /*e00c*/
    void RECON_THISCALL SetDeclarationMode(unsigned); /*e01e*/
    struct CompilerParameterSet *parameters; /* original +0, initiallyNULL */
    WORD flags;                   /* original +4 */
    BYTE unmodeled_06_0b[6];        /* opaque qualifier representation */
    struct CompilerParameterContext *context; /* original +c */
    unsigned kind;                /* +10, actual value5 */
    unsigned scope_open;          /* +14, conditional EndParameterScope marker */
    unsigned declaration_mode;    /* +18, merge maps values1/2 to reference flags */
    void RECON_THISCALL Normalize(void);
    void RECON_THISCALL Append(CompilerNameEntry *, CompilerType *);
    void RECON_THISCALL AppendSymbol(CompilerSymbol *);
    void RECON_THISCALL AppendEmptySymbol(CompilerSymbol *); /*1e2c4*/
};
typedef char CompilerParameterDescription_layout[(sizeof(CompilerParameterDescription) == 28 && offsetof(CompilerParameterDescription, context) == 12 && offsetof(CompilerParameterDescription, kind) == 16 && offsetof(CompilerParameterDescription, scope_open) == 20) ? 1 : -1];
struct CompilerCompositeType {
    CompilerCompositeType *next;
    CompilerFunctionType function;
};
typedef char CompilerFunctionType_size[sizeof(CompilerFunctionType) == 16 ? 1 : -1];
typedef char CompilerCompositeType_size[sizeof(CompilerCompositeType) == 20 ? 1 : -1];
struct CompilerDeclaratorQualifiers {
    void RECON_THISCALL Clear(void); /*d539*/
    CompilerDeclaratorQualifiers *RECON_THISCALL Initialize(WORD); /*d541*/
    void RECON_THISCALL Set(WORD, unsigned); /*24526*/
    void RECON_THISCALL Merge(CompilerDeclaratorQualifiers *); /*864f8*/
    WORD flags;
    unsigned value;
};
struct CompilerDeclarator {
    void RECON_THISCALL AppendModifier(struct ParserTypeModifier *); /*d739*/
    WORD RECON_THISCALL MergeModifierQualifiers(WORD, WORD); /*40aed, lowWORD result consumed*/
    void RECON_THISCALL AddArrayBound(__int64 *); /*1eea1*/
    void RECON_THISCALL AddArrayExpression(CompilerExpression *); /*47b83*/
    void RECON_THISCALL Reset(void); /*1300 pending physical alias*/
    CompilerDeclarator *RECON_THISCALL Finish(unsigned); /*dfc6*/
    void RECON_THISCALL ApplyParserQualifiers(CompilerDeclaration *); /*45a9a*/
    void RECON_THISCALL PrepareAllocationType(void); /*433bf*/
    CompilerType *RECON_THISCALL ResolvedType(void); /*43429*/
    void RECON_THISCALL SetType(CompilerType *); /*86525*/
    CompilerType *base_type; /* original +0 */
    CompilerType *type; /* original +3c, resolved result */
    CompilerSymbol *symbol; /* original +38 */
    CompilerDeclarationContext context; /* original+1c..37 */
    CompilerTypeShape *shape, *last_shape; /* original +4/+8 */
    CompilerDeclaratorQualifiers qualifiers; /* original +c/+10 */
    unsigned shape_flags; /* original +14 */
    CompilerParameterDescription *parameters; /* original +18 */
    unsigned flags; /* original +40 */
    /* Remaining original72-byte storage stays unmodeled. */
    CompilerDeclarator *RECON_THISCALL Initialize(CompilerType *, const void *);
    CompilerDeclarator *RECON_THISCALL InitializeSymbol(CompilerSymbol *);
    void RECON_THISCALL ApplyDeclaration(CompilerDeclaration *);
    void RECON_THISCALL AddFunction(CompilerParameterDescription *);
    void RECON_THISCALL AppendShape(CompilerTypeShape *);
    void RECON_THISCALL AddPointer(CompilerDeclaratorQualifiers *, CompilerDeclaratorQualifiers *);
    void RECON_THISCALL AddReference(CompilerDeclaratorQualifiers *); /*2b3df*/
    WORD RECON_THISCALL ValidateReferenceQualifiers(WORD); /*28b78*/
    void RECON_THISCALL ApplyQualifiers(CompilerDeclaratorQualifiers *);
    void RECON_THISCALL Resolve(void);
    void RECON_THISCALL ValidateSpecialFunction(void);
    void RECON_THISCALL NormalizeBase(void);
    void RECON_THISCALL AddArrayShape(CompilerTypeShape *);
    void RECON_THISCALL ValidateReferenceShape(CompilerTypeShape *);
    void RECON_THISCALL ValidateShapes(void);
    WORD RECON_THISCALL DefaultCallingConvention(CompilerTypeShape *);
    WORD RECON_THISCALL PropagateQualifiers(CompilerTypeShape *, WORD);
    void RECON_THISCALL ValidateContext(void);
    CompilerType *RECON_THISCALL BuildType(CompilerTypeShape *);
};
struct CompilerSymbol;
#endif
