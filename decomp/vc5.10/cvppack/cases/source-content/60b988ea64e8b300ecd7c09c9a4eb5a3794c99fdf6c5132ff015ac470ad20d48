/* Follow module-local aliases [00402c20,00402c8e). */
#include "types.h"
#include "pack.h"
#include <recon.h>

TypeRecord *lookup_global_type(unsigned index, unsigned *from_global)
{
    TypeRecord *record;
    *from_global = 0;
    if (!module_type_low && index == 0xffff - module_type_base)
        return empty_type_record;
    record = &type_records[index];
    if (record->status & 0x1000) {
        *from_global = record->global_index;
        /* The recursive result flag reuses this now-dead pointer argument. */
        return lookup_global_type(record->global_index - 0x1000,
                                  (unsigned *)&from_global);
    }
    return record;
}
