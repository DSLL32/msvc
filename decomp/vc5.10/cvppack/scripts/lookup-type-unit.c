/* Native-only output ownership examples, not replacement original oracles. */
#define RECON_SHIM_IMPL
#include "types.h"
#include "pack.h"
#include <recon.h>
#undef main

int main(void)
{
    TypeRecord records[3], sentinel;
    TypeRecord *result;
    unsigned output;
    memset(records, 0, sizeof(records));
    memset(&sentinel, 0, sizeof(sentinel));
    type_records = records;
    module_type_base = 0x1000;
    module_type_low = 1;
    empty_type_record = &sentinel;
    records[0].status = records[1].status = 0x1000;
    records[0].global_index = 0x1001;
    records[1].global_index = 0x1002;
    output = 0xfeedface;
    result = lookup_global_type(0, &output);
    if (result != &records[2] || output != 0x1001) return 1;
    output = 0xfeedface;
    result = lookup_global_type(2, &output);
    if (result != &records[2] || output) return 2;
    module_type_low = 0;
    output = 0xfeedface;
    result = lookup_global_type(0xffff - module_type_base, &output);
    if (result != &sentinel || output) return 3;
    puts("recursive-parent-output direct-zero sentinel-zero");
    return 0;
}
