/* Rewrite aligned module symbols [0040a790,0040aa00). */
#include "types.h"
#include "pack.h"
#include <recon.h>

#pragma intrinsic(memcpy)

unsigned char *symbol_input, *symbol_output;
unsigned discard_symbol_depth, packed_symbol_bytes;

/* Emit a segment-change symbol [0040df30,0040df5a). */
unsigned short write_segment_record(unsigned char *buffer, unsigned short segment)
{
    *(unsigned short *)(buffer + 8) = segment;
    *(unsigned short *)buffer = 10;
    *(unsigned short *)(buffer + 2) = 5;
    *(unsigned *)(buffer + 4) = 0;
    buffer[10] = buffer[11] = 0;
    return 12;
}

/* The ordered kind dispatch is program data, including the final default row. */
SymbolRewriterState symbol_rewriters = { {
    { 0x1006, 0, rewrite_bp_relative_symbol },
    { 0x0207, 0, enter_symbol_scope },
    { 0x100a, 0, rewrite_procedure32_symbol },
    { 0x100b, 0, rewrite_procedure32_symbol },
    { 0x0006, 0, leave_symbol_scope },
    { 0x1007, 0, rewrite_symbol_type4 },
    { 0x1008, 0, rewrite_global_data_symbol },
    { 0x1009, 0, rewrite_symbol_type4 },
    { 0x0209, 0, NULL },
    { 0x1001, 0, rewrite_symbol_type4 },
    { 0x100e, 0, rewrite_symbol_type4 },
    { 0x100f, 0, rewrite_global_data_symbol },
    { 0x020f, 0, NULL },
    { 0x0100, 0, rewrite_register16_symbol },
    { 0x0107, 0, enter_symbol_scope },
    { 0x0104, 0, rewrite_procedure16_symbol },
    { 0x0105, 0, rewrite_procedure16_symbol },
    { 0x0101, 0, rewrite_data16_symbol },
    { 0x0102, 0, rewrite_global_data16_symbol },
    { 0x0103, 0, rewrite_data16_symbol },
    { 0x0109, 0, NULL },
    { 0x1003, 0, rewrite_type_name_symbol },
    { 0x100d, 0, rewrite_bp_relative_symbol },
    { 0x1010, 0, rewrite_mips_procedure_symbol },
    { 0x1011, 0, rewrite_mips_procedure_symbol },
    { 0x000a, 0, NULL },
    { 0x010c, 0, rewrite_data16_symbol },
    { 0x0108, 0, enter_symbol_scope },
    { 0x0208, 0, enter_symbol_scope },
    { 0x0106, 0, enter_symbol_scope },
    { 0x0206, 0, enter_symbol_scope },
    { 0x010a, 0, NULL },
    { 0x020a, 0, NULL },
    { 0x1002, 0, rewrite_type_name_symbol },
    { 0x0005, 0, NULL },
    { 0x0007, 0, NULL },
    { 0x0001, 0, NULL },
    { 0x0009, 0, rewrite_object_name_symbol },
    { 0x1000, 0, NULL }
} };

void rewrite_module_symbols(void *lines, CodeViewDirectoryEntry *entry,
                            ModuleInfo *info, unsigned short *symbols,
                            unsigned short *references)
{
    unsigned char *end = (unsigned char *)lines + entry->size;
    unsigned char *buffer;
    unsigned short length, kind, i, padding;
    unsigned size;
    symbol_input = lines;
    discard_symbol_depth = public_scope_depth = 0;
    buffer = malloc(entry->size + *symbols - *references);
    if (!buffer) fatal_error(1001, NULL, NULL);
    symbol_output = buffer;
    *(unsigned *)symbol_output = *(unsigned *)symbol_input;
    symbol_output += 4;
    symbol_input += 4;
    for (i = 0; i < symbol_segment_count; ++i)
        symbol_output += write_segment_record(symbol_output, symbol_segments[i]);
    while (symbol_input < end) {
        length = (unsigned short)(symbol_input[0] | (symbol_input[1] << 8)) + 2;
        kind = symbol_input[2] | (symbol_input[3] << 8);
        if (kind != 7) {
            for (i = 0; i < 38; ++i)
                if (symbol_rewriters.entries[i].kind == kind) break;
            if (symbol_rewriters.entries[i].rewrite) symbol_rewriters.entries[i].rewrite();
            if (*(unsigned short *)(symbol_input + 2) != 8) {
                memcpy(symbol_output, symbol_input, length);
                padding = 3 - ((length - 1) & 3);
                *(unsigned short *)symbol_output = length + padding - 2;
                symbol_output += length;
                while (padding--) *symbol_output++ = 0;
            }
        }
        symbol_input += length;
    }
    size = symbol_output - buffer;
    if (!link_symbol_scopes(buffer, size)) {
        warning(4003, module_name(current_module_info), NULL);
        size = 0;
    } else record_symbol_references(buffer, size, info->module);
    entry->data = buffer;
    entry->size = size;
    packed_symbol_bytes += size;
    info->symbols = buffer;
    info->symbols_size = size;
}
