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

OldSymbolConverter old_symbol_converters[14] = {
    { 0x00, 0x107, convert_old_symbol_block16, 0x207, convert_old_symbol_block32 },
    { 0x01, 0x104, convert_old_symbol_procedure16, 0x204, convert_old_symbol_procedure32 },
    { 0x02, 0x6, convert_old_symbol_end, 0x6, convert_old_symbol_end },
    { 0x04, 0x100, convert_old_symbol_bprel16, 0x200, convert_old_symbol_bprel32 },
    { 0x05, 0x101, convert_old_symbol_data16, 0x201, convert_old_symbol_data32 },
    { 0x0b, 0x109, convert_old_symbol_label16, 0x209, convert_old_symbol_label32 },
    { 0x0c, 0x108, convert_old_symbol_block16, 0x208, convert_old_symbol_block32 },
    { 0x0d, 0x2, convert_old_symbol_register, 0x2, convert_old_symbol_register },
    { 0x0e, 0x3, convert_old_symbol_constant, 0x3, convert_old_symbol_constant },
    { 0x0f, 0x104, convert_old_symbol_procedure16, 0x204, convert_old_symbol_procedure32 },
    { 0x16, 0x10a, convert_old_symbol_register_relative16, 0x20a, convert_old_symbol_register_relative32 },
    { 0x12, 0xb, convert_old_symbol12, 0xb, convert_old_symbol12 },
    { 0x10, 0x0, convert_old_symbol_skip, 0x0, convert_old_symbol_skip },
    { 0x11, 0x0, convert_old_symbol_change_segment, 0x0, convert_old_symbol_change_segment },
};

void convert_old_symbols(unsigned char *end)
{
    unsigned char kind, wide;
    unsigned short i, converted;
    while (old_symbol_input < end) {
        wide = (unsigned char)(old_symbol_input[1] & 0x80);
        kind = (unsigned char)(old_symbol_input[1] & 0x7f);
        for (i = 0; i < 14; ++i)
            if (old_symbol_converters[i].kind == kind)
                break;
        if (i == 14)
            old_symbol_input += old_symbol_input[0]+1;
        else if (wide) {
            converted = old_symbol_converters[i].wide_kind;
            if (converted) *(unsigned short *)(old_symbol_output+2) = converted;
            old_symbol_converters[i].wide();
        } else {
            converted = old_symbol_converters[i].narrow_kind;
            if (converted) *(unsigned short *)(old_symbol_output+2) = converted;
            old_symbol_converters[i].narrow();
        }
    }
}

void convert_old_symbol_change_segment(void)
{
    old_symbol_segment = *(unsigned short *)(old_symbol_input+2);
    old_symbol_input += old_symbol_input[0]+1;
}

void convert_old_symbol_procedure16(void)
{
    unsigned char *record = old_symbol_output;
    unsigned char *name;
    unsigned short length = (unsigned short)(old_symbol_input[0]+15);
    unsigned short name_length = (unsigned short)(old_symbol_input[0]-14);
    unsigned padding = 3-((length-1)&3);
    *(unsigned short *)record = (unsigned short)(((length+3)&0xfffc)-2);
    *(unsigned long *)(record+4) = 0;
    *(unsigned long *)(record+8) = 0;
    *(unsigned long *)(record+12) = 0;
    *(unsigned short *)(record+24) = old_symbol_segment;
    old_symbol_input += 2;
    *(unsigned short *)(record+22) = *(unsigned short *)old_symbol_input;
    old_symbol_input += 2;
    *(unsigned short *)(record+26) = *(unsigned short *)old_symbol_input;
    old_symbol_input += 2;
    *(unsigned short *)(record+16) = *(unsigned short *)old_symbol_input;
    old_symbol_input += 2;
    *(unsigned short *)(record+18) = *(unsigned short *)old_symbol_input;
    old_symbol_input += 2;
    *(unsigned short *)(record+20) = *(unsigned short *)old_symbol_input;
    old_symbol_input += 4;
    record[28] = *old_symbol_input++;
    name = record+29;
    while (name_length--) *name++ = *old_symbol_input++;
    while (padding--) *name++ = 0;
    *(unsigned short *)(record+26) = convert_old_type(*(unsigned short *)(record+26));
    old_symbol_output = name;
}

void convert_old_symbol_bprel16(void)
{
    unsigned char *record = old_symbol_output;
    unsigned short length = (unsigned short)(old_symbol_input[0]+3);
    unsigned short count = (unsigned short)(old_symbol_input[0]-1);
    unsigned padding = 3-((length-1)&3);
    *(unsigned short *)record = (unsigned short)(((length+3)&0xfffc)-2);
    old_symbol_input += 2;
    old_symbol_output += 4;
    while (count--) *old_symbol_output++ = *old_symbol_input++;
    while (padding--) *old_symbol_output++ = 0;
    *(unsigned short *)(record+6) = convert_old_type(*(unsigned short *)(record+6));
}

