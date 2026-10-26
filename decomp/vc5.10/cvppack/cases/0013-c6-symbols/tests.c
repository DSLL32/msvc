#include <stddef.h>
#include <recon-tool.h>
#include "legacy-types.h"
#include "legacy-symbols.h"
#include <recon.h>

/* Pointer identities are checked by the shared callback audit. These scalar
 * tags/kinds and ABI offsets are the remaining facts of the actual tables at
 * 00412140,004121a8 and004122b8. No target or dependency code runs here. */
static int legacy_dispatch_layout(void)
{
    static const unsigned char leaves[13] = {
        0x7a,0x91,0x79,0x78,0x75,0x80,0x5c,0x72,0x8d,0x8e,0x8c,0xa6,0xa7
    };
    static const unsigned char sizing[14] = {
        0,1,2,4,5,0x0b,0x0c,0x0d,0x0e,0x0f,0x10,0x11,0x16,0x12
    };
    static const unsigned char converting[14] = {
        0,1,2,4,5,0x0b,0x0c,0x0d,0x0e,0x0f,0x16,0x12,0x10,0x11
    };
    static const unsigned short narrow[14] = {
        0x107,0x104,6,0x100,0x101,0x109,0x108,0x1001,0x1002,0x104,0x10a,0x1004,0,0
    };
    static const unsigned short wide[14] = {
        0x207,0x100a,6,0x1006,0x1007,0x209,0x208,0x1001,0x1002,0x100a,0x20a,0x1004,0,0
    };
    unsigned i;
    if (sizeof(LegacyTypeConverter) != 8 || offsetof(LegacyTypeConverter,convert) != 4 ||
        sizeof(LegacySymbolSizer) != 12 || offsetof(LegacySymbolSizer,narrow) != 4 ||
        offsetof(LegacySymbolSizer,wide) != 8 || sizeof(LegacySymbolConverter) != 16 ||
        offsetof(LegacySymbolConverter,narrow_kind) != 2 ||
        offsetof(LegacySymbolConverter,narrow) != 4 ||
        offsetof(LegacySymbolConverter,wide_kind) != 8 ||
        offsetof(LegacySymbolConverter,wide) != 12) return 0;
    for (i = 0; i < 13; ++i)
        if (legacy_type_converters[i].leaf != leaves[i]) return 0;
    for (i = 0; i < 14; ++i) {
        if (legacy_symbol_sizers[i].kind != sizing[i] ||
            legacy_symbol_converters[i].kind != converting[i] ||
            legacy_symbol_converters[i].narrow_kind != narrow[i] ||
            legacy_symbol_converters[i].wide_kind != wide[i]) return 0;
    }
    return 1;
}
const ReconTest cvpack_legacy_tests[1] = {
    { "legacy-dispatch-layout", 0, legacy_dispatch_layout }
};
