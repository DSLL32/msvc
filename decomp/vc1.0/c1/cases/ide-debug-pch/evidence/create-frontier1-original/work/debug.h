#ifndef DEBUG_PCH_H
#define DEBUG_PCH_H
#include "ide-models.h"
#define DEBUG_BIAS IDE_EXPAND(IDE_STEP)

enum DebugOpcode {
    debug_negative = -32769,
    debug_idle = -3,
    debug_ready = 7,
    debug_wide = 70000
};
struct DebugNode {
    struct DebugNode *next;
    enum DebugOpcode opcode;
    unsigned flags;
    double samples[2];
};
typedef int (__cdecl *DebugCallback)(const struct DebugNode *, enum DebugOpcode);
union DebugPayload {
    struct DebugNode node;
    long words[8];
};
struct DebugPacket {
    union DebugPayload payload;
    DebugCallback dispatch;
    const char *caption;
};
#pragma pack(2)
struct DebugCompact {
    char kind;
    unsigned short count;
    struct DebugNode *owner;
};
#pragma pack()

extern struct DebugPacket debug_packet;
extern struct DebugCompact debug_compact;
int inspect_debug_node(const struct DebugNode *, enum DebugOpcode);
int initialize_debug_packet(void);

static __inline int debug_adjust(int value)
{
    if (value < 0) return DEBUG_BIAS;
    return value + DEBUG_BIAS;
}
#endif
