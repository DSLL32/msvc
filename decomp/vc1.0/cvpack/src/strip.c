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

unsigned short next_coff_symbol(CoffSymbol *symbol, unsigned long *index)
{
    unsigned long position;
    *index += symbol->auxiliary_count+1;
    if (*index >= strip_file_header.NumberOfSymbols) return 0;
    position = strip_symbol_offset+*index*18;
    if (position != (unsigned long)strip_read_offset) {
        seek_file(strip_handle, position, 0);
        strip_read_offset = position;
    }
    read_file(strip_handle, symbol, sizeof(*symbol));
    strip_read_offset += sizeof(*symbol);
    return 1;
}

unsigned short collect_strip_records(void)
{
    CoffSymbol symbol, block;
    CoffAuxiliary function_aux, block_aux;
    unsigned long first_index, index, line_offset;
    CoffFunction *record, *previous = NULL;
    unsigned short finished, has_lines;

    if (!find_first_coff_function(&symbol, &function_aux, &first_index)) return 1;
    index = first_index;
    do {
        record = malloc(sizeof(*record));
        if (!record) fatal_error(13, NULL, NULL);
        line_offset = function_aux.line_offset;
        if (line_offset && fill_coff_function(record, &symbol, &function_aux, index)) {
            read_coff_function_block(&function_aux, &block, &block_aux, &index);
            finished = has_lines = 0;
            do {
                next_coff_symbol(&block, &index);
                switch (classify_coff_symbol(&block)) {
                case 0:
                    fatal_error(37, NULL, NULL);
                    break;
                case 2:
                    read_coff_function_lines(record, previous, &line_offset,
                                             (unsigned short)block.value);
                    has_lines = 1;
                    break;
                case 3:
                    if (record->line_count) previous = publish_coff_function(record);
                    else { free(record->name); free(record); }
                    record = malloc(sizeof(*record));
                    if (!record) fatal_error(13, NULL, NULL);
                    if (!fill_coff_function(record, &symbol, &function_aux, index))
                        fatal_error(37, NULL, NULL);
                    break;
                case 4:
#ifdef RECON_NATIVE_TEST
                    if (symbol.section < 1 || symbol.section > strip_file_header.NumberOfSections)
                        __asm { int 3 }
#endif
                    set_coff_function_end(record, function_aux.misc.total_size
                        -strip_section_rvas[symbol.section-1]+symbol.value-1);
                    previous = NULL;
                    if (!has_lines) { free(record->name); free(record); }
                    else record = publish_coff_function(record);
                    finished = 1;
                    break;
                }
            } while (!finished);
        }
    } while (find_next_coff_function(&symbol, &function_aux, &index));
    return 1;
}

unsigned short find_first_coff_function(CoffSymbol *symbol,
                                      CoffAuxiliary *auxiliary,
                                      unsigned long *index)
{
    unsigned short done = 0;
    if (!strip_symbol_offset) return 0;
    *index = 0;
    if (!read_coff_symbol(symbol, 0)) fatal_error(37, NULL, NULL);
    while (!((symbol->type & 0x30) == 0x20 && symbol->auxiliary_count) && !done)
        done = (unsigned short)(next_coff_symbol(symbol, index) < 1);
    if ((symbol->type & 0x30) == 0x20 && symbol->auxiliary_count) {
        if (!read_coff_auxiliary(auxiliary)) fatal_error(37, NULL, NULL);
        return 1;
    }
    return 0;
}

unsigned short read_coff_auxiliary(CoffAuxiliary *auxiliary)
{
    unsigned long end = strip_symbol_offset+strip_file_header.NumberOfSymbols*18;
    if (end <= (unsigned long)strip_read_offset) return 0;
    read_file(strip_handle, auxiliary, sizeof(*auxiliary));
    strip_read_offset += sizeof(*auxiliary);
    return 1;
}

unsigned short fill_coff_function(CoffFunction *record, const CoffSymbol *symbol,
                                 const CoffAuxiliary *auxiliary,
                                 unsigned long index)
{
    char filename[260];
    CoffSymbol block;
    CoffAuxiliary block_aux;
    unsigned long block_index, offset, i;
    unsigned char *clear = (unsigned char *)record;

    for (i = 0; i < sizeof(*record); ++i) clear[i] = 0;
#ifdef RECON_NATIVE_TEST
    if (symbol->section < 1 || symbol->section > strip_file_header.NumberOfSections)
        __asm { int 3 }
#endif
    offset = symbol->value-strip_section_rvas[symbol->section-1];
    if (!read_coff_file_name(filename, index)) fatal_error(37, NULL, NULL);
    set_coff_function_name(record, filename);
    assign_coff_function_module(record, symbol->section, offset);
    if (!read_coff_function_block(auxiliary, &block, &block_aux, &block_index))
        return 0;
    set_coff_function_section(record, symbol->section);
    set_coff_function_line(record, block_aux.misc.line.number);
    set_coff_function_offset(record, offset);
    return 1;
}

