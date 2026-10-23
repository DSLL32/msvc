/* Old primitive encodings to CodeView encodings [004025a0,004025cd).
 * The low five bits select a scalar; bits 5..6 select near/far/huge pointers. */
#include "types.h"
#include <recon.h>

unsigned map_primitive_type(unsigned index)
{
    static const unsigned short scalars[32] = {
        0x10, 0x11, 0x12, 0, 0x20, 0x21, 0x22, 0,
        0x40, 0x41, 0x42, 0x22, 0x51, 0x53, 0x52, 0,
        0x30, 0x31, 0x32, 0, 7, 7, 7, 0,
        5, 0, 0, 0, 3, 0, 0, 0
    };
    unsigned scalar, pointer;
    if (!(index & 0x80)) return index < 3 ? index : 7;
    scalar = scalars[index & 0x1f];
    pointer = (index & 0x60) << 3;
    if (!scalar || scalar == 7) return scalar;
    if (scalar == 5 && pointer) return 0;
    return scalar | pointer;
}
