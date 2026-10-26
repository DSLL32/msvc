#ifndef C1_INPUT_H
#define C1_INPUT_H
#include "compiler-state.h"
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

/* Block-backed word stack. The constructor establishes position 4 and a
 * 128-byte block whose first word is the previous-block link. */
/* Initialize writes only the first link. Recorded Trace begins at DWORD5;
 * the intervening four DWORDs remain unmodeled and unwritten. Each later
 * block has the same authentic128-byte extent and twenty-seven token slots. */
struct CompilerContextBlock {
    CompilerContextBlock *next;
    BYTE unmodeled_04[16];
    unsigned tokens[27];
};
typedef char CompilerContextBlock_size[sizeof(CompilerContextBlock) == 128 ? 1 : -1];
typedef char CompilerContextBlock_tokens[offsetof(CompilerContextBlock, tokens) == 20 ? 1 : -1];
struct CompilerContextFrame {
    struct CompilerLexer *stream;
    CompilerContextFrame *next;
    void *object;
    unsigned value;
    unsigned first, second;
};
typedef char CompilerContextFrame_size[sizeof(CompilerContextFrame) == 24 ? 1 : -1];
struct CompilerContextBuffer {
    CompilerContextBlock *block;
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
    /* Other bits and natural padding have no modeled value. */
    unsigned char active : 1;
    unsigned unread_count;
    unsigned unread_tokens[5];
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

typedef char CompilerPlainInput_size[sizeof(CompilerPlainInput) == 8 ? 1 : -1];
typedef char CompilerRecordedInput_size[sizeof(CompilerRecordedInput) == 20 ? 1 : -1];
typedef char CompilerLexer_size[sizeof(CompilerLexer) == 88 ? 1 : -1];
struct CompilerRecordedLexer : CompilerTokenStream {
    void *initial_object;
    unsigned initial_value;
    void *current_object;
    unsigned current_value;
    CompilerContextBuffer buffer, read_buffer, start_buffer;
    unsigned arena_kind;
    unsigned char active : 1;
    RECON_FORCEINLINE CompilerRecordedLexer(void *object, unsigned value, unsigned arena) {
        initial_object = current_object = object;
        initial_value = current_value = value;
        buffer.Initialize(arena);
        read_buffer = start_buffer = buffer;
        arena_kind = arena;
        active = 0;
    }
    unsigned RECON_THISCALL Read(void);
    unsigned RECON_THISCALL Peek(void);
    void RECON_THISCALL Unread(unsigned);
    void RECON_THISCALL Trace(unsigned);
    int RECON_THISCALL Valid(void);
    void RECON_THISCALL Reset(void);
    void *RECON_THISCALL Destroy(unsigned);
    CompilerRecordedLexer *RECON_THISCALL Initialize(unsigned, int);
};
typedef char CompilerRecordedLexer_extent[sizeof(CompilerRecordedLexer) == 64 ? 1 : -1];
typedef char CompilerRecordedLexer_object_offset[offsetof(CompilerRecordedLexer, initial_object) == 4 ? 1 : -1];
typedef char CompilerRecordedLexer_current_offset[offsetof(CompilerRecordedLexer, current_object) == 12 ? 1 : -1];
typedef char CompilerRecordedLexer_buffer_offset[offsetof(CompilerRecordedLexer, buffer) == 20 ? 1 : -1];
typedef char CompilerRecordedLexer_arena_offset[offsetof(CompilerRecordedLexer, arena_kind) == 56 ? 1 : -1];

#endif
