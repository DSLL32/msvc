#include <windows.h>
#include <string.h>
#include "symbols.h"
#include "files.h"
#include <recon.h>

extern void *__stdcall allocate_link_memory(unsigned,unsigned);
extern BYTE incremental_enabled;
void *__stdcall hash_allocate(HashState *,MemoryArena **,unsigned,unsigned);

StringTable *__stdcall create_string_table(StringTable **table,unsigned buckets,
                                           unsigned capacity)
{
    *table=(StringTable *)allocate_link_memory(1,sizeof(StringTable));
    initialize_hash_table(&(*table)->hash,buckets,capacity,string_key_name,0);
    (*table)->first=NULL;
    (*table)->last=&(*table)->first;
    return *table;
}
void __stdcall initialize_hash_table(HashTable *hash,unsigned buckets,unsigned capacity,
                                     KeyNameFunction key_name,WORD flags)
{
    HashState *state=(HashState *)allocate_link_memory(1,sizeof(HashState));
    hash->state=state;
    state->bucket_count=buckets;
    state->base_buckets=buckets;
    state->split_bucket=0;
    state->reserved10=0;
    state->reserved24=0;
    state->initial_buckets=buckets;
    state->directory_capacity=capacity;
    state->key_name=key_name;
    state->flags=0;
    state->flags|=flags;
    state->bucket_arena=NULL;
    state->directory_arena=NULL;
    state->incremental_mode=incremental_enabled;
    state->directory=(void ***)hash_allocate(state,&state->directory_arena,capacity,4);
    state->directory[0]=(void **)hash_allocate(state,&state->directory_arena,1,4);
    state->directory[0][0]=hash_allocate(state,&state->bucket_arena,buckets,4);
}
void *__stdcall hash_allocate(HashState *state,MemoryArena **arena,unsigned count,unsigned size)
{
    if (state->incremental_mode) return allocate_link_memory(count,size);
    return reserve_memory(arena,count*size);
}
const char *__stdcall string_key_name(const void *key,const void *context)
{ __asm { int 3 } return NULL; }

void __stdcall grow_hash(HashState *,const void *);
#pragma intrinsic(strcmp)
HashEntry *__stdcall lookup_hash(const char *name,HashState *state,BYTE create,
    const void *strings,BYTE *created)
{
    DWORD hash=0,bucket,previous,character;
    const signed char *cursor;
    HashEntry **slot,*entry;
    const char *key;
    if ((state->entries<<4)/state->bucket_count>48) grow_hash(state,strings);
    for (cursor=(const signed char *)name;*cursor;++cursor) {
        character=(DWORD)(LONG)*cursor;
        previous=(hash>>1)^hash;
        hash+=character<<18;
        hash+=character+previous;
    }
    hash%=0x1eca9d3b;
    bucket=hash%state->base_buckets;
    if (bucket<state->split_bucket) bucket=hash%(state->base_buckets*2);
    slot=(HashEntry **)state->directory[bucket/state->initial_buckets][0]+
        bucket%state->initial_buckets;
    for (entry=*slot;entry;entry=entry->next) {
        key=state->key_name(entry->value,strings);
        if (!strcmp(name,key)) return entry;
    }
    if (!create) return NULL;
    *created=1;
    if (state->incremental_mode) entry=(HashEntry *)allocate_incremental(8);
    else entry=(HashEntry *)reserve_memory(&state->bucket_arena,8);
    entry->value=NULL;
    entry->next=NULL;
    entry->next=*slot;
    *slot=entry;
    ++state->entries;
    return entry;
}
void __stdcall grow_hash(HashState *state,const void *strings) { __asm { int 3 } }
