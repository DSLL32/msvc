/* Segmented linear hash table and external-symbol table initialization. */
#include "options.h"
#pragma intrinsic(strcmp,strlen,strcpy)
char short_symbol_name[9];

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

External *lookup_external(SymbolTable *table,unsigned short short_name,
                          const char *name,unsigned char *created,
                          unsigned char *imports)
{
    unsigned long offset=0;
    unsigned char inserted=0;
    const char *key=name;
    External **slot, *external;
    if (!short_name) offset=intern_symbol_name(table,name);
    else key=strncpy(short_symbol_name,name,8);
    slot=(External **)find_hash_entry(key,table->hash,1,&table->strings,&inserted);
    if (inserted) {
        *slot=calloc(1,sizeof(**slot));
        if (!*slot) out_of_memory();
        external=*slot;
        if (created) *created=1;
        external->symbol.StorageClass=IMAGE_SYM_CLASS_EXTERNAL;
        if (!short_name) external->symbol.N.Name.Long=offset;
        else strncpy((char *)&external->symbol,key,8);
        external->owner.contribution=NULL;
        external->flags|=EXTERNAL_DEFINED;
        set_external_defined(external,0,table);
    }
    if (*key==0x7f && imports) *imports=1;
    return *slot;
}

void **find_hash_entry(const char *key,HashTable *table,unsigned char insert,
                       void *context,unsigned char *created)
{
    unsigned long bucket;
    HashNode **head,*node;
    if ((double)table->element_count/(double)table->bucket_count>3.0)
        expand_hash(table,context);
    bucket=hash_bucket(key,table);
    head=&table->directory[bucket/table->buckets_per_segment]->buckets[
        bucket%table->buckets_per_segment];
    for (node=*head;node;node=node->next)
        if (!strcmp(key,table->key(node->value,context))) return &node->value;
    if (!insert) return NULL;
    *created=1;
    pool_saved_remaining=pool_remaining;
    if (pool_remaining<sizeof(*node)) node=allocate_pool_block(sizeof(*node));
    else {
        pool_remaining-=sizeof(*node);
        node=(HashNode *)(pool_block+pool_capacity-pool_saved_remaining);
    }
    node->value=NULL;
    node->next=NULL;
    node->next=*head;
    *head=node;
    ++table->element_count;
    return &node->value;
}

/* Linear hashing splits buckets gradually as the table grows. */
unsigned long hash_bucket(const char *key,HashTable *table)
{
    unsigned long hash=0,high,bucket;
    while (*key) {
        hash=(unsigned char)*key++ + hash*4;
        high=hash&0xc000;
        if (high) hash ^= (high>>11)^high;
        hash ^= (hash>>3)+(hash<<5);
    }
    hash%=0x100007;
    bucket=hash%table->base_buckets;
    if (bucket<table->next_split) bucket=hash%(table->base_buckets*2);
    return bucket;
}

void set_external_defined(External *external,unsigned char defined,SymbolTable *table)
{
    External **position;
    ModuleReference *reference;
    const char *name,*other;
    char saved_name[9];
    if (((external->flags&EXTERNAL_DEFINED)!=0)==(defined!=0)) return;
    if (defined) {
        if (external->flags&EXTERNAL_MULTIPLE_REFERENCES) {
            while (external->owner.references) {
                reference=external->owner.references;
                external->owner.references=reference->next;
                free(reference);
            }
            external->flags &= ~EXTERNAL_MULTIPLE_REFERENCES;
        }
        external->flags |= EXTERNAL_DEFINED;
        if (external->undefined_next)
            external->undefined_next->value.previous=external->value.previous;
        *external->value.previous=external->undefined_next;
        if (table->undefined_tail==&external->undefined_next)
            table->undefined_tail=external->value.previous;
        external->value.final_value=0;
        external->undefined_next=NULL;
        return;
    }
    external->flags &= ~EXTERNAL_DEFINED;
    if (old_library_format) {
        if (external->symbol.N.Name.Short)
            name=strncpy(short_symbol_name,(char *)&external->symbol,8);
        else name=table->strings.data+external->symbol.N.Name.Long;
        /* The shared short-name buffer changes while examining the list. */
        if (strlen(name)<=8) {
            strcpy(saved_name,name);
            name=saved_name;
        }
        position=&table->undefined;
        while (*position) {
            if ((*position)->symbol.N.Name.Short)
                other=strncpy(short_symbol_name,(char *)&(*position)->symbol,8);
            else other=table->strings.data+(*position)->symbol.N.Name.Long;
            if (strcmp(name,other)<=0) break;
            position=&(*position)->undefined_next;
        }
    } else position=table->undefined_tail;
    external->undefined_next=*position;
    external->value.previous=position;
    *position=external;
    if (external->undefined_next)
        external->undefined_next->value.previous=&external->undefined_next;
    if (table->undefined_tail==position) table->undefined_tail=&external->undefined_next;
    external->owner.contribution=NULL;
}
