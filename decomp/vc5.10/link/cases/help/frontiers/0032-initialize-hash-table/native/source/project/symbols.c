#include <windows.h>
#include "symbols.h"
#include "files.h"
#include <recon.h>

extern void *__stdcall allocate_link_memory(unsigned,unsigned);
extern BYTE incremental_enabled;
void *__stdcall hash_allocate(HashState *,void **,unsigned,unsigned);

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
    state->physical_buckets=buckets;
    state->bucket_count=buckets;
    state->entries=0;
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
void *__stdcall hash_allocate(HashState *state,void **arena,unsigned count,unsigned size)
{ __asm { int 3 } return NULL; }
const char *__stdcall string_key_name(const void *key,const void *context)
{ __asm { int 3 } return NULL; }