void convert_old_symbol_end(void)
{
    unsigned short length = (unsigned short)(old_symbol_input[0]+3);
    unsigned short count = (unsigned short)(old_symbol_input[0]-1);
    unsigned padding = 3-((length-1)&3);
    *(unsigned short *)old_symbol_output = (unsigned short)(((length+3)&0xfffc)-2);
    old_symbol_input += 2;
    old_symbol_output += 4;
    while (count--) *old_symbol_output++ = *old_symbol_input++;
    while (padding--) *old_symbol_output++ = 0;
}

void convert_old_symbol_data16(void)
{
    unsigned char *record = old_symbol_output;
    unsigned short length = (unsigned short)(old_symbol_input[0]+3);
    unsigned short count = (unsigned short)(old_symbol_input[0]-1);
    unsigned padding = 3-((length-1)&3);
    *(unsigned short *)record = (unsigned short)(((length+3)&0xfffc)-2);
    old_symbol_input += 2;
    old_symbol_output += 4;
    while (count--) *old_symbol_output++ = *old_symbol_input++;
    while (padding--) *old_symbol_output++ = 0;
    *(unsigned short *)(record+8) = convert_old_type(*(unsigned short *)(record+8));
}

/* C6 register numbers are translated to the newer format.
 * The original inclusive bound also reads the next table's first WORD for
 * register 32; retain that initialized 0x0b09 value rather than widening it. */
unsigned short old_register_numbers[33] = {
    1, 2, 3, 4, 5, 6, 7, 8,
    9, 10, 11, 12, 13, 14, 15, 16,
    17, 18, 19, 20, 21, 22, 23, 24,
    25, 26, 27, 28, 29, 30, 128, 0, 0x0b09
};

void convert_old_symbol_register(void)
{
    unsigned char *record = old_symbol_output;
    unsigned char *name;
    unsigned short length = (unsigned short)(old_symbol_input[0]+4);
    unsigned short count = (unsigned short)(old_symbol_input[0]-4);
    unsigned short reg;
    unsigned padding = 3-((length-1)&3);
    *(unsigned short *)record = (unsigned short)(((length+3)&0xfffc)-2);
    old_symbol_input += 2;
    *(unsigned short *)(record+4) = *(unsigned short *)old_symbol_input;
    old_symbol_input += 2;
    reg = *old_symbol_input++;
    if (reg <= 32) reg = old_register_numbers[reg];
    *(unsigned short *)(record+6) = reg;
    name = record+8;
    while (count--) *name++ = *old_symbol_input++;
    while (padding--) *name++ = 0;
    *(unsigned short *)(record+4) = convert_old_type(*(unsigned short *)(record+4));
    old_symbol_output = name;
}

void size_old_label16(void)
{
    module_type_base += 7;
}

void size_old_block16(void)
{
    module_type_base += 15;
    ++symbol_scope_depth;
}

void convert_old_symbol_label16(void)
{
    unsigned char *record = old_symbol_output;
    unsigned char *name;
    unsigned short length = (unsigned short)(old_symbol_input[0]+5);
    unsigned short count = (unsigned short)(old_symbol_input[0]-4);
    unsigned padding = 3-((length-1)&3);
    *(unsigned short *)record = (unsigned short)(((length+3)&0xfffc)-2);
    *(unsigned short *)(record+6) = old_symbol_segment;
    old_symbol_input += 2;
    *(unsigned short *)(record+4) = *(unsigned short *)old_symbol_input;
    old_symbol_input += 2;
    record[8] = *old_symbol_input++;
    name = record+9;
    while (count--) *name++ = *old_symbol_input++;
    while (padding--) *name++ = 0;
    old_symbol_output = name;
}

void convert_old_symbol_block16(void)
{
    unsigned char *record = old_symbol_output;
    unsigned char *name;
    unsigned short count = (unsigned short)(old_symbol_input[0]-5);
    unsigned short length;
    unsigned padding;
    int has_name = count != 0;
    if (!has_name) count = 1;
    length = (unsigned short)(count+18);
    padding = 3-((length-1)&3);
    *(unsigned short *)record = (unsigned short)(((length+3)&0xfffc)-2);
    *(unsigned long *)(record+4) = 0;
    *(unsigned long *)(record+8) = 0;
    *(unsigned short *)(record+16) = old_symbol_segment;
    old_symbol_input += 2;
    *(unsigned short *)(record+14) = *(unsigned short *)old_symbol_input;
    old_symbol_input += 2;
    *(unsigned short *)(record+12) = *(unsigned short *)old_symbol_input;
    old_symbol_input += 2;
    name = record+18;
    if (has_name)
        while (count--) *name++ = *old_symbol_input++;
    else
        *name++ = 0;
    while (padding--) *name++ = 0;
    old_symbol_output = name;
}

