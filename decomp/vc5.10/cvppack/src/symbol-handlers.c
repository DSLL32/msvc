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
