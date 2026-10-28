#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "link-driver.h"
#include "full-link-functions.h"
#include "pass-one.h"
#include "input.h"
#include "memory.h"
#include "sections.h"
#include "comdat.h"
#include "debug-output.h"
#include "librarian.h"
#include <recon.h>

extern "C" {
extern BYTE verbose_link,pdb_enabled,incremental_enabled,incremental_reuse;
extern BYTE mac_target,separate_types;
extern const char *output_file_name,*pdb_file_name;
extern Contribution *incremental_code;
extern StringNode *mac_toc_symbol,*mac_function_symbol;
extern OutputModule *output_module;
extern ModuleListNode *precompiled_modules;
extern Contribution **output_contribution_map;
DWORD pdb_module_counter;
DWORD relocation_errors;
void __stdcall prepare_incremental_pass(LinkContext *,Contribution *,DWORD *,DWORD *,DWORD *,DWORD *);
void __stdcall resume_incremental_pass(LinkContext *,DWORD *,DWORD *,DWORD *,DWORD *);
DWORD __cdecl pdb_get_signature(void);
DWORD __cdecl pdb_get_age(void);
BYTE __cdecl pdb_get_separate_types(void);
void __stdcall pdb_set_separate_types(BYTE);
void __stdcall pdb_select_image(const char *);
void __stdcall pdb_define_section(const char *,DWORD,DWORD);
void __stdcall prepare_mac_pass_two(LinkContext *);
BYTE __stdcall pdb_select_library(OutputLibrary *);
BYTE __stdcall is_precompiled_module(OutputModule *);
DWORD __cdecl incremental_thunk_count(void);
void __stdcall pdb_write_thunk_map(DWORD,DWORD,DWORD,WORD,DWORD,DWORD,DWORD);
void __stdcall finish_pdb_symbols(LinkContext *);
void __stdcall pdb_open_module(const void *,const void *,DWORD);
void __stdcall pdb_close_module(OutputModule *,const void *,DWORD);
void __stdcall pdb_add_contribution(WORD,DWORD,DWORD,DWORD,DWORD,DWORD);
void __cdecl pdb_finish_contributions(void);
}

void RECON_THISCALL LinkContext::PassTwo(void)
{
    DWORD thunk_storage=0,thunk_metadata,thunk_first=0,thunk_count=0;
    /* The original allocates this DWORD slot without initialization, and
     * writes only its low BYTE in PDB branches. Zero unwritten native bytes.
     * PDB-disabled consumers do not use it; this does not qualify undefined
     * upper bytes for a future PDB-enabled dependency. */
    DWORD library_state;
#ifdef __linux__
    library_state=0;
#endif
    LibraryEnumerator libraries;
    ModuleEnumerator modules;
    ModuleListNode *node;
    OutputLibrary *library;
    OutputModule *module;
    if(verbose_link)link_message(9013);
    if(incremental_enabled) {
        if(incremental_reuse)
            resume_incremental_pass(this,&thunk_storage,&thunk_metadata,&thunk_first,&thunk_count);
        else prepare_incremental_pass(this,incremental_code,
            &thunk_storage,&thunk_metadata,&thunk_first,&thunk_count);
    }
    if(pdb_enabled) {
        pdb_current_signature=pdb_get_signature();
        pdb_current_age=pdb_get_age();
        if(incremental_reuse) {
            if(pdb_current_signature!=pdb_signature || pdb_current_age<pdb_age)
                link_fatal(NULL,1209,pdb_file_name);
            pdb_age=pdb_current_age;
            pdb_select_image(output_file_name);
            if(incremental_status_code)return;
            if((!mac_target || options.mppc32_types) &&
               pdb_get_separate_types()!=separate_types)
                link_fatal(NULL,1209,pdb_file_name);
        } else {
            pdb_module_counter=0;
            pdb_signature=pdb_current_signature;
            pdb_age=pdb_current_age;
            if(!mac_target || options.mppc32_types)pdb_set_separate_types(separate_types);
            if(mac_target) {
                OutputSection *section=mac_toc_symbol->contribution->group->section;
                /* Full DWORD carrier at44de14: contribution high bits, index low16.
                 * The unqualified PDB receiver has no high-bit semantic claim. */
                pdb_define_section("__TocTb",
                    ((DWORD)mac_toc_symbol->contribution&0xffff0000)|section->index,
                    mac_toc_symbol->final_value-section->rva);
                section=mac_function_symbol->contribution->group->section;
                /* The second physical carrier instead comes from the group pointer. */
                pdb_define_section("__FTInfo",
                    ((DWORD)mac_function_symbol->contribution->group&0xffff0000)|section->index,
                    mac_function_symbol->final_value-section->rva);
            }
        }
    }
    if(mac_target && !incremental_enabled)prepare_mac_pass_two(this);
    for(node=precompiled_modules;node;node=node->next) {
        module=node->module;
        if(pdb_enabled)*(BYTE *)&library_state=pdb_select_library(module->library);
#ifdef __linux__
        if(pdb_enabled)recon_unsupported("PassTwo unqualified PDB library-state DWORD");
#endif
        module->WriteOutput(this,library_state);
        if(incremental_reuse && incremental_status_code)goto failed;
    }
    initialize_library_enumerator(&libraries,(OutputLibrary *)pointer_list.first);
    while(next_library(&libraries)) {
        library=libraries.current;
        if(pdb_enabled)*(BYTE *)&library_state=pdb_select_library(library);
        initialize_module_enumerator(&modules,library);
        while(next_module(&modules)) {
            module=modules.current;
            if(incremental_reuse || !is_precompiled_module(module)) {
#ifdef __linux__
                if(pdb_enabled)recon_unsupported("PassTwo unqualified PDB library-state DWORD");
#endif
                module->WriteOutput(this,library_state);
                if(incremental_reuse && incremental_status_code)goto failed;
            }
        }
    }
    if(!incremental_reuse)output_module->WriteCommonSymbols(options.effective_reference_opt);
    current_phase[0]=0;
    if(relocation_errors && !(options.force & 1))link_fatal(NULL,1165);
    if(pdb_enabled) {
        if(incremental_enabled && incremental_code) {
            OutputSection *section=incremental_code->group->section;
            DWORD offset=incremental_thunk_count();
            offset+=incremental_code->rva-section->rva;
            DWORD count=incremental_thunk_count();
            pdb_write_thunk_map(thunk_storage,thunk_metadata,count,section->index,
                offset,thunk_first,thunk_count);
            release_memory((void *)thunk_storage);
        }
        finish_pdb_symbols(this);
        if(!incremental_reuse) {
            DWORD index;
            pdb_open_module("* Linker *","",0);
            for(index=0;index<output_module->contribution_count;++index) {
                Contribution *contribution=output_contribution_map[index];
                OutputSection *section;
                if(!contribution)break;
                section=contribution->group->section;
                if(section!=debug_section)
                    pdb_add_contribution(section->index,contribution->rva-section->rva,
                        contribution->raw_size-contribution->padding,
                        contribution->characteristics,*(DWORD *)contribution->reserved30,
                        *(DWORD *)(contribution->reserved30+4));
            }
            pdb_close_module(output_module,"* Linker *",0);
        }
        pdb_finish_contributions();
    }
    if(verbose_link)link_message(9014);
    return;
failed:
    pdb_finish_contributions();
}

