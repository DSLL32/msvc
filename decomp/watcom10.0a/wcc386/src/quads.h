#ifndef WCC_QUADS_H
#define WCC_QUADS_H
#include "symbols.h"
#include <recon.h>
/* The old leaf record is24 bytes. Only the constructor's class/type fields
 * have been encountered; name the remaining union members as consumers
 * are reconstructed rather than import the later tree representation. */
typedef struct leaf {
    unsigned char leaf_class,attributes;
    unsigned short leaf_flags;
    TYPEPTR leaf_type;
    unsigned payload[4];
} LEAF,*LEAFPTR;
extern unsigned QuadIndex,LeafIndex,LeafFreeList;
extern unsigned FirstQuad,LastStatementQuad,LastQuad,DefaultLeaf;
extern struct seg_info QuadSegs[49],LeafSegs[98];
#pragma pack(1)
struct int_file_info {
    unsigned curr_buf_num,last_buf_num,curr_seg_num,items_per_buf,size_of_item;
    struct seg_info *seg_table;
    SEGADDR_T curr_buf_seg;
    char *buffer;
    char curr_buf_dirty;
};
#pragma pack()
extern struct int_file_info QuadFileInfo,LeafFileInfo;
void *AccessFileInfo(unsigned index,struct int_file_info *info);
void InitFileInfo(struct int_file_info *info,struct seg_info *segments,unsigned item_size);
void InitIntermediateFiles(void);
void InitDataQuads(void);
void InitLeaf(LEAFPTR leaf);
unsigned NewLeaf(void);
void LeafGet(LEAFPTR leaf,unsigned index);
void LeafReplace(LEAFPTR leaf,unsigned index);
unsigned GenLeaf(LEAFPTR leaf);
#endif
