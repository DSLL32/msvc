/* C6 symbol-size dispatch; historically called decode_legacy_lines.
 * These are symbol records, not the separate source-line subsection. */
#include "legacy-symbols.h"
#include <recon.h>

LegacySymbolSizer legacy_symbol_sizers[14] = {
    { 0x00, size_legacy_block16, size_legacy_procedure16 },
    { 0x01, size_legacy_procedure16, size_legacy_procedure32 },
    { 0x02, size_legacy_end, size_legacy_end },
    { 0x04, size_legacy_offset16, size_legacy_offset32 },
    { 0x05, size_legacy_offset16, size_legacy_offset32 },
    { 0x0b, size_legacy_offset32, size_legacy_offset32 },
    { 0x0c, size_legacy_block16, size_legacy_procedure16 },
    { 0x0d, size_legacy_register, size_legacy_register },
    { 0x0e, size_legacy_constant, size_legacy_constant },
    { 0x0f, size_legacy_procedure16, size_legacy_procedure32 },
    { 0x10, size_legacy_skip, size_legacy_skip },
    { 0x11, size_legacy_segment, size_legacy_segment },
    { 0x16, size_legacy_register_relative16, _setargv },
    { 0x12, size_legacy_offset16, size_legacy_offset16 }
};

/* [00408b40,00408bd6). */
void decode_legacy_lines(void *lines, unsigned size)
{
    unsigned char *end = (unsigned char *)lines + size;
    unsigned char kind, wide;
    unsigned index;
    symbol_segment_count = 0;
    symbol_input = lines;
    while (symbol_input < end) {
        kind = symbol_input[1] & 0x7f;
        wide = symbol_input[1] >> 7;
        for (index = 0; index < 14; ++index)
            if (legacy_symbol_sizers[index].kind == kind) break;
        if (index == 14) warning(4004, module_name(current_module_info), NULL);
        else if (wide) legacy_symbol_sizers[index].wide();
        else legacy_symbol_sizers[index].narrow();
        symbol_input += symbol_input[0] + 1;
    }
    total_line_bytes += size;
}

/* [00408c70,00408ce2). */
void size_legacy_segment(void)
{
    unsigned short segment = *(unsigned short *)(symbol_input + 2);
    if (!has_symbol_segment(segment)) {
        module_symbol_count += 7;
        symbol_segments[symbol_segment_count++] = segment;
        if (symbol_segment_count > 254)
            fatal_error(1015, module_name(current_module_info), NULL);
    } else module_reference_count += 6;
}

/* [00408c00,00408c14). */
void size_legacy_procedure16(void)
{
    module_symbol_count += 17;
    ++public_scope_depth;
}

/* [00408d20,00408d29). */
void size_legacy_register(void)
{ module_symbol_count += 8; }

/* [00408c60,00408c69). */
void size_legacy_offset16(void)
{ module_symbol_count += 5; }

/* [00408d10,00408d19). */
void size_legacy_offset32(void)
{ module_symbol_count += 7; }

/* [00408be0,00408bf4). */
void size_legacy_block16(void)
{
    module_symbol_count += 15;
    ++public_scope_depth;
}

/* [00408c20,00408c34). */
void size_legacy_end(void)
{
    module_symbol_count += 2;
    --public_scope_depth;
}

/* [00408d30,00408d39). */
void size_legacy_constant(void)
{ module_symbol_count += 11; }

/* [00408cf0,00408d02). */
void size_legacy_skip(void)
{ module_reference_count += symbol_input[0] + 1; }

/* Separate original cursors415328/415324 and segmentWORD41531c. */
unsigned char *legacy_symbol_input, *legacy_symbol_output;
unsigned short legacy_symbol_segment;
#pragma intrinsic(memcpy)

/* Convert the old symbol stream into signature2 aligned symbols.
 * The queued UDT list is the existing DWORD4154e0, formerly misnamed
 * pending_module_bytes while only its zero initialization was reached.
 * [00408d60,00408f23). */
