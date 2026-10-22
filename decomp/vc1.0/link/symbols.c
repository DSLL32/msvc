/* Segmented linear hash table and external-symbol table initialization. */
#include "options.h"
#include "link-pass.h"
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

/* Hash callbacks reuse a short-name buffer independently of lookup's key. */
char hash_short_name[9]="        ";
char *hash_short_name_buffer=hash_short_name;
const char *symbol_key(const void *symbol,void *strings)
{
    const External *external=symbol;
    TextBuffer *buffer=strings;
    if (!external->symbol.N.Name.Short)
        return buffer->data+external->symbol.N.Name.Long;
    return strncpy(hash_short_name_buffer,(const char *)&external->symbol,8);
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
        if (external->state.undefined_next)
            external->state.undefined_next->value.previous=external->value.previous;
        *external->value.previous=external->state.undefined_next;
        if (table->undefined_tail==&external->state.undefined_next)
            table->undefined_tail=external->value.previous;
        external->value.final_value=0;
        external->state.undefined_next=NULL;
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
            position=&(*position)->state.undefined_next;
        }
    } else position=table->undefined_tail;
    external->state.undefined_next=*position;
    external->value.previous=position;
    *position=external;
    if (external->state.undefined_next)
        external->state.undefined_next->value.previous=&external->state.undefined_next;
    if (table->undefined_tail==position) table->undefined_tail=&external->state.undefined_next;
    external->owner.contribution=NULL;
}

struct StringNode {
    unsigned long offset;
    StringNode *lower,*upper;
};

unsigned long intern_symbol_name(SymbolTable *table,const char *name)
{
    StringNode **position=&table->string_tree,*node;
    int order;
    while ((node=*position)!=NULL) {
        order=strcmp(name,table->strings.data+node->offset);
        if (!order) return node->offset;
        position=order<0?&node->lower:&node->upper;
    }
    pool_saved_remaining=pool_remaining;
    if (pool_remaining<sizeof(*node)) node=allocate_pool_block(sizeof(*node));
    else {
        pool_remaining-=sizeof(*node);
        node=(StringNode *)(pool_block+pool_capacity-pool_saved_remaining);
    }
    *position=node;
    if (!table->strings.data) {
        reserve_text(&table->strings,16384);
        table->strings.length+=4;
    }
    node->offset=table->strings.length;
    append_text(&table->strings,name,strlen(name)+1);
    node->lower=node->upper=NULL;
    return node->offset;
}

/* A lone reference fits in the owner union; subsequent ones form a list. */
void record_undefined_reference(External *external,Module *module)
{
    ModuleReference *reference;
    if (!(external->flags&EXTERNAL_MULTIPLE_REFERENCES)) {
        if (!external->owner.module) {
            external->owner.module=module;
            return;
        }
        reference=malloc(sizeof(*reference));
        if (!reference) out_of_memory();
        reference->module=external->owner.module;
        reference->next=NULL;
        external->flags |= EXTERNAL_MULTIPLE_REFERENCES;
        external->owner.references=reference;
    }
    reference=malloc(sizeof(*reference));
    if (!reference) out_of_memory();
    reference->module=module;
    reference->next=external->owner.references;
    external->owner.references=reference;
}

unsigned char external_referenced_by(External *external,Module *module)
{
    ReferenceEnumerator references;
    initialize_reference_enumerator(&references,external);
    while (next_reference(&references)) {
        if (references.current==module) {
            end_reference_enumerator(&references);
            return 1;
        }
    }
    return 0;
}

void initialize_reference_enumerator(ReferenceEnumerator *scan,External *external)
{
    scan->next=next_reference;
    scan->end=end_reference_enumerator;
    scan->external=external;
    if (external->flags&EXTERNAL_MULTIPLE_REFERENCES)
        scan->remaining=external->owner.references;
}

unsigned char next_reference(ReferenceEnumerator *scan)
{
    if (!scan->external) return 0;
    if (!(scan->external->flags&EXTERNAL_MULTIPLE_REFERENCES)) {
        scan->current=scan->external->owner.module;
        scan->external=NULL;
        return scan->current!=NULL;
    }
    if (!scan->remaining) return 0;
    scan->current=scan->remaining->module;
    scan->remaining=scan->remaining->next;
    return 1;
}

void end_reference_enumerator(ReferenceEnumerator *scan)
{
}

