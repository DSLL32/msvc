#include <stddef.h>
#include <recon-tool.h>
#include "legacy-symbols.h"
#include <recon.h>

/* Common callback checks prove pointer identities. These are the remaining
 * scalar/ABI facts of the actual table at00412288..004122b8. */
static int base_symbol_layout(void)
{
    static const unsigned char kinds[3] = {4, 5, 0x0d};
    static const unsigned short narrow[3] = {0x100, 0x101, 0x1001};
    static const unsigned short wide[3] = {0x1006, 0x1007, 0x1001};
    unsigned i;
    if (sizeof(LegacyBaseSymbolConverter) != 16 ||
        offsetof(LegacyBaseSymbolConverter, narrow) != 4 ||
        offsetof(LegacyBaseSymbolConverter, wide) != 12) return 0;
    for (i = 0; i < 3; ++i)
        if (legacy_base_symbol_converters[i].kind != kinds[i] ||
            legacy_base_symbol_converters[i].narrow_kind != narrow[i] ||
            legacy_base_symbol_converters[i].wide_kind != wide[i]) return 0;
    return 1;
}
const ReconTest cvpack_pointer_tests[1] = {
    { "base-symbol-layout", 0, base_symbol_layout }
};
