#ifndef WPP_VSTK_H
#define WPP_VSTK_H
#include <recon.h>
typedef struct vstk_blk VSTK_BLK;
typedef struct vstk_ctl {
    void *current;
    VSTK_BLK *top, *freed;
    unsigned short per_block, size;
} VSTK_CTL;
struct vstk_blk { VSTK_BLK *last; char data[1]; };
void VstkOpen(VSTK_CTL *stack, unsigned size, unsigned count);
#endif
