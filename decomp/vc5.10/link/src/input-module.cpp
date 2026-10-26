/* The library search is an original thiscall boundary. C++ expresses that
   member call without adding an adapter or changing the POD record layout. */
#include <windows.h>
#include <string.h>
#include <mbstring.h>
#include "input.h"
#include "memory.h"
#include "files.h"
#include "full-link-functions.h"
#include "pass-one.h"
#include "options.h"
#include "comdat.h"
#include <recon.h>

extern "C" {
void __stdcall initialize_module_enumerator(ModuleEnumerator *,OutputLibrary *);
BYTE __stdcall next_module(ModuleEnumerator *);
extern BYTE incremental_enabled, incremental_reuse, mac_target;
extern MemoryArena *output_module_arena;
MemoryArena *contribution_arena;
/* Separate backend flag, not the Mac flag shared with /machine:MPPC. */
BYTE native_ppc_symbol_maps;
WORD incremental_module_index;
DWORD incremental_module_capacity;
BYTE *incremental_module_bits, *incremental_module_changed_bits;
void *__stdcall reallocate_memory(void *,unsigned);
DWORD __cdecl allocate_module_index(void);
}
#pragma intrinsic(memcpy,memset)
OutputModule *__stdcall create_input_module(const char *original_name,
    const char *name,const void *identity,IMAGE_FILE_HEADER *header,
    OutputLibrary *library,BYTE *created,BYTE copy_sections,DWORD section_offset,
    IMAGE_SECTION_HEADER *sections,BYTE *section_flags,void *optional_data,
    DWORD auxiliary)
{
    DWORD section_count=header->NumberOfSections;
    DWORD symbol_count=header->NumberOfSymbols;
    DWORD previous_bytes,bytes,aligned_count,bit_words;
    OutputModule *module=library->FindModule(name,identity);
    if (created) *created=(module==NULL);
    if (module) return module;
    if (incremental_enabled) {
        module=(OutputModule *)allocate_link_memory(1,sizeof(OutputModule));
        if (section_count)
            module->contributions=allocate_link_memory(1,section_count*64);
        else module->contributions=NULL;
    } else {
        module=(OutputModule *)reserve_memory(&output_module_arena,sizeof(OutputModule));
        if (section_count)
            module->contributions=reserve_memory(&contribution_arena,section_count*64);
        else module->contributions=NULL;
    }
    module->section_map=NULL;
    module->identity=original_name ? duplicate_link_string(original_name) : identity;
    module->timestamp=header->TimeDateStamp;
    if (name) module->name=duplicate_link_string(name);
    if (incremental_enabled) {
        if (!incremental_reuse) {
            if (!incremental_module_capacity) {
                incremental_module_index=0;
                incremental_module_capacity=4096;
                incremental_module_bits=(BYTE *)zero_allocate(512);
                incremental_module_changed_bits=(BYTE *)zero_allocate(
                    (incremental_module_capacity+7)/8);
            }
            ++incremental_module_index;
            if (incremental_module_index>=incremental_module_capacity) {
                previous_bytes=(incremental_module_capacity+7)/8;
                incremental_module_capacity+=4096;
                bytes=(incremental_module_capacity+7)/8;
                if (previous_bytes!=bytes) {
                    incremental_module_bits=(BYTE *)reallocate_memory(
                        incremental_module_bits,bytes);
                    incremental_module_changed_bits=(BYTE *)reallocate_memory(
                        incremental_module_changed_bits,bytes);
                    memset(incremental_module_bits+previous_bytes,0,bytes-previous_bytes);
                    memset(incremental_module_changed_bits+previous_bytes,0,bytes-previous_bytes);
                }
            }
            *(WORD *)&module->module_index=incremental_module_index;
        } else *(WORD *)&module->module_index=0;
    } else module->module_index=allocate_module_index();
    module->symbol_offset=header->PointerToSymbolTable;
    if (copy_sections) module->flags |= 0x40;
    module->section_offset=section_offset;
    if (copy_sections) {
        aligned_count=(section_count+3)&~3U;
        if (incremental_enabled) {
            module->section_headers=(IMAGE_SECTION_HEADER *)allocate_link_memory(1,section_count*40);
            module->section_flags=(BYTE *)allocate_link_memory(1,aligned_count);
            /* This extra allocation is present in the original incremental
               path even though its result is not retained by this function. */
            allocate_link_memory(1,aligned_count);
            module->optional_data=allocate_link_memory(1,60);
        } else {
            module->section_headers=(IMAGE_SECTION_HEADER *)reserve_memory(&output_module_arena,section_count*40);
            module->section_flags=(BYTE *)reserve_memory(&output_module_arena,aligned_count);
            module->optional_data=reserve_memory(&output_module_arena,60);
        }
        memcpy(module->section_headers,sections,section_count*40);
        memcpy(module->section_flags,section_flags,section_count);
        memcpy(module->optional_data,optional_data,60);
    } else {
        module->section_headers=NULL;
        module->section_flags=NULL;
        module->optional_data=NULL;
    }
    module->auxiliary=auxiliary;
    module->symbol_count=symbol_count;
    module->optional_header_size=header->SizeOfOptionalHeader;
    module->characteristics=header->Characteristics;
    module->section_count=section_count;
    module->library=library;
    module->next=library->module_head;
    if (incremental_enabled) {
        module->incremental_state=(WORD *)allocate_link_memory(1,12);
        module->incremental_state[0]=8;
    }
    if (mac_target || native_ppc_symbol_maps) {
        bit_words=(symbol_count>>5)+1;
        if (incremental_enabled) module->symbol_bits[0]=(DWORD *)allocate_link_memory(bit_words,4);
        else module->symbol_bits[0]=(DWORD *)zero_allocate(bit_words*4);
        if (incremental_enabled) module->symbol_bits[1]=(DWORD *)allocate_link_memory(bit_words,4);
        else module->symbol_bits[1]=(DWORD *)zero_allocate(bit_words*4);
        if (incremental_enabled) module->symbol_indices=(DWORD *)allocate_link_memory(symbol_count,4);
        else module->symbol_indices=(DWORD *)zero_allocate(symbol_count*4);
    }
    library->module_head=module;
    return module;
}
OutputModule *RECON_THISCALL OutputLibrary::FindModule(const char *name,const void *identity)
{
    ModuleEnumerator enumerator;
    OutputModule *module=NULL;
    initialize_module_enumerator(&enumerator,this);
    while (next_module(&enumerator)) {
        if (identity ? identity==enumerator.current->identity :
            !_mbsicmp((const unsigned char *)name,
                (const unsigned char *)enumerator.current->name)) {
            module=enumerator.current;
            end_section_enumerator((SectionEnumerator *)&enumerator);
            break;
        }
    }
    return module;
}
void __stdcall initialize_module_enumerator(ModuleEnumerator *enumerator,OutputLibrary *library)
{
    enumerator->next=next_module;
    enumerator->end=(void (__stdcall *)(ModuleEnumerator *))end_section_enumerator;
    enumerator->current=NULL;
    enumerator->library=library;
}
BYTE __stdcall next_module(ModuleEnumerator *enumerator)
{
    if (enumerator->library) {
        if (!enumerator->current)
            enumerator->current=enumerator->library->module_head;
        else enumerator->current=enumerator->current->next;
    }
    return enumerator->current!=NULL;
}

