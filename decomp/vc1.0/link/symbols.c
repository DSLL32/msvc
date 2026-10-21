/* Segmented linear hash table and external-symbol table initialization. */
#include "symbols.h"

void initialize_hash(HashTable **output,unsigned long buckets,
                     unsigned long segments,HashKey key,unsigned short flags)
{
    HashTable *table;
    *output=calloc(1,sizeof(**output));
    if (!*output) out_of_memory();
    table=*output;
    table->bucket_count=table->base_buckets=buckets;
    table->next_split=0;
    table->split_count=0;
    table->enumerators=NULL;
    table->buckets_per_segment=buckets;
    table->directory_capacity=segments;
    table->key=key;
    table->flags=flags;
    table->directory=calloc(segments,sizeof(*table->directory));
    if (!table->directory) out_of_memory();
    table->directory[0]=calloc(1,sizeof(*table->directory[0]));
    if (!table->directory[0]) out_of_memory();
    table->directory[0]->buckets=calloc(buckets,sizeof(HashNode *));
    if (!table->directory[0]->buckets) out_of_memory();
}

void initialize_symbols(SymbolTable **output)
{
    *output=calloc(1,sizeof(**output));
    if (!*output) out_of_memory();
    initialize_hash(&(*output)->hash,1024,128,symbol_key,0);
    (*output)->undefined=NULL;
    (*output)->undefined_tail=&(*output)->undefined;
}

/* Not yet reached. The key callback receives a symbol and string-buffer context. */
const char *symbol_key(const void *symbol,void *strings)
{
    __asm { int 3 }
    return NULL;
}
