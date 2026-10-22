/* Optimization state saved in precompiled and intermediate streams. */
#include "initialization.h"

int optimization_enabled = 1; /* original 0x9fbec */
int *packed_option_values[21] = {
    &normalized_Gs, &normalized_Zr, &optimization_enabled,
    &normalized_PLMN, &normalized_PLMF, &normalized_Gn,
    &normalized_Oa, &normalized_Ow, &options.option_Os,
    &options.option_Oc, &options.option_Og, &options.option_Oe,
    &normalized_Ol, &normalized_Op, &normalized_Oz,
    &options.option_Or, &options.option_Oo, &normalized_Oq,
    &options.option_Ov, &options.option_Of, &normalized_Oy
};

/* Original 0x74690..0x746bc. */
unsigned pack_option_flags(void)
{
    unsigned result = 0, mask = 0x80000000;
    unsigned i;
    for (i = 0; i < 21; ++i, mask >>= 1)
        if (packed_option_values[i] && *packed_option_values[i]) result |= mask;
    return result;
}
