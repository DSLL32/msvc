/* Individual symbol rewrite callbacks share the current input cursor. */
#include "pack.h"
#include <recon.h>

/* Keep the counted object name for a precompiled-type module [0040ad00,0040ad64). */
void rewrite_object_name_symbol(void)
{
    unsigned char *name;
    if (has_precompiled) {
        name = symbol_input + 8;
        current_module_info->precompiled_object_name = malloc(name[0] + 1);
        if (!current_module_info->precompiled_object_name)
            fatal_error(1001, NULL, NULL);
        memmove(current_module_info->precompiled_object_name, name, name[0] + 1);
    }
}

/* Keep a procedure only when its segment/address belongs to this module
 * [0040aec0,0040af1b). */
void rewrite_procedure32_symbol(void)
{
    ++public_scope_depth;
    if (!module_contains_address(current_module_info,
                                 *(unsigned short *)(symbol_input + 36),
                                 *(unsigned *)(symbol_input + 32))) {
        ++discard_symbol_depth;
        *(unsigned short *)(symbol_input + 2) = 8;
        return;
    }
    *(unsigned *)(symbol_input + 28) = map_type_index(*(unsigned *)(symbol_input + 28));
}

/* MIPS procedures have register-save fields before the DWORD type/address.
 * Keep their actual section contribution and remap the wide type index.
 * [0040af20,0040af7b), cdecl callback with no arguments. */
void rewrite_mips_procedure_symbol(void)
{
    ++public_scope_depth;
    if (!module_contains_address(current_module_info,
                                 *(unsigned short *)(symbol_input + 52),
                                 *(unsigned *)(symbol_input + 48))) {
        ++discard_symbol_depth;
        *(unsigned short *)(symbol_input + 2) = 8;
        return;
    }
    *(unsigned *)(symbol_input + 44) = map_type_index(*(unsigned *)(symbol_input + 44));
}

/* Module contribution ranges [0040af80,0040afe9). */
int module_contains_address(ModuleInfo *info, unsigned short segment, unsigned offset)
{
    unsigned char *data = info->data;
    unsigned short count, i;
    ModuleCodeSegment *entry;
    if (!data) fatal_error(1001, NULL, NULL);
    count = *(unsigned short *)(data + 4);
    entry = (ModuleCodeSegment *)(data + 8);
    for (i = 0; i < count; ++i, ++entry)
        if (offset - entry->offset < entry->size && segment == entry->segment)
            return 1;
    return 0;
}

/* Remap the type in a BP-relative symbol [0040ac70,0040aca5). */
void rewrite_bp_relative_symbol(void)
{
    if (discard_symbol_depth) *(unsigned short *)(symbol_input + 2) = 8;
    if (!discard_symbol_depth)
        *(unsigned *)(symbol_input + 8) = map_type_index(*(unsigned *)(symbol_input + 8));
}

/* Open a nested block, preserving an enclosing discard [0040ab90,0040abbb). */
void enter_symbol_scope(void)
{
    ++public_scope_depth;
    if (discard_symbol_depth) {
        *(unsigned short *)(symbol_input + 2) = 8;
        ++discard_symbol_depth;
    }
}

/* Close one symbol scope [0040abc0,0040abeb). */
void leave_symbol_scope(void)
{
    --public_scope_depth;
    if (discard_symbol_depth) {
        *(unsigned short *)(symbol_input + 2) = 8;
        --discard_symbol_depth;
    }
}

/* Remap a UDT/constant type and move unscoped names into the global hash
 * [0040acb0,0040acfe). */
void rewrite_type_name_symbol(void)
{
    if (discard_symbol_depth) *(unsigned short *)(symbol_input + 2) = 8;
    if (!discard_symbol_depth) {
        *(unsigned *)(symbol_input + 4) = map_type_index(*(unsigned *)(symbol_input + 4));
        if (!public_scope_depth) insert_global_symbol(symbol_input, symbol_record_key);
    }
}

/* Remap and intern a global-data symbol [0040ae00,0040ae45). */
void rewrite_global_data_symbol(void)
{
    if (discard_symbol_depth) *(unsigned short *)(symbol_input + 2) = 8;
    if (!discard_symbol_depth) {
        *(unsigned *)(symbol_input + 4) = map_type_index(*(unsigned *)(symbol_input + 4));
        insert_global_symbol(symbol_input, symbol_record_key);
    }
}

/* Remap the DWORD type field in register/data/constant records
 * [0040ac30,0040ac65), cdecl callback. */
void rewrite_symbol_type4(void)
{
    if (discard_symbol_depth) *(unsigned short *)(symbol_input + 2) = 8;
    if (!discard_symbol_depth)
        *(unsigned *)(symbol_input + 4) = map_type_index(*(unsigned *)(symbol_input + 4));
}

/* Remap the WORD type in a 16-bit data record [0040ad70,0040ada9). */
void rewrite_data16_symbol(void)
{
    if (discard_symbol_depth) *(unsigned short *)(symbol_input + 2) = 8;
    if (!discard_symbol_depth)
        *(unsigned short *)(symbol_input + 8) =
            (unsigned short)map_type_index(*(unsigned short *)(symbol_input + 8));
}

/* Remap and intern a 16-bit global-data record [0040adb0,0040adf9). */
void rewrite_global_data16_symbol(void)
{
    if (discard_symbol_depth) *(unsigned short *)(symbol_input + 2) = 8;
    if (!discard_symbol_depth) {
        *(unsigned short *)(symbol_input + 8) =
            (unsigned short)map_type_index(*(unsigned short *)(symbol_input + 8));
        insert_global_symbol(symbol_input, symbol_record_key);
    }
}

/* Keep a 16-bit procedure within its module contribution, then map its WORD
 * type [0040ae50,0040aeb2). */
void rewrite_procedure16_symbol(void)
{
    ++public_scope_depth;
    if (!module_contains_address(current_module_info,
                                 *(unsigned short *)(symbol_input + 24),
                                 *(unsigned short *)(symbol_input + 22))) {
        ++discard_symbol_depth;
        *(unsigned short *)(symbol_input + 2) = 8;
        return;
    }
    *(unsigned short *)(symbol_input + 26) =
        (unsigned short)map_type_index(*(unsigned short *)(symbol_input + 26));
}

/* Historical logical name: this is S_BPREL16's WORD type at offset6,
 * not the register-number record [0040abf0,0040ac29). */
void rewrite_register16_symbol(void)
{
    if (discard_symbol_depth) *(unsigned short *)(symbol_input + 2) = 8;
    if (!discard_symbol_depth)
        *(unsigned short *)(symbol_input + 6) =
            (unsigned short)map_type_index(*(unsigned short *)(symbol_input + 6));
}