/* Real downstream boundaries retain their own identities and physical ABIs. */
extern "C" {
void __stdcall prepare_incremental_pass(LinkContext *,Contribution *,DWORD *,DWORD *,DWORD *,DWORD *) { __asm { int 3 } }
void __stdcall resume_incremental_pass(LinkContext *,DWORD *,DWORD *,DWORD *,DWORD *) { __asm { int 3 } }
DWORD __cdecl pdb_get_signature(void) { __asm { int 3 } return 0; }
DWORD __cdecl pdb_get_age(void) { __asm { int 3 } return 0; }
BYTE __cdecl pdb_get_separate_types(void) { __asm { int 3 } return 0; }
void __stdcall pdb_set_separate_types(BYTE) { __asm { int 3 } }
void __stdcall pdb_select_image(const char *) { __asm { int 3 } }
void __stdcall pdb_define_section(const char *,DWORD,DWORD) { __asm { int 3 } }

BYTE __stdcall pdb_select_library(OutputLibrary *) { __asm { int 3 } return 0; }
BYTE __stdcall is_precompiled_module(OutputModule *module)
{
    ModuleListNode *node;
    for(node=precompiled_modules;node;node=node->next)
        if(node->module==module)return 1;
    return 0;
}
DWORD __cdecl incremental_thunk_count(void) { __asm { int 3 } return 0; }
void __stdcall pdb_write_thunk_map(DWORD,DWORD,DWORD,WORD,DWORD,DWORD,DWORD) { __asm { int 3 } }
void __stdcall finish_pdb_symbols(LinkContext *) { __asm { int 3 } }
void __stdcall pdb_open_module(const void *,const void *,DWORD) { __asm { int 3 } }
void __stdcall pdb_close_module(OutputModule *,const void *,DWORD) { __asm { int 3 } }
void __stdcall pdb_add_contribution(WORD,DWORD,DWORD,DWORD,DWORD,DWORD) { __asm { int 3 } }
void __cdecl pdb_finish_contributions(void) { __asm { int 3 } }
}
/* This is an encountered prefix of the converted object's metadata, not a
 * claim about its full allocation or the unrelated file-cache record. */
