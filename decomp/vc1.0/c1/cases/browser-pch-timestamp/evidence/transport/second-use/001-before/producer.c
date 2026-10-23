#include "api.h"
const char producer_stamp[] = API_STAMP;
const char producer_label[] = API_LABEL;
unsigned __cdecl API_NAME(checksum)(unsigned value)
{
    return (value ^ 0x35u) + API_BIAS(2u);
}
unsigned __cdecl API_NAME(record)(const ApiRecord *record, ApiCallback callback)
{
    return API_APPLY(callback,record->value) + record->tag +
           record->count + record->enabled;
}
