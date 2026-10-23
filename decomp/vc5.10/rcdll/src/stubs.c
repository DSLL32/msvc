#include "rcdll.h"
#include "command.h"
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
int __stdcall rc_preprocess(int count, char **arguments, int unused)
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

unsigned __stdcall rc_write_bytes(FILE *file,const void *data,unsigned bytes)
{
    __asm int 3
    return 0;
}
int __stdcall rc_detect_encoding(FILE *file)
{
    __asm int 3
    return 0;
}
int __stdcall rc_read_wide_line(WCHAR *line,unsigned capacity,int wide,FILE *file)
{
    __asm int 3
    return 0;
}
WCHAR *__stdcall rc_skip_white(WCHAR *text,int terminate)
{
    __asm int 3
    return NULL;
}
int __stdcall rc_prefix_equal(const WCHAR *prefix,const WCHAR *text)
{
    __asm int 3
    return 0;
}
void __stdcall rc_notify_progress(WPARAM kind,LPARAM value)
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
