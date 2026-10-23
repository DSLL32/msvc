/* Native-only recursive comparison examples; not additional original oracles. */
#define RECON_SHIM_IMPL
#include "types.h"
#include "pack.h"
#include <recon.h>
#undef main

int main(void)
{
    TypeRecord records[2];
    unsigned char data[12], other[12], child[8];
    unsigned char *block[512];
    memset(records, 0, sizeof(records));
    memset(data, 0, sizeof(data));
    memset(other, 0, sizeof(other));
    memset(child, 0, sizeof(child));
    memset(block, 0, sizeof(block));
    type_records = records;
    module_type_base = 0x1000;
    module_type_low = 1;
    global_type_blocks[0] = block;
    records[0].data = data;
    records[1].data = child;
    *(unsigned short *)data = *(unsigned short *)other = 10;
    if (!compare_recursive_type(&records[0], 0x1000, other, 0x1002) ||
        records[0].status != 2 || records[0].mapped_index != 0x1002) return 1;
    other[11] = 1;
    if (compare_recursive_type(&records[0], 0x1000, other, 0x1002) ||
        (records[0].status & 1)) return 2;
    other[11] = 0;
    records[0].status = 0;
    records[0].reference_count = 1;
    records[0].reference_offsets[0] = 4;
    *(unsigned *)(data + 4) = 0x1001;
    *(unsigned *)(other + 4) = 0x1002;
    block[2] = child;
    *(unsigned short *)child = 6;
    if (!compare_recursive_type(&records[0], 0x1000, other, 0x1003) ||
        records[0].status != 2 || records[1].status != 2 ||
        records[1].mapped_index != 0x1002) return 3;
    records[0].status = 0;
    records[1].status = 1;
    records[1].mapped_index = 0x1003;
    if (compare_recursive_type(&records[0], 0x1000, other, 0x1003) ||
        (records[0].status & 1)) return 4;
    puts("equal different recursive conflicting-alias");
    return 0;
}
