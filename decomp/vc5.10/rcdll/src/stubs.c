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
void __stdcall rc_write_apstudio_header(FILE *file,DWORD size,WORD id)
{
    __asm int 3
}

void __stdcall rc_copy_resource_token(RcResourceToken *to,const RcResourceToken *from)
{
    __asm int 3
}

void __stdcall rc_resource_input_fatal(unsigned message)
{
    __asm int 3
}
void __stdcall rc_read_apstudio_directive(int unused_mode,DWORD unused_character)
{
    __asm int 3
}
void __stdcall rc_resource_data_fatal(unsigned message)
{
    __asm int 3
}
void __stdcall rc_write_apstudio_symbol(const WCHAR *first,const WCHAR *second,
                                        const void *unused,WORD value,BYTE kind)
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



void __stdcall rc_parse_menu(int extended,RcResource *resource)
{
    __asm int 3
}

void __stdcall rc_resource_error(unsigned code)
{
    __asm int 3
}






void __cdecl rc_flush_font_directory(void)
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



int __stdcall rc_parse_font_resource(RcResource *resource)
{
    __asm int 3
    return 0;
}




long __stdcall rc_copy_riff_resource(long size)
{
    __asm int 3
    return 0;
}

DWORD __stdcall rc_mac_long(DWORD value)
{
    __asm int 3
    return 0;
}

unsigned __stdcall rc_mac_word(WORD value)
{
    __asm int 3
    return 0;
}

void __stdcall rc_mac_group_icon(RcResourceType *type,RcResource *resource)
{
    __asm int 3
}

void __stdcall rc_mac_group_cursor(RcResourceType *type,RcResource *resource)
{
    __asm int 3
}

void __cdecl rc_write_mac_resource_file(void)
{
    __asm int 3
}

void __stdcall rc_parse_toolbar_resource(RcResource *resource)
{
    __asm int 3
}


void __stdcall rc_resource_notice_strings(unsigned code,const WCHAR *first,const WCHAR *second,DWORD extra)
{
    __asm int 3
}


void __stdcall rc_parse_dialog_class(RcDialog *dialog)
{
    __asm int 3
}






void __stdcall rc_patch_control_data_size(WORD size,int enabled)
{
    __asm int 3
}
void __cdecl rc_convert_dialog_count(void)
{
    __asm int 3
}

void __stdcall rc_write_mac_string(const WCHAR *text,int flags,int counted)
{
    __asm int 3
}

void __stdcall rc_concatenate_resource_string(const WCHAR *text,int flags)
{
    __asm int 3
}



char *__stdcall rc_format_resource_ordinal(WORD id,char *text,int radix)
{
    __asm int 3
    return NULL;
}
unsigned __stdcall rc_write_apstudio_string(FILE *file,const WCHAR *text)
{
    __asm int 3
    return 0;
}





void __stdcall rc_read_resource_marker(int mode,DWORD character)
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
