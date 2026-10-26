#ifndef LINK_SYMBOLS_H
#define LINK_SYMBOLS_H
#include <windows.h>
#include <stddef.h>
#include "memory.h"
#include <recon.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef struct ByteBuffer { BYTE *data; unsigned used, capacity; } ByteBuffer;
typedef struct StringTreeNode {
    DWORD offset;
    struct StringTreeNode *left,*right;
} StringTreeNode;
typedef char StringTreeNodeSize[(sizeof(StringTreeNode)==12)?1:-1];
typedef const char *(__stdcall *KeyNameFunction)(const void *,const void *);
typedef struct HashState {
    DWORD split_bucket, base_buckets, bucket_count, entries, reserved10;
    unsigned directory_capacity, initial_buckets;
    WORD flags, reserved1e;
    KeyNameFunction key_name;
    DWORD reserved24;
    void ***directory;
    MemoryArena *directory_arena, *bucket_arena;
    BYTE incremental_mode, reserved35[3];
} HashState;
typedef struct HashTable {
    HashState *state;
    /* These views describe the same embedded buffer, not separate storage. */
    union { const void *string_storage; ByteBuffer strings; };
    StringTreeNode *string_tree;
    void *lookup_buffers[3];
} HashTable;
typedef struct HashEntry {
    void *value;
    struct HashEntry *next;
} HashEntry;
typedef char HashEntrySize[(sizeof(HashEntry)==8)?1:-1];
HashEntry *__stdcall lookup_hash(const char *,HashState *,BYTE,const void *,BYTE *);
typedef struct StringNode StringNode;
typedef struct StringTable {
    HashTable hash;
    StringNode *undefined_first;
    StringNode **undefined_last;
#ifdef __cplusplus
    StringNode *RECON_THISCALL Lookup(const unsigned char *,BYTE *);
    void RECON_THISCALL SortSymbols(void);
    void RECON_THISCALL ReleaseLookupIndex(void);
    StringNode * RECON_THISCALL LookupSymbol(WORD kind,const char *name,BYTE *created);
    StringNode * RECON_THISCALL FindSymbol(const char *name);
#endif
} StringTable;
StringTable *__stdcall create_string_table(StringTable **,unsigned,unsigned);
void __stdcall initialize_hash_table(HashTable *,unsigned,unsigned,KeyNameFunction,WORD);
const char *__stdcall string_key_name(const void *,const void *);
typedef char HashStateSize[(sizeof(HashState)==56)?1:-1];
typedef char StringTreeAt10[(offsetof(HashTable,string_tree)==0x10)?1:-1];
typedef char HashTableSize[(sizeof(HashTable)==32)?1:-1];
typedef char StringStorageAt04[(offsetof(HashTable,string_storage)==4)?1:-1];
typedef char StringTableSize[(sizeof(StringTable)==40)?1:-1];

#ifdef __cplusplus
}
#endif
#endif
