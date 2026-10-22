/* Named graph vertices pending reconstruction at their paired frontiers. */
#include "resource.h"

void __cdecl advance_argument(void)
{
    __asm int 3
}

WORD __cdecl parse_word_expression(void)
{
    __asm int 3
    return 0;
}

DWORD __stdcall parse_style(DWORD initial)
{
    __asm int 3
    return 0;
}

BOOL __stdcall parse_string_table(struct ResourceInfo *resource)
{
    __asm int 3
    return 0;
}

void __cdecl emit_font_directory(void)
{
    __asm int 3
}

void __stdcall emit_string_tables(struct ResourceType *type)
{
    __asm int 3
}

void __stdcall parse_raw_data(struct ResourceInfo *resource)
{
    __asm int 3
}

unsigned int __cdecl read_resource_file(void)
{
    __asm int 3
    return 0;
}

unsigned int __stdcall read_icon_file_header(unsigned int size)
{
    __asm int 3
    return 0;
}

unsigned int __cdecl read_bitmap_file_header(void)
{
    __asm int 3
    return 0;
}

void __stdcall parse_menu(int depth, struct ResourceInfo *resource)
{
    __asm int 3
}

void __stdcall parse_dialog(struct ResourceInfo *resource)
{
    __asm int 3
}

void __stdcall write_resource(struct ResourceType *type, struct ResourceInfo *resource)
{
    __asm int 3
}

BOOL __stdcall parse_font(struct ResourceInfo *resource)
{
    __asm int 3
    return 0;
}

void __stdcall parse_accelerators(struct ResourceInfo *resource)
{
    __asm int 3
}

void __stdcall split_icon_file(struct ResourceType *type, struct ResourceInfo *resource, int kind)
{
    __asm int 3
}

void __cdecl parse_version_info(void)
{
    __asm int 3
}

void __stdcall parse_dialog_include(struct ResourceInfo *resource)
{
    __asm int 3
}
