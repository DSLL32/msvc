#ifndef C1XX_FRONTEND_INIT_H
#define C1XX_FRONTEND_INIT_H
#include "frontend.h"
#include "input.h"
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
    virtual void RECON_THISCALL Slot28(unsigned) = 0;
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
struct CompilerTranslationUnit {
    CompilerTranslationUnit *RECON_THISCALL Initialize(void);
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
struct CompilerLookup {
    /* Its genuine initializer remains pending, so no storage is modeled. */
    CompilerLookup *RECON_THISCALL Initialize(CompilerType *, const void *);
    void RECON_THISCALL Resolve(void);
};
struct CompilerSymbol;
#endif
