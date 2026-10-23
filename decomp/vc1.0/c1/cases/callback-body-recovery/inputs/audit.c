/* Damaged generated callbacks: the emitter dropped parameter lists. */
#include "dispatch.h"
int broken_handler
{
    int local = 3;
    return local;
}
typedef int BrokenFactory
{
    return 4;
}
int recovered_tail(unsigned value)
{
    struct DispatchRecord record;
    record.identifier = value + 7;
    record.callback = 0;
    return (int)record.identifier;
}
