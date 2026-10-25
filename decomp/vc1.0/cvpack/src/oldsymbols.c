/* C6 local-symbol sizing dispatch, shared by the 16/32-bit record families. */
#include "oldsymbols.h"
#include "types.h"
#include "image.h"
#include <recon.h>

OldSymbolSizer old_symbol_sizers[14] = {
    { 0x00, size_old_block16, size_old_block32 },
    { 0x01, size_old_procedure16, size_old_procedure32 },
    { 0x02, size_old_end, size_old_end },
    { 0x04, size_old_bprel16, size_old_bprel32 },
    { 0x05, size_old_data16, size_old_data32 },
    { 0x0b, size_old_label16, size_old_label32 },
    { 0x0c, size_old_block16, size_old_block32 },
    { 0x0d, size_old_register, size_old_register },
    { 0x0e, size_old_constant, size_old_constant },
    { 0x0f, size_old_procedure16, size_old_procedure32 },
    { 0x10, size_old_skip, size_old_skip },
    { 0x11, size_old_change_segment, size_old_change_segment },
    { 0x16, size_old_register_relative16, size_old_register_relative32 },
    { 0x12, size_old_symbol12, size_old_symbol12 },
};

extern unsigned char *symbol_input;
extern unsigned short extra_type_count;

/* Historical name: this pass sizes converted C6 local symbols. */
void prepare_old_types(void *symbols, unsigned long size)
{
    unsigned char *end = (unsigned char *)symbols + size;
    unsigned char kind, wide;
    unsigned i;
    extra_type_count = 0;
    symbol_input = symbols;
    while (symbol_input < end) {
        wide = (unsigned char)(symbol_input[1] & 0x80);
        kind = (unsigned char)(symbol_input[1] & 0x7f);
        for (i = 0; i < 14; ++i)
            if (old_symbol_sizers[i].kind == kind)
                break;
        if (i == 14)
            warning(4, module_name(current_module), NULL);
        else if (wide)
            old_symbol_sizers[i].wide();
        else
            old_symbol_sizers[i].narrow();
        symbol_input += symbol_input[0] + 1;
    }
    total_new_type_bytes += size;
}

extern unsigned short extra_types[254];

void size_old_change_segment(void)
{
    unsigned short segment = *(unsigned short *)(symbol_input + 2);
    if (!has_extra_type(segment)) {
        module_type_base += 7;
        extra_types[extra_type_count++] = segment;
        if (extra_type_count > 254)
            fatal_error(27, module_name(current_module), NULL);
    } else
        module_type_count += 6;
}

void size_old_procedure16(void)
{
    ++symbol_scope_depth;
    module_type_base += 17;
}

void size_old_bprel16(void)
{
    module_type_base += 5;
}

void size_old_end(void)
{
    module_type_base += 2;
    --symbol_scope_depth;
}

void size_old_data16(void)
{
    module_type_base += 5;
}

void size_old_register(void)
{
    module_type_base += 6;
}

unsigned short old_symbol_segment;
unsigned char *old_symbol_input, *old_symbol_output;
extern unsigned long total_packed_symbol_bytes;

#pragma intrinsic(memcpy)

/* Historical name: convert the old module's local-symbol stream. */
void pack_old_types(void *symbols, CvDirectoryEntry *entry, void *module_record,
                    ModuleInfo *module)
{
    unsigned char *buffer, *start, *end;
    unsigned long output_size;
    unsigned short i;
    UDTSymbol *symbol;

    old_symbol_input = symbols;
    if (!extra_type_count) {
        module_type_base += 13;
        extra_types[extra_type_count++] = *(unsigned short *)((unsigned char *)module_record+8);
    }
    module_type_base += 4;
    old_symbol_segment = extra_types[0];
    end = (unsigned char *)symbols+entry->size;
    output_size = entry->size+(unsigned long)module_symbol_count
                  -(unsigned long)module_type_count+(unsigned long)module_type_base;
    buffer = malloc(output_size);
    if (!buffer) fatal_error(14, NULL, NULL);
    old_symbol_output = buffer;
    if (!buffer) fatal_error(14, NULL, NULL);
    start = old_symbol_output;
    *(unsigned long *)old_symbol_output = 1;
    old_symbol_output += 4;
    for (i = 0; i < extra_type_count; ++i)
        old_symbol_output += write_search_symbol(old_symbol_output, extra_types[i]);
    convert_old_symbols(end);
    while (module_udt_symbols) {
        memcpy(old_symbol_output, &module_udt_symbols->length,
               module_udt_symbols->length+2);
        old_symbol_output += module_udt_symbols->length+2;
        symbol = module_udt_symbols;
        module_udt_symbols = symbol->next;
        free(symbol);
    }
    output_size = old_symbol_output-start;
    if (!link_symbol_scopes(start, output_size)) {
        warning(3, module_name(current_module), NULL);
        output_size = 0;
    }
    entry->location.data = buffer;
    entry->size = output_size;
    total_packed_symbol_bytes += output_size;
    module->symbol_size = output_size;
    module->symbols = buffer;
}