typedef struct ConvertedObjectName {
    DWORD reserved[3];
    const char *name;
} ConvertedObjectName;
extern "C" const char *current_object_name;
const char *current_object_name;
void RECON_THISCALL OutputModule::WriteOutput(LinkContext *context,DWORD pdb_library_state)
{
    int saved_file=0;
    BYTE emit_symbols;
    /* Entry PUSH ECX supplies the upper bytes of this stack local; they are
     * defined receiver bits, not unwritten padding. Preserve that own role. */
    DWORD output_state=(DWORD)this;
    if(incremental_reuse && !(flags & 1))return;
    if(flags & 0x80) {
        const char *name=!(library->flags & 16) ? library->name : (const char *)identity;
        current_input_file=open_argument_file(name,0x8000,0);
        saved_file=current_input_file;
        ReopenOutput(saved_file);
    } else current_input_file=open_module_file(this,0x8000,0);
    if(!(library->flags & 16))
        current_object_name=(flags & 0x40) ? ((ConvertedObjectName *)optional_data)->name :
            (const char *)identity;
    else current_object_name=NULL;
    format_module_name(this,current_phase);
    emit_symbols=incremental_reuse ? (BYTE)((flags>>1)&1) : 1;
    *(BYTE *)&output_state=emit_symbols;
    if(pdb_enabled && emit_symbols) {
#ifdef __linux__
        recon_unsupported("WriteOutput unqualified PDB library-state DWORD");
#endif
        const void *name=!(library->flags & 16) ? (const void *)library->name : identity;
        pdb_open_module(this->name,name,pdb_library_state);
    }
    ProcessOutput(context,output_state);
    if(flags & 0x80) {
        current_input_file=saved_file;
        close_argument_file(saved_file,(library->flags>>4)&1);
    }
    if(!(flags & 0x40))close_argument_file(current_input_file,(library->flags>>4)&1);
    if(incremental_reuse && incremental_status_code)return;
    if(pdb_enabled && emit_symbols) {
#ifdef __linux__
        recon_unsupported("WriteOutput unqualified PDB library-state DWORD");
#endif
        const void *name=!(library->flags & 16) ? (const void *)this->name : identity;
        pdb_close_module(this,name,pdb_library_state);
    }
    /* The original ANDs WORD98h withfff0h; its upper byte stays unchanged. */
    if(incremental_reuse)flags &= 0xf0;
}
typedef char ModuleOutputFields[(offsetof(OutputModule,debug_symbol_index)==0x20 &&
    offsetof(OutputModule,input_symbols)==0x24 &&
    offsetof(OutputModule,debug_stream)==0x70 &&
    offsetof(OutputModule,serialized_debug)==0x74 &&
    offsetof(OutputModule,serialized_debug_size)==0x78)?1:-1];
extern "C" {
extern char *current_module_strings;
extern void *module_debug_table;
extern DWORD emitted_symbol_count,coff_symbol_offset;
extern int output_file;
extern BYTE file_access_statistics;
DWORD module_debug_index;
BYTE skip_module_debug_symbols;
char *__stdcall read_module_string_table(const char *,DWORD,DWORD *);
IMAGE_SYMBOL *__stdcall read_module_symbols(DWORD,DWORD,BYTE);
void __stdcall release_module_strings(char *);
void __stdcall release_module_symbols(IMAGE_SYMBOL *);
BYTE __stdcall is_discardable_comdat(Contribution *);
void __stdcall process_output_symbol(LinkContext *,OutputModule *,IMAGE_SYMBOL *,
    IMAGE_SYMBOL **,DWORD *,DWORD *,DWORD);
void __stdcall write_output_line_numbers(LinkContext *,Contribution *);
DWORD __stdcall debug_stream_size(DebugLineModule *);
void __stdcall copy_debug_stream(DebugLineModule *,void *,DWORD);
void __stdcall release_debug_stream(DebugLineModule *);
}

