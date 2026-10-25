#include "rcdll.h"
#include "command.h"
#include "preprocess.h"
#include "directives.h"
#include "macro-expansion.h"
#include "resource-output.h"
#include "resource-input.h"
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
void __stdcall rc_write_apstudio_header(FILE *file,DWORD size,WORD id)
{
    __asm int 3
}
unsigned __cdecl rc_read_resource_code_unit(void)
{
    __asm int 3
    return 0;
}
void __stdcall rc_copy_resource_token(RcResourceToken *to,const RcResourceToken *from)
{
    __asm int 3
}
unsigned __stdcall rc_scan_resource_token(unsigned flags)
{
    __asm int 3
    return 0;
}
void __stdcall rc_resource_input_fatal(unsigned message)
{
    __asm int 3
}
void __stdcall rc_read_apstudio_directive(int unused_mode,DWORD unused_character)
{
    __asm int 3
}
void *__stdcall rc_reserve_resource_bytes(WORD bytes)
{
    __asm int 3
    return NULL;
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














void __cdecl rc_skip_line(void)
{
    __asm int 3
    
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






void __stdcall rc_macro_buffer_fatal(unsigned message)
{
    __asm int 3
}






void __stdcall rc_emit_macro_reference(RcMacro *macro)
{
    __asm int 3
}





void __stdcall rc_append_macro_text(const WCHAR *text)
{
    __asm int 3
}

