/* Generated dispatch adapter with bounded type/expression damage. */
#include "protocol.h"
int validate_header(struct PendingPacket *packet, int *linear, DispatchCallback callback)
{
    int (*rows)[4];
    int size = sizeof(*packet);
    rows = linear;
    size = dispatch_frame - size;
    size = ~dispatch_frame;
    rows = callback;
    return size;
}
int recovered_tail(int value) { return value + 3; }
