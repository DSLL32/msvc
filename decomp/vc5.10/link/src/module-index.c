/* Index records use the same eight-byte buffer allocated during startup. */
#include <windows.h>
#include "initializers.h"
#include <recon.h>

typedef struct ModuleIndexRecord {
    DWORD value[2];
} ModuleIndexRecord;
typedef char ModuleIndexRecordSize[(sizeof(ModuleIndexRecord)==8)?1:-1];

static BYTE reset_module_indices=1;
/* These two values are copied into each assigned record. No writer or more
   specific meaning has been found in the original image. */
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
                records[index].value[0]=previous[index].value[0];
                records[index].value[1]=previous[index].value[1];
            }
            crt_operator_delete(buffer8.base);
        }
        buffer8.base=records;
        buffer8.capacity=capacity;
    }
    buffer8.remaining=requested;
    records=(ModuleIndexRecord *)buffer8.base;
    records[requested-1].value[0]=module_index_value.value[0];
    records[requested-1].value[1]=module_index_value.value[1];
    return buffer8.remaining-1;
}
