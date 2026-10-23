/* PE headers used while rewriting the original file's auxiliary records. */
#include "image.h"
#include <recon.h>

int strip_handle;
long strip_file_header_offset, strip_section_header_offset;
long strip_read_offset, strip_symbol_offset;
IMAGE_FILE_HEADER strip_file_header;
IMAGE_OPTIONAL_HEADER strip_optional_header;
void *strip_records;

unsigned short initialize_strip_sections(void);
unsigned short collect_strip_records(void);
void write_strip_records(void);
void free_strip_records(void);

unsigned short strip_debug(int handle)
{
    long position;
    unsigned short signature;
    unsigned long nt_signature;
    IMAGE_DOS_HEADER dos;

    strip_handle = handle;
    strip_file_header_offset = 0;
    position = tell_file(strip_handle);
    seek_file(strip_handle, 0, 0);
    read_file(strip_handle, &signature, 2);
    seek_file(strip_handle, -2, 1);
    if (signature == 0x5a4d) {
        read_file(strip_handle, &dos, sizeof(dos));
        seek_file(strip_handle, dos.e_lfanew, 0);
    }
    read_file(strip_handle, &nt_signature, 4);
    if (nt_signature != 0x4550) fatal_error(37, NULL, NULL);
    strip_file_header_offset = tell_file(strip_handle);
    read_file(strip_handle, &strip_file_header, sizeof(strip_file_header));
    if (strip_file_header.SizeOfOptionalHeader) {
#ifdef RECON_NATIVE_TEST
        /* The original has no bound for this fixed-size global object. */
        if (strip_file_header.SizeOfOptionalHeader > sizeof(strip_optional_header))
            __asm { int 3 }
#endif
        read_file(strip_handle, &strip_optional_header,
                  strip_file_header.SizeOfOptionalHeader);
    }
    strip_section_header_offset = tell_file(strip_handle);
    strip_read_offset = tell_file(strip_handle);
    strip_symbol_offset = strip_file_header.PointerToSymbolTable;
    if (strip_symbol_offset && strip_file_header.NumberOfSymbols) {
        if (!initialize_strip_sections()) return 0;
        if (!collect_strip_records()) {
            seek_file(strip_handle, position, 0);
            free_strip_records();
            return 0;
        }
        statistics_enabled = (unsigned short)(strip_records != NULL);
        write_strip_records();
        seek_file(strip_handle, position, 0);
        free_strip_records();
        return 1;
    }
    seek_file(strip_handle, position, 0);
    return 1;
}
