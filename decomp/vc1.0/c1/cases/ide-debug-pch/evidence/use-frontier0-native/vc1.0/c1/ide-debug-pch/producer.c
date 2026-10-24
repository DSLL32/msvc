#include "debug.h"
struct DebugPacket debug_packet;
struct DebugCompact debug_compact;

int inspect_debug_node(const struct DebugNode *node, enum DebugOpcode opcode)
{
    return (int)node->samples[0] + opcode + DEBUG_BIAS;
}
int initialize_debug_packet(void)
{
    debug_packet.payload.node.next = 0;
    debug_packet.payload.node.opcode = debug_ready;
    debug_packet.payload.node.flags = 5;
    debug_packet.payload.node.samples[0] = 3.0;
    debug_packet.payload.node.samples[1] = 0.5;
    debug_packet.dispatch = inspect_debug_node;
    debug_packet.caption = "debug pch";
    debug_compact.kind = 2;
    debug_compact.count = 1;
    debug_compact.owner = &debug_packet.payload.node;
    return debug_adjust(3);
}
