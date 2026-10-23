#include "rcdll.h"
#include "command.h"
#include "preprocess.h"
#include "directives.h"
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




unsigned short __stdcall rc_character_class(unsigned short character)
{
    __asm int 3
    return 0;
}
int __cdecl rc_scan_comment(void)
{
    __asm int 3
    return 0;
}
int __cdecl rc_splice_line(void)
{
    __asm int 3
    return 0;
}

WCHAR __cdecl rc_read_nonspace(void)
{
    __asm int 3
    return 0;
}

void __stdcall rc_read_identifier(WCHAR character)
{
    __asm int 3
    
}

unsigned char __stdcall rc_directive_code(const WCHAR *word)
{
    __asm int 3
    return 0;
}

void __stdcall rc_expect_end(const WCHAR *word)
{
    __asm int 3
    
}

int __stdcall rc_skip_inactive(int level)
{
    __asm int 3
    return 0;
}

int __cdecl rc_push_condition(void)
{
    __asm int 3
    return 0;
}

int __cdecl rc_evaluate_condition(void)
{
    __asm int 3
    return 0;
}

void __cdecl rc_parse_define(void)
{
    __asm int 3
    
}

void __cdecl rc_next_token(void)
{
    __asm int 3
    
}

void __cdecl rc_skip_line(void)
{
    __asm int 3
    
}

WCHAR * __cdecl rc_parse_angle_name(void)
{
    __asm int 3
    return NULL;
}

int __cdecl rc_search_current_directory(void)
{
    __asm int 3
    return 0;
}

void __cdecl rc_search_include_directories(void)
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

WCHAR __cdecl rc_get_logical_character(void)
{
    __asm int 3
    return 0;
}