void rewrite_legacy_lines(void *lines, CodeViewDirectoryEntry *entry,
    void *module_buffer, ModuleInfo *info)
{
    unsigned char *buffer, *end = (unsigned char *)lines + entry->size;
    unsigned size, index;
    LegacyTypeSymbol *symbol;
    legacy_symbol_input = lines;
    if (!symbol_segment_count) {
        module_symbol_count += 13;
        symbol_segment_count = 1;
        symbol_segments[0] = *(unsigned short *)((unsigned char *)module_buffer + 8);
    }
    module_symbol_count += 4;
    legacy_symbol_segment = symbol_segments[0];
    buffer = malloc(entry->size + module_symbol_count + module_procedure_count - module_reference_count);
    if (!buffer) fatal_error(1001, NULL, NULL);
    legacy_symbol_output = buffer;
    *(unsigned *)legacy_symbol_output = 2;
    legacy_symbol_output += 4;
    for (index = 0; index < symbol_segment_count; ++index)
        legacy_symbol_output += write_segment_record(legacy_symbol_output, symbol_segments[index]);
    convert_legacy_symbols(end);
    while (pending_legacy_symbols) {
        symbol = pending_legacy_symbols;
        memcpy(legacy_symbol_output, &symbol->length, symbol->length + 2);
        legacy_symbol_output += symbol->length + 2;
        pending_legacy_symbols = symbol->next;
        free(symbol);
    }
    size = legacy_symbol_output - buffer;
    if (!link_symbol_scopes(buffer, size)) {
        warning(4003, module_name(current_module_info), NULL);
        size = 0;
    }
    entry->data = buffer;
    entry->size = size;
    packed_symbol_bytes += size;
    info->symbols_size = size;
    info->symbols = buffer;
}

/* C6 input tags select both the aligned output kind and the width-specific
 * converter. Zero kinds are control records with no emitted symbol header. */
LegacySymbolConverter legacy_symbol_converters[14] = {
    { 0x00, 0x107, convert_legacy_block16, 0x207, convert_legacy_block32 },
    { 0x01, 0x104, convert_legacy_procedure16, 0x100a, convert_legacy_procedure32 },
    { 0x02, 0x006, convert_legacy_end, 0x006, convert_legacy_end },
    { 0x04, 0x100, convert_legacy_bprel16, 0x1006, convert_legacy_bprel32 },
    { 0x05, 0x101, convert_legacy_data16, 0x1007, convert_legacy_data32 },
    { 0x0b, 0x109, convert_legacy_label16, 0x209, convert_legacy_label32 },
    { 0x0c, 0x108, convert_legacy_block16, 0x208, convert_legacy_block32 },
    { 0x0d, 0x1001, convert_legacy_register, 0x1001, convert_legacy_register },
    { 0x0e, 0x1002, convert_legacy_constant, 0x1002, convert_legacy_constant },
    { 0x0f, 0x104, convert_legacy_procedure16, 0x100a, convert_legacy_procedure32 },
    { 0x16, 0x10a, convert_legacy_register_relative16, 0x20a, _setargv },
    { 0x12, 0x1004, convert_legacy_data32, 0x1004, convert_legacy_data32 },
    { 0x10, 0, convert_legacy_skip, 0, convert_legacy_skip },
    { 0x11, 0, convert_legacy_change_segment, 0, convert_legacy_change_segment }
};

/* [00408f30,00408fb7). */
void convert_legacy_symbols(unsigned char *end)
{
    unsigned short index, kind;
    unsigned char *cursor = legacy_symbol_input;
    unsigned char tag, wide;
    while (cursor < end) {
        tag = cursor[1] & 0x7f;
        wide = cursor[1] >> 7;
        for (index = 0; index < 14; ++index)
            if (legacy_symbol_converters[index].kind == tag) break;
        if (index >= 14) {
            cursor += cursor[0] + 1;
            legacy_symbol_input = cursor;
        } else {
            if (wide) {
                kind = legacy_symbol_converters[index].wide_kind;
                if (kind) *(unsigned short *)(legacy_symbol_output + 2) = kind;
                legacy_symbol_converters[index].wide();
            } else {
                kind = legacy_symbol_converters[index].narrow_kind;
                if (kind) *(unsigned short *)(legacy_symbol_output + 2) = kind;
                legacy_symbol_converters[index].narrow();
            }
            cursor = legacy_symbol_input;
        }
    }
}