unsigned short read_coff_file_name(char *filename, unsigned long index)
{
    CoffFileName *node = strip_file_names, *selected = node;
    unsigned long i;

    while (node->symbol <= index) {
        selected = node;
        node = node->next;
        if (!node) break;
    }
    for (i = 0; selected->name[i]; ++i) filename[i] = selected->name[i];
    filename[i] = 0;
    return 1;
}

void set_coff_function_name(CoffFunction *record, const char *filename)
{
    unsigned long length, i;

    for (length = 0; filename[length]; ++length) {}
    record->name = malloc(length+1);
    if (!record->name) fatal_error(13, NULL, NULL);
    for (i = 0; i <= length; ++i) record->name[i] = filename[i];
}

/* The last matching module is the starting point of the next circular scan. */
ModuleInfo *strip_module_hint;

void assign_coff_function_module(CoffFunction *record, unsigned short section,
                                 unsigned long offset)
{
    ModuleInfo *module, *first;
    ModuleContribution *contribution;
    unsigned short i, count;

    if (!strip_module_hint) strip_module_hint = module_list;
    module = first = strip_module_hint;
    if (module) do {
        if (module->record_size) {
#ifdef RECON_NATIVE_TEST
            if (!module->record || module->record_size < 8) __asm { int 3 }
#endif
            count = *(unsigned short *)(module->record+4);
#ifdef RECON_NATIVE_TEST
            if (count > (module->record_size-8)/sizeof(*contribution))
                __asm { int 3 }
#endif
            contribution = (ModuleContribution *)(module->record+8);
            for (i = 0; i < count; ++i, ++contribution) {
                if (contribution->segment == section &&
                    offset >= contribution->offset &&
                    contribution->offset+contribution->size > offset) {
                    strip_module_hint = module;
                    record->module = module->module;
                    return;
                }
            }
        }
        module = module->next;
        if (!module) module = module_list;
    } while (module != first);
    fatal_error(37, NULL, NULL);
}

unsigned short read_coff_function_block(const CoffAuxiliary *function,
                                        CoffSymbol *block,
                                        CoffAuxiliary *auxiliary,
                                        unsigned long *index)
{
    if (!function->tag_index) fatal_error(37, NULL, NULL);
    if (!read_coff_symbol(block, function->tag_index)) fatal_error(37, NULL, NULL);
    *index = function->tag_index;
    if (block->storage_class != 0x65) fatal_error(37, NULL, NULL);
    if (!block->auxiliary_count) return 0;
    if (!read_coff_auxiliary(auxiliary)) fatal_error(37, NULL, NULL);
    return 1;
}

void set_coff_function_section(CoffFunction *record, unsigned short section)
{
    record->section = section;
}

void set_coff_function_line(CoffFunction *record, unsigned short line)
{
    record->first_line = line;
}

void set_coff_function_offset(CoffFunction *record, unsigned long offset)
{
    record->offset = offset;
}

void read_coff_function_lines(CoffFunction *record, CoffFunction *previous,
                              unsigned long *offset, unsigned short count)
{
    long position = strip_read_offset;
    unsigned short first = record->first_line, i;
    unsigned long bytes;
    CoffLine *line;

    record->line_count = count;
    bytes = count*sizeof(*line);
    record->lines = malloc(bytes);
    if (!record->lines) fatal_error(13, NULL, NULL);
    seek_file(strip_handle, *offset, 0);
    read_file(strip_handle, record->lines, bytes);
    line = record->lines;
    for (i = 0; i < count; ++i, ++line) {
        if (!line->line) {
            line->offset = record->offset;
            line->line += first;
        } else if (line->line == 0x7fff) line->line = first;
        else line->line += first;
    }
    if (count) record->offset = record->lines[0].offset;
    if (previous && count) set_coff_function_end(previous, record->offset-1);
    *offset = tell_file(strip_handle);
    seek_file(strip_handle, position, 0);
    strip_read_offset = position;
}

void set_coff_function_end(CoffFunction *record, unsigned long end)
{
    record->end = end;
}
