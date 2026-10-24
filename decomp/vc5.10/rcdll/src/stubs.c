#include "rcdll.h"
#include "command.h"
#include "preprocess.h"
#include "directives.h"
#include "macro-expansion.h"
#include <recon.h>

/* Named, ABI-correct INT3 stubs. None is accepted reconstruction. */
BOOL WINAPI Handler(DWORD event)
{
    __asm int 3
    return FALSE;
}
void __stdcall rc_fatal(char *message)
{
    __asm int 3
}
long __stdcall rc_seek(FILE *file, long offset, int origin)
{
    __asm int 3
    return 0;
}
int __stdcall rc_initialize_resource(void)
{
    __asm int 3
    return 0;
}
int __stdcall rc_initialize_input(FILE *file)
{
    __asm int 3
    return 0;
}
int __stdcall rc_parse_resources(void)
{
    __asm int 3
    return 0;
}
int __stdcall rc_flush_resources(FILE *file)
{
    __asm int 3
    return 0;
}
unsigned __stdcall rc_align_output(FILE *file)
{
    __asm int 3
    return 0;
}
void __stdcall rc_cleanup(void)
{
    __asm int 3
}

WCHAR *__stdcall rc_skip_white(WCHAR *text,int terminate)
{
    __asm int 3
    return NULL;
}
void __stdcall rc_input_error(unsigned code)
{
    __asm int 3
}
void __stdcall rc_input_fatal(unsigned code)
{
    __asm int 3
}
void __stdcall rc_input_warning(unsigned code)
{
    __asm int 3
}

int __stdcall rc_match_option(const WCHAR *pattern,const WCHAR *argument)
{
    __asm int 3
    return 0;
}
void __cdecl rc_undefine_macro(void)
{
    __asm int 3
}
void __cdecl rc_run_preprocessor(void)
{
    __asm int 3
}
void __cdecl rc_close_preprocessor(void)
{
    __asm int 3
}

int __stdcall rc_macro_text_differs(const WCHAR *value,const WCHAR *prior,unsigned length)
{
    __asm int 3
    return 0;
}




int __cdecl rc_splice_line(void)
{
    __asm int 3
    return 0;
}






int __stdcall rc_skip_inactive(int level)
{
    __asm int 3
    return 0;
}


void __cdecl rc_skip_line(void)
{
    __asm int 3
    
}


int __cdecl rc_search_current_directory(void)
{
    __asm int 3
    return 0;
}


UINT __cdecl rc_parse_pragma(void)
{
    __asm int 3
    return 0;
}

int __stdcall rc_change_codepage(UINT page)
{
    __asm int 3
    return 0;
}



void __cdecl rc_line_comment(void)
{
    __asm int 3
}

void __cdecl rc_block_comment(void)
{
    __asm int 3
}

int __cdecl rc_open_source_frame(void)
{
    __asm int 3
    return 0;
}

void __stdcall rc_copy_quoted(WCHAR delimiter)
{
    __asm int 3
}

unsigned char __cdecl rc_read_character_constant(void)
{
    __asm int 3
    return 0;
}

unsigned __stdcall rc_escape_character(unsigned character)
{
    __asm int 3
    return 0;
}

unsigned char __stdcall rc_read_fraction(WCHAR *end)
{
    __asm int 3
    return 0;
}

int __stdcall rc_read_macro_identifier(WCHAR first)
{
    __asm int 3
    return 0;
}


unsigned char __stdcall rc_based_integer_kind(int value)
{
    __asm int 3
    return 0;
}

unsigned char __stdcall rc_long_integer_kind(int value)
{
    __asm int 3
    return 0;
}

unsigned char __stdcall rc_unsigned_integer_kind(unsigned value)
{
    __asm int 3
    return 0;
}



void __cdecl rc_skip_line_comment(void)
{
    __asm int 3
}

void __cdecl rc_parse_macro_parameters(void)
{
    __asm int 3
}


WCHAR *__stdcall rc_copy_replacement_quoted(WCHAR *cursor,WCHAR delimiter)
{
    __asm int 3
    return NULL;
}

void __stdcall rc_macro_buffer_fatal(unsigned message)
{
    __asm int 3
}

int __stdcall rc_collect_macro_identifier(WCHAR first)
{
    __asm int 3
    return 0;
}

void __stdcall rc_expand_macro_arguments(RcMacro *macro,int count)
{
    __asm int 3
}
int __cdecl rc_macro_is_wide(void)
{
    __asm int 3
    return 0;
}
void __stdcall rc_copy_expanded_quoted(WCHAR delimiter,int wide)
{
    __asm int 3
}

void __stdcall rc_pack_macro_argument(const WCHAR *begin,const WCHAR *end)
{
    __asm int 3
}
void __cdecl rc_advance_macro_text(void)
{
    __asm int 3
}

void __stdcall rc_emit_macro_reference(RcMacro *macro)
{
    __asm int 3
}
