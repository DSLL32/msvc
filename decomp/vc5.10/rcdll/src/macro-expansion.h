#ifndef RCDLL_MACRO_EXPANSION_H
#define RCDLL_MACRO_EXPANSION_H
#include <stddef.h>
#include "directives.h"
#include <recon.h>

typedef struct RcMacroFrame {
    WCHAR *source,*output,*number;
    RcMacro *macro;
    unsigned char arguments,expanded;
    unsigned short reserved;
} RcMacroFrame;
typedef char RcMacroFrameLayout[
    sizeof(RcMacroFrame)==20 && offsetof(RcMacroFrame,macro)==12 &&
    offsetof(RcMacroFrame,arguments)==16 && offsetof(RcMacroFrame,reserved)==18
    ? 1 : -1];
extern RcMacroFrame rc_macro_frames[64];
extern WCHAR *rc_saved_macro_number,*rc_saved_macro_output;
extern int rc_expansion_line,rc_expanded_argument_count;
void __stdcall rc_push_macro(RcMacro *macro);
void __cdecl rc_select_macro_text(void);
int __cdecl rc_finish_macro_frame(void);
void __stdcall rc_expand_macro_arguments(RcMacro *macro,int count);
int __cdecl rc_macro_is_wide(void);
void __stdcall rc_copy_expanded_quoted(WCHAR delimiter,int wide);
#endif