/* [00409b20,00409b3e). */
void convert_legacy_change_segment(void)
{
    legacy_symbol_segment = *(unsigned short *)(legacy_symbol_input + 2);
    legacy_symbol_input += legacy_symbol_input[0] + 1;
}

/* [00409b40,00409c7e). */
void convert_legacy_procedure16(void)
{
    unsigned char *record = legacy_symbol_output, *name;
    unsigned short length = legacy_symbol_input[0] + 15;
    unsigned short count = (unsigned short)(legacy_symbol_input[0] - 14);
    unsigned padding = 3 - ((length - 1) & 3);
    *(unsigned *)(record + 4) = 0;
    *(unsigned *)(record + 8) = 0;
    *(unsigned *)(record + 12) = 0;
    *(unsigned short *)record = ((length + 3) & 0xfffc) - 2;
    *(unsigned short *)(record + 24) = legacy_symbol_segment;
    legacy_symbol_input += 2;
    *(unsigned short *)(record + 22) = *(unsigned short *)legacy_symbol_input;
    legacy_symbol_input += 2;
    *(unsigned short *)(record + 26) = *(unsigned short *)legacy_symbol_input;
    legacy_symbol_input += 2;
    *(unsigned short *)(record + 16) = *(unsigned short *)legacy_symbol_input;
    legacy_symbol_input += 2;
    *(unsigned short *)(record + 18) = *(unsigned short *)legacy_symbol_input;
    legacy_symbol_input += 2;
    *(unsigned short *)(record + 20) = *(unsigned short *)legacy_symbol_input;
    legacy_symbol_input += 4;
    record[28] = *legacy_symbol_input++;
    name = record + 29;
    while (count--) *name++ = *legacy_symbol_input++;
    while (padding--) *name++ = 0;
    *(unsigned short *)(record + 26) = (unsigned short)map_legacy_index(*(unsigned short *)(record + 26));
    legacy_symbol_output = name;
}

/* [004092a0,00409376). The inclusive32 bound also observes the next
 * table's initialized firstWORD0b09; retain that specific neighboring value. */
void convert_legacy_register(void)
{
    static const unsigned short registers[33] = {
        1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,
        17,18,19,20,21,22,23,24,25,26,27,28,29,30,128,0,0x0b09
    };
    unsigned char *record = legacy_symbol_output, *name;
    unsigned short length = legacy_symbol_input[0] + 6;
    unsigned short count = (unsigned short)(legacy_symbol_input[0] - 4), reg;
    unsigned padding = 3 - ((length - 1) & 3);
    *(unsigned short *)record = ((length + 3) & 0xfffc) - 2;
    legacy_symbol_input += 2;
    *(unsigned *)(record + 4) = *(unsigned short *)legacy_symbol_input;
    legacy_symbol_input += 2;
    reg = *legacy_symbol_input++;
    if (reg <= 32) reg = registers[reg];
    *(unsigned short *)(record + 8) = reg;
    name = record + 10;
    while (count--) *name++ = *legacy_symbol_input++;
    while (padding--) *name++ = 0;
    *(unsigned *)(record + 4) = map_legacy_index(*(unsigned *)(record + 4));
    legacy_symbol_output = name;
}

/* [00409ec0,00409fa7). */
void convert_legacy_bprel16(void)
{
    unsigned char *record = legacy_symbol_output;
    unsigned short length = legacy_symbol_input[0] + 3;
    unsigned short count = (unsigned short)(legacy_symbol_input[0] - 1);
    unsigned padding = 3 - ((length - 1) & 3);
    *(unsigned short *)record = ((length + 3) & 0xfffc) - 2;
    legacy_symbol_input += 2;
    legacy_symbol_output += 4;
    while (count--) *legacy_symbol_output++ = *legacy_symbol_input++;
    while (padding--) *legacy_symbol_output++ = 0;
    *(unsigned short *)(record + 6) = (unsigned short)map_legacy_index(*(unsigned short *)(record + 6));
}

