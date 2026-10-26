#ifndef WCC_QUADS_H
#define WCC_QUADS_H
#include <stddef.h>
#include "symbols.h"
#include <recon.h>
/* The old leaf record is24 bytes. Its encountered scalar constants
 * and symbol-handle variants retain the earlier layout; the later tree
 * representation does not apply. */
#pragma pack(1)
typedef struct leaf {
    unsigned char leaf_class,attributes;
    unsigned short leaf_flags;
    TYPEPTR leaf_type;
    union {
        unsigned words[4];
        void *cg_name;
        struct { unsigned spare,index;void *handle; } label;
        unsigned array_address; /* old address-of-array marker */
        struct { unsigned spare; SYM_HANDLE handle; int offset; } symbol;
        struct { unsigned offset; SYM_HANDLE handle; unsigned references,back_handle; } string;
        struct {
            unsigned spare;
            union { int integer; double floating; } value;
        } constant;
    } u;
} LEAF,*LEAFPTR;
#pragma pack()
typedef char old_string_back_offset[(offsetof(LEAF,u.string.back_handle)==18)?1:-1];
typedef char old_string_offset_offset[(offsetof(LEAF,u.string.offset)==8)?1:-1];
/* The old quad retains token opcode, source location and 16-bit leaf indices. */
#pragma pack(1)
typedef struct quad {
    unsigned char quad_class,quad_flags;
    unsigned short src_line,src_file,operand1,operand2,result;
} QUAD;
#pragma pack()
void QuadGet(QUAD *quad,unsigned index);
unsigned NewQuad(void);
void QuadReplace(QUAD *quad,unsigned index);
extern unsigned QuadIndex,LeafIndex,LeafFreeList;
extern unsigned FirstQuad,FirstCallQuad,LastCallQuad,DefaultLeaf;
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
void FreeLeaf(unsigned index);
void LeafReplace(LEAFPTR leaf,unsigned index);
unsigned GenLeaf(LEAFPTR leaf);
#endif