void RECON_THISCALL OutputModule::ProcessOutput(LinkContext *context,DWORD output_state)
{
    DWORD *symbol_indices=NULL;
    DWORD offset,string_bytes,error,index;
    IMAGE_SYMBOL *symbols,*cursor,*symbol;
    ModuleContributionEnumerator contributions;
    Contribution *contribution;
    ContributionGroup *group;
    if(verbose_link)printf("     %s\n",current_phase);
    if(!section_map)ReadSectionHeaders(NULL);
    if(library->flags & 16)offset=0;
    else offset=(flags & 0x40) ?
        *(DWORD *)((BYTE *)optional_data+12) : (DWORD)identity;
    current_module_strings=read_module_string_table(
        (library->flags & 16) ? (const char *)identity : library->name,
        offset+symbol_count*18+symbol_offset,&string_bytes);
    if(symbol_count) {
        if(library->flags & 16)offset=0;
        else offset=(flags & 0x40) ?
            *(DWORD *)((BYTE *)optional_data+12) : (DWORD)identity;
        symbols=read_module_symbols(offset+symbol_offset,symbol_count,1);
    } else symbols=NULL;
    input_symbols=symbols;
    error=0;
    seek_file(output_file,coff_symbol_offset+emitted_symbol_count*18,FILE_BEGIN,&error);
    if(library->flags & 16)offset=0;
    else offset=(flags & 0x40) ?
        *(DWORD *)((BYTE *)optional_data+12) : (DWORD)identity;
    error=0;
    seek_file(current_input_file,offset+symbol_offset,FILE_BEGIN,&error);
    if(incremental_enabled)symbol_indices=(DWORD *)zero_allocate(symbol_count*4);
    if(module_debug_table) {
        ((DebugModuleRecord *)module_debug_table)[module_debug_index].module=this;
        debug_symbol_index=0xffffffff;
    }
    skip_module_debug_symbols=0;
    if((BYTE)output_state) {
        if(module_debug_table)
            ((DebugModuleRecord *)module_debug_table)[module_debug_index].name=name;
        initialize_module_contribution_enumerator(&contributions,this);
        while(next_module_contribution(&contributions)) {
            contribution=contributions.current;
            if(contribution && (contribution->characteristics & 0x4000))continue;
            if(contribution->group==debug_type_group ||
               contribution->group==debug_precompiled_group) {
                contribution->ProcessOutput(context,symbols,symbol_indices,contributions.index);
                if(incremental_status_code)return;
            }
        }
    }
    index=0;
    cursor=symbols;
    if(symbol_count)do {
        symbol=cursor++;
        process_output_symbol(context,this,symbol,&cursor,&index,
            incremental_enabled ? symbol_indices+index : NULL,output_state);
        ++index;
    } while(index!=symbol_count);
    initialize_module_contribution_enumerator(&contributions,this);
    while(next_module_contribution(&contributions)) {
        contribution=contributions.current;
        if(contribution && (contribution->characteristics & 0x4000))continue;
        if(contribution->characteristics & 0x800)continue;
        if(context->options.effective_reference_opt && is_discardable_comdat(contribution))continue;
        if(!(BYTE)output_state && contribution->group->section==debug_section)continue;
        if(contribution->group==debug_type_group ||
           contribution->group==debug_precompiled_group)continue;
        group=contribution->group;
        if(group->first==contribution && group->reserved0c) {
            BYTE zero=0;
            DWORD remaining=group->reserved0c;
            error=0;
            seek_file(output_file,group->file_offset,FILE_BEGIN,&error);
            while(remaining--) {
                /* Each write reloads the original output descriptor. */
                int file_number=output_file;
                if(file_access_statistics)record_file_access();
                FileRecord *file=file_slots[file_number];
                if(file->flags & 8)mapped_write(file,&zero,1);
                else buffered_write(file,&zero,1);
            }
        }
        if(contribution->group!=debug_symbol_group || !skip_module_debug_symbols) {
            contribution->ProcessOutput(context,symbols,symbol_indices,contributions.index);
            if((BYTE)output_state && contribution->raw_size)
                write_output_line_numbers(context,contribution);
        }
    }
    if((BYTE)output_state)WriteCommonSymbols(context->options.effective_reference_opt);
    release_module_strings(current_module_strings);
    current_module_strings=NULL;
    if(context->options.debug_formats & 2) {
        if(!pdb_enabled) {
            DWORD bytes=debug_stream_size(debug_stream);
            if(bytes) {
                serialized_debug=(BYTE *)allocate_memory(bytes);
                copy_debug_stream(debug_stream,serialized_debug,bytes);
            }
            serialized_debug_size=bytes;
            release_debug_stream(debug_stream);
        } else if((BYTE)output_state)WriteDebug(context->options.effective_reference_opt);
    }
    if(symbols)release_module_symbols(symbols);
    if(incremental_enabled)release_memory(symbol_indices);
    ++module_debug_index;
    section_map=NULL;
}
extern "C" {
extern BYTE *next_base_relocation;
extern ContributionGroup *mac_procedure_group;
void *__stdcall reallocate_memory(void *,unsigned);
void __stdcall release_relocations(IMAGE_RELOCATION *);
void __stdcall write_bss_contribution(Contribution *);
int __stdcall pdb_write_types(Contribution *,void *,DWORD,BYTE);
void __stdcall pdb_write_symbols(void *,DWORD,DWORD);
BYTE __stdcall pdb_write_fixups(WORD,void *,DWORD);
BYTE __stdcall write_incremental_procedures(Contribution *,void *);
}
#pragma intrinsic(memset)
void RECON_THISCALL Contribution::ProcessOutput(LinkContext *context,IMAGE_SYMBOL *symbols,
    DWORD *symbol_indices,DWORD section_index)
{
    static BYTE initialized;
    static BYTE *buffer;
    static DWORD buffer_capacity=1024;
    BYTE mapped=0;
    BYTE *data=NULL;
    DWORD offset,error,count;
    FileRecord *file;
    if(!(initialized & 1)) {
        initialized |= 1;
        buffer=(BYTE *)allocate_memory(buffer_capacity);
    }
    if((characteristics & 0x800) || !raw_size)return;
    if((characteristics & 0x1e0)==0x80) {
        write_bss_contribution(this);
        return;
    }
    if(!pdb_enabled && (context->options.debug_formats & 2)) {
        if(group==debug_symbol_group) {
            DebugModuleRecord *record=(DebugModuleRecord *)module_debug_table+module_debug_index;
            if(!record->symbol_file_offset)record->symbol_file_offset=file_offset;
            else file_offset=record->symbol_file_offset+record->symbol_bytes;
            /* Reload the table just as the original does after the store. */
            record=(DebugModuleRecord *)module_debug_table+module_debug_index;
            record->symbol_bytes+=raw_size-padding;
        } else if(group==debug_type_group || group==debug_precompiled_group) {
            DebugModuleRecord *record=(DebugModuleRecord *)module_debug_table+module_debug_index;
            if(!record->type_file_offset) {
                record->type_file_offset=file_offset;
                record=(DebugModuleRecord *)module_debug_table+module_debug_index;
                record->precompiled_types=(group==debug_precompiled_group);
            } else file_offset=record->type_file_offset+record->type_bytes;
            record=(DebugModuleRecord *)module_debug_table+module_debug_index;
            record->type_bytes+=raw_size-padding;
        }
    }
    if(((module->library->flags & 16) || !(module->flags & 0x40)) &&
       (!pdb_enabled || group->section!=debug_section) &&
       (!incremental_enabled || !mac_target || group!=mac_procedure_group) &&
       group->section!=procedure_section) {
        data=(BYTE *)map_file_range(output_file,file_offset,raw_size);
        mapped=(data!=NULL);
    }
    if(!mapped) {
        if(raw_size>buffer_capacity) {
            buffer_capacity=raw_size;
            buffer=(BYTE *)reallocate_memory(buffer,raw_size);
        }
        data=buffer;
    }
    if(!(module->library->flags & 16) && (module->flags & 0x40))
        ReadConvertedData(data,raw_size-padding);
    else {
        if(module->library->flags & 16)offset=0;
        else offset=(module->flags & 0x40) ?
            *(DWORD *)((BYTE *)module->optional_data+12) : (DWORD)module->identity;
        error=0;
        seek_file(current_input_file,input_raw_offset+offset,FILE_BEGIN,&error);
        DWORD bytes=raw_size-padding;
        int file_number=current_input_file;
        if(file_access_statistics)record_file_access();
        file=file_slots[file_number];
        if(file->flags & 8)mapped_read(file,data,bytes);
        else buffered_read(file,data,bytes);
    }
    if(module!=output_module && relocation_count) {
        BYTE *previous=next_base_relocation;
        IMAGE_RELOCATION *relocations=ReadRelocations(&count);
        process_relocations(this,relocations,count,data,symbols,context,symbol_indices);
        release_relocations(relocations);
        group->section->has_base_relocations |= (previous!=next_base_relocation);
    }
    if(pdb_enabled && group->section==debug_section) {
        if(group==debug_type_group || group==debug_precompiled_group) {
            int result=pdb_write_types(this,data,raw_size-padding,(BYTE)!incremental_reuse);
            if(!result)return;
            if(result==2) { skip_module_debug_symbols=1; return; }
            if(result!=1) { link_contribution_fatal(this,1000); return; }
            if(!incremental_reuse)link_contribution_fatal(this,1000);
            incremental_status_code=16;
            return;
        }
        if(group==debug_symbol_group) {
            pdb_write_symbols(data,raw_size-padding,section_index);
            return;
        }
        if(group==debug_fixup_group) {
            if(pdb_write_fixups((WORD)module->module_index,data,(raw_size-padding)>>4))return;
            if(!incremental_reuse)link_contribution_fatal(this,1000);
            incremental_status_code=17;
            return;
        }
    }
    if((mac_target && incremental_enabled && group==mac_procedure_group) ||
       (!mac_target && group->section==procedure_section)) {
        BYTE result=mac_target ? write_incremental_procedures(this,data) :
            context->auxiliary->WriteProcedures(context->file_header.Machine,data,this);
        if(result)return;
        if(!incremental_reuse)link_contribution_fatal(this,1000);
        incremental_status_code=21;
        return;
    }
    if(padding)memset(data+raw_size-padding,
        ((characteristics & 0x1e0)==0x20 &&
         context->file_header.Machine==IMAGE_FILE_MACHINE_I386) ? 0xcc : 0,padding);
    if(mapped)return;
    error=0;
    seek_file(output_file,file_offset,FILE_BEGIN,&error);
    count=raw_size;
    int file_number=output_file;
    if(file_access_statistics)record_file_access();
    file=file_slots[file_number];
    if(file->flags & 8)mapped_write(file,data,count);
    else buffered_write(file,data,count);
}
/* The encountered short-import format supplies only section kinds 0..8. */
ConvertedSectionEmitter converted_data_emitters[9]={
    &LibrarianContext::EmitImportDebug,&LibrarianContext::EmitImportDebugRelocations,
    &LibrarianContext::EmitImportCode,&LibrarianContext::EmitImportFunctionDebug,
    &LibrarianContext::EmitImportOrdinalThunk,&LibrarianContext::EmitImportOrdinalThunk,
    &LibrarianContext::EmitImportNamedThunk,&LibrarianContext::EmitImportNamedThunk,
    &LibrarianContext::EmitImportHintName
};
void RECON_THISCALL Contribution::ReadConvertedData(void *buffer,DWORD bytes)
{
    BYTE kind=module->section_flags ? module->section_flags[
        ((LONG)((DWORD)this-(DWORD)module->contributions))>>6] : 255;
    if(kind>=9) { __asm { int 3 } }
    (((LibrarianContext *)module->auxiliary)->*converted_data_emitters[kind])(
        (LibrarianImportInfo *)module->optional_data,buffer,(void *)bytes);
}
BYTE RECON_THISCALL LinkAuxiliaryData::WriteProcedures(WORD,void *,Contribution *) { __asm { int 3 } return 0; }
extern "C" {
void __stdcall write_bss_contribution(Contribution *contribution)
{
    static const BYTE zeros[16]={0};
    OutputSection *section=contribution->group->section;
    DWORD offset=contribution->rva-section->rva,bytes,error=0;
    void *mapped;
    FileRecord *file;
    int file_number;
    if(offset>=section->initialized_file_size)return;
    bytes=contribution->raw_size;
    if(bytes>=section->initialized_file_size-offset)
        bytes=section->initialized_file_size-offset;
    mapped=map_file_range(output_file,contribution->file_offset,bytes);
    if(mapped) { memset(mapped,0,bytes);return; }
    seek_file(output_file,contribution->file_offset,FILE_BEGIN,&error);
    while(bytes>sizeof(zeros)) {
        file_number=output_file;
        if(file_access_statistics)record_file_access();
        file=file_slots[file_number];
        if(file->flags & 8)mapped_write(file,zeros,sizeof(zeros));
        else buffered_write(file,zeros,sizeof(zeros));
        bytes-=sizeof(zeros);
    }
    file_number=output_file;
    if(file_access_statistics)record_file_access();
    file=file_slots[file_number];
    if(file->flags & 8)mapped_write(file,zeros,bytes);
    else buffered_write(file,zeros,bytes);
}
int __stdcall pdb_write_types(Contribution *,void *,DWORD,BYTE) { __asm { int 3 } return 0; }
void __stdcall pdb_write_symbols(void *,DWORD,DWORD) { __asm { int 3 } }
BYTE __stdcall pdb_write_fixups(WORD,void *,DWORD) { __asm { int 3 } return 0; }
BYTE __stdcall write_incremental_procedures(Contribution *,void *) { __asm { int 3 } return 0; }
}
void RECON_THISCALL OutputModule::WriteDebug(BYTE) { __asm { int 3 } }
extern "C" {
void __stdcall adjust_debug_lines(IMAGE_LINENUMBER *,DWORD,Contribution *,IMAGE_SYMBOL *,DWORD,DWORD,BYTE,BYTE);
void __cdecl link_contribution_warning(Contribution *,unsigned,...);
void __stdcall write_output_line_numbers(LinkContext *context,Contribution *contribution)
{
    DWORD count=contribution->module==output_module ? 0 : contribution->line_number_count;
    DWORD bytes,offset,error,index;
    IMAGE_LINENUMBER *lines;
    FileRecord *file;
    if(!count || ((context->options.debug_level==0 || context->options.debug_level==1) &&
                  !context->options.map_lines))return;
    bytes=count*sizeof(IMAGE_LINENUMBER);
    lines=(IMAGE_LINENUMBER *)allocate_memory(bytes);
    OutputModule *module=contribution->module;
    if(module->library->flags & 16)offset=0;
    else offset=(module->flags & 0x40) ?
        *(DWORD *)((BYTE *)module->optional_data+12) : (DWORD)module->identity;
    index=(LONG)((DWORD)contribution-(DWORD)module->contributions)>>6;
    error=0;
    seek_file(current_input_file,module->section_map[index].line_number_offset+offset,
        FILE_BEGIN,&error);
    int file_number=current_input_file;
    if(file_access_statistics)record_file_access();
    file=file_slots[file_number];
    if(file->flags & 8)mapped_read(file,lines,bytes);
    else buffered_read(file,lines,bytes);
    if((context->options.debug_formats & 2) || context->options.map_lines) {
        module=contribution->module;
        adjust_debug_lines(lines,count,contribution,module->input_symbols,module->symbol_count,
            module->debug_symbol_index,(BYTE)((context->options.debug_formats>>1)&1),
            context->options.map_lines);
    }
    if(context->options.debug_formats & 1) {
        module=contribution->module;
        WORD base_line=0;
        for(index=0;index<count;++index) {
            IMAGE_LINENUMBER *line=lines+index;
            if(!line->Linenumber) {
                DWORD symbol_index=line->Type.SymbolTableIndex,next_index;
                IMAGE_SYMBOL *function,*block;
                if(module->symbol_count<=symbol_index+1)goto invalid_lines;
                function=module->input_symbols+symbol_index;
                if(!function->NumberOfAuxSymbols)goto invalid_lines;
                next_index=*(DWORD *)(function+1);
                if(module->symbol_count<=next_index+1)goto invalid_lines;
                block=module->input_symbols+next_index;
                if(block->StorageClass!=IMAGE_SYM_CLASS_FUNCTION || !block->NumberOfAuxSymbols)
                    goto invalid_lines;
                base_line=((IMAGE_AUX_SYMBOL *)(block+1))->Sym.Misc.LnSz.Linenumber;
                line->Linenumber=base_line;
                line->Type.VirtualAddress=function->Value;
            } else {
                DWORD input_rva=contribution->module->section_map[
                    (LONG)((DWORD)contribution-(DWORD)contribution->module->contributions)>>6
                    ].virtual_address;
                line->Type.VirtualAddress-=input_rva;
                line->Type.VirtualAddress+=contribution->rva;
                if(line->Linenumber==0x7fff)line->Linenumber=0;
                line->Linenumber+=base_line;
            }
        }
        error=0;
        seek_file(output_file,contribution->group->section->line_number_offset,FILE_BEGIN,&error);
        file_number=output_file;
        if(file_access_statistics)record_file_access();
        file=file_slots[file_number];
        if(file->flags & 8)mapped_write(file,lines,bytes);
        else buffered_write(file,lines,bytes);
        contribution->group->section->line_number_offset+=bytes;
    }
    release_memory(lines);
    return;
invalid_lines:
    link_contribution_warning(contribution,4200);
    release_memory(lines);
}
void __cdecl link_contribution_warning(Contribution *,unsigned,...) { __asm { int 3 } }
}
void RECON_THISCALL OutputModule::ReopenOutput(int) { __asm { int 3 } }
void RECON_THISCALL OutputModule::WriteCommonSymbols(BYTE reference_optimization)
{
    CommonSymbol *node=common_symbols;
    while(node) {
        StringNode *symbol=node->symbol;
        Contribution *contribution=symbol->contribution;
        while(contribution && (contribution->characteristics & 0x4000))
            contribution=contribution->associated_contribution;
        /* The original terminates the entire list at the first non-common. */
        if(!(symbol->flags & 2))return;
        if(!reference_optimization || !is_discardable_comdat(contribution))
            write_bss_contribution(contribution);
        node=node->next;
    }
}

