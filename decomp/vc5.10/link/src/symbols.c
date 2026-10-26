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
void __stdcall grow_hash(HashState *state,const void *strings) { __asm { int 3 } }

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
extern DWORD undefined_symbols;
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
        --undefined_symbols;
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

/* Original47f02c is the head of12-byte weak-external/default records.
 * Its insertion and lookup bodies remain pending; cleanup owns this head. */
typedef struct WeakDefault {
    StringNode *symbol,*default_symbol;
    struct WeakDefault *next;
} WeakDefault;
typedef char WeakDefaultSize[(sizeof(WeakDefault)==12)?1:-1];
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
