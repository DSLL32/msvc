#include <windows.h>
#include <string.h>
#include <stdlib.h>
#include <mbstring.h>
#include "symbols.h"
#include "files.h"
#include "full-link.h"
#include "full-link-functions.h"
#include "sections.h"
#include "options.h"
#include "weak-symbols.h"
#include <recon.h>

extern void *__stdcall allocate_link_memory(unsigned,unsigned);
extern BYTE incremental_enabled;
void *__stdcall hash_allocate(HashState *,MemoryArena **,unsigned,unsigned);

StringTable *__stdcall create_string_table(StringTable **table,unsigned buckets,
                                           unsigned capacity)
{
    *table=(StringTable *)allocate_link_memory(1,sizeof(StringTable));
    initialize_hash_table(&(*table)->hash,buckets,capacity,string_key_name,0);
    (*table)->undefined_first=NULL;
    (*table)->undefined_last=&(*table)->undefined_first;
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
    state->enumerator=NULL;
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
/* Hash callbacks share this separate short-name result at original4948e8.
 * The eight-byte copy pads short names; the ninth remains the BSS terminator. */
static char hash_short_name[9];
const char *__stdcall string_key_name(const void *key,const void *context)
{
    const IMAGE_SYMBOL *symbol=(const IMAGE_SYMBOL *)key;
    const ByteBuffer *strings=(const ByteBuffer *)context;
    if (!symbol->N.Name.Short)
        return (const char *)strings->data+symbol->N.Name.Long;
    return strncpy(hash_short_name,(const char *)symbol->N.ShortName,8);
}

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
WORD __stdcall get_hash_flags(HashState *);
void __stdcall set_hash_flags(HashState *,WORD);
/* Split one bucket of the linear hash, retaining chain order on both sides. */
void __stdcall grow_hash(HashState *state,const void *strings)
{
    DWORD split=state->split_bucket;
    DWORD added=split+state->base_buckets;
    DWORD old_index=split%state->initial_buckets;
    DWORD new_index=added%state->initial_buckets;
    void **old_block,**new_block;
    HashEntry *entry,*previous=NULL,*tail=NULL,*next;
    HashEntry **old_bucket,**new_bucket;
    const signed char *name;
    DWORD hash,previous_hash,character,bucket;
    if(added>=state->directory_capacity*state->initial_buckets) {
        set_hash_flags(state,(WORD)(get_hash_flags(state)|1));
        return;
    }
    ++state->reserved10;
    old_block=state->directory[split/state->initial_buckets];
    if(!new_index) {
        state->directory[added/state->initial_buckets]=(void **)hash_allocate(
            state,&state->directory_arena,1,4);
        state->directory[added/state->initial_buckets][0]=hash_allocate(
            state,&state->bucket_arena,state->initial_buckets,4);
    }
    new_block=state->directory[added/state->initial_buckets];
    ++state->split_bucket;
    if(state->split_bucket==state->base_buckets) {
        state->base_buckets*=2;
        state->split_bucket=0;
    }
    ++state->bucket_count;
    old_bucket=(HashEntry **)old_block[0]+old_index;
    new_bucket=(HashEntry **)new_block[0]+new_index;
    entry=*old_bucket;
    *new_bucket=NULL;
    while(entry) {
        name=(const signed char *)state->key_name(entry->value,strings);
        hash=0;
        while(*name) {
            character=(DWORD)(LONG)*name++;
            previous_hash=(hash>>1)^hash;
            hash+=character<<18;
            hash+=character+previous_hash;
        }
        hash%=0x1eca9d3b;
        bucket=hash%state->base_buckets;
        if(bucket<state->split_bucket)bucket=hash%(state->base_buckets*2);
        next=entry->next;
        if(bucket==added) {
            if(tail)tail->next=entry;
            else *new_bucket=entry;
            if(previous)previous->next=next;
            else *old_bucket=next;
            tail=entry;
            tail->next=NULL;
        } else previous=entry;
        entry=next;
    }
}

extern BYTE incremental_enabled;
void __stdcall release_memory(void *);
void __stdcall set_symbol_defined(StringNode *symbol,BYTE defined,StringTable *table)
{
    SymbolAuxiliary *auxiliary;
    StringNode **previous;
    if ((symbol->flag_byte & 1)==(defined!=0)) return;
    if (defined) {
        if (!incremental_enabled && (symbol->flags & 0x1000)) {
            while (symbol->auxiliary) {
                auxiliary=symbol->auxiliary;
                symbol->auxiliary=auxiliary->next;
                release_memory(auxiliary);
            }
            symbol->flags &= ~0x1000U;
        }
        symbol->flags |= 1;
        if (symbol->undefined_next)
            symbol->undefined_next->undefined_previous=symbol->undefined_previous;
        *symbol->undefined_previous=symbol->undefined_next;
        if (table->undefined_last==&symbol->undefined_next)
            table->undefined_last=symbol->undefined_previous;
        symbol->undefined_previous=NULL;
        symbol->undefined_next=NULL;
    } else {
        symbol->flags &= ~1U;
        previous=table->undefined_last;
        symbol->undefined_previous=previous;
        symbol->undefined_next=*previous;
        *previous=symbol;
        if (symbol->undefined_next)
            symbol->undefined_next->undefined_previous=&symbol->undefined_next;
        if (table->undefined_last==previous) table->undefined_last=&symbol->undefined_next;
        if (!incremental_enabled) symbol->auxiliary=NULL;
    }
}

/* Private exported MSVCRT entry used by the imported-destructor diagnostic. */
extern char *__cdecl __unDName(char *,const char *,int,
    void *(__cdecl *)(size_t),void (__cdecl *)(void *),unsigned short);
extern char short_symbol_name[9];
void __stdcall define_input_symbol(StringNode *symbol,Contribution *contribution,
    DWORD value,SHORT section,WORD type,DWORD flags,OutputModule *module,StringTable *table)
{
    WORD name_kind;
    const char *name;
    char *decoded;
    set_symbol_defined(symbol,1,table);
    if (symbol->flags & 0x40c0) {
        symbol->flags &= ~0x40c0U;
        symbol->symbol.StorageClass=IMAGE_SYM_CLASS_EXTERNAL;
        --weak_external_count;
    }
    symbol->symbol.Value=value;
    symbol->symbol.SectionNumber=section;
    symbol->contribution=contribution;
    symbol->symbol.Type=type;
    symbol->input_flags=(WORD)flags;
    if (contribution && contribution->group->section==import_section) {
        name=get_symbol_name(symbol,table->hash.string_storage,&name_kind,NULL);
        if (name_kind==1) name=strncpy(short_symbol_name,name,8);
        if (!_mbsnbcmp((const unsigned char *)name,(const unsigned char *)"__imp_??_",9) && (name[9]=='E' || name[9]=='G')) {
            decoded=__unDName(NULL,name+6,0,malloc,free,0x800);
            if (decoded) {
                link_warning(NULL,4103,decoded);
                if (decoded!=name+6) free(decoded);
            }
        }
    }
    if (incremental_enabled) {
        if (module->defined_symbols) symbol->defined_next=module->defined_symbols;
        module->defined_symbols=symbol;
    }
}

void __stdcall record_symbol_reference(StringNode *symbol,void *owner)
{
    SymbolAuxiliary *references,*additional;
    unsigned index;
    if (!incremental_enabled && (symbol->flags & 1)) return;
    if (!(symbol->flags & 0x1000)) {
        if (!symbol->reference_owner) {
            symbol->reference_owner=owner;
            return;
        }
        references=(SymbolAuxiliary *)allocate_link_memory(1,sizeof(SymbolAuxiliary));
        references->owners[0]=symbol->reference_owner;
        references->owners[1]=owner;
        references->next=NULL;
        symbol->auxiliary=references;
        symbol->flags |= 0x1000;
        return;
    }
    references=symbol->auxiliary;
    /* The first owner is occupied when a list node is installed. */
    for (index=1;index<4;++index) {
        if (!references->owners[index]) {
            references->owners[index]=owner;
            return;
        }
    }
    additional=(SymbolAuxiliary *)allocate_link_memory(1,sizeof(SymbolAuxiliary));
    additional->owners[0]=owner;
    additional->next=symbol->auxiliary;
    symbol->auxiliary=additional;
}

void __stdcall remember_common_symbol(CommonSymbol **list,StringNode *symbol)
{
    CommonSymbol *entry=(CommonSymbol *)allocate_memory(sizeof(CommonSymbol));
    entry->symbol=symbol;
    entry->next=*list;
    *list=entry;
}

WeakDefault *weak_defaults;
void __cdecl finish_symbol_tables(void)
{
    WeakDefault *entry=weak_defaults;
    while(entry) {
        WeakDefault *next=entry->next;
        release_memory(entry);
        weak_defaults=next;
        entry=next;
    }
    weak_defaults=NULL;
}

unsigned __stdcall count_hash_symbols(HashState *table)
{
    return table->entries;
}

extern void *__stdcall zero_allocate(unsigned);
void __stdcall initialize_hash_enumerator(HashState *state)
{
    HashEnumerator *previous=state->enumerator;
    state->enumerator=(HashEnumerator *)zero_allocate(sizeof(HashEnumerator));
    state->enumerator->previous=previous;
    state->enumerator->bucket=0;
    state->enumerator->index=0;
    state->enumerator->next=NULL;
}

HashEntry *__stdcall next_hash_entry(HashState *state)
{
    HashEnumerator *enumerator=state->enumerator;
    HashEntry *entry;
    DWORD bucket;
    HashEntry **entries;
    if(enumerator->index>=state->entries)return NULL;
    entry=enumerator->next;
    if(entry) {
        enumerator->next=entry->next;
        ++state->enumerator->index;
        return entry;
    }
    while(enumerator->bucket<state->bucket_count) {
        bucket=enumerator->bucket;
        entries=(HashEntry **)state->directory[bucket/state->initial_buckets][0];
        entry=entries[bucket%state->initial_buckets];
        enumerator->bucket=bucket+1;
        if(entry) {
            ++state->enumerator->index;
            state->enumerator->next=entry->next;
            return entry;
        }
        enumerator=state->enumerator;
    }
    return NULL;
}

extern void __stdcall release_memory(void *);
void __stdcall end_hash_enumerator(HashState *state)
{
    HashEnumerator *enumerator=state->enumerator;
    state->enumerator=enumerator->previous;
    release_memory(enumerator);
}

void __stdcall destroy_hash_table(HashTable *);
void __stdcall clear_buffer(ByteBuffer *);
void __stdcall destroy_string_table(StringTable **table)
{
    destroy_hash_table(&(*table)->hash);
    clear_buffer(&(*table)->hash.strings);
    free(*table);
    *table=NULL;
}

void __stdcall destroy_hash_table(HashTable *table)
{
    HashState *state=table->state;
    unsigned bucket,block,freed_block=0;
    HashEntry *entry;
    if(!state->incremental_mode) {
        release_memory_arena(state->directory_arena);
        release_memory_arena(state->bucket_arena);
    } else {
        for(bucket=0;bucket<state->bucket_count;++bucket) {
            block=bucket/state->initial_buckets;
            entry=((HashEntry **)state->directory[block][0])[bucket%state->initial_buckets];
            while(entry)entry=entry->next;
            if(block>freed_block) {
                free(state->directory[freed_block][0]);
                free(state->directory[freed_block]);
                ++freed_block;
            }
        }
        free(state->directory[freed_block][0]);
        free(state->directory[freed_block]);
        free(state->directory);
    }
    free(state);
    table->state=NULL;
}