/* [0040a0a0,0040a187). */
void convert_legacy_data16(void)
{
    unsigned char *record = legacy_symbol_output;
    unsigned short length = legacy_symbol_input[0] + 3;
    unsigned short count = (unsigned short)(legacy_symbol_input[0] - 1);
    unsigned padding = 3 - ((length - 1) & 3);
    *(unsigned short *)record = ((length + 3) & 0xfffc) - 2;
    legacy_symbol_input += 2;
    legacy_symbol_output += 4;
    while (count--) *legacy_symbol_output++ = *legacy_symbol_input++;
    while (padding--) *legacy_symbol_output++ = 0;
    *(unsigned short *)(record + 8) = (unsigned short)map_legacy_index(*(unsigned short *)(record + 8));
}

/* [00409980,00409a43). */
void convert_legacy_label16(void)
{
    unsigned char *record = legacy_symbol_output, *name;
    unsigned short length = legacy_symbol_input[0] + 5;
    unsigned short count = (unsigned short)(legacy_symbol_input[0] - 4);
    unsigned padding = 3 - ((length - 1) & 3);
    *(unsigned short *)record = ((length + 3) & 0xfffc) - 2;
    *(unsigned short *)(record + 6) = legacy_symbol_segment;
    legacy_symbol_input += 2;
    *(unsigned short *)(record + 4) = *(unsigned short *)legacy_symbol_input;
    legacy_symbol_input += 2;
    record[8] = *legacy_symbol_input++;
    name = record + 9;
    while (count--) *name++ = *legacy_symbol_input++;
    while (padding--) *name++ = 0;
    legacy_symbol_output = name;
}

/* [00409780,00409871). */
void convert_legacy_block16(void)
{
    unsigned char *record = legacy_symbol_output, *name;
    unsigned short count = (unsigned short)(legacy_symbol_input[0] - 5);
    unsigned present = count != 0;
    unsigned length, padding;
    if (!present) count = 1;
    length = 18 + count;
    padding = 3 - ((length - 1) & 3);
    *(unsigned short *)record = ((length + 3) & 0xfffc) - 2;
    *(unsigned *)(record + 4) = 0;
    *(unsigned *)(record + 8) = 0;
    *(unsigned short *)(record + 16) = legacy_symbol_segment;
    legacy_symbol_input += 2;
    *(unsigned short *)(record + 14) = *(unsigned short *)legacy_symbol_input;
    legacy_symbol_input += 2;
    *(unsigned short *)(record + 12) = *(unsigned short *)legacy_symbol_input;
    legacy_symbol_input += 2;
    name = record + 18;
    if (present) while (count--) *name++ = *legacy_symbol_input++;
    else *name++ = 0;
    while (padding--) *name++ = 0;
    legacy_symbol_output = name;
}

/* [00409de0,00409eb4). */
void convert_legacy_end(void)
{
    unsigned char *record = legacy_symbol_output;
    unsigned short length = legacy_symbol_input[0] + 3;
    unsigned short count = (unsigned short)(legacy_symbol_input[0] - 1);
    unsigned padding = 3 - ((length - 1) & 3);
    *(unsigned short *)record = ((length + 3) & 0xfffc) - 2;
    legacy_symbol_input += 2;
    legacy_symbol_output += 4;
    while (count--) *legacy_symbol_output++ = *legacy_symbol_input++;
    while (padding--) *legacy_symbol_output++ = 0;
}

/* [00409380,0040974a), counted-value dispatch[0040974c,00409778).
 * Signed scalar classification is inlined at the original conversion sites. */
