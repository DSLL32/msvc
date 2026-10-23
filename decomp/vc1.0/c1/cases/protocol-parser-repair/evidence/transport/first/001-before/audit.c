int compiled_prefix(int value) { return value + 3; }
#include "protocol.h"

int recovered_tail(int value)
{
    WireHeader header;
    header.length = (unsigned)value;
    header.flags = 5;
    return (int)(header.length + header.flags);
}
