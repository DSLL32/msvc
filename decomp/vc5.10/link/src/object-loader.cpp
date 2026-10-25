/* Actual object-loader calls include a nonvirtual OutputModule thiscall.
   This file retains C linkage for the ordinary stdcall application functions. */
#include <windows.h>
#include "input.h"
#include "memory.h"
#include "full-link-functions.h"
#include <recon.h>

extern "C" {
extern BYTE mac_target,incremental_reuse;
extern DWORD coff_string_size;
extern BYTE *module_section_map;
extern IMAGE_SECTION_HEADER *module_section_headers;
extern unsigned module_section_capacity;
extern BYTE file_access_statistics;
void __stdcall resize_module_section_buffers(unsigned);
char *current_module_strings;
DWORD *current_module_symbol_indices,*section_symbol_ranges;
IMAGE_SYMBOL *__stdcall read_module_symbols(DWORD,DWORD,BYTE);
char *__stdcall read_module_string_table(const char *,DWORD,DWORD *);
void __stdcall add_input_section(LinkContext *,OutputModule *,DWORD,
    IMAGE_SECTION_HEADER *,IMAGE_SYMBOL *,InputArguments *);
void __stdcall process_module_symbols(LinkContext *,OutputModule *,void *,
    IMAGE_SYMBOL *,WORD,BYTE);
BYTE __stdcall process_incremental_sections(LinkContext *,OutputModule *,InputArguments *);
void __stdcall record_module_comdats(OutputModule *);
void __stdcall release_module_strings(char *);
void __stdcall release_module_symbols(IMAGE_SYMBOL *);
}

void __stdcall load_object_module(LinkContext *context,void *parent,
    OutputModule *module,WORD flags)
{
    IMAGE_SECTION_HEADER *headers;
    IMAGE_SYMBOL *symbols;
    DWORD offset,index,strings_size=0;
    InputArguments pending_arguments;
    module->ReadSectionHeaders(&headers);
    if (!(module->library->flags & 16))
        offset=(module->flags & 0x40) ?
            *(DWORD *)((BYTE *)module->optional_data+12) : (DWORD)module->identity;
    else offset=0;
    symbols=read_module_symbols(offset+module->symbol_offset,
        module->symbol_count,0);
    if (!(module->library->flags & 16))
        offset=(module->flags & 0x40) ?
            *(DWORD *)((BYTE *)module->optional_data+12) : (DWORD)module->identity;
    else offset=0;
    /* The original leaves strings_size undefined if the computed offset is zero;
       zero-initialize it rather than reproduce that unused-stack defect. */
    current_module_strings=read_module_string_table(
        (const char *)((module->library->flags & 16) ?
            module->identity : module->library->name),
        offset+module->symbol_count*18+module->symbol_offset,&strings_size);
    coff_string_size+=strings_size;
    current_module_symbol_indices=(DWORD *)zero_allocate(module->symbol_count*4);
    if (mac_target && !(module->characteristics & 0x4000))
        process_module_symbols(context,module,parent,symbols,flags,1);
    pending_arguments.first=pending_arguments.last=NULL;
    pending_arguments.count=0;
    for (index=0;index<module->section_count;++index)
        add_input_section(context,module,index+1,headers+index,symbols,&pending_arguments);
    if (incremental_reuse) process_incremental_sections(context,module,&pending_arguments);
    process_module_symbols(context,module,parent,symbols,flags,0);
    if (context->options.effective_reference_opt || context->options.fold_identical_comdats)
        record_module_comdats(module);
    module->section_map=NULL;
    release_module_strings(current_module_strings);
    release_module_symbols(symbols);
    release_memory(current_module_symbol_indices);
    current_module_strings=NULL;
    if (section_symbol_ranges) {
        release_memory(section_symbol_ranges);
        section_symbol_ranges=NULL;
    }
    if (incremental_reuse) clear_argument_list(&pending_arguments);
}

IMAGE_SECTION_HEADER *RECON_THISCALL OutputModule::ReadSectionHeaders(IMAGE_SECTION_HEADER **result)
{
    IMAGE_SECTION_HEADER *headers;
    Contribution *contribution;
    FileRecord *file;
    DWORD bytes,offset,error=0,index;
    if (section_count>module_section_capacity)
        resize_module_section_buffers(section_count);
    section_map=(SectionFileMap *)module_section_map;
    if (flags & 0x40) headers=section_headers;
    else {
        bytes=section_count*sizeof(IMAGE_SECTION_HEADER);
        offset=section_offset+optional_header_size;
        if (!(library->flags & 16)) offset+=(DWORD)identity;
        seek_file(current_input_file,offset,0,&error);
        if (file_access_statistics) record_file_access();
        file=file_slots[current_input_file];
        if (file->flags & 8) mapped_read(file,module_section_headers,bytes);
        else buffered_read(file,module_section_headers,bytes);
        headers=module_section_headers;
    }
    for (index=0;index<section_count;++index) {
        contribution=(Contribution *)contributions+index;
        contribution->relocation_count=headers[index].NumberOfRelocations;
        contribution->line_number_count=headers[index].NumberOfLinenumbers;
        contribution->relocation_offset=headers[index].PointerToRelocations;
        contribution->input_raw_offset=headers[index].PointerToRawData;
        section_map[index].virtual_address=headers[index].VirtualAddress;
        section_map[index].line_number_offset=headers[index].PointerToLinenumbers;
    }
    if (result) *result=headers;
    return headers;
}
char *__stdcall read_module_string_table(const char *name,DWORD offset,DWORD *size)
{
    __asm { int 3 }
    return NULL;
}
void __stdcall add_input_section(LinkContext *context,OutputModule *module,DWORD index,
    IMAGE_SECTION_HEADER *header,IMAGE_SYMBOL *symbols,InputArguments *pending)
{
    __asm { int 3 }
}
void __stdcall process_module_symbols(LinkContext *context,OutputModule *module,
    void *parent,IMAGE_SYMBOL *symbols,WORD flags,BYTE first_pass)
{
    __asm { int 3 }
}
BYTE __stdcall process_incremental_sections(LinkContext *context,OutputModule *module,
    InputArguments *pending)
{
    __asm { int 3 }
    return 0;
}
void __stdcall record_module_comdats(OutputModule *module) { __asm { int 3 } }
void __stdcall release_module_strings(char *strings) { __asm { int 3 } }
void __stdcall release_module_symbols(IMAGE_SYMBOL *symbols) { __asm { int 3 } }

void __stdcall resize_module_section_buffers(unsigned count) { __asm { int 3 } }
