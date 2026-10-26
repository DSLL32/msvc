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

int RECON_THISCALL CompilerPlainInput::Begin(const void *context) { return 1; }
int RECON_THISCALL CompilerPlainInput::End(const void *context) { return 1; }
void RECON_THISCALL CompilerPlainInput::Flush(void) {}
void RECON_THISCALL CompilerPlainInput::Mark(void) {}
void RECON_THISCALL CompilerPlainInput::Close(void) { ::operator delete(this); }
void RECON_THISCALL CompilerPlainInput::Location(unsigned first, unsigned second) {}
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
{
#ifdef RECON_NATIVE_TEST
    if (arena >= 6) recon_unsupported("context buffer: unmodeled allocation arena");
#endif
    CompilerArena *owner = &c1_bss.arenas[arena];
    position = 4;
    CompilerContextBlock *memory = (CompilerContextBlock *)owner->cursor;
    owner->cursor += 128;
    if (owner->cursor > owner->committed_limit) owner->CommitExtent();
    if (memory) memory->next = NULL;
    block = memory;
    arena_kind = arena;
    return this;
}
void RECON_THISCALL CompilerContextState::Push(CompilerLexer *lexer, unsigned arena,
    unsigned first, unsigned second, const CompilerContextState *source)
{
    if (arena == 6) arena = lexer->arena_kind;
#ifdef RECON_NATIVE_TEST
    if (arena >= 6) recon_unsupported("context push: unmodeled allocation arena");
#endif
    CompilerArena *owner = &c1_bss.arenas[arena];
    CompilerContextFrame *frame = (CompilerContextFrame *)owner->cursor;
    owner->cursor += 24;
    if (owner->cursor > owner->committed_limit) owner->CommitExtent();
    if (frame) {
        frame->stream = lexer;
        frame->next = saved_frame;
        frame->object = source->object;
        frame->value = source->value;
        frame->first = first;
        frame->second = second;
    }
    saved_frame = frame;
    c1_bss.current_context.object = lexer->initial_object;
    c1_bss.current_context.value = lexer->initial_value;
}
unsigned RECON_THISCALL CompilerLexer::Peek(void) { __asm { int 3 } return 0; }
void RECON_THISCALL CompilerLexer::Unread(unsigned token)
{
#ifdef RECON_NATIVE_TEST
    if (unread_count >= 5) recon_unsupported("lexer unread stack exceeds owned storage");
#endif
    unread_tokens[unread_count++] = token;
}
void RECON_THISCALL CompilerLexer::Trace(unsigned token) { __asm { int 3 } }
int RECON_THISCALL CompilerLexer::Valid(void) { return 1; }
/* Canonical physical RET0 body, shared by the declared no-op aliases. */
void RECON_THISCALL CompilerLexer::Reset(void) {}
void *RECON_THISCALL CompilerLexer::Destroy(unsigned flags) { __asm { int 3 } return this; }
