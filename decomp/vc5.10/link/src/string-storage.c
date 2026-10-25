#include <windows.h>
#include <string.h>
#include "symbols.h"
#include "options.h"
#include "memory.h"
#include <recon.h>

/* The twelve-byte string tree nodes use this separate4096-byte pool.
 * These are the encountered original globals, not a copy of buffer12. */
BYTE *symbol_string_pool;
DWORD symbol_string_pool_remaining,symbol_string_pool_previous;
DWORD symbol_string_pool_size=4096;
void *__stdcall allocate_symbol_string_block(unsigned);
void __stdcall reserve_byte_buffer(ByteBuffer *,unsigned);

#pragma intrinsic(strcmp,strlen)
DWORD __stdcall intern_symbol_string(StringTable *table,const char *name)
{
    ByteBuffer *buffer=&table->hash.strings;
    StringTreeNode *node=table->hash.string_tree;
    StringTreeNode **link=&table->hash.string_tree;
    DWORD remaining;
    int comparison;
    while (node) {
        comparison=strcmp(name,(const char *)buffer->data+node->offset);
        if (!comparison) return node->offset;
        link=comparison<0 ? &node->left : &node->right;
        node=*link;
    }
    remaining=symbol_string_pool_remaining;
    symbol_string_pool_previous=remaining;
    if (remaining>=sizeof(StringTreeNode)) {
        symbol_string_pool_remaining=remaining-sizeof(StringTreeNode);
        node=(StringTreeNode *)(symbol_string_pool+symbol_string_pool_size-remaining);
    } else node=(StringTreeNode *)allocate_symbol_string_block(sizeof(StringTreeNode));
    if (!table->hash.string_tree) table->hash.string_tree=node;
    else *link=node;
    if (!buffer->data) {
        reserve_byte_buffer(buffer,0x4000);
        buffer->used+=4;
    }
    node->offset=buffer->used;
    append_bytes(buffer,name,strlen(name)+1);
    node->right=NULL;
    node->left=NULL;
    return node->offset;
}
extern void *__stdcall zero_allocate(unsigned);
void *__stdcall allocate_symbol_string_block(unsigned bytes)
{
    symbol_string_pool=(BYTE *)zero_allocate(symbol_string_pool_size);
    symbol_string_pool_remaining=symbol_string_pool_size-bytes;
    return symbol_string_pool;
}
