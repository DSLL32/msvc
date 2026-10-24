#include "debug.h"
int main(void)
{
    enum DebugOpcode request = debug_ready;
    struct DebugPacket *packet = &debug_packet;
    if (initialize_debug_packet() != 10) return 1;
    if (packet->dispatch(&packet->payload.node, request) != 17) return 2;
    if (debug_compact.kind != 2 || debug_compact.count != 1 ||
        debug_compact.owner != &packet->payload.node) return 3;
    if (packet->payload.node.opcode != request || packet->caption[0] != 'd') return 4;
    return debug_adjust(-1) != DEBUG_BIAS;
}