extern "C" void __stdcall check_input_modules(LinkContext *context,OutputLibrary *library)
{
    ModuleEnumerator enumerator;
    initialize_module_enumerator(&enumerator,library);
    if (!next_module(&enumerator)) {
        link_warning(NULL,4001);
        initialize_machine(context);
    }
    end_section_enumerator((SectionEnumerator *)&enumerator);
}

extern "C" OutputModule *output_module;
extern "C" char short_symbol_name[9];
extern "C" void __stdcall place_common_symbol(LinkContext *context,StringNode *symbol)
{
    Contribution *contribution;
    const char *name;
    DWORD flags,size;
    WORD name_kind;
    if(!(symbol->flags & 2))return;
    contribution=symbol->contribution;
    while(contribution && (contribution->characteristics & 0x4000))
        contribution=contribution->associated_contribution;
    if(contribution)return;
    size=symbol->symbol.Value;
    if(size<=context->options.gp_size) {
        name=".sdata";
        flags=0xc0000040;
    } else {
        name=".bss";
        flags=0xc0000080;
    }
    if(size<=1)flags|=0x100000;
    else if(size<=2)flags|=0x200000;
    else if(size<=4)flags|=0x300000;
    else if(size<=8)flags|=0x400000;
    else if(size<=16)flags|=0x500000;
    else flags|=0x600000;
    symbol->contribution=create_contribution(name,size,flags,flags,output_module,
        &context->sections,context);
    if(context->options.effective_reference_opt || context->options.fold_identical_comdats) {
        name=get_symbol_name(symbol,context->symbols->hash.string_storage,&name_kind,NULL);
        if(name_kind==1)name=strncpy(short_symbol_name,name,8);
        attach_comdat(symbol->contribution,name,0);
    }
    symbol->symbol.Value=0;
}
extern "C" void __stdcall pad_common_symbol(StringNode *,DWORD)
{ __asm { int 3 } }
extern "C" void __stdcall place_module_common_symbols(LinkContext *context,OutputModule *module)
{
    CommonSymbol *item=module->common_symbols;
    Contribution *contribution;
    DWORD total=0;
    while(item) {
        place_common_symbol(context,item->symbol);
        /* Placement can replace the symbol's contribution. Reload it. */
        contribution=item->symbol->contribution;
        while(contribution && (contribution->characteristics & 0x4000))
            contribution=contribution->associated_contribution;
        total+=contribution->raw_size;
        item=item->next;
    }
    if(incremental_enabled && total)
        pad_common_symbol(module->common_symbols->symbol,total);
}

