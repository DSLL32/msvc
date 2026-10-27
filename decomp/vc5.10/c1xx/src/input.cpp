#include <new.h>
#include "input.h"
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

int RECON_THISCALL CompilerPlainInput::Begin(const void *context) { return 1; }
int RECON_THISCALL CompilerPlainInput::End(const void *context) { return 1; }
void RECON_THISCALL CompilerPlainInput::Flush(void) {}
void RECON_THISCALL CompilerPlainInput::Mark(void) {}
void RECON_THISCALL CompilerPlainInput::Close(void)
{
    ::operator delete(this);
}
void RECON_THISCALL CompilerPlainInput::Location(unsigned first, unsigned second) {}
int RECON_THISCALL CompilerPlainText::TakeText(BYTE **bytes, unsigned *length) { __asm { int 3 } return 0; }
int RECON_THISCALL CompilerPlainText::MapLocation(unsigned first, unsigned second, unsigned third) { __asm { int 3 } return 0; }
/* PlainText::Reset aliases the single pending CompilerLexer::Reset body. */
int RECON_THISCALL CompilerRecordedInput::Begin(const void *context) { __asm { int 3 } return 0; }
int RECON_THISCALL CompilerRecordedInput::End(const void *context) { __asm { int 3 } return 0; }
void RECON_THISCALL CompilerRecordedInput::Flush(void) { __asm { int 3 } }
void RECON_THISCALL CompilerRecordedInput::Mark(void) { __asm { int 3 } }
void RECON_THISCALL CompilerRecordedInput::Close(void) { __asm { int 3 } }
void RECON_THISCALL CompilerRecordedInput::Location(unsigned first, unsigned second) { __asm { int 3 } }
int RECON_THISCALL CompilerRecordedText::TakeText(BYTE **bytes, unsigned *length) { __asm { int 3 } return 0; }
int RECON_THISCALL CompilerRecordedText::MapLocation(unsigned first, unsigned second, unsigned third) { __asm { int 3 } return 0; }
void RECON_THISCALL CompilerRecordedText::Reset(void) { __asm { int 3 } }
