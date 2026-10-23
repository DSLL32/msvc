/* PE headers used while rewriting the original file's auxiliary records. */
#include "strip.h"
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

/* Raw section offsets and the COFF .file chain are shared by the strip passes. */
unsigned long *strip_section_rvas;
CoffFileName *strip_file_names;

unsigned short initialize_strip_sections(void)
{
    long position, result;
    unsigned long index, previous, i, length;
    unsigned short done = 0, bytes;
    CoffSymbol symbol;
    IMAGE_SECTION_HEADER section;
    CoffFileName *last = NULL, *node;
    char *auxiliary;
    int different;

    position = tell_file(strip_handle);
    strip_section_rvas = malloc(strip_file_header.NumberOfSections*4);
    if (!strip_section_rvas) fatal_error(13, NULL, NULL);
    seek_file(strip_handle, strip_section_header_offset, 0);
    for (i = 0; i < strip_file_header.NumberOfSections; ++i) {
        read_file(strip_handle, &section, sizeof(section));
        strip_section_rvas[i] = section.VirtualAddress;
    }
    if (!strip_symbol_offset) {
        seek_file(strip_handle, position, 0);
        strip_read_offset = position;
        return 0;
    }
    index = 0;
    if (!read_coff_symbol(&symbol, 0)) fatal_error(37, NULL, NULL);
    while (classify_coff_symbol(&symbol) != 3 && !done)
        done = (unsigned short)(next_coff_symbol(&symbol, &index) < 1);
    if (done) return 0;
    previous = index;
    do {
        bytes = (unsigned short)(symbol.auxiliary_count*18);
        auxiliary = malloc(bytes);
        if (!auxiliary) fatal_error(13, NULL, NULL);
        read_file(strip_handle, auxiliary, bytes);
#ifdef RECON_NATIVE_TEST
        /* Unknown malformed names must stop before an unbounded native read. */
        for (length = 0; length < bytes && auxiliary[length]; ++length) {}
        if (length == bytes) __asm { int 3 }
#endif
        different = 1;
        if (last) {
            for (i = 0; auxiliary[i] == last->name[i] && auxiliary[i]; ++i) {}
            different = auxiliary[i] != last->name[i];
        }
        if (different) {
            node = malloc(sizeof(*node));
            if (!node) fatal_error(13, NULL, NULL);
            for (length = 0; auxiliary[length]; ++length) {}
            ++length;
            node->name = malloc(length);
            if (!node->name) fatal_error(13, NULL, NULL);
            for (i = 0; i < length; ++i) node->name[i] = auxiliary[i];
            node->symbol = index;
            node->next = NULL;
            if (last) last->next = node;
            else strip_file_names = node;
            last = node;
        }
        done = (unsigned short)(symbol.value == previous);
        if (!done) {
            index = symbol.value;
            if (!read_coff_symbol(&symbol, index)) fatal_error(37, NULL, NULL);
        }
        free(auxiliary);
    } while (!done);
    result = seek_file(strip_handle, position, 0);
    strip_read_offset = position;
    return (unsigned short)result;
}

unsigned short read_coff_symbol(CoffSymbol *symbol, unsigned long index)
{
    unsigned long position = strip_symbol_offset+index*18;
    if (strip_file_header.NumberOfSymbols <= index) return 0;
    if ((unsigned long)strip_read_offset != position)
        seek_file(strip_handle, position, 0);
    read_file(strip_handle, symbol, sizeof(*symbol));
    strip_read_offset = position+sizeof(*symbol);
    return 1;
}

int classify_coff_symbol(const CoffSymbol *symbol)
{
    if ((symbol->type & 0x30) == 0x20 && symbol->auxiliary_count) return 0;
    if (symbol->storage_class == 0x65) {
        if (symbol->name[1] == 'b') return 1;
        if (symbol->name[1] == 'e') return 4;
        if (symbol->name[1] == 'l') return 2;
    } else if (symbol->storage_class == 0x67) return 3;
    return 5;
}