void size_old_register_relative16(void)
{
    module_type_base += 9;
}

void size_old_constant(void)
{
    module_type_base += 9;
}

void size_old_symbol12(void)
{
    module_type_base += 5;
}

void size_old_skip(void)
{
    module_type_count += symbol_input[0] + 1;
}

/* This legacy converter consumes the fixed narrow record, including two
   trailing words; it does not remap either through the type compactor. */
void convert_old_symbol_register_relative16(void)
{
    unsigned char *record = old_symbol_output;
    unsigned short length = old_symbol_input[0]+6;
    unsigned padding = 3-((length-1)&3);
    *(unsigned short *)record = (unsigned short)(((length+3)&0xfffc)-2);
    old_symbol_input += 2;
    *(unsigned short *)(record+4) = *(unsigned short *)old_symbol_input;
    old_symbol_input += 2;
    *(unsigned short *)(record+6) = old_symbol_segment;
    *(unsigned short *)(record+8) = *old_symbol_input++;
    *(unsigned short *)(record+10) = *(unsigned short *)old_symbol_input;
    old_symbol_input += 2;
    *(unsigned short *)(record+12) = *(unsigned short *)old_symbol_input;
    old_symbol_input += 2;
    old_symbol_output += 14;
    while (padding--) *old_symbol_output++ = 0;
}

#define OLD_SIGNED_CONSTANT(t) ((((t)&0xf0)==0x10 && !((t)&0x700)) || (t)==0x72 || (t)==0x74 || (t)==0x70)
void convert_old_symbol_constant(void)
{
    unsigned char *record = old_symbol_output, *input, *output;
    unsigned short type, count, length, padding, leaf, i;
    old_symbol_input += 2;
    type = convert_old_type(*(unsigned short *)old_symbol_input);
    old_symbol_input += 2;
    *(unsigned short *)(record+4) = type;
    output = record+6;
    if (*old_symbol_input >= 0x80)
        convert_old_numeric_leaf(&old_symbol_input, &output);
    else {
        input = old_symbol_input;
        count = *input++;
        leaf = 0;
        if (type >= 0x1000) leaf = 0x8010;
        else switch (count) {
        case 1:
            if (OLD_SIGNED_CONSTANT(type)) leaf = 0x8000;
            if (leaf) {
                *(unsigned short *)output = leaf;
                output += 2;
            }
            *(unsigned short *)output = *input++;
            output += 2;
            break;
        case 2:
            if (*(unsigned short *)input >= 0x8000) {
                *(unsigned short *)output = OLD_SIGNED_CONSTANT(type) ? 0x8001 : 0x8002;
                output += 2;
            }
            *(unsigned short *)output = *(unsigned short *)input;
            input += 2;
            output += 2;
            break;
        case 4:
            if ((type&0xf0)==0x40 && !(type&0x700)) leaf = 0x8005;
            else leaf = OLD_SIGNED_CONSTANT(type) ? 0x8003 : 0x8004;
            *(unsigned short *)output = leaf;
            output += 2;
            *(unsigned long *)output = *(unsigned long *)input;
            input += 4;
            output += 4;
            break;
        case 8:
            if ((type&0xf0)==0x50 && !(type&0x700)) leaf = 0x800c;
            else if ((type&0xf0)==0x40 && !(type&0x700)) leaf = 0x8006;
            else leaf = OLD_SIGNED_CONSTANT(type) ? 0x8009 : 0x800a;
            break;
        case 10: leaf = 0x8007; break;
        case 16:
            leaf = (type&0xf0)==0x50 && !(type&0x700) ? 0x800d : 0x8008;
            break;
        default: leaf = 0x8010; break;
        }
        if (leaf == 0x8010) {
            *(unsigned short *)output = leaf;
            *(unsigned short *)(output+2) = count;
            output += 4;
            for (i = 0; i < count; ++i) *output++ = *input++;
        } else if (count == 8 || count == 10 || count == 16) {
            *(unsigned short *)output = leaf;
            output += 2;
            memcpy(output, input, count);
            output += count;
            input += count;
        }
        old_symbol_input = input;
    }
    count = *old_symbol_input++;
    *output++ = (unsigned char)count;
    while (count--) *output++ = *old_symbol_input++;
    length = (unsigned short)(output-record);
    padding = 3-((length-1)&3);
    *(unsigned short *)record = length+padding-2;
    while (padding--) *output++ = 0;
    old_symbol_output = output;
}
#undef OLD_SIGNED_CONSTANT

