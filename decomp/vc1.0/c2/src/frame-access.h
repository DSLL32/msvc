#ifndef FRAME_ACCESS_H
#define FRAME_ACCESS_H
#include "frame-symbols.h"
#include "linked-lists.h"
/* Constructor35150 allocates16 bytes in zeroed arena4. Low flag bits
   select operand1/2; bits4/8 mark protected and retained definitions. */
typedef struct FrameAccess {
    struct FrameAccess *next;
    MachineInstruction *instruction;
    int depth;
    unsigned char flags;
} FrameAccess;
typedef char FrameAccessSizeCheck[sizeof(FrameAccess) == 16 ? 1 : -1];
int __fastcall frame_access_distance(FrameAccess *left, FrameAccess *right);
int __fastcall compare_frame_accesses(void *left, void *right);
int __fastcall collect_frame_accesses(FrameAccess **uses, FrameAccess **definitions,
    MachineContext *context, unsigned use_stack_pointer);
void __fastcall combine_frame_accesses(FrameAccess *uses, FrameAccess *definitions);
FrameAccess *__fastcall new_frame_access(FrameAccess *next, MachineInstruction *instruction,
    unsigned char operand, int depth, int protected_block);
#endif
