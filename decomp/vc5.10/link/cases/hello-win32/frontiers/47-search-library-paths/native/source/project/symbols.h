#ifndef LINK_SYMBOLS_H
#define LINK_SYMBOLS_H
#include <windows.h>
#include "memory.h"
#include <recon.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef const char *(__stdcall *KeyNameFunction)(const void *,const void *);
typedef struct HashState {
    DWORD entries, bucket_count, physical_buckets, reserved0c, reserved10;
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
    DWORD reserved[7];
} HashTable;
typedef struct StringNode StringNode;
typedef struct StringTable {
    HashTable hash;
    StringNode *first;
    StringNode **last;
#ifdef __cplusplus
    StringNode *RECON_THISCALL Lookup(const unsigned char *,BYTE *);
#endif
} StringTable;
StringTable *__stdcall create_string_table(StringTable **,unsigned,unsigned);
void __stdcall initialize_hash_table(HashTable *,unsigned,unsigned,KeyNameFunction,WORD);
const char *__stdcall string_key_name(const void *,const void *);
typedef char HashStateSize[(sizeof(HashState)==56)?1:-1];
typedef char StringTableSize[(sizeof(StringTable)==40)?1:-1];

#ifdef __cplusplus
}
#endif
#endif
