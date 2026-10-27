/* COFF weak-external defaults use the actual auxiliary index/search pair. */
#include <windows.h>
#include <string.h>
#include "weak-symbols.h"
#include "input.h"
#include "files.h"
#include "memory.h"
#include "full-link-functions.h"
#include <recon.h>

extern "C" {
extern char *current_module_strings,short_symbol_name[9];
extern BYTE incremental_reuse,file_access_statistics;
extern BYTE incremental_enabled,mac_target;
IMAGE_SYMBOL *__stdcall next_coff_symbol(IMAGE_SYMBOL **);
void __stdcall read_file_symbol(int,IMAGE_SYMBOL *);
char *__stdcall display_symbol_name(const char *,DWORD);
char *__stdcall format_module_name(OutputModule *,char *);
void __cdecl link_fatal(const char *,unsigned,...);
void __cdecl record_file_access(void);

void __stdcall remember_weak_default(StringNode *symbol,StringNode *default_symbol)
{
    WeakDefault *entry=weak_defaults;
    while(entry) {
        if(entry->symbol==symbol) {
            entry->default_symbol=default_symbol;
            return;
        }
        entry=entry->next;
    }
    entry=(WeakDefault *)allocate_memory(12);
    entry->symbol=symbol;
    entry->default_symbol=default_symbol;
    entry->next=weak_defaults;
    weak_defaults=entry;
}

/* Consumed eight-byte prefix of the real eighteen-byte auxiliary record. */
typedef struct WeakAuxiliary { DWORD default_index,search; } WeakAuxiliary;

void __stdcall collect_weak_external(StringTable *table,IMAGE_SYMBOL **cursor,
    IMAGE_SYMBOL *raw,StringNode *symbol,OutputModule *module,BYTE *changed,
    BYTE created,DWORD flags)
{
    WeakAuxiliary *auxiliary;
    IMAGE_SYMBOL fallback;
    StringNode *default_symbol;
    const char *name;
    char *display,module_name[520];
    FileRecord *record;
    DWORD saved_position,base,error;
    WORD kind;
    if(raw->NumberOfAuxSymbols!=1) {
        name=get_symbol_name(symbol,table->hash.string_storage,&kind,NULL);
        if(kind==1)name=strncpy(short_symbol_name,name,8);
        display=display_symbol_name(name,1);
        format_module_name(module,module_name);
        link_fatal(module_name,1129,display);
    }
    auxiliary=(WeakAuxiliary *)next_coff_symbol(cursor);
    if(file_access_statistics)record_file_access();
    record=file_slots[current_input_file];
    saved_position=(record->flags&8) ? record->file_size :
        ((FileBufferHeader *)record->base)->cursor;
    if(!(module->library->flags&16)) {
        /* Preserve the physical saved file-base DWORD at+12 for copied
         * module input. No other optional-data fields are inferred here. */
        base=(module->flags&0x40) ? ((DWORD *)module->optional_data)[3] :
            (DWORD)module->identity;
    } else base=0;
    error=0;
    seek_file(current_input_file,base+module->symbol_offset+auxiliary->default_index*18,
        FILE_BEGIN,&error);
    read_file_symbol(current_input_file,&fallback);
    if(created || (!(symbol->flags&1) && auxiliary->search==3)) {
        ++weak_external_count;
        symbol->symbol.StorageClass=IMAGE_SYM_CLASS_WEAK_EXTERNAL;
        name=get_symbol_name((StringNode *)&fallback,current_module_strings,&kind,NULL);
        default_symbol=table->LookupSymbol(kind,name,changed);
        remember_weak_default(symbol,default_symbol);
        symbol->input_flags=(WORD)flags;
        switch(auxiliary->search) {
        case 1:symbol->flags|=0x40;break;
        case 2:symbol->flags|=0x80;break;
        case 3:symbol->flags|=0x4000;break;
        }
        if(incremental_reuse && created && (raw->Type&0x30)==0x20)
            symbol->flags|=0x20000;
    } else if(incremental_reuse) {
        name=get_symbol_name((StringNode *)&fallback,current_module_strings,&kind,NULL);
        default_symbol=table->LookupSymbol(kind,name,changed);
        remember_weak_default(symbol,default_symbol);
        switch(auxiliary->search) {
        case 1:symbol->flags|=0x40;break;
        case 2:symbol->flags|=0x80;break;
        case 3:symbol->flags|=0x4000;break;
        }
    }
    error=0;
    seek_file(current_input_file,saved_position,FILE_BEGIN,&error);
}

void __stdcall resolve_weak_externals(LinkContext *context,DWORD flags)
{
    WeakDefault *entry=weak_defaults;
    StringNode *symbol,*default_symbol;
    OutputModule *module;
    while(entry) {
        symbol=entry->symbol;
        default_symbol=entry->default_symbol;
        if((symbol->flags&flags) && (default_symbol->flags&1)) {
            symbol->symbol.Value=default_symbol->symbol.Value+
                default_symbol->contribution->group->section->rva;
            symbol->symbol.SectionNumber=default_symbol->symbol.SectionNumber;
            symbol->symbol.Type=default_symbol->symbol.Type;
            set_symbol_defined(symbol,1,context->symbols);
            symbol->contribution=default_symbol->contribution;
            symbol->final_value=default_symbol->final_value;
            if(mac_target) {
                symbol->attributes|=4;
                if(default_symbol->attributes&0x10) {
                    symbol->attributes|=0x10;
                    symbol->reserved36=default_symbol->reserved36;
                }
                if(default_symbol->attributes&2)symbol->attributes|=2;
                if(default_symbol->attributes&0x20)symbol->attributes|=0x20;
                if(default_symbol->attributes&0x80)symbol->attributes|=0x80;
            }
            if(incremental_enabled) {
                module=symbol->contribution->module;
                if(module->defined_symbols)symbol->defined_next=module->defined_symbols;
                module->defined_symbols=symbol;
            }
        }
        entry=entry->next;
    }
}
}