void initialize_undefined_enumerator(UndefinedEnumerator *scan,SymbolTable *table)
{
    scan->next=next_undefined;
    scan->end=end_undefined_enumerator;
    scan->remaining=table->undefined;
}

unsigned char next_undefined(UndefinedEnumerator *scan)
{
    scan->current=scan->remaining;
    if (!scan->current) return 0;
    scan->remaining=scan->current->state.undefined_next;
    return 1;
}

int compare_string_pointers(const void *left,const void *right)
{
    return strcmp(*(const char *const *)left,*(const char *const *)right);
}

void prepend_external(ExternalLink **list,External *external)
{
    ExternalLink *entry=malloc(sizeof(*entry));
    if (!entry) out_of_memory();
    entry->external=external;
    entry->next=*list;
    *list=entry;
}

/* Comparator context shared by the cached name sort. */
TextBuffer sort_string_buffer;
External **get_externals_by_name(SymbolTable *table)
{
    if (!table->sorted_by_name) {
        sort_string_buffer=table->strings;
        table->sorted_by_name=sort_external_table(table,compare_external_names);
    }
    return table->sorted_by_name;
}

External **sort_external_table(SymbolTable *table,ExternalCompare compare)
{
    External **entries,**cursor;
    unsigned long count,remaining;
    set_hash_flags(table->hash,2);
    count=count_externals(table);
    entries=calloc(count,sizeof(*entries));
    if (!entries) out_of_memory();
    begin_external_enumeration(table);
    cursor=entries;
    for (remaining=count;remaining;--remaining) *cursor++=next_external(table);
    end_external_enumeration(table);
    qsort(entries,count,sizeof(*entries),compare);
    return entries;
}

void set_hash_flags(HashTable *table,unsigned short flags)
{
    table->flags=flags;
}


unsigned long count_externals(SymbolTable *table)
{
    return count_hash_entries(table->hash);
}


unsigned long count_hash_entries(HashTable *table)
{
    return table->element_count;
}


void begin_external_enumeration(SymbolTable *table)
{
    begin_hash_enumeration(table->hash);
}


void begin_hash_enumeration(HashTable *table)
{
    HashCursor *previous=table->enumerators;
    table->enumerators=calloc(1,sizeof(*table->enumerators));
    if (!table->enumerators) out_of_memory();
    table->enumerators->previous=previous;
    table->enumerators->bucket=0;
    table->enumerators->returned=0;
    table->enumerators->next=NULL;
}


External *next_external(SymbolTable *table)
{
    HashNode *entry=next_hash_entry(table->hash);
    return entry ? entry->value : NULL;
}


HashNode *next_hash_entry(HashTable *table)
{
    HashCursor *scan=table->enumerators;
    HashNode *entry;
    unsigned long bucket;
    if (scan->returned>=table->element_count) return NULL;
    if (scan->next) {
        entry=scan->next;
        scan->next=entry->next;
        ++scan->returned;
        return entry;
    }
    while ((bucket=scan->bucket)<table->bucket_count) {
        entry=table->directory[bucket/table->buckets_per_segment]->buckets[
                              bucket%table->buckets_per_segment];
        scan->bucket=bucket+1;
        if (entry) {
            ++scan->returned;
            scan->next=entry->next;
            return entry;
        }
    }
    return NULL;
}


void end_external_enumeration(SymbolTable *table)
{
    end_hash_enumeration(table->hash);
}


void end_hash_enumeration(HashTable *table)
{
    HashCursor *scan=table->enumerators;
    table->enumerators=scan->previous;
    free(scan);
}


/* Both short COFF names must remain valid until the comparison completes. */
char sort_short_name_left[9]="        ";
char sort_short_name_right[9]="        ";
int compare_external_names(const void *left,const void *right)
{
    External *a=*(External *const *)left,*b=*(External *const *)right;
    const char *aname=sort_short_name_left,*bname=sort_short_name_right;
    if (!a->symbol.N.Name.Short)
        aname=sort_string_buffer.data+a->symbol.N.Name.Long;
    else strncpy(sort_short_name_left,(const char *)&a->symbol,8);
    if (!b->symbol.N.Name.Short)
        bname=sort_string_buffer.data+b->symbol.N.Name.Long;
    else strncpy(sort_short_name_right,(const char *)&b->symbol,8);
    return strcmp(aname,bname);
}

