#ifndef C1XX_FRONTEND_INIT_H
#define C1XX_FRONTEND_INIT_H
#include "frontend.h"
#include <recon.h>

/* Only the encountered virtual slots have semantic names. Unrecovered
 * preceding slots are declarations, never instantiated or given fake bodies. */
struct CompilerBackend {
    virtual void Finish(void) = 0;
};
struct CompilerDebugModule {
    virtual int Finish(void) = 0;
    virtual int Close(void) = 0;
};
struct CompilerInput {
    virtual void UnrecoveredSlot0(void) = 0;
    virtual void UnrecoveredSlot1(void) = 0;
    virtual void UnrecoveredSlot2(void) = 0;
    virtual void UnrecoveredSlot3(void) = 0;
    virtual void Close(void) = 0;
};
/* External MSPDB interface, not a reconstruction of that dependency. */
struct PDB {
    virtual void UnrecoveredSlot0(void) = 0;
    virtual void UnrecoveredSlot1(void) = 0;
    virtual void UnrecoveredSlot2(void) = 0;
    virtual int QueryLastError(char *text) = 0;
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
