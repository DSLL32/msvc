/* Native-only nonzero identity control; not an original behavior oracle. */
#define RECON_SHIM_IMPL
#include "types.h"
#include <recon.h>
#undef main

int main(void)
{
    TypeRecord record;
    memset(&record, 0, sizeof(record));
    discard_types = 1;
    record.mapped_index = 12345;
    allocate_global_type(&record);
    if (record.mapped_index || record.status || discard_types != 1) return 120;
    type_records = &record;
    type_record_count = 1;
    module_type_base = 0x1000;
    record.mapped_index = 12345;
    if (compact_type_index(0x1000) || record.mapped_index || record.status != 2) return 121;
    record.mapped_index = 12345;
    record.status = 0;
    if (compact_legacy_index(0x200) || record.mapped_index || record.status != 2) return 122;
    return discard_types == 1 ? 0 : 123;
}
