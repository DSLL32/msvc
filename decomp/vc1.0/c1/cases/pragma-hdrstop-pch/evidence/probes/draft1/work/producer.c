#include "api.h"
#pragma hdrstop
/* Packing entries are reused only after the PCH has been written. */
#pragma pack(push, wire, 2)
struct WireRecord { char tag; long count; short delta; };
#pragma pack(pop, wire)
#pragma pack(push, bytes, 1)
struct ByteRecord { char tag; long count; short delta; };
#pragma pack(pop, bytes)
int __cdecl calibrate_record(const Calibration *record, int value)
{
    return CALIBRATION_GAIN(value) + record->bias * record->scale +
        (int)(record->flags & 15) + sizeof(struct WireRecord) +
        sizeof(struct ByteRecord);
}
