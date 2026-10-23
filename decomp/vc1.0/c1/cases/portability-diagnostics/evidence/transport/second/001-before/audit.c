#include "portability.h"

struct AuditRecord { int value; unsigned flags; };

int cast_probe(void)
{
    struct AuditRecord record = (struct AuditRecord)7;
    return record.value;
}

int recovered_tail(int value)
{
    unsigned bits = (unsigned)value;
    return (int)((bits * 5U + 9U) & 255U);
}
