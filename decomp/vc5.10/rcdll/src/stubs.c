#include "rcdll.h"
#include "command.h"
#include "preprocess.h"
#include "directives.h"
#include "macro-expansion.h"
#include "resource-output.h"
#include "resource-input.h"
#include "resources.h"
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
void __stdcall rc_resource_input_fatal(unsigned message)
{
    __asm int 3
}
void __stdcall rc_resource_data_fatal(unsigned message)
{
    __asm int 3
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

int __stdcall rc_change_codepage(UINT page)
{
    __asm int 3
    return 0;
}

void __cdecl rc_line_comment(void)
{
    __asm int 3
}





void __stdcall rc_macro_buffer_fatal(unsigned message)
{
    __asm int 3
}



void __stdcall rc_resource_error(unsigned code)
{
    __asm int 3
}

void __stdcall rc_report_resource_error(unsigned code)
{
    __asm int 3
}

long __stdcall rc_convert_icon_file(long size)
{
    __asm int 3
    return 0;
}

void __stdcall rc_concatenate_resource_string(const WCHAR *text,int flags)
{
    __asm int 3
}

void __stdcall rc_resource_error_name(unsigned code,const void *name)
{
    __asm int 3
}

void __stdcall rc_resource_file_fatal(unsigned code,const WCHAR *name)
{
    __asm int 3
}

DWORD __cdecl rc_parse_resource_octal(void)
{
    __asm int 3
    return 0;
}

void __stdcall rc_resource_input_fatal_name(unsigned code,const WCHAR *name)
{
    __asm int 3
}

void __stdcall rc_resource_file_warning(unsigned code,const WCHAR *name)
{
    __asm int 3
}

DWORD __stdcall rc_mac_color_brightness(const WORD *rgb)
{ __asm int 3
  return 0;
}

void __stdcall rc_reduce_cursor_width(BYTE *out,const BYTE *in,int stride,int height)
{ __asm int 3 }
void __stdcall rc_reduce_cursor_height(BYTE *out,const BYTE *in,int stride,int height,int scale)
{ __asm int 3 }

void __stdcall rc_map_mac_bytes(char *text,int bytes)
{ __asm int 3 }
