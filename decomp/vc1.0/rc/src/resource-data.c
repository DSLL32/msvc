/* Storage used to assemble one resource's binary payload. */
#include "resource-data.h"
#include <recon.h>
unsigned char *resource_data;
unsigned int resource_data_size;
FILE *resource_data_file;

/* Original 0x0100a630..0x0100a640. */
void __cdecl allocate_resource_data(void)
{
    resource_data = resource_alloc(65500);
}
/* Original 0x0100a640..0x0100a655. */
void __cdecl begin_resource_data(void)
{
    resource_data_size = 0;
    resource_data_file = 0;
}
/* Original 0x0100a680..0x0100a68c. */
void __cdecl release_resource_data(void)
{
    resource_free(resource_data);
}

/* Original 0x0100a660..0x0100a675. NULL queries the current input stream. */
FILE * __stdcall resource_stream(FILE *stream)
{
    if (stream)
        resource_data_file = stream;
    return resource_data_file;
}

/* Original 0x0100a690..0x0100a6ad. */
void __stdcall put_wide_string(WCHAR *text)
{
    do {
        put_word(*text);
    } while (*text++);
}

/* Original 0x0100a6b0..0x0100a6d7. */
void __stdcall concatenate_wide_string(WCHAR *text)
{
    if (!*(WORD *)(resource_data + resource_data_size - 2))
        resource_data_size -= 2;
    put_wide_string(text);
}

/* Original 0x0100a6e0..0x0100a716. */
void __stdcall put_byte(BYTE value)
{
    if (resource_data_size > 65500 - 1)
        resource_data_error("Resource too large", "");
    resource_data[resource_data_size++] = value;
}

/* Original 0x0100a720..0x0100a79c. */
void __stdcall put_dword(DWORD value)
{
    if (resource_data_size > 65500 - 4)
        resource_data_error("Resource too large", "");
    resource_data[resource_data_size++] = (BYTE)value;
    resource_data[resource_data_size++] = (BYTE)(value >> 8);
    resource_data[resource_data_size++] = (BYTE)(value >> 16);
    resource_data[resource_data_size++] = (BYTE)(value >> 24);
}

/* Original 0x0100a7a0..0x0100a7e0. Preserve this version's remainder cases. */
void __cdecl align_resource_data(void)
{
    switch ((int)resource_data_size % 4) {
    case 1: put_byte(0); break;
    case 2: put_word(0); break;
    case 3: put_word(0); put_byte(0); break;
    }
}

/* Original 0x0100a7e0..0x0100a82b. */
void __stdcall put_word(WORD value)
{
    if (resource_data_size > 65500 - 2)
        resource_data_error("Resource Too Large", "");
    resource_data[resource_data_size++] = (BYTE)value;
    resource_data[resource_data_size++] = (BYTE)(value >> 8);
}

/* Original 0x0100a830..0x0100a85f. The source count is a WORD. */
void __stdcall put_bytes(void *data, WORD length)
{
    BYTE *next = data;
    while (length--)
        put_byte(*next++);
}

/* Original 0x0100b3d0..0x0100b3dc. */
BYTE * __cdecl resource_data_end(void)
{
    return resource_data + resource_data_size;
}
/* Original 0x0100b3e0..0x0100b3e7. Only AX is the return value. */
WORD __cdecl resource_data_length(void)
{
    return (WORD)resource_data_size;
}
