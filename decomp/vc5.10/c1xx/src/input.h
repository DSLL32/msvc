#ifndef C1XX_INPUT_H
#define C1XX_INPUT_H
#include "frontend.h"
#include <recon.h>

/* The two input interfaces occupy separate four-byte subobjects. Composition
 * keeps the secondary receiver at owner+4 under both C++ ABIs, avoiding a
 * compiler-generated adjustment thunk absent from the original graph. */
struct CompilerInput {
    RECON_FORCEINLINE CompilerInput() {}
    virtual int RECON_THISCALL Begin(const void *) = 0;
    virtual int RECON_THISCALL End(const void *) = 0;
    virtual void RECON_THISCALL Flush(void) = 0;
    virtual void RECON_THISCALL Mark(void) = 0;
    virtual void RECON_THISCALL Close(void) = 0;
    virtual void RECON_THISCALL Location(unsigned, unsigned) = 0;
};
struct CompilerTextInput {
    RECON_FORCEINLINE CompilerTextInput() {}
    virtual int RECON_THISCALL TakeText(BYTE **, unsigned *) = 0;
    virtual int RECON_THISCALL MapLocation(unsigned, unsigned, unsigned) = 0;
    virtual void RECON_THISCALL Reset(void) = 0;
};
struct CompilerPlainText : CompilerTextInput {
    RECON_FORCEINLINE CompilerPlainText() {}
    int RECON_THISCALL TakeText(BYTE **, unsigned *);
    int RECON_THISCALL MapLocation(unsigned, unsigned, unsigned);
    void RECON_THISCALL Reset(void);
};
struct CompilerPlainInput : CompilerInput {
    CompilerPlainText text;
    RECON_FORCEINLINE CompilerPlainInput() {}
    int RECON_THISCALL Begin(const void *);
    int RECON_THISCALL End(const void *);
    void RECON_THISCALL Flush(void);
    void RECON_THISCALL Mark(void);
    void RECON_THISCALL Close(void);
    void RECON_THISCALL Location(unsigned, unsigned);
};
struct CompilerRecordedText : CompilerTextInput {
    RECON_FORCEINLINE CompilerRecordedText() {}
    int RECON_THISCALL TakeText(BYTE **, unsigned *);
    int RECON_THISCALL MapLocation(unsigned, unsigned, unsigned);
    void RECON_THISCALL Reset(void);
};
struct CompilerRecordedInput : CompilerInput {
    CompilerRecordedText text;
    BYTE *token_bytes;
    unsigned token_length;
    /* Begin/Mark supplies this position before use, not the constructor. */
    unsigned token_start;
    RECON_FORCEINLINE CompilerRecordedInput()
        : token_bytes((BYTE *)-1), token_length(0) {}
    int RECON_THISCALL Begin(const void *);
    int RECON_THISCALL End(const void *);
    void RECON_THISCALL Flush(void);
    void RECON_THISCALL Mark(void);
    void RECON_THISCALL Close(void);
    void RECON_THISCALL Location(unsigned, unsigned);
};

#endif