extern "C" {
DWORD __stdcall local_import_relocation_type(LinkContext *);
void __stdcall record_debug_fixup(WORD,WORD,DWORD,DWORD);
}
void RECON_THISCALL LinkContext::WriteLocalImports(Contribution *contribution,IncludeSymbol **aliases)
{
    if(!*aliases || file_header.Machine==0x601)return;
    BYTE vxd=(is_vxd==1);
    DWORD relocation_type=local_import_relocation_type(this),error=0;
    seek_file(output_file,contribution->file_offset,FILE_BEGIN,&error);
    while(*aliases) {
        IncludeSymbol *alias=*aliases;
        StringNode *import=alias->symbol,*target=alias->next->symbol;
        DWORD address=(incremental_enabled && target->reserved28) ?
            incremental_code->rva+target->reserved28 : target->final_value;
        DWORD value=address+optional_header.ImageBase;
        int file_number=output_file;
        if(file_access_statistics)record_file_access();
        FileRecord *file=file_slots[file_number];
        if(file->flags & 8)mapped_write(file,&value,4);
        else buffered_write(file,&value,4);
        Contribution *from=import->contribution,*to=target->contribution;
        while(from && (from->characteristics & 0x4000))from=from->associated_contribution;
        while(to && (to->characteristics & 0x4000))to=to->associated_contribution;
        import->final_value=import->symbol.Value+from->rva;
        if(options.debug_formats & 8)
            record_debug_fixup((WORD)relocation_type,0,import->final_value,address);
        if(vxd) {
            OutputSection *source=from->group->section,*destination=to->group->section;
            if(!options.fixed_image) {
                LinkBaseRelocation *record=(LinkBaseRelocation *)next_base_relocation;
                record->type=3;
                record->address=((import->final_value-source->rva)&0xffffff)|
                    ((DWORD)(SHORT)source->index<<24);
                record->section=target->symbol.SectionNumber;
                record->value=((address-destination->rva)&0xffffff)|
                    ((DWORD)(SHORT)destination->index<<24);
                next_base_relocation+=12;
            }
        } else if(!options.rom && !options.fixed_image) {
            LinkBaseRelocation *record=(LinkBaseRelocation *)next_base_relocation;
            record->type=3;
            record->address=import->final_value;
            record->section=target->symbol.SectionNumber;
            record->value=0;
            next_base_relocation+=12;
        }
        alias=*aliases;
        *aliases=alias->next->next;
        release_memory(alias->next);
        release_memory(alias);
    }
}
extern "C" DWORD __stdcall local_import_relocation_type(LinkContext *) { __asm { int 3 } return 0; }

