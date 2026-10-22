/* Optimization state bits saved in precompiled and intermediate streams. */
#include "initialization.h"
#include "pragma.h"

int optimization_enabled = 1; /* original 0x6bf98 */
int *packed_option_values[21] = {
    &normalized_Gs, &normalized_Zr, &optimization_enabled,
    &normalized_PLMN, &normalized_PLMF, &normalized_Gn,
    &normalized_Oa, &normalized_Ow, &options.option_Os,
    &options.option_Oc, &options.option_Og, &options.option_Oe,
    &normalized_Ol, &normalized_Op, &normalized_Oz,
    &options.option_Or, &options.option_Oo, &normalized_Oq,
    &options.option_Ov, &options.option_Of, &normalized_Oy
};

/* Original 0x4bee0..0x4bf0c. */
unsigned pack_option_flags(void)
{
    unsigned result = 0, mask = 0x80000000;
    unsigned i;
    for (i = 0; i < 21; ++i, mask >>= 1)
        if (packed_option_values[i] && *packed_option_values[i])
            result |= mask;
    return result;
}

/* Select the independently changeable optimization options in #pragma optimize. */
unsigned pragma_optimization_restore_mask = 0x03ffb800;
unsigned pragma_optimization_disable_mask = 0x037eb800;
PragmaOptimizationRule pragma_optimization_rules[] = {
    { 'a', 1, &normalized_Oa }, { 'w', 1, &normalized_Ow },
    { 't', 1, &options.option_Os }, { 's', 0, &options.option_Os },
    { 'c', 1, &options.option_Oc }, { 'g', 1, &options.option_Og },
    { 'e', 1, &options.option_Oe }, { 'l', 1, &normalized_Ol },
    { 'p', 1, &normalized_Op }, { 'z', 1, &normalized_Oz },
    { 'n', 0, &normalized_Oz }, { 'r', 1, &options.option_Or },
    { 'o', 1, &options.option_Oo }, { 'y', 1, &normalized_Oy },
    { 'q', 1, &normalized_Oq }, { 'f', 1, &options.option_Of },
    { 'v', 1, &options.option_Ov }, { 0, 0, 0 }
};

/* Original 0x4bf10..0x4bf3b. Restore each independently mutable option
   from the high bit, advancing even for an unused option-table entry. */
void unpack_option_flags(unsigned flags)
{
    unsigned i;
    for (i = 0; i < 21; ++i) {
        if (packed_option_values[i]) *packed_option_values[i] = (flags >> 31) != 0;
        flags <<= 1;
    }
}
