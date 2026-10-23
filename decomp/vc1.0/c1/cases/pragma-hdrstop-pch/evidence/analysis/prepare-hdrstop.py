from pathlib import Path
import json
b=Path('/root/msvc2/decomp/vc1.0/c1');c=b/'cases/pragma-hdrstop-pch';(c/'inputs').mkdir(parents=True)
(c/'inputs/api.h').write_text('''#ifndef CALIBRATION_API_H
#define CALIBRATION_API_H
#define CALIBRATION_GAIN(x) ((x) * 3 + 5)
#define CALIBRATION_BIAS 7
typedef struct Calibration {
    signed char bias;
    unsigned short scale;
    unsigned flags;
} Calibration;
typedef int (__cdecl *Calibrate)(const Calibration *, int);
int __cdecl calibrate_record(const Calibration *, int);
#endif
''')
(c/'inputs/producer.c').write_text('''#include "api.h"
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
''')
(c/'inputs/consumer.c').write_text('''#include "api.h"
#pragma hdrstop
int main(void)
{
    Calibration record = { -3, 4, 10 };
    Calibrate callback = calibrate_record;
    int first = callback(&record, 6);
    record.bias = CALIBRATION_BIAS;
    record.scale = 2;
    return first != 36 || calibrate_record(&record, -2) != 41;
}
''')
q=dict(authorization='Root approved SINGLE public pragma-hdrstop bareYc/Yu PCH producer/consumer checksum, posthdrstop namedpack reuse. Originalfullcompile/link/runtime0 plus actualtransport/PCH/path/mtime beforefreeze. Optionalfilename/browser only if supported. No retiredpackingpointer workaround/privateflags.',baseline_commit='ff5df66c3b54b2ef89e6a1643255fcdea18a0b23',baseline_accepted_count=999,status='draft original preflight; no accepted fixture yet',cases=[dict(case='pragma-hdrstop-pch',phases=['c1-create','c1-use'],status='original syntax/runtime preflight pending')]);(b/'cases/approved-queue.json').write_text(json.dumps(q,indent=2)+'\n')
