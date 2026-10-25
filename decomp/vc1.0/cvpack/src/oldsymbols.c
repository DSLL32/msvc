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
