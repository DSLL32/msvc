#ifndef WPP_CARVE_H
#define WPP_CARVE_H
#include <recon.h>
typedef struct carve_block CARVE_BLOCK;
typedef struct carve_free CARVE_FREE;
struct carve_block { CARVE_BLOCK *next; char data[1]; };
struct carve_free { CARVE_FREE *next, *previous; };
typedef struct carve_control {
    unsigned elm_size, elm_count, blk_top, blk_count;
    void *cursor;
    CARVE_BLOCK *blk_list;
    CARVE_FREE *free_ring;
    void *index;
} *carve_t;
/* The old control precedes the reordered2002 cv_t and is32bytes. */
typedef char CarveControlSize[(sizeof(struct carve_control)==32)?1:-1];
carve_t CarveCreate(unsigned size, unsigned count);
void *CarveAlloc(carve_t cv);
void *newBlk(carve_t cv);
#endif
