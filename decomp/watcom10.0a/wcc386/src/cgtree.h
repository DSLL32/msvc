#ifndef WCC_CGTREE_H
#define WCC_CGTREE_H
#include "address.h"
#include <recon.h>
/* Old tree.h: DWORD kids precedes byte class/flags/op; no alignment field. */
#pragma pack(4)
typedef struct tree_node {
    type_def *tipe;
    union { struct tree_node *left;name *name;address_name *addr;void *handle; } u;
    unsigned kids;
    unsigned char class,flags,op;
    struct tree_node *rite;
    type_def *optipe;
    name *base;
} tree_node;
#pragma pack()
typedef char old_tree_size[(sizeof(tree_node)==28)?1:-1];
typedef char old_tree_class[(offsetof(tree_node,class)==12)?1:-1];
typedef char old_tree_kids[(offsetof(tree_node,kids)==8)?1:-1];
typedef char old_tree_base[(offsetof(tree_node,base)==24)?1:-1];
/* The bit variant substitutes its operation type/bit coordinates for the
 * normal node's rite/optipe tail. Both allocate the same28-byte union. */
#pragma pack(4)
typedef struct bit_tree_node {
    type_def *tipe;
    union { tree_node *left;name *name;address_name *addr; } u;
    unsigned kids;
    unsigned char class,flags;
    type_def *optipe;
    unsigned char start,len,is_signed;
} bit_tree_node;
#pragma pack()
typedef char old_tree_rite[(offsetof(tree_node,rite)==16)?1:-1];
typedef char old_bit_signed[(offsetof(bit_tree_node,is_signed)==22)?1:-1];
#endif
