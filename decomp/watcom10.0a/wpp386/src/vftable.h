#ifndef WPP_VFTABLE_H
#define WPP_VFTABLE_H
#include <stddef.h>
#include "types.h"
#include <recon.h>
/* Encountered1994 table prefix:24B common header, then remaining count
 * and control flags. The later header's ctor_disp field is absent. */
typedef struct class_table CLASS_TABLE;
#pragma pack(1)
struct class_table {
    CLASS_TABLE *next;
    long vb_offset,vb_index,delta,exact_delta;
    unsigned count;
};
typedef struct thunk_cast {
    long vb_offset,vb_index,delta;
} THUNK_CAST;
typedef struct thunk_action {
    struct symbol *sym,*thunk;
    long delta;
    THUNK_CAST in;
    struct symbol *override;
    THUNK_CAST out;
    unsigned ctor_disp:1,input_virtual:1,output_virtual:1,non_empty:1,
        last_entry:1,possible_ambiguity:1,reserved:26;
} THUNK_ACTION;
typedef struct class_vftable {
    CLASS_TABLE h;
    unsigned amt_left;
    unsigned ambiguities:1,corrupted:1,reserved:30;
    THUNK_ACTION data[1];
} CLASS_VFTABLE;
typedef struct class_vbtable {
    CLASS_TABLE h;
    TYPE data[1];
} CLASS_VBTABLE;
#pragma pack()
typedef char ClassVfTablePrefixSize[(offsetof(CLASS_VFTABLE,data)==32 && sizeof(THUNK_ACTION)==44)?1:-1];
#endif
