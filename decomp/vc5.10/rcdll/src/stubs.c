#include "rcdll.h"
#include "command.h"
#include "preprocess.h"
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
void __stdcall rc_list_macro(RcMacro *macro)
{
    __asm int 3
}

int __cdecl rc_refill_source(void)
{
    __asm int 3
    return 0;
}
WCHAR *__stdcall rc_duplicate_source_name(const WCHAR *path)
{
    __asm int 3
    return NULL;
}
void __cdecl rc_emit_source_line(void)
{
    __asm int 3
}
void __stdcall rc_write_preprocessed(const void *data,unsigned size,unsigned count,FILE *file)
{
    __asm int 3
}
void __cdecl rc_scan_line_start(void)
{
    __asm int 3
}
