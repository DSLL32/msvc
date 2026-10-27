#include <windows.h>
#include <stdio.h>
#include <setjmp.h>
#include <string.h>
#include <time.h>
#include "link.h"
#include "input.h"
#include "arguments.h"
#include "omf.h"
#include <recon.h>

/* Original 4759d0 owns a Win32 jmp_buf. Native uses its host jmp_buf
 * representation; its private register-save bytes are not compared. */
jmp_buf omf_error_jump;
FILE *omf_stream;
extern BYTE let_exceptions_escape;
void __stdcall read_omf_module(const char *);
void __cdecl write_omf_coff(void);

BYTE __stdcall convert_omf_file(const char *input,const char *output)
{
    __try {
        if(!setjmp(omf_error_jump)) {
            omf_stream=fopen(input,"rb");
            if(!omf_stream)link_fatal(NULL,1104,input);
            read_omf_module(input);
            fclose(omf_stream);
            omf_stream=fopen(output,"wb");
            if(!omf_stream)link_fatal(NULL,1104,output);
            write_omf_coff();
            fclose(omf_stream);
            return 1;
        }
    }
    __except(!let_exceptions_escape) {
    }
    return 0;
}

/* Original47c2a8..47cea8 owns the separate line-list descriptors, following
 * the complete section-header array. Only their encountered fields are typed. */
OmfLineList omf_line_lists[256];
DWORD omf_output_line_count; /*478244*/
#pragma intrinsic(memcpy)
void __cdecl write_omf_coff(void)
{
    IMAGE_FILE_HEADER header;
    IMAGE_SECTION_HEADER *section;
    DWORD data_position,relocation_position,line_position,i,auxiliary;
    WORD section_index,j,line_count;
    OmfRelocationNode *relocation,*next_relocation;
    OmfLineNode *line,*next_line;
    BYTE *auxiliary_bytes;
    omf_file_symbol_reference=0;
    header.Machine=IMAGE_FILE_MACHINE_I386;
    header.NumberOfSections=omf_output_section_count;
    header.TimeDateStamp=(DWORD)time(NULL);
    header.PointerToSymbolTable=sizeof(header)+
        (DWORD)omf_output_section_count*sizeof(IMAGE_SECTION_HEADER)+
        omf_initialized_data_size+omf_relocation_count*10+omf_output_line_count*6;
    header.NumberOfSymbols=omf_symbol_count;
    header.SizeOfOptionalHeader=0;
    header.Characteristics=0x100;
    fwrite(&header,sizeof(header),1,omf_stream);
    data_position=sizeof(header)+(DWORD)omf_output_section_count*sizeof(IMAGE_SECTION_HEADER);
    relocation_position=data_position+omf_initialized_data_size;
    line_position=relocation_position+omf_relocation_count*10;
    for(section_index=0;section_index<omf_output_section_count;++section_index) {
        section=&omf_output_sections[section_index];
        section->PointerToRawData=(!(section->Characteristics&IMAGE_SCN_CNT_UNINITIALIZED_DATA) &&
            section->SizeOfRawData)?data_position:0;
        section->PointerToRelocations=section->NumberOfRelocations?relocation_position:0;
        section->PointerToLinenumbers=section->NumberOfLinenumbers?line_position:0;
        if(omf_function_auxiliary_index) {
            /* This is the +8 pointer field of the actual function AUX slot. */
            memcpy((BYTE *)(omf_symbols+omf_function_auxiliary_index)+8,&line_position,4);
            omf_function_auxiliary_index=0;
        }
        fwrite(section,sizeof(*section),1,omf_stream);
        if(!(section->Characteristics&IMAGE_SCN_CNT_UNINITIALIZED_DATA))
            data_position+=section->SizeOfRawData;
        relocation_position+=(DWORD)section->NumberOfRelocations*10;
        line_position+=(DWORD)section->NumberOfLinenumbers*6;
    }
    for(section_index=0;section_index<omf_output_section_count;++section_index) {
        section=&omf_output_sections[section_index];
        if(section->SizeOfRawData && section->PointerToRawData)
            fwrite(omf_section_data[section_index],1,section->SizeOfRawData,omf_stream);
        release_memory(omf_section_data[section_index]);
        omf_section_data[section_index]=NULL;
    }
    for(section_index=0;section_index<omf_output_section_count;++section_index) {
        section=&omf_output_sections[section_index];
        if(section->Characteristics!=0x200 && section->NumberOfRelocations) {
            relocation=omf_relocations[section_index].head;
            for(j=0;j<section->NumberOfRelocations;++j) {
                /* The first10B are precisely offset/symbol/type, not padding. */
                fwrite(relocation,10,1,omf_stream);
                next_relocation=relocation->next;
                release_memory(relocation);
                relocation=next_relocation;
            }
            omf_relocations[section_index].head=NULL;
        }
    }
    for(section_index=0;section_index<omf_output_section_count;++section_index) {
        section=&omf_output_sections[section_index];
        if(section->Characteristics&IMAGE_SCN_CNT_CODE) {
            line_count=section->NumberOfLinenumbers;
            if(line_count) {
                line=omf_line_lists[section_index].head;
                for(j=0;j<line_count;++j) {
                    fwrite(line,6,1,omf_stream);
                    next_line=line->next;
                    release_memory(line);
                    line=next_line;
                }
                omf_line_lists[section_index].head=NULL;
            }
            section->NumberOfLinenumbers=line_count;
        }
    }
    for(section_index=0;section_index<omf_output_section_count;++section_index) {
        auxiliary=omf_section_auxiliary_indices[section_index];
        if(auxiliary && omf_symbols) {
            section=&omf_output_sections[section_index];
            auxiliary_bytes=(BYTE *)(omf_symbols+auxiliary);
            memcpy(auxiliary_bytes+4,&section->NumberOfRelocations,2);
            memcpy(auxiliary_bytes+6,&section->NumberOfLinenumbers,2);
        }
        omf_section_auxiliary_indices[section_index]=0;
    }
    fseek(omf_stream,header.PointerToSymbolTable,SEEK_SET);
    if(omf_symbol_count) {
        for(i=0;i<omf_symbol_count;++i)
            fwrite(omf_symbols+i,sizeof(IMAGE_SYMBOL),1,omf_stream);
        fwrite(&omf_string_length,4,1,omf_stream);
        if(omf_string_length) {
            omf_string_length-=4;
            fwrite(omf_strings+4,1,omf_string_length,omf_stream);
            release_memory(omf_strings);
        }
    }
    release_memory(omf_symbols);
    omf_symbols=NULL;
    omf_symbol_count=0;
    omf_output_line_count=0;
    omf_relocation_count=0;
    omf_string_length=0;
    omf_initialized_data_size=0;
    omf_output_section_count=0;
}