#define LEGACY_SIGNED_CONSTANT(t) ((((t)&0xf0)==0x10 && !((t)&0x700)) || (t)==0x68 || (t)==0x72 || (t)==0x74 || (t)==0x76 || (t)==0x78 || (t)==0x70)
void convert_legacy_constant(void)
{
    unsigned char *record = legacy_symbol_output, *input, *output;
    unsigned type;
    unsigned short count, length, padding, leaf, i;
    legacy_symbol_input += 2;
    type = map_legacy_index(*(unsigned short *)legacy_symbol_input);
    legacy_symbol_input += 2;
    *(unsigned *)(record + 4) = type;
    output = record + 8;
    if (*legacy_symbol_input >= 0x80)
        convert_legacy_numeric_leaf(&legacy_symbol_input, &output);
    else {
        input = legacy_symbol_input;
        count = *input++;
        leaf = 0;
        if (type >= 0x1000) leaf = 0x8010;
        else switch (count) {
        case 1:
            if (LEGACY_SIGNED_CONSTANT(type)) leaf = 0x8000;
            if (leaf) {
                *(unsigned short *)output = leaf;
                output += 2;
            }
            *(unsigned short *)output = *input++;
            output += 2;
            break;
        case 2:
            if (*(unsigned short *)input >= 0x8000) {
                *(unsigned short *)output = LEGACY_SIGNED_CONSTANT(type) ? 0x8001 : 0x8002;
                output += 2;
            }
            *(unsigned short *)output = *(unsigned short *)input;
            input += 2;
            output += 2;
            break;
        case 4:
            if ((type & 0xf0) == 0x40 && !(type & 0x700)) leaf = 0x8005;
            else leaf = LEGACY_SIGNED_CONSTANT(type) ? 0x8003 : 0x8004;
            *(unsigned short *)output = leaf;
            output += 2;
            *(unsigned *)output = *(unsigned *)input;
            input += 4;
            output += 4;
            break;
        case 8:
            if ((type & 0xf0) == 0x50 && !(type & 0x700)) leaf = 0x800c;
            else if ((type & 0xf0) == 0x40 && !(type & 0x700)) leaf = 0x8006;
            else leaf = LEGACY_SIGNED_CONSTANT(type) ? 0x8009 : 0x800a;
            break;
        case 10: leaf = 0x8007; break;
        case 16:
            leaf = (type & 0xf0) == 0x50 && !(type & 0x700) ? 0x800d : 0x8008;
            break;
        default: leaf = 0x8010; break;
        }
        if (leaf == 0x8010) {
            *(unsigned short *)output = leaf;
            *(unsigned short *)(output + 2) = count;
            output += 4;
            for (i = 0; i < count; ++i) *output++ = *input++;
        } else if (count == 8 || count == 10 || count == 16) {
            *(unsigned short *)output = leaf;
            output += 2;
            memcpy(output, input, count);
            output += count;
            input += count;
        }
        legacy_symbol_input = input;
    }
    count = *legacy_symbol_input++;
    *output++ = (unsigned char)count;
    while (count--) *output++ = *legacy_symbol_input++;
    length = (unsigned short)(output - record);
    padding = 3 - ((length - 1) & 3);
    *(unsigned short *)record = length + padding - 2;
    while (padding--) *output++ = 0;
    legacy_symbol_output = output;
}
#undef LEGACY_SIGNED_CONSTANT

