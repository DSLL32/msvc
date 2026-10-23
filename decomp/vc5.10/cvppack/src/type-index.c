/* Index a completed recursive type [0040b890,0040b8ff). */
#include "types.h"
#include <recon.h>

unsigned interned_nodes_remaining;
InternedType *interned_node_cursor;

void __fastcall index_recursive_type(unsigned char *data, unsigned index)
{
    InternedType *node;
    unsigned short bucket;
    if (!interned_nodes_remaining) {
        node = allocate_bytes(0x3000);
        interned_nodes_remaining = 1024;
    } else node = interned_node_cursor;
    --interned_nodes_remaining;
    interned_node_cursor = node + 1;
    bucket = hash_type_record(data);
    node->data = data;
    node->index = index;
    node->next = ((InternedType **)type_table_secondary)[bucket];
    ((InternedType **)type_table_secondary)[bucket] = node;
    ++((unsigned short *)type_table_secondary_counts)[bucket];
}