void RECON_THISCALL LinkContext::ApplySectionAttributes(void)
{
    LibraryEnumerator libraries;
    ModuleEnumerator modules;
    initialize_library_enumerator(&libraries,(OutputLibrary *)pointer_list.first);
    while(next_library(&libraries)) {
        initialize_module_enumerator(&modules,libraries.current);
        while(next_module(&modules))place_module_common_symbols(this,modules.current);
    }
    place_module_common_symbols(this,output_module);
}

/* Counts the compiler identities contributing to the encoded DOS extension. */
struct CompilerUse {
    CompilerUse *next;
    DWORD identity,count;
};
typedef char CompilerUseSize[(sizeof(CompilerUse)==12)?1:-1];
DWORD RECON_THISCALL LinkContext::BuildDosExtension(void **buffer)
{
    LibraryEnumerator libraries;
    ModuleEnumerator modules;
    CompilerUse *uses=NULL,*use,*next;
    DWORD distinct=0,checksum,bytes,*output;
    if(options.rom) {
        *buffer=NULL;
        return 0;
    }
    initialize_library_enumerator(&libraries,(OutputLibrary *)pointer_list.first);
    while(next_library(&libraries)) {
        initialize_module_enumerator(&modules,libraries.current);
        while(next_module(&modules)) {
            for(use=uses;use && use->identity!=modules.current->compiler_id;use=use->next) ;
            if(use)++use->count;
            else {
                use=(CompilerUse *)allocate_memory(sizeof(CompilerUse));
                use->next=uses;
                uses=use;
                ++distinct;
                use->count=1;
                use->identity=modules.current->compiler_id;
            }
        }
        end_section_enumerator((SectionEnumerator *)&modules);
    }
    end_section_enumerator((SectionEnumerator *)&libraries);
    checksum=dos_image_size;
    for(use=uses;use;use=use->next)
        checksum+=(use->identity << (use->count & 31)) |
            (use->identity >> ((0-use->count) & 31));
    bytes=32+(distinct+((checksum>>5)%3))*8;
    output=(DWORD *)zero_allocate(bytes);
    *buffer=output;
    *output++=checksum ^ 0x536e6144; /* DanS */
    *output++=checksum;
    *output++=checksum;
    *output++=checksum;
    for(use=uses;use;use=next) {
        *output++=use->identity ^ checksum;
        *output++=use->count ^ checksum;
        next=use->next;
        release_memory(use);
    }
    *output++=0x68636952; /* Rich */
    *output=checksum;
    return bytes;
}

extern "C" {
void __stdcall initialize_comdat_root_enumerator(ComdatRootEnumerator *,ComdatRoot *);
BYTE __stdcall next_comdat_root(ComdatRootEnumerator *);
void __stdcall mark_live_comdat(StringTable *,Contribution *);
}
extern "C" void __stdcall eliminate_unused_comdats(ComdatRoot *roots,StringTable *symbols)
{
    ComdatRootEnumerator enumerator;
    /* Unknown tags retain this defined incoming pointer, then the prior result,
     * exactly as the original register flow. The producer writes only tags1..3. */
    Contribution *contribution=(Contribution *)roots;
    initialize_comdat_root_enumerator(&enumerator,roots);
    while(next_comdat_root(&enumerator)) {
        ComdatRoot *root=enumerator.current;
        switch(root->kind) {
        case 3:
            contribution=symbols->FindSymbol(root->name)->contribution;
            break;
        case 2:
            contribution=(root->symbol->flags & 1) ? root->symbol->contribution : NULL;
            break;
        case 1:
            contribution=root->owner;
            break;
        }
        if(contribution)mark_live_comdat(symbols,contribution);
    }
}
