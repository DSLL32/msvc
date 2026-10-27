/* Escape/refill markers share the actual macro expansion stack. */
#include "frontend-init.h"
#include <recon.h>

extern "C" {
unsigned __cdecl process_trigraph(void); /*2c445*/
int __cdecl splice_input_line(void); /*41155*/
int __cdecl pop_macro_expansion(void); /*f09f*/
void __cdecl refill_lexer_source(void); /*42228*/
void __cdecl record_macro_argument(BYTE *, BYTE *); /*1a16d*/
void __cdecl begin_macro_parameter_replacement(void); /*1a1d8*/
}

extern "C" unsigned __cdecl process_special_character(void)
{
    BYTE previous = frontend_bss.source_cursor[-1], marker;
    int index;
    if (previous == '?') return process_trigraph();
    if (previous == '\\') {
        if (!frontend_bss.macro_expansion_depth && splice_input_line()) return 1;
        return 0x10;
    }
    if (!frontend_bss.macro_expansion_depth) {
        refill_lexer_source();
        return 1;
    }
    marker = *frontend_bss.source_cursor++;
    if (marker == 'D') return pop_macro_expansion() ? 8 : 4;
    if (marker == 'R') {
        index = frontend_bss.macro_expansion_depth;
        if (index < 0 || index >= 256) { __asm { int 3 } }
        CompilerMacroExpansionFrame *frame = &frontend_bss.macro_expansion_frames[index];
        frame->disabled = 0;
        frontend_bss.source_cursor = frame->source;
        if (index == 1) {
            frontend_bss.macro_token_cursor = frontend_bss.work_buffers[0].data;
            frontend_bss.macro_text_cursor = frontend_bss.work_buffers[1].data;
        }
        if (!frontend_bss.suppress_macro_end_notification && frontend_bss.browser_filename)
            frontend_data.backend->Slot2c();
        --frontend_bss.macro_expansion_depth;
        frontend_bss.suppress_macro_end_notification = 0;
        return 0x20;
    }
    if (marker == 'A') {
        index = frontend_bss.macro_expansion_depth;
        if (index < 0 || index >= 256) { __asm { int 3 } }
        record_macro_argument(frontend_bss.macro_expansion_frames[index].text,
            frontend_bss.macro_text_cursor);
        index = frontend_bss.macro_expansion_depth;
        if (index < 0 || index >= 256) { __asm { int 3 } }
        frontend_bss.macro_text_cursor = frontend_bss.macro_expansion_frames[index].text;
        begin_macro_parameter_replacement();
        return 2;
    }
    return 0;
}

extern "C" unsigned __cdecl process_trigraph(void)
{
    BYTE character, replacement;
read_second:
    character = *frontend_bss.source_cursor++;
    if (character != '?') {
        if (!character && !frontend_bss.macro_expansion_depth) {
            refill_lexer_source();
            frontend_bss.source_cursor[-1] = '?';
            goto read_second;
        }
        --frontend_bss.source_cursor;
        return 0x40;
    }
read_third:
    character = *frontend_bss.source_cursor++;
    switch (character) {
    case 0:
        if (!frontend_bss.macro_expansion_depth) {
            refill_lexer_source();
            goto read_third;
        }
        goto not_trigraph;
    case '=': replacement = '#'; break;
    case '(': replacement = '['; break;
    case '/': replacement = '\\'; break;
    case ')': replacement = ']'; break;
    case '\'': replacement = '^'; break;
    case '<': replacement = '{'; break;
    case '!': replacement = '|'; break;
    case '>': replacement = '}'; break;
    case '-': replacement = '~'; break;
    default:
    not_trigraph:
        --frontend_bss.source_cursor;
        frontend_bss.source_cursor[-1] = '?';
        --frontend_bss.source_cursor;
        frontend_bss.source_cursor[-1] = '?';
        return 0x40;
    }
    frontend_bss.source_cursor[-1] = replacement;
    --frontend_bss.source_cursor;
    return 1;
}

extern "C" void __cdecl refill_lexer_source(void) { __asm { int 3 } }
extern "C" void __cdecl record_macro_argument(BYTE *, BYTE *) { __asm { int 3 } }
extern "C" void __cdecl begin_macro_parameter_replacement(void) { __asm { int 3 } }