extern "C" {
extern const char *default_phase;
extern BYTE *base_relocations;
extern OutputSection *relocation_section;
int __cdecl compare_base_relocations(const void *,const void *);
int __cdecl compare_extended_base_relocations(const void *,const void *);
DWORD __stdcall reuse_incremental_relocations(void *);
}
void RECON_THISCALL LinkContext::FinishNativeSections(void)
{
    default_phase="EmitRelocations";
    if(options.fixed_image || mac_target)return;
    int (__cdecl *compare)(const void *,const void *)=options.new_relocations ?
        compare_extended_base_relocations : compare_base_relocations;
    qsort(base_relocations,(next_base_relocation-base_relocations)/12,12,compare);
    if(is_vxd==1) { WriteVxdRelocations();return; }
    if(incremental_reuse) {
        optional_header.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size=
            reuse_incremental_relocations(reserved264);
        return;
    }
    SectionEnumerator enumerator;
    initialize_section_enumerator(&enumerator,&sections);
    while(next_output_section(&enumerator)) {
        OutputSection *section=enumerator.current;
        if(section->has_base_relocations && (section->characteristics & 0x10000000))
            link_warning(NULL,4092,section->name);
    }
    end_section_enumerator(&enumerator);
    WriteBaseRelocations();
}
extern "C" {
int __cdecl compare_base_relocations(const void *left,const void *right)
{
    const LinkBaseRelocation *a=(const LinkBaseRelocation *)left;
    const LinkBaseRelocation *b=(const LinkBaseRelocation *)right;
    return (LONG)(a->address-b->address);
}
int __cdecl compare_extended_base_relocations(const void *,const void *) { __asm { int 3 } return 0; }
DWORD __stdcall reuse_incremental_relocations(void *) { __asm { int 3 } return 0; }
}