/* [0040fde0,0040ff39), dispatch[0040ff3c,0040ff60). */
unsigned short convert_legacy_numeric_leaf(unsigned char **source, unsigned char **destination)
{
    unsigned char *input = *source, *output = *destination;
    unsigned char kind = *input++;
    /* Unsupported tags leave the original return count uninitialized.
     * Zero that undefined result; the case uses defined85/86 tags. */
    unsigned short length = 0, count, value, i;
    if (kind < 0x80) {
        *(unsigned short *)output = kind;
        *source = input;
        *destination = output + 2;
        return 2;
    }
    switch (kind) {
    case 0x82:
        count = *input++;
        *(unsigned short *)output = 0x8010;
        *(unsigned short *)(output + 2) = count;
        output += 4;
        for (i = 0; i < count; ++i) *output++ = *input++;
        /* Original returns one less than the number of written bytes. */
        length = count + 3;
        break;
    case 0x85:
        value = *(unsigned short *)input;
        if (value >= 0x8000) {
            *(unsigned short *)output = 0x8002;
            output += 2;
            length = 4;
        } else length = 2;
        *(unsigned short *)output = value;
        output += 2;
        input += 2;
        break;
    case 0x86:
        *(unsigned short *)output = 0x8004;
        *(unsigned *)(output + 2) = *(unsigned *)input;
        input += 4;
        output += 6;
        length = 6;
        break;
    case 0x88:
        *(unsigned short *)output = 0x8000;
        output[2] = *input++;
        output += 3;
        length = 3;
        break;
    case 0x89:
        *(unsigned short *)output = 0x8001;
        *(unsigned short *)(output + 2) = *(unsigned short *)input;
        input += 2;
        output += 4;
        length = 4;
        break;
    case 0x8a:
        *(unsigned short *)output = 0x8003;
        *(unsigned *)(output + 2) = *(unsigned *)input;
        input += 4;
        output += 6;
        length = 6;
        break;
    }
    *destination = output;
    *source = input;
    return length;
}

/* [0040a190,0040a273). */
void convert_legacy_data32(void)
{
    unsigned char *record = legacy_symbol_output;
    unsigned short length = legacy_symbol_input[0] + 3;
    unsigned short count = (unsigned short)(legacy_symbol_input[0] - 1);
    unsigned padding = 3 - ((length - 1) & 3);
    *(unsigned short *)record = ((length + 3) & 0xfffc) - 2;
    legacy_symbol_input += 2;
    legacy_symbol_output += 4;
    while (count--) *legacy_symbol_output++ = *legacy_symbol_input++;
    while (padding--) *legacy_symbol_output++ = 0;
    *(unsigned *)(record + 4) = map_legacy_index(*(unsigned *)(record + 4));
}

/* [00409dc0,00409dd4). */
void convert_legacy_skip(void)
{ legacy_symbol_input += legacy_symbol_input[0] + 1; }

/* [00408d40,00408d54). */
void size_legacy_procedure32(void)
{
    module_symbol_count += 25;
    ++public_scope_depth;
}

/* [00409c80,00409dbf). */
void convert_legacy_procedure32(void)
{
    unsigned char *record = legacy_symbol_output, *name;
    unsigned short length = legacy_symbol_input[0] + 23;
    unsigned short count = (unsigned short)(legacy_symbol_input[0] - 16);
    unsigned padding = 3 - ((length - 1) & 3);
    *(unsigned *)(record + 4) = 0;
    *(unsigned *)(record + 8) = 0;
    *(unsigned *)(record + 12) = 0;
    *(unsigned short *)record = ((length + 3) & 0xfffc) - 2;
    *(unsigned short *)(record + 36) = legacy_symbol_segment;
    legacy_symbol_input += 2;
    *(unsigned *)(record + 32) = *(unsigned *)legacy_symbol_input;
    legacy_symbol_input += 4;
    *(unsigned *)(record + 28) = *(unsigned short *)legacy_symbol_input;
    legacy_symbol_input += 2;
    *(unsigned *)(record + 16) = *(unsigned short *)legacy_symbol_input;
    legacy_symbol_input += 2;
    *(unsigned *)(record + 20) = *(unsigned short *)legacy_symbol_input;
    legacy_symbol_input += 2;
    *(unsigned *)(record + 24) = *(unsigned short *)legacy_symbol_input;
    legacy_symbol_input += 4;
    record[38] = *legacy_symbol_input++;
    name = record + 39;
    while (count--) *name++ = *legacy_symbol_input++;
    while (padding--) *name++ = 0;
    *(unsigned *)(record + 28) = map_legacy_index(*(unsigned *)(record + 28));
    legacy_symbol_output = name;
}

