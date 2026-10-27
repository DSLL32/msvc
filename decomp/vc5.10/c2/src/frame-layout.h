#ifndef C2_FRAME_LAYOUT_H
#define C2_FRAME_LAYOUT_H
#include "function-records.h"
#include <recon.h>

typedef struct C2FrameSlotUse {
    struct C2FrameSlotUse *next;
    C2FunctionRegister *reg;
} C2FrameSlotUse;
typedef struct C2FrameSlot {
    long offset;
    unsigned bytes;
    C2FrameSlotUse *uses;
    struct C2FrameSlot *next;
} C2FrameSlot;
typedef char C2FrameSlotUseSize[sizeof(C2FrameSlotUse) == 8 ? 1 : -1];
typedef char C2FrameSlotSize[sizeof(C2FrameSlot) == 16 ? 1 : -1];
extern C2FrameSlot *frame_slots;
#endif
