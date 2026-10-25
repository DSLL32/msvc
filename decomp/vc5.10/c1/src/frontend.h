#ifndef C1_FRONTEND_H
#define C1_FRONTEND_H
#include <stdio.h>
#include "compiler-state.h"
#include "input.h"
#include <recon.h>

/* Preserve C1's direct MSVCRT FILE-array import rather than the SDK's /MD
 * __p__iob accessor. Native stdout retains the shared shim declaration. */
#if defined(_MSC_VER) && !defined(RECON_NATIVE_TEST)
#undef _iob
#undef stdout
extern "C" __declspec(dllimport) FILE _iob[];
#define stdout (&_iob[1])
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
};
/* External MSPDB interface; preceding slots remain opaque declarations. */
struct PDB {
    virtual void RECON_THISCALL UnrecoveredSlot0(void) = 0;
    virtual void RECON_THISCALL UnrecoveredSlot1(void) = 0;
    virtual void RECON_THISCALL UnrecoveredSlot2(void) = 0;
    virtual int RECON_THISCALL QueryLastError(char *text) = 0;
};
struct CompilerTranslationUnit {
    CompilerTranslationUnit *RECON_THISCALL Initialize(void);
    void RECON_THISCALL Finish(void);
};
struct CompilerDependency {
    void *records;
    void RECON_THISCALL Finish(int success);
};
#endif
