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
    virtual void RECON_THISCALL SetSymbolId(void *, unsigned) = 0;
    virtual void RECON_THISCALL Slot58(void) = 0;
    virtual void RECON_THISCALL Slot5c(void) = 0;
};
struct CompilerDebugModule {
    virtual int RECON_THISCALL Finish(void) = 0;
    virtual int RECON_THISCALL Close(void) = 0;
    virtual void RECON_THISCALL Slot08(void) = 0;
    virtual void RECON_THISCALL Slot0c(void) = 0;
    virtual void RECON_THISCALL Slot10(void) = 0;
    virtual void RECON_THISCALL Slot14(void) = 0;
    virtual void RECON_THISCALL Slot18(void) = 0;
    virtual void RECON_THISCALL Slot1c(void) = 0;
    virtual void RECON_THISCALL Slot20(void) = 0;
    virtual void RECON_THISCALL Slot24(void) = 0;
    virtual void RECON_THISCALL Slot28(void) = 0;
    virtual void RECON_THISCALL Slot2c(void) = 0;
    virtual int RECON_THISCALL Slot30(const char *, int) = 0;
    virtual void RECON_THISCALL Slot34(void) = 0;
    virtual void RECON_THISCALL Slot38(void) = 0;
    virtual void RECON_THISCALL Slot3c(void) = 0;
    virtual void RECON_THISCALL Slot40(void) = 0;
    virtual void RECON_THISCALL Slot44(void) = 0;
    virtual int RECON_THISCALL Slot48(void) = 0;
    virtual void RECON_THISCALL Slot4c(void) = 0;
    virtual void RECON_THISCALL Slot50(void) = 0;
    virtual void RECON_THISCALL Slot54(void) = 0;
    virtual void RECON_THISCALL Slot58(void) = 0;
    virtual void RECON_THISCALL Slot5c(void) = 0;
    virtual void RECON_THISCALL Slot60(void) = 0;
    virtual void RECON_THISCALL Slot64(void) = 0;
    virtual void RECON_THISCALL Slot68(void) = 0;
    virtual void RECON_THISCALL Slot6c(void) = 0;
    virtual void RECON_THISCALL Slot70(void) = 0;
    virtual int RECON_THISCALL Slot74(const char *) = 0;
    virtual void RECON_THISCALL Slot78(void) = 0;
    virtual void RECON_THISCALL Slot7c(void) = 0;
    virtual void RECON_THISCALL Slot80(void) = 0;
    virtual void RECON_THISCALL Slot84(void) = 0;
    virtual int RECON_THISCALL Slot88(void *) = 0;
};
/* External MSPDB interface; preceding slots remain opaque declarations. */
struct PDB {
    virtual void RECON_THISCALL UnrecoveredSlot0(void) = 0;
    virtual void RECON_THISCALL UnrecoveredSlot1(void) = 0;
    virtual void RECON_THISCALL UnrecoveredSlot2(void) = 0;
    virtual int RECON_THISCALL QueryLastError(char *text) = 0;
};
/* Scope-table interface at original RVA5b480, in its ten-slot order. Only
 * construction is reached so far; the individual operations remain pending. */
struct CompilerSymbolTable {
    CompilerSymbolTable *next;
    BYTE level;
    CompilerSymbolTable *outer;
    CompilerSymbolBinding **buckets;
    unsigned bucket_mask;
    RECON_FORCEINLINE CompilerSymbolTable()
        : next(NULL), level(0), outer(NULL), bucket_mask(1023) {}
    virtual int RECON_THISCALL HasBuckets(void);
    virtual void RECON_THISCALL Insert(CompilerIrSymbol *symbol);
    virtual void *RECON_THISCALL Find(CompilerNameEntry *name);
    virtual void *RECON_THISCALL Lookup(CompilerNameEntry *name);
    virtual void *RECON_THISCALL FindOrdinary(CompilerNameEntry *name);
    virtual void RECON_THISCALL FindNested(CompilerNameEntry *name, void **result);
    virtual CompilerIrSymbol **RECON_THISCALL FindLink(CompilerIrSymbol *symbol);
    virtual void RECON_THISCALL Serialize(void);
    virtual void RECON_THISCALL Check(void);
    virtual void RECON_THISCALL Link(CompilerIrSymbol *symbol);
};
typedef char CompilerSymbolTable_size[sizeof(CompilerSymbolTable) == 24 ? 1 : -1];
struct CompilerSourceFile;
struct CompilerTranslationUnit {
    CompilerSymbolTable *root, *current;
    /* Push/pop index this array by a byte depth, checked against128. Only
     * slot0 is written by Initialize; other values remain unmodeled. */
    CompilerSymbolTable *scopes[128];
    BYTE depth;
    int lookup_mode;
    CompilerSymbolTable *retired_scopes;
    CompilerIrSymbol *symbols;
    CompilerSourceFile *source_files;
    CompilerTranslationUnit *RECON_THISCALL Initialize(void);
    CompilerIrSymbol **RECON_THISCALL EndParameterScope(void);
    CompilerSourceFile *RECON_THISCALL FindSourceFile(const char *name, unsigned hash);
    CompilerSourceFile *RECON_THISCALL LookupSourceFile(CompilerNameEntry *name);
    void RECON_THISCALL PushScope(CompilerSymbolTable *);
    void RECON_THISCALL Finish(void);
};
typedef char CompilerTranslationUnit_size[sizeof(CompilerTranslationUnit) == 540 ? 1 : -1];
struct CompilerDependency {
    void *records;
    void RECON_THISCALL Finish(int success);
    void RECON_THISCALL RecordSource(const char *, int descriptor, int ordinary);
    int RECON_THISCALL QuerySource(void **);
};
#endif
