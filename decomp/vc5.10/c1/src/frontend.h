#ifndef C1_FRONTEND_H
#define C1_FRONTEND_H
#include <stdio.h>
#include "compiler-state.h"
#include "input.h"
#include <recon.h>

/* Preserve C1's direct MSVCRT FILE-array import rather than the SDK's /MD
 * __p__iob accessor. Native streams retain the shared shim declaration. */
#if defined(_MSC_VER) && !defined(RECON_NATIVE_TEST)
#undef _iob
#undef stdout
#undef stdin
extern "C" __declspec(dllimport) FILE _iob[];
#define stdout (&_iob[1])
#define stdin (&_iob[0])
#endif

/* Browser output interface, in the actual 24-slot order. Unrecovered slot
 * meanings remain explicit; pointer arguments stay opaque until consumed. */
struct CompilerBackend {
    RECON_FORCEINLINE CompilerBackend() {}
    virtual void RECON_THISCALL Finish(void) = 0;
    virtual void RECON_THISCALL Slot04(void) = 0;
    virtual void RECON_THISCALL Slot08(void) = 0;
    virtual void RECON_THISCALL Slot0c(void) = 0;
    virtual void RECON_THISCALL Slot10(void) = 0;
    virtual void RECON_THISCALL Slot14(unsigned) = 0;
    virtual void RECON_THISCALL Slot18(void) = 0;
    virtual void RECON_THISCALL Slot1c(void *) = 0;
    virtual void RECON_THISCALL Slot20(void *) = 0;
    virtual void RECON_THISCALL Slot24(void *) = 0;
    virtual void RECON_THISCALL Slot28(void *) = 0;
    virtual void RECON_THISCALL Slot2c(void) = 0;
    virtual void RECON_THISCALL Slot30(void *) = 0;
    virtual void RECON_THISCALL Slot34(void) = 0;
    virtual void RECON_THISCALL Slot38(void) = 0;
    virtual void RECON_THISCALL Slot3c(void *) = 0;
    virtual void RECON_THISCALL Slot40(void) = 0;
    virtual void RECON_THISCALL ResetMode(void) = 0;
    virtual int RECON_THISCALL Disable(void) = 0;
    virtual void RECON_THISCALL CheckLocation(void) = 0;
    virtual unsigned RECON_THISCALL SymbolId(const void *) = 0;
    /* Physical result-pointer ABI: two stack slots, EAX=result, RET8. */
    virtual unsigned *RECON_THISCALL SetSymbolId(unsigned *, unsigned) = 0;
    virtual void RECON_THISCALL Slot58(void) = 0;
    virtual void RECON_THISCALL Slot5c(void) = 0;
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
    virtual int RECON_THISCALL Slot30(const char *, int) = 0;
    virtual int RECON_THISCALL Slot34(unsigned) = 0;
    virtual int RECON_THISCALL Slot38(unsigned, unsigned) = 0;
    virtual int RECON_THISCALL Slot3c(unsigned, unsigned, unsigned *) = 0;
    virtual int RECON_THISCALL Slot40(void) = 0;
    virtual int RECON_THISCALL Slot44(void) = 0;
    virtual int RECON_THISCALL Slot48(void) = 0;
    virtual int RECON_THISCALL Slot4c(void) = 0;
    virtual int RECON_THISCALL Slot50(void) = 0;
    virtual int RECON_THISCALL Slot54(int, int *) = 0;
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
    virtual int RECON_THISCALL Slot88(void *) = 0;
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

/* The null debug module owns only its vptr. Folded methods are declaration-only
 * aliases; each physical target retains one canonical body in the registry. */
struct CompilerNullDebugModule : CompilerDebugModule {
    RECON_FORCEINLINE CompilerNullDebugModule() {}
    /* Defined first to supply the ELF key function; base owns slot order. */
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
    int RECON_THISCALL Slot30(const char *, int);
    int RECON_THISCALL Slot34(unsigned);
    int RECON_THISCALL Slot38(unsigned, unsigned);
    int RECON_THISCALL Slot3c(unsigned, unsigned, unsigned *);
    int RECON_THISCALL Slot40(void);
    int RECON_THISCALL Slot44(void);
    int RECON_THISCALL Slot48(void);
    int RECON_THISCALL Slot4c(void);
    int RECON_THISCALL Slot50(void);
    int RECON_THISCALL Slot54(int, int *);
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
    int RECON_THISCALL Slot88(void *);
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
typedef char CompilerNullDebugModule_size[sizeof(CompilerNullDebugModule) == 4 ? 1 : -1];
/* External MSPDB interface; preceding slots remain opaque declarations. */
struct PDB {
    virtual void RECON_THISCALL UnrecoveredSlot0(void) = 0;
    virtual void RECON_THISCALL UnrecoveredSlot1(void) = 0;
    virtual void RECON_THISCALL UnrecoveredSlot2(void) = 0;
    virtual int RECON_THISCALL QueryLastError(char *text) = 0;
    virtual unsigned RECON_THISCALL QuerySignature(void) = 0;
    virtual unsigned RECON_THISCALL QueryAge(void) = 0;
    virtual void RECON_THISCALL UnrecoveredSlot18(void) = 0;
    virtual void RECON_THISCALL UnrecoveredSlot1c(void) = 0;
    virtual void RECON_THISCALL UnrecoveredSlot20(void) = 0;
    virtual int RECON_THISCALL Commit(void) = 0;
    virtual int RECON_THISCALL Close(void) = 0;
};
/* MSPDB name map: only the encountered ID-to-string slot is typed. The
 * preceding provider-owned methods and wider object remain opaque. */
struct NameMap {
    virtual int RECON_THISCALL Slot00(void) = 0;
    virtual void RECON_THISCALL UnrecoveredSlot04(void) = 0;
    virtual void RECON_THISCALL UnrecoveredSlot08(void) = 0;
    virtual int RECON_THISCALL QueryName(unsigned, const char **) = 0;
    virtual void RECON_THISCALL UnrecoveredSlot10(void) = 0;
    virtual void RECON_THISCALL UnrecoveredSlot14(void) = 0;
    virtual int RECON_THISCALL Slot18(void) = 0;
    static __declspec(dllimport) int __cdecl open(PDB *, int, NameMap **)
        RECON_MSPDB_NAMEMAP_OPEN;
};
/* Only this direct finalizer ABI is known; object extent remains unmodeled. */
struct CompilerMreState { int RECON_THISCALL Finish(void); };
/* PCH release reads only the vptr and kind+12; intervening bytes and the
 * complete image-object extent remain unmodeled. No instance is constructed. */
struct CompilerPchImage {
    virtual void RECON_THISCALL UnrecoveredSlot00(void) = 0;
    virtual void RECON_THISCALL UnrecoveredSlot04(void) = 0;
    virtual void RECON_THISCALL UnrecoveredSlot08(void) = 0;
    virtual void RECON_THISCALL Release(void) = 0;
    BYTE unmodeled_04[8];
    unsigned kind;
};
struct CompilerParserToken {
    BYTE kind;
    BYTE unmodeled_01[7];
    CompilerParserValue value;
};
typedef char CompilerParserToken_value[offsetof(CompilerParserToken, value) == 8 ? 1 : -1];
typedef char CompilerParserToken_extent[sizeof(CompilerParserToken) == 32 ? 1 : -1];
/* The parser advances semantic values by24 bytes and states by2. Declaration
 * states carry their established fields; other alternatives remain guarded. */
struct CompilerParserStackValue {
    CompilerParserValue value;
};
struct CompilerParser {
    CompilerParserToken *lookahead;
    int syntax_errors, recovery_shifts;
    short state;
    WORD unmodeled_0e;
    short *state_top;
    CompilerParserStackValue *value_top;
    CompilerParserStackValue values[340];
    short states[340];
    CompilerParser *RECON_THISCALL Initialize(void);
    void RECON_THISCALL Reset(void);
    int RECON_THISCALL Run(void);
    void RECON_THISCALL Finish(void);
    void RECON_THISCALL ClearRecovery(void);
};
typedef char CompilerParser_size[sizeof(CompilerParser) == 8864 ? 1 : -1];
typedef char CompilerParser_states[offsetof(CompilerParser, states) == 0x1ff8 ? 1 : -1];

struct CompilerScopeTable;
struct CompilerParameterBindings;
/* Authentic eight-byte lookup result, shared by ordinary and tag lookup. */
struct CompilerScopeLookup {
    CompilerIrSymbol *symbol;
    CompilerScopeTable *scope;
};
typedef char CompilerScopeLookup_size[sizeof(CompilerScopeLookup) == 8 ? 1 : -1];

/* The two actual scope tables share this20-byte prefix and ten-slot
 * interface. The hash table appends a mask; parameter scopes reuse+16 as
 * their incoming parameter value. No wider scope allocation is implied. */
struct CompilerScopeTable {
    CompilerScopeTable *next;
    BYTE level;
    union { CompilerScopeTable *outer; void *unmodeled_0c; };
    union { CompilerSymbolBinding **buckets; unsigned parameter_value; CompilerParameterBindings *parameter_bindings; };
    RECON_FORCEINLINE CompilerScopeTable(unsigned nesting)
        : next(NULL), level((BYTE)nesting), outer(NULL) {}
    CompilerIrSymbol **RECON_THISCALL MergeDeclaration(CompilerIrSymbol *);
    virtual int RECON_THISCALL HasBuckets(void) = 0;
    virtual void RECON_THISCALL Insert(CompilerIrSymbol *symbol) = 0;
    virtual void *RECON_THISCALL Find(CompilerNameEntry *name) = 0;
    virtual void *RECON_THISCALL Lookup(CompilerNameEntry *name) = 0;
    virtual void *RECON_THISCALL FindOrdinary(CompilerNameEntry *name) = 0;
    virtual void RECON_THISCALL FindNested(CompilerNameEntry *name, void **result) = 0;
    virtual CompilerIrSymbol **RECON_THISCALL FindLink(CompilerIrSymbol *symbol) = 0;
    virtual void RECON_THISCALL Serialize(void) = 0;
    virtual void RECON_THISCALL Check(void) = 0;
    virtual void RECON_THISCALL Link(CompilerIrSymbol *symbol) = 0;
};
typedef char CompilerScopeTable_size[sizeof(CompilerScopeTable) == 20 ? 1 : -1];
struct CompilerSymbolTable : CompilerScopeTable {
    unsigned bucket_mask;
    RECON_FORCEINLINE CompilerSymbolTable()
        : CompilerScopeTable(0), bucket_mask(1023) {}
    RECON_FORCEINLINE CompilerSymbolTable(unsigned nesting, CompilerScopeTable *enclosing)
        : CompilerScopeTable(nesting), bucket_mask(15) { outer = enclosing; }
    int RECON_THISCALL HasBuckets(void);
    void RECON_THISCALL Insert(CompilerIrSymbol *symbol);
    void *RECON_THISCALL Find(CompilerNameEntry *name);
    void *RECON_THISCALL Lookup(CompilerNameEntry *name);
    void *RECON_THISCALL FindOrdinary(CompilerNameEntry *name);
    void RECON_THISCALL FindNested(CompilerNameEntry *name, void **result);
    CompilerIrSymbol **RECON_THISCALL FindLink(CompilerIrSymbol *symbol);
    void RECON_THISCALL Serialize(void);
    void RECON_THISCALL Check(void);
    void RECON_THISCALL Link(CompilerIrSymbol *symbol);
};
typedef char CompilerSymbolTable_size[sizeof(CompilerSymbolTable) == 24 ? 1 : -1];
/* Authentic twelve-byte parameter list, constructed by parameter Link.
 * Nodes are the symbols' embedded binding headers, with a previous link
 * in the parameter-specific symbol union. */
struct CompilerParameterBindings {
    unsigned arena_kind;
    union { CompilerSymbolBinding *first; CompilerParameterBindings *free_next; };
    CompilerSymbolBinding *last;
    CompilerIrSymbol *RECON_THISCALL Find(CompilerNameEntry *);
    CompilerIrSymbol *RECON_THISCALL FindTag(CompilerNameEntry *);
};
typedef char CompilerParameterBindings_extent[sizeof(CompilerParameterBindings) == 12 ? 1 : -1];
struct CompilerParameterScope : CompilerScopeTable {
    RECON_FORCEINLINE CompilerParameterScope(int value) : CompilerScopeTable(1)
    { parameter_value = (unsigned)value; }
    /* Alias-only inline declaration: Insert owns this vtable's native key
     * function; the shared code-free alias supplies the canonical body. */
    inline int RECON_THISCALL HasBuckets(void);
    void RECON_THISCALL Insert(CompilerIrSymbol *symbol);
    void *RECON_THISCALL Find(CompilerNameEntry *name);
    void *RECON_THISCALL Lookup(CompilerNameEntry *name);
    void *RECON_THISCALL FindOrdinary(CompilerNameEntry *name);
    void RECON_THISCALL FindNested(CompilerNameEntry *name, void **result);
    CompilerIrSymbol **RECON_THISCALL FindLink(CompilerIrSymbol *symbol);
    void RECON_THISCALL Serialize(void);
    void RECON_THISCALL Check(void);
    void RECON_THISCALL Link(CompilerIrSymbol *symbol);
};
typedef char CompilerParameterScope_size[sizeof(CompilerParameterScope) == 20 ? 1 : -1];
typedef char CompilerScopeTable_value_offset[offsetof(CompilerScopeTable, parameter_value) == 16 ? 1 : -1];
typedef char CompilerSymbolTable_mask_offset[offsetof(CompilerSymbolTable, bucket_mask) == 20 ? 1 : -1];
struct CompilerSourceFile;
struct CompilerTranslationUnit {
    CompilerSymbolTable *root;
    CompilerScopeTable *current;
    /* Push/pop index this array by a byte depth, checked against128. Only
     * slot0 is written by Initialize; later live slots are written by PushScope.
     * Unwritten slots have no default or equality claim. */
    CompilerScopeTable *scopes[128];
    BYTE depth;
    int lookup_mode;
    CompilerScopeTable *retired_scopes;
    CompilerSymbolBinding *symbols; /* Auxiliary binding chain at+532, also searched for segment names. */
    CompilerSourceFile *source_files;
    CompilerTranslationUnit *RECON_THISCALL Initialize(void);
    CompilerIrSymbol **RECON_THISCALL EndParameterScope(void);
    CompilerIrSymbol **RECON_THISCALL FindName(CompilerNameEntry *);
    CompilerIrSymbol *RECON_THISCALL FindAssemblySymbol(CompilerNameEntry *);
    CompilerIrSymbol **RECON_THISCALL FindTag(CompilerNameEntry *, int);
    CompilerIrSymbol **RECON_THISCALL FindVisibleTag(CompilerNameEntry *);
    CompilerIrSymbol **RECON_THISCALL FindScopedDeclaration(CompilerIrSymbol *);
    void RECON_THISCALL EnterParameterScope(int);
    void RECON_THISCALL EnterBlockScope(void);
    void RECON_THISCALL FinishScope(CompilerScopeTable *);
    void RECON_THISCALL FinishFunction(void);
    void RECON_THISCALL InsertDeclaration(void *);
    CompilerScopeTable *RECON_THISCALL DeclarationScope(void);
    CompilerSourceFile *RECON_THISCALL FindSourceFile(const char *name, unsigned hash);
    CompilerSourceFile *RECON_THISCALL LookupSourceFile(CompilerNameEntry *name);
    CompilerIrSymbol *RECON_THISCALL FindAllocatedSegment(CompilerNameEntry *);
    void RECON_THISCALL PushScope(CompilerScopeTable *);
    void RECON_THISCALL RegisterScope(CompilerScopeTable *);
    void RECON_THISCALL UnregisterScope(void);
    void RECON_THISCALL PopScope(void);
    void RECON_THISCALL Finish(void);
};
typedef char CompilerTranslationUnit_size[sizeof(CompilerTranslationUnit) == 540 ? 1 : -1];
/* Source-only callback presence used by source teardown. No callback is
 * invoked through this view, and current native dependency modes are held. */
struct CompilerDependencyCallbacks {
    void *unmodeled_00[2];
    void *source_ready; /* Presence only: no callback ABI inferred or invoked. */
};
struct CompilerDependency {
    union { void *records; CompilerDependencyCallbacks *callbacks; };
    void RECON_THISCALL NotifySource(void);
    void RECON_THISCALL Finish(int success);
    void RECON_THISCALL SetSourceName(const char *);
    void RECON_THISCALL RecordSource(const char *, int descriptor, int ordinary);
    int RECON_THISCALL QuerySource(void **);
};
#endif
