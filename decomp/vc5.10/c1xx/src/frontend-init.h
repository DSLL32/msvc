#ifndef C1XX_FRONTEND_INIT_H
#define C1XX_FRONTEND_INIT_H
#include "frontend.h"
#include "input.h"
#include "scope.h"
#include "hash-table.h"
#include <recon.h>

/* Only the encountered virtual slots have semantic names. Unrecovered
 * preceding slots are declarations, never instantiated or given fake bodies. */
/* Browser interface: exact 24-slot order from the two original vtables.
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
    virtual void RECON_THISCALL Slot20(unsigned) = 0;
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
    virtual void RECON_THISCALL Slot5c(void) = 0;
};
struct CompilerDebugModule {
    virtual int RECON_THISCALL Finish(void) = 0;
    virtual int RECON_THISCALL Close(void) = 0;
    virtual void RECON_THISCALL UnrecoveredSlot2(void) = 0;
    virtual void RECON_THISCALL UnrecoveredSlot3(void) = 0;
    virtual void RECON_THISCALL UnrecoveredSlot4(void) = 0;
    virtual void RECON_THISCALL UnrecoveredSlot5(void) = 0;
    virtual void RECON_THISCALL UnrecoveredSlot6(void) = 0;
    virtual void RECON_THISCALL UnrecoveredSlot7(void) = 0;
    virtual void RECON_THISCALL UnrecoveredSlot8(void) = 0;
    virtual void RECON_THISCALL UnrecoveredSlot9(void) = 0;
    virtual void RECON_THISCALL UnrecoveredSlot10(void) = 0;
    virtual void RECON_THISCALL UnrecoveredSlot11(void) = 0;
    virtual void RECON_THISCALL UnrecoveredSlot12(void) = 0;
    virtual void RECON_THISCALL UnrecoveredSlot13(void) = 0;
    virtual void RECON_THISCALL UnrecoveredSlot14(void) = 0;
    virtual void RECON_THISCALL UnrecoveredSlot15(void) = 0;
    virtual void RECON_THISCALL UnrecoveredSlot16(void) = 0;
    virtual void RECON_THISCALL UnrecoveredSlot17(void) = 0;
    virtual void RECON_THISCALL UnrecoveredSlot18(void) = 0;
    virtual void RECON_THISCALL UnrecoveredSlot19(void) = 0;
    virtual void RECON_THISCALL UnrecoveredSlot20(void) = 0;
    virtual void RECON_THISCALL UnrecoveredSlot21(void) = 0;
    virtual void RECON_THISCALL UnrecoveredSlot22(void) = 0;
    virtual void RECON_THISCALL UnrecoveredSlot23(void) = 0;
    virtual void RECON_THISCALL UnrecoveredSlot24(void) = 0;
    virtual void RECON_THISCALL UnrecoveredSlot25(void) = 0;
    virtual void RECON_THISCALL UnrecoveredSlot26(void) = 0;
    virtual void RECON_THISCALL UnrecoveredSlot27(void) = 0;
    virtual void RECON_THISCALL UnrecoveredSlot28(void) = 0;
    virtual void RECON_THISCALL UnrecoveredSlot29(void) = 0;
    virtual void RECON_THISCALL UnrecoveredSlot30(void) = 0;
    virtual void RECON_THISCALL UnrecoveredSlot31(void) = 0;
    virtual void RECON_THISCALL UnrecoveredSlot32(void) = 0;
};
/* External MSPDB interface, not a reconstruction of that dependency. */
struct PDB {
    virtual void RECON_THISCALL UnrecoveredSlot0(void) = 0;
    virtual void RECON_THISCALL UnrecoveredSlot1(void) = 0;
    virtual void RECON_THISCALL UnrecoveredSlot2(void) = 0;
    virtual int RECON_THISCALL QueryLastError(char *text) = 0;
};
struct CompilerHashTable;
struct CompilerTranslationUnit {
    CompilerScope *root, *current;         /* original +0/+4 */
    CompilerScope *scopes[128];            /* +8; only slot0 written here */
    BYTE depth;                            /* +208; d86e bounds it by 128 */
    CompilerSavedLookup *saved_lookup;    /* +20c */
    CompilerScope *retired_scopes;        /* +210 */
    CompilerSymbolLink *deferred_symbols; /* +214 */
    struct CompilerTypeAlias *type_aliases; /* +218 */
    BYTE lookup_context;                   /* +21c: 1 initially, later diagnostic tag */
    int suppress_lookup_errors;           /* +220 */
    int lookup_override;                  /* +224 */
    /* Original +228 has no modeled storage or default claim. */
    CompilerSymbol *global_namespace;     /* +22c */
    CompilerScope *deferred_scopes;       /* +230 */
    CompilerSymbolLink *pending_symbols;  /* +234 */
    CompilerTranslationRecord *records;  /* +238, pointee remains opaque */
    CompilerTranslationUnit *RECON_THISCALL Initialize(void);
    CompilerSymbolLink **RECON_THISCALL EndParameterScope(void);
    void RECON_THISCALL Finish(void);
};
struct CompilerDependency {
    struct DependencyRecords *records;
    /* Original remaining 12 bytes stay unmodeled until their own consumer. */
    int RECON_THISCALL Finish(int success);
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
    unsigned unread_count;
    /* Original +44..57 remain unmodeled, not a native padding array. */
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
/* The original 40-byte declaration's encountered semantic fields only.
 * Its other zeroed fields and layout are unmodeled before real consumers. */
struct CompilerDeclaration {
    unsigned flags; /* original +4 */
    CompilerType *type; /* original +1c */
};
/* Parsed parameter entries become a doubly linked argument chain. These
 * views expose only the actual pointer stores; other payload stays opaque. */
struct CompilerParameterLink {
    CompilerParameterLink *previous; /* original +8, first.previous is tail */
    CompilerParameterLink *next;     /* original +14 */
};
struct CompilerParameterEntry {
    CompilerParameterEntry *next; /* original +4 */
    CompilerParameterLink *parameter; /* original +10 */
};
struct CompilerParameterContext {
    CompilerParameterEntry *head; /* original +0 */
};
struct CompilerParameterSet {
    CompilerSymbolLink *head; /* original +4, borrowed symbol-link list */
};
struct CompilerTypedSymbol : CompilerNamedSymbol {
    CompilerType *type; /* original +10 in this parameter-symbol variant */
};
/* Original +4 is unmodeled. The actual pointer tail begins at+8; the
 * native compact view starts it after count, without a placeholder field. */
struct CompilerParameterTypes {
    unsigned count;
    CompilerType *types[1];
};
struct CompilerFunctionType {
    CompilerType *type; /* original +0 */
    BYTE symbol_count; /* +4; natural padding+5..7 is not a semantic field */
    CompilerParameterLink *symbols; /* +8 */
    CompilerParameterTypes *parameters; /* +c */
};
/* Function-parameter description, distinct from an interned type shape.
 * Only the builder's stores are modeled; the full28-byte layout is unknown. */
struct CompilerParameterDescription {
    struct CompilerParameterSet *parameters; /* original +0, initiallyNULL */
    WORD flags;                   /* original +4 */
    struct CompilerParameterContext *context; /* original +c */
    unsigned kind;                /* +10, actual value5 */
    unsigned scope_open;          /* +14, conditional EndParameterScope marker */
    void RECON_THISCALL Normalize(void);
};
struct CompilerCompositeType {
    CompilerCompositeType *next;
    CompilerFunctionType function;
};
typedef char CompilerFunctionType_size[sizeof(CompilerFunctionType) == 16 ? 1 : -1];
typedef char CompilerCompositeType_size[sizeof(CompilerCompositeType) == 20 ? 1 : -1];
struct CompilerDeclaratorQualifiers {
    WORD flags;
    unsigned value;
};
struct CompilerDeclarator {
    CompilerType *base_type; /* original +0 */
    CompilerType *type; /* original +3c, resolved result */
    CompilerSymbol *symbol; /* original +38 */
    CompilerDeclarationContext context; /* two mapped BYTEs at original+1c/+1d */
    CompilerTypeShape *shape, *last_shape; /* original +4/+8 */
    CompilerDeclaratorQualifiers qualifiers; /* original +c/+10 */
    unsigned shape_flags; /* original +14 */
    CompilerParameterDescription *parameters; /* original +18 */
    unsigned flags; /* original +40 */
    /* Remaining original72-byte storage stays unmodeled. */
    CompilerDeclarator *RECON_THISCALL Initialize(CompilerType *, const void *);
    void RECON_THISCALL AddFunction(CompilerParameterDescription *);
    void RECON_THISCALL AppendShape(CompilerTypeShape *);
    void RECON_THISCALL AddPointer(CompilerDeclaratorQualifiers *, CompilerDeclaratorQualifiers *);
    void RECON_THISCALL ApplyQualifiers(CompilerDeclaratorQualifiers *);
    void RECON_THISCALL Resolve(void);
    void RECON_THISCALL ValidateSpecialFunction(void);
    void RECON_THISCALL NormalizeBase(void);
    void RECON_THISCALL AddArrayShape(CompilerTypeShape *);
    void RECON_THISCALL AddMemberPointerShape(CompilerTypeShape *);
    void RECON_THISCALL ValidateShapes(void);
    WORD RECON_THISCALL DefaultCallingConvention(CompilerTypeShape *);
    WORD RECON_THISCALL PropagateQualifiers(CompilerTypeShape *, WORD);
    void RECON_THISCALL ValidateContext(void);
    CompilerType *RECON_THISCALL BuildType(CompilerTypeShape *);
};
struct CompilerSymbol;
#endif
