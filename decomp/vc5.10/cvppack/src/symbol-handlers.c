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

/* Close one symbol scope [0040abc0,0040abeb). */
void leave_symbol_scope(void)
{
    --public_scope_depth;
    if (discard_symbol_depth) {
        *(unsigned short *)(symbol_input + 2) = 8;
        --discard_symbol_depth;
    }
}