/* The incremental relocation map is the existing context storage at +264.
 * Only the two fields consumed by this writer have recovered roles. */
struct IncrementalRelocationMap {
    DWORD *block_offsets;
    DWORD reserved04;
    DWORD base_rva;
};
typedef char RelocationMapBaseAt08[
    (offsetof(IncrementalRelocationMap,base_rva)==8)?1:-1];
struct RelocationBufferedPosition {
    BYTE reserved00[12];
    DWORD position;
};

/* Keep the individual descriptor reloads and mapped/buffered write choices.
 * These are inline file operations in the original writer. */
#define WRITE_BASE_BYTES(source,bytes) do { \
    int number=output_file; \
    if(file_access_statistics)record_file_access(); \
    FileRecord *file=file_slots[number]; \
    if(file->flags & 8)mapped_write(file,(source),(bytes)); \
    else buffered_write(file,(source),(bytes)); \
} while(0)
#define BASE_FILE_POSITION(result) do { \
    int number=output_file; \
    if(file_access_statistics)record_file_access(); \
    FileRecord *file=file_slots[number]; \
    (result)=(file->flags & 8)?file->file_size: \
        ((RelocationBufferedPosition *)file->base)->position; \
} while(0)

void RECON_THISCALL LinkContext::WriteBaseRelocations(void)
{
    LinkBaseRelocation *record=(LinkBaseRelocation *)base_relocations;
    LinkBaseRelocation *limit=(LinkBaseRelocation *)next_base_relocation;
    ContributionGroup *group;
    IMAGE_BASE_RELOCATION block;
    IncrementalRelocationMap *map=(IncrementalRelocationMap *)reserved264;
    DWORD block_offset=relocation_section->file_offset;
    DWORD end=block_offset+relocation_section->raw_size;
    DWORD cursor,size,error=0,zero=0,entry;
    SHORT section=0;
    block.VirtualAddress=(record==limit)?0:record->address & 0xfffff000;
    for(group=relocation_section->groups;group;group=group->next)
        if(end>=group->file_offset)end=group->file_offset;
    seek_file(output_file,block_offset+8,FILE_BEGIN,&error);
    while(record!=limit) {
        DWORD page=record->address & 0xfffff000;
        if(page!=block.VirtualAddress) {
            BASE_FILE_POSITION(cursor);
            size=cursor-block_offset;
            if(size & 2) {
                size+=2;
                WRITE_BASE_BYTES(&zero,2);
                cursor+=2;
            }
            block.SizeOfBlock=size;
            error=0;
            seek_file(output_file,block_offset,FILE_BEGIN,&error);
            WRITE_BASE_BYTES(&block,8);
            if(incremental_enabled && size!=8)
                map->block_offsets[(block.VirtualAddress-map->base_rva)>>12]=block_offset;
            block_offset=cursor;
            block.VirtualAddress=page;
            section=0;
            error=0;
            seek_file(output_file,cursor+8,FILE_BEGIN,&error);
        }
        if(section!=record->section && options.new_relocations) {
            section=record->section;
            entry=((DWORD)section & 0xfff)|0x6000;
            WRITE_BASE_BYTES(&entry,2);
        }
        entry=(WORD)(record->type<<12)|(record->address & 0xfff);
        WRITE_BASE_BYTES(&entry,2);
        if(record->type==4)WRITE_BASE_BYTES(&record->value,2);
        ++record;
    }
    BASE_FILE_POSITION(cursor);
    size=cursor-block_offset;
    if(size & 2) {
        size+=2;
        WRITE_BASE_BYTES(&zero,2);
        cursor+=2;
    }
    block.SizeOfBlock=size;
    error=0;
    seek_file(output_file,block_offset,FILE_BEGIN,&error);
    WRITE_BASE_BYTES(&block,8);
    if(incremental_enabled && size!=8)
        map->block_offsets[(block.VirtualAddress-map->base_rva)>>12]=block_offset;
    optional_header.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size=
        cursor-relocation_section->file_offset;
    if(end<cursor)link_fatal(output_file_name,1130,cursor-end);
    /* This original section field retains the completed relocation cursor. */
    relocation_section->reserved08=cursor;
    size=end-cursor;
    if(size) {
        void *padding=zero_allocate(size);
        error=0;
        seek_file(output_file,cursor,FILE_BEGIN,&error);
        WRITE_BASE_BYTES(padding,size);
        release_memory(padding);
    }
    release_memory(base_relocations);
}
#undef BASE_FILE_POSITION
#undef WRITE_BASE_BYTES
