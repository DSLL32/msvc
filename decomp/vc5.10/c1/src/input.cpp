#include <new.h>
#include "frontend.h"
#include <recon.h>

extern "C" int __cdecl create_input(int record, CompilerInput **input,
                                    CompilerTextInput **text)
{
    if (record) {
        void *memory = ::operator new(20);
        CompilerRecordedInput *object = memory ? new(memory) CompilerRecordedInput : NULL;
        *input = object;
        *text = object ? &object->text : NULL;
    } else {
        void *memory = ::operator new(8);
        CompilerPlainInput *object = memory ? new(memory) CompilerPlainInput : NULL;
        *input = object;
        *text = object ? &object->text : NULL;
    }
    /* Allocation failure is represented by the two NULL outputs, not FALSE. */
    return 1;
}

int RECON_THISCALL CompilerPlainInput::Begin(const void *context) { __asm { int 3 } return 0; }
int RECON_THISCALL CompilerPlainInput::End(const void *context) { __asm { int 3 } return 0; }
void RECON_THISCALL CompilerPlainInput::Flush(void) { __asm { int 3 } }
void RECON_THISCALL CompilerPlainInput::Mark(void) { __asm { int 3 } }
void RECON_THISCALL CompilerPlainInput::Close(void) { __asm { int 3 } }
void RECON_THISCALL CompilerPlainInput::Location(unsigned first, unsigned second) { __asm { int 3 } }
int RECON_THISCALL CompilerPlainText::TakeText(BYTE **bytes, unsigned *length) { __asm { int 3 } return 0; }
int RECON_THISCALL CompilerPlainText::MapLocation(unsigned first, unsigned second, unsigned third) { __asm { int 3 } return 0; }
/* Reset shares the original Lexer::Reset body through the alias identity map. */
int RECON_THISCALL CompilerRecordedInput::Begin(const void *context) { __asm { int 3 } return 0; }
int RECON_THISCALL CompilerRecordedInput::End(const void *context) { __asm { int 3 } return 0; }
void RECON_THISCALL CompilerRecordedInput::Flush(void) { __asm { int 3 } }
void RECON_THISCALL CompilerRecordedInput::Mark(void) { __asm { int 3 } }
void RECON_THISCALL CompilerRecordedInput::Close(void) { __asm { int 3 } }
void RECON_THISCALL CompilerRecordedInput::Location(unsigned first, unsigned second) { __asm { int 3 } }
int RECON_THISCALL CompilerRecordedText::TakeText(BYTE **bytes, unsigned *length) { __asm { int 3 } return 0; }
int RECON_THISCALL CompilerRecordedText::MapLocation(unsigned first, unsigned second, unsigned third) { __asm { int 3 } return 0; }
void RECON_THISCALL CompilerRecordedText::Reset(void) { __asm { int 3 } }

CompilerContextBuffer *RECON_THISCALL CompilerContextBuffer::Initialize(unsigned arena)
{ __asm { int 3 } return this; }
void RECON_THISCALL CompilerContextState::Push(CompilerLexer *lexer, unsigned arena,
    unsigned first, unsigned second, const CompilerContextState *source)
{ __asm { int 3 } }
unsigned RECON_THISCALL CompilerLexer::Read(void) { __asm { int 3 } return 0; }
unsigned RECON_THISCALL CompilerLexer::Peek(void) { __asm { int 3 } return 0; }
void RECON_THISCALL CompilerLexer::Unread(unsigned token) { __asm { int 3 } }
void RECON_THISCALL CompilerLexer::Trace(unsigned token) { __asm { int 3 } }
int RECON_THISCALL CompilerLexer::Valid(void) { __asm { int 3 } return 0; }
void RECON_THISCALL CompilerLexer::Reset(void) { __asm { int 3 } }
void *RECON_THISCALL CompilerLexer::Destroy(unsigned flags) { __asm { int 3 } return this; }
