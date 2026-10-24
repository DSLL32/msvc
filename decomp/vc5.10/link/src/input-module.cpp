/* The library search is an original thiscall boundary. C++ expresses that
   member call without adding an adapter or changing the POD record layout. */
#include <windows.h>
#include <string.h>
#include <mbstring.h>
#include "input.h"
#include "memory.h"
#include "files.h"
#include "full-link-functions.h"
#include <recon.h>

typedef struct ModuleEnumerator {
    BYTE (__stdcall *next)(struct ModuleEnumerator *);
    void (__stdcall *end)(struct ModuleEnumerator *);
    OutputLibrary *library;
    OutputModule *current;
} ModuleEnumerator;
typedef char ModuleEnumeratorSize[(sizeof(ModuleEnumerator)==16)?1:-1];
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
    module->reserved2c=NULL;
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
    __asm { int 3 }
    return 0;
}
void *__stdcall reallocate_memory(void *memory,unsigned bytes)
{
    __asm { int 3 }
    return NULL;
}
DWORD __cdecl allocate_module_index(void)
{
    __asm { int 3 }
    return 0;
}
