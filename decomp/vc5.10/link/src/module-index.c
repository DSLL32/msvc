/* Index records use the same eight-byte buffer allocated during startup. */
#include <windows.h>
#include "initializers.h"
#include <recon.h>

static BYTE reset_module_indices=1;
/* New slots copy the original empty symbol/string cache template. */
static ModuleIndexRecord module_index_value;

DWORD __cdecl allocate_module_index(void)
{
    DWORD requested,capacity,index;
    ModuleIndexRecord *records,*previous;
    if (reset_module_indices) {
        buffer8.remaining=0;
        reset_module_indices=0;
    }
    requested=buffer8.remaining+1;
    if (requested>buffer8.capacity) {
        capacity=(buffer8.capacity*3)>>1;
        if (requested>capacity) capacity=requested;
        records=(ModuleIndexRecord *)crt_operator_new(capacity*8);
        if (!records) return 0;
        if (buffer8.base) {
            previous=(ModuleIndexRecord *)buffer8.base;
            for (index=0;index<buffer8.remaining;++index) {
                records[index].symbols=previous[index].symbols;
                records[index].strings=previous[index].strings;
            }
            crt_operator_delete(buffer8.base);
        }
        buffer8.base=records;
        buffer8.capacity=capacity;
    }
    buffer8.remaining=requested;
    records=(ModuleIndexRecord *)buffer8.base;
    records[requested-1].symbols=module_index_value.symbols;
    records[requested-1].strings=module_index_value.strings;
    return buffer8.remaining-1;
}
