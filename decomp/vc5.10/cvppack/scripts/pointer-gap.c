/* Native-only unresolved-kind control, outside the reconstructed graph. */
#define RECON_SHIM_IMPL
#include "types.h"
#include "pack.h"
#include <recon.h>
#undef main

int main(int argc, char **argv)
{
    unsigned char bytes[32];
    TypeRecord record;
    memset(bytes, 0, sizeof(bytes));
    memset(&record, 0, sizeof(record));
    *(unsigned short *)bytes = 18;
    *(unsigned short *)(bytes + 2) = 0x1002;
    *(unsigned *)(bytes + 4) = 0x74;
    *(unsigned *)(bytes + 8) = argc > 1 ? 4 : 10;
    *(unsigned short *)(bytes + 14) = 0xffff;
    record.data = bytes;
    module_type_low = 2;
    compact_legacy_pointer(&record);
    if (argc > 1) return 120;
    return *(unsigned *)(bytes + 4) == 0x74 ? 0 : 121;
}
