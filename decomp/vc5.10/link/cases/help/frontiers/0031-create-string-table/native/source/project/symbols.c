#include <windows.h>
#include "symbols.h"
#include "files.h"
#include <recon.h>

extern void *__stdcall allocate_link_memory(unsigned,unsigned);

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
{ __asm { int 3 } }
const char *__stdcall string_key_name(const void *key,const void *context)
{ __asm { int 3 } return NULL; }
