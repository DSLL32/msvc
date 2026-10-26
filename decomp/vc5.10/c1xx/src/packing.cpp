#include <stddef.h>
#include <string.h>
#include "frontend.h"
#include "packing.h"
#include "parser-state.h"
#include <recon.h>
#pragma intrinsic(strcmp)

typedef char packing_stack_layout[(sizeof(CompilerPacking) == 8 &&
    offsetof(CompilerPacking, available) == 4) ? 1 : -1];
typedef char packing_frame_layout[(sizeof(PackingFrame) == 12 &&
    offsetof(PackingFrame, alignment) == 4 &&
    offsetof(PackingFrame, identifier) == 8) ? 1 : -1];

extern "C" BYTE *allocate_packing_storage(unsigned bytes);
extern "C" PackingFrame *push_packing(unsigned alignment, void *identifier);
extern "C" PackingFrame *clear_packing_frame(PackingFrame *frame);

extern "C" void initialize_packing(void)
{
    PackingFrame *frame;
    unsigned alignment;

    frontend_data.options.packing_stack =
        (CompilerPacking *)allocate_packing_storage(sizeof(CompilerPacking));
    frame = frontend_data.options.packing_stack->AcquireFrame();
    alignment = frontend_data.options.packing_alignment == -1 ?
        8 : frontend_data.options.packing_alignment;
    frame->alignment = alignment;
    frame->identifier = NULL;
    push_packing(0, NULL);
}

extern "C" BYTE *allocate_packing_storage(unsigned bytes)
{
    CompilerArena *arena = &frontend_bss.arenas[0];
    BYTE *result = arena->cursor;

    arena->cursor += (bytes + 3) & ~3U;
    if (arena->cursor > arena->committed_limit)
        arena->CommitExtent();
    return result;
}

PackingFrame *RECON_THISCALL CompilerPacking::AcquireFrame(void)
{
    PackingFrame *frame = available;

    if (frame) {
        available = frame->previous;
        clear_packing_frame(frame);
    } else {
        CompilerArena *arena = &frontend_bss.arenas[0];
        frame = (PackingFrame *)arena->cursor;
        arena->cursor += sizeof(PackingFrame);
        if (arena->cursor > arena->committed_limit)
            arena->CommitExtent();
    }
    frame->previous = top;
    top = frame;
    return frame;
}

extern "C" PackingFrame *push_packing(unsigned alignment, void *identifier)
{
    CompilerPacking *stack = frontend_data.options.packing_stack;
    PackingFrame *frame;

    if (!alignment)
        alignment = stack->top->alignment;
    frame = stack->AcquireFrame();
    frame->alignment = alignment;
    frame->identifier = identifier;
    return frame;
}

extern "C" PackingFrame *clear_packing_frame(PackingFrame *frame)
{
    __asm { int 3 }
    return frame;
}

extern "C" ParserTokenView *__cdecl expect_directive_token(BYTE);
extern "C" unsigned __cdecl validate_packing_alignment(unsigned); /*410e8*/
extern "C" void __cdecl pop_packing(unsigned, void *); /*42489; caller ignores EAX*/

extern "C" void __cdecl pop_packing(unsigned alignment, void *identifier)
{
    CompilerPacking *stack = frontend_data.options.packing_stack;
    PackingFrame *frame = stack->top;
    if (!frame->previous || !frame->previous->previous) {
        report_language_diagnostic(3, 0xa1);
        if (!alignment) alignment = frontend_data.options.packing_alignment == -1 ?
            8 : frontend_data.options.packing_alignment;
    } else {
        stack->PopFrame();
        while (frame->identifier != identifier) {
            if (!identifier) {
                report_language_diagnostic(3, 0x9f, frame->identifier);
                break;
            }
            /* PopFrame has changed the captured frame's link to the free
             * list. Read that actual post-call link, not a saved stack link. */
            if (!frame->previous) {
                report_language_diagnostic(3, 0xa0, identifier);
                if (!alignment) alignment = frontend_data.options.packing_alignment == -1 ?
                    8 : frontend_data.options.packing_alignment;
                break;
            }
            stack = frontend_data.options.packing_stack;
            frame = stack->top;
            stack->PopFrame();
        }
    }
    if (alignment) frame->alignment = alignment;
}
void RECON_THISCALL CompilerPacking::PopFrame(void)
{
    if (this && top) {
        PackingFrame *frame = top;
        top = frame->previous;
        frame->previous = available;
        available = frame;
    }
}

/* pack([push|pop,] [identifier,] [alignment]). The numeric spelling already
 * carries its real type/value arm; this routine consumes the low DWORD. */
extern "C" void __cdecl process_pack_pragma(void)
{
    ParserTokenView *token;
    unsigned operation = 0, alignment = 0;
    void *identifier = NULL;
    const char *word;
    PackingFrame *frame;
    if (!expect_directive_token(118)) return;
    token = ((ParserFrontendContextView *)&frontend_bss.saved_context_stack)->ReadToken();
    if (token->kind == 6) {
        word = ((CompilerNameEntry *)token->value.pointer)->name;
        if (!strcmp(word, "push")) operation = 1;
        else if (!strcmp(word, "pop")) operation = 2;
        else { report_language_diagnostic(1,79,6); return; }
        token = ((ParserFrontendContextView *)&frontend_bss.saved_context_stack)->ReadToken();
        if (token->kind == 126) goto apply;
        ((CompilerTokenStream *)frontend_bss.saved_context_stack.top->object)->Unread((unsigned)token);
        if (!expect_directive_token(55)) return;
        token = ((ParserFrontendContextView *)&frontend_bss.saved_context_stack)->ReadToken();
        if (token->kind == 6) {
            identifier = token->value.pointer;
            token = ((ParserFrontendContextView *)&frontend_bss.saved_context_stack)->ReadToken();
            if (token->kind == 126) goto apply;
            ((CompilerTokenStream *)frontend_bss.saved_context_stack.top->object)->Unread((unsigned)token);
            if (!expect_directive_token(55)) return;
            token = expect_directive_token(101);
            if (!token) return;
            goto numeric_alignment;
        }
        if (token->kind == 101) goto numeric_alignment;
        /* The original applies the operation for other tokens here too. */
        goto apply;
    }
    if (token->kind == 101) goto numeric_alignment;
    if (token->kind == 126) {
        alignment = frontend_data.options.packing_alignment == -1 ? 8 : frontend_data.options.packing_alignment;
        goto apply;
    }
    ((CompilerTokenStream *)frontend_bss.saved_context_stack.top->object)->Unread((unsigned)token);
    expect_directive_token(126);
    return;
numeric_alignment:
    alignment = validate_packing_alignment((unsigned)token->value.literal.value);
    if (!expect_directive_token(126)) return;
apply:
    if (operation == 1) push_packing(alignment, identifier);
    else if (operation == 2) pop_packing(alignment, identifier);
    else {
        /* Capture the frame before validation, as the original does. A
         * numeric argument is deliberately validated again on this path. */
        frame = frontend_data.options.packing_stack->top;
        alignment = validate_packing_alignment(alignment);
        frame->alignment = alignment;
    }
}

extern "C" unsigned __cdecl validate_packing_alignment(unsigned alignment)
{
    if (alignment == 1 || alignment == 2 || alignment == 4 ||
        alignment == 8 || alignment == 16)
        return alignment;
    report_language_diagnostic(1, 86);
    /* The diagnostic can change the stack. Read the fallback only afterward. */
    return frontend_data.options.packing_stack->top->alignment;
}