/* [00409fb0,0040a093). */
void convert_legacy_bprel32(void)
{
    unsigned char *record = legacy_symbol_output;
    unsigned short length = legacy_symbol_input[0] + 3;
    unsigned short count = (unsigned short)(legacy_symbol_input[0] - 1);
    unsigned padding = 3 - ((length - 1) & 3);
    *(unsigned short *)record = ((length + 3) & 0xfffc) - 2;
    legacy_symbol_input += 2;
    legacy_symbol_output += 4;
    while (count--) *legacy_symbol_output++ = *legacy_symbol_input++;
    while (padding--) *legacy_symbol_output++ = 0;
    *(unsigned *)(record + 8) = map_legacy_index(*(unsigned *)(record + 8));
}

/* [00409a50,00409b11). */
void convert_legacy_label32(void)
{
    unsigned char *record = legacy_symbol_output, *name;
    unsigned short length = legacy_symbol_input[0] + 5;
    unsigned short count = (unsigned short)(legacy_symbol_input[0] - 6);
    unsigned padding = 3 - ((length - 1) & 3);
    *(unsigned short *)record = ((length + 3) & 0xfffc) - 2;
    *(unsigned short *)(record + 8) = legacy_symbol_segment;
    legacy_symbol_input += 2;
    *(unsigned *)(record + 4) = *(unsigned *)legacy_symbol_input;
    legacy_symbol_input += 4;
    record[10] = *legacy_symbol_input++;
    name = record + 11;
    while (count--) *name++ = *legacy_symbol_input++;
    while (padding--) *name++ = 0;
    legacy_symbol_output = name;
}

/* [00409880,00409972). */
void convert_legacy_block32(void)
{
    unsigned char *record = legacy_symbol_output, *name;
    unsigned short count = (unsigned short)(legacy_symbol_input[0] - 7);
    unsigned present = count != 0;
    unsigned length, padding;
    if (!present) count = 1;
    length = 22 + count;
    padding = 3 - ((length - 1) & 3);
    *(unsigned short *)record = ((length + 3) & 0xfffc) - 2;
    *(unsigned *)(record + 4) = 0;
    *(unsigned *)(record + 8) = 0;
    *(unsigned short *)(record + 20) = legacy_symbol_segment;
    legacy_symbol_input += 2;
    *(unsigned *)(record + 16) = *(unsigned *)legacy_symbol_input;
    legacy_symbol_input += 4;
    *(unsigned *)(record + 12) = *(unsigned short *)legacy_symbol_input;
    legacy_symbol_input += 2;
    name = record + 22;
    if (present) while (count--) *name++ = *legacy_symbol_input++;
    else *name++ = 0;
    while (padding--) *name++ = 0;
    legacy_symbol_output = name;
}

/* [00408c40,00408c49). */
void size_legacy_register_relative16(void)
{ module_symbol_count += 9; }

/* [0040a280,0040a37f). The original copies the counted one-character name
 * as a WORD and preserves the WORD type, rather than remapping it here. */
void convert_legacy_register_relative16(void)
{
    unsigned char *record = legacy_symbol_output;
    unsigned short length = legacy_symbol_input[0] + 6;
    unsigned padding = 3 - ((length - 1) & 3);
    *(unsigned short *)record = ((length + 3) & 0xfffc) - 2;
    legacy_symbol_input += 2;
    *(unsigned short *)(record + 4) = *(unsigned short *)legacy_symbol_input;
    legacy_symbol_input += 2;
    *(unsigned short *)(record + 6) = legacy_symbol_segment;
    *(unsigned short *)(record + 8) = *legacy_symbol_input++;
    *(unsigned short *)(record + 10) = *(unsigned short *)legacy_symbol_input;
    legacy_symbol_input += 2;
    *(unsigned short *)(record + 12) = *(unsigned short *)legacy_symbol_input;
    legacy_symbol_input += 2;
    legacy_symbol_output += 14;
    while (padding--) *legacy_symbol_output++ = 0;
}