void convert_old_symbol12(void)
{
    unsigned char *record = old_symbol_output;
    unsigned short count = old_symbol_input[0]-1;
    unsigned short length = old_symbol_input[0]+3;
    unsigned padding = 3-((length-1)&3);
    *(unsigned short *)record = (unsigned short)(((length+3)&0xfffc)-2);
    old_symbol_input += 2;
    old_symbol_output += 4;
    while (count--) *old_symbol_output++ = *old_symbol_input++;
    while (padding--) *old_symbol_output++ = 0;
    *(unsigned short *)(record+4) = convert_old_type(*(unsigned short *)(record+4));
}

void convert_old_symbol_skip(void)
{
    old_symbol_input += old_symbol_input[0]+1;
}

void size_old_procedure32(void)
{
    ++symbol_scope_depth;
    module_type_base += 23;
}

void size_old_bprel32(void)
{
    module_type_base += 5;
}

void size_old_data32(void)
{
    module_type_base += 5;
}

void size_old_label32(void)
{
    module_type_base += 7;
}

void size_old_block32(void)
{
    module_type_base += 17;
    ++symbol_scope_depth;
}

void convert_old_symbol_procedure32(void)
{
    unsigned char *record = old_symbol_output, *name;
    unsigned short length = old_symbol_input[0]+21;
    unsigned short count = old_symbol_input[0]-16;
    unsigned padding = 3-((length-1)&3);
    *(unsigned short *)record = (unsigned short)(((length+3)&0xfffc)-2);
    *(unsigned long *)(record+4) = 0;
    *(unsigned long *)(record+8) = 0;
    *(unsigned long *)(record+12) = 0;
    *(unsigned short *)(record+32) = old_symbol_segment;
    old_symbol_input += 2;
    *(unsigned long *)(record+28) = *(unsigned long *)old_symbol_input;
    old_symbol_input += 4;
    *(unsigned short *)(record+34) = *(unsigned short *)old_symbol_input;
    old_symbol_input += 2;
    *(unsigned long *)(record+16) = *(unsigned short *)old_symbol_input;
    old_symbol_input += 2;
    *(unsigned long *)(record+20) = *(unsigned short *)old_symbol_input;
    old_symbol_input += 2;
    *(unsigned long *)(record+24) = *(unsigned short *)old_symbol_input;
    old_symbol_input += 4;
    record[36] = *old_symbol_input++;
    name = record+37;
    while (count--) *name++ = *old_symbol_input++;
    while (padding--) *name++ = 0;
    *(unsigned short *)(record+34) = convert_old_type(*(unsigned short *)(record+34));
    old_symbol_output = name;
}

void convert_old_symbol_bprel32(void)
{
    unsigned char *record = old_symbol_output;
    unsigned short length = old_symbol_input[0]+3;
    unsigned short count = old_symbol_input[0]-1;
    unsigned padding = 3-((length-1)&3);
    *(unsigned short *)record = (unsigned short)(((length+3)&0xfffc)-2);
    old_symbol_input += 2;
    old_symbol_output += 4;
    while (count--) *old_symbol_output++ = *old_symbol_input++;
    while (padding--) *old_symbol_output++ = 0;
    *(unsigned short *)(record+8) = convert_old_type(*(unsigned short *)(record+8));
}

void convert_old_symbol_data32(void)
{
    unsigned char *record = old_symbol_output;
    unsigned short length = old_symbol_input[0]+3;
    unsigned short count = old_symbol_input[0]-1;
    unsigned padding = 3-((length-1)&3);
    *(unsigned short *)record = (unsigned short)(((length+3)&0xfffc)-2);
    old_symbol_input += 2;
    old_symbol_output += 4;
    while (count--) *old_symbol_output++ = *old_symbol_input++;
    while (padding--) *old_symbol_output++ = 0;
    *(unsigned short *)(record+10) = convert_old_type(*(unsigned short *)(record+10));
}

void convert_old_symbol_label32(void)
{
    unsigned char *record = old_symbol_output, *name;
    unsigned short length = old_symbol_input[0]+5;
    unsigned short count = old_symbol_input[0]-6;
    unsigned padding = 3-((length-1)&3);
    *(unsigned short *)record = (unsigned short)(((length+3)&0xfffc)-2);
    *(unsigned short *)(record+8) = old_symbol_segment;
    old_symbol_input += 2;
    *(unsigned long *)(record+4) = *(unsigned long *)old_symbol_input;
    old_symbol_input += 4;
    record[10] = *old_symbol_input++;
    name = record+11;
    while (count--) *name++ = *old_symbol_input++;
    while (padding--) *name++ = 0;
    old_symbol_output = name;
}
