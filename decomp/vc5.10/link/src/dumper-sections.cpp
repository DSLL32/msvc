/* Whole-section dump dispatch uses the shared cache and per-file owners. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include "dumper.h"
#include "input.h"
#include "memory.h"
#include "arguments.h"
#include "sections.h"
#include "full-link-functions.h"
#include "debug-output.h"
#include <recon.h>

extern "C" {
extern const char *default_phase;
extern DWORD dump_file_type,dump_file_size;
extern BYTE file_access_statistics;
extern InputArguments section_attributes;
extern FILE *map_output;
void __cdecl link_unresolved_error(const char *,unsigned,...);
char *__stdcall get_input_section_name(IMAGE_SECTION_HEADER *,const char *);
/* Exact code-free alias to the already-qualified empty RET12 owner. No
 * receiver or formal is consumed by that physical shared body. */
void __stdcall build_pe_dumper_labels(DWORD,DWORD,void *);
void __stdcall dump_section_header(DWORD,IMAGE_SECTION_HEADER *);
void __stdcall dump_disassembly(DumperSectionState *,void *,DWORD,IMAGE_SECTION_HEADER *,DWORD,DWORD,const void *,DWORD);
void __stdcall dump_raw_data(DWORD,const void *,DWORD);
void __stdcall dump_import_addresses(const void *,DWORD,DWORD);
void __stdcall dump_fpo(DumperSectionState *,DWORD,DWORD);
void __stdcall print_dumper_address(DumperSectionState *,const char *,DWORD);
IMAGE_SECTION_HEADER *__stdcall find_dumper_section(DWORD,WORD *);
IMAGE_SYMBOL *__stdcall find_dumper_symbol(DumperSectionState *,DWORD,DWORD,DWORD *);
void __stdcall dump_object_pdata(LinkContext *,IMAGE_SECTION_HEADER *,DWORD);
void __stdcall dump_directives(DWORD,DWORD);
void __stdcall dump_debug_section_header(IMAGE_SECTION_HEADER *);
void __stdcall dump_debug_directories(DumperSectionState *,IMAGE_SECTION_HEADER *);
void __stdcall dump_architecture(DumperSectionState *,IMAGE_SECTION_HEADER *);
void __stdcall dump_image_pdata(DumperSectionState *,LinkContext *);
void __stdcall dump_dependents(IMAGE_SECTION_HEADER *);
void __stdcall dump_delay_dependents(IMAGE_SECTION_HEADER *);
void __stdcall dump_imports(IMAGE_SECTION_HEADER *);
void __stdcall dump_delay_imports(IMAGE_SECTION_HEADER *);
void __stdcall dump_exports(IMAGE_SECTION_HEADER *);
void __stdcall dump_base_reloc_block(IMAGE_BASE_RELOCATION *,const void *);
void __stdcall dump_separate_pdata(const void *,DWORD);
void __stdcall dump_mppc_loader(const void *);
void __stdcall dump_rom_relocations(IMAGE_SECTION_HEADER *,DWORD,const void *,DWORD);
void __stdcall dump_coff_relocations(IMAGE_SECTION_HEADER *,DWORD,const void *,DWORD);
void __stdcall dump_line_numbers(DWORD);
void __stdcall dump_relocation_symbol(FILE *,const char *,IMAGE_SYMBOL *);
void __stdcall release_memory(void *);
BYTE __stdcall dumper_file_range_valid(DWORD,DWORD);
DWORD __stdcall count_pe_dumper_labels(DWORD,DWORD);
int __cdecl compare_dumper_labels(const void *,const void *);
DWORD __stdcall count_x86_dumper_labels(DWORD,DWORD,const void *,DWORD,DWORD,DWORD);
void __stdcall build_x86_dumper_labels(DWORD,DWORD,DWORD,const void *,DWORD,DWORD,DWORD,DumperLabel *);
DWORD __stdcall count_mips_dumper_labels(DWORD,DWORD,const void *,DWORD,DWORD,DWORD);
void __stdcall build_mips_dumper_labels(DWORD,DWORD,DWORD,const void *,DWORD,DWORD,DWORD,DumperLabel *);
DWORD __stdcall count_alpha_dumper_labels(DWORD,DWORD,const void *,DWORD,DWORD,DWORD);
void __stdcall build_alpha_dumper_labels(DWORD,DWORD,DWORD,const void *,DWORD,DWORD,DWORD,DumperLabel *);
DWORD __stdcall count_sh_dumper_labels(DWORD,DWORD,const void *,DWORD,DWORD,DWORD);
void __stdcall build_sh_dumper_labels(DWORD,DWORD,DWORD,const void *,DWORD,DWORD,DWORD,DumperLabel *);
DWORD __stdcall count_mppc_dumper_labels(DWORD,DWORD,const void *,DWORD,DWORD,DWORD);
void __stdcall build_mppc_dumper_labels(DWORD,DWORD,DWORD,const void *,DWORD,DWORD,DWORD,DumperLabel *);
DWORD __stdcall count_ppc_dumper_labels(DWORD,DWORD,const void *,DWORD,DWORD,DWORD);
void __stdcall build_ppc_dumper_labels(DWORD,DWORD,DWORD,const void *,DWORD,DWORD,DWORD,DumperLabel *);
DWORD __stdcall count_arm_dumper_labels(DWORD,DWORD,const void *,DWORD,DWORD,DWORD);
void __stdcall build_arm_dumper_labels(DWORD,DWORD,DWORD,const void *,DWORD,DWORD,DWORD,DumperLabel *);
}

typedef DWORD (__stdcall *CountDumperLabels)(DWORD,DWORD,const void *,DWORD,DWORD,DWORD);
typedef void (__stdcall *BuildDumperLabels)(DWORD,DWORD,DWORD,const void *,DWORD,DWORD,DWORD,DumperLabel *);

#define SECTION_READ(data,count) do { \
    if(file_access_statistics)record_file_access(); \
    record=file_slots[current_input_file]; \
    if(record->flags&8)mapped_read(record,(data),(count)); \
    else buffered_read(record,(data),(count)); \
} while(0)
#define DIRECTORY_IN_SECTION(index) \
    (dumper_context->optional_header.DataDirectory[index].VirtualAddress && \
     dumper_context->optional_header.DataDirectory[index].VirtualAddress>=section.VirtualAddress && \
     dumper_context->optional_header.DataDirectory[index].VirtualAddress<section.VirtualAddress+section.SizeOfRawData)

#pragma intrinsic(strcmp,strlen)
extern "C" void __stdcall dump_sections(DumperSectionState *state,DWORD extra_offset,
    DWORD header_bytes,DWORD archive)
{
    IMAGE_SECTION_HEADER section;
    IMAGE_BASE_RELOCATION reloc;
    FileRecord *record;
    InputArgument *filter;
    OutputSection *summary;
    const char *name;
    BYTE *data,*reloc_data;
    BYTE mapped,found;
    WORD section_index,filter_index;
    DWORD position,error,bytes,start,end,count,remaining,image_base,gp;
    DumperLabel *labels;
    CountDumperLabels count_labels;
    BuildDumperLabels build_labels;
    default_phase="DumpSections";
    position=archive_member_offset+extra_offset+header_bytes+
        dumper_context->file_header.SizeOfOptionalHeader;
    bytes=dumper_context->file_header.NumberOfSections*sizeof(IMAGE_SECTION_HEADER);
    dump_section_headers=(IMAGE_SECTION_HEADER *)allocate_memory(bytes);
    error=0;seek_file(current_input_file,position,0,&error);
    SECTION_READ(dump_section_headers,bytes);
    filter=section_attributes.first;
    for(filter_index=0;filter_index<section_attributes.count;++filter_index,filter=filter->next) {
        found=0;
        for(section_index=1;section_index<=dumper_context->file_header.NumberOfSections;++section_index) {
            name=get_input_section_name(dump_section_headers+section_index-1,dump_string_table);
            if(!strcmp(name,filter->name)) {found=1;break;}
        }
        if(!found)link_warning(NULL,4039,filter->name);
    }
    for(section_index=1;section_index<=dumper_context->file_header.NumberOfSections;++section_index) {
        section=dump_section_headers[section_index-1];
        name=get_input_section_name(&section,dump_string_table);
        if(section_attributes.count) {
            filter=section_attributes.first;
            for(filter_index=0;filter_index<section_attributes.count;++filter_index,filter=filter->next)
                if(!strcmp(name,filter->name))break;
            if(filter_index==section_attributes.count)continue;
        }
        bytes=dump_file_type!=1 && section.Misc.VirtualSize ?
            section.Misc.VirtualSize:section.SizeOfRawData;
        if(dumper_context->options.dump_summary) {
            summary=create_output_section(NULL,name,section.Characteristics,
                &dumper_context->sections,&dumper_context->optional_header);
            if(dump_file_type==2)summary->raw_size+=
                ((bytes-1)/dumper_context->optional_header.SectionAlignment+1)*
                dumper_context->optional_header.SectionAlignment;
            else summary->raw_size+=bytes;
        }
        if(bytes>section.SizeOfRawData)bytes=section.SizeOfRawData;
        if(dumper_context->options.dump_headers || section_attributes.count)
            dump_section_header(section_index,&section);
        data=NULL;mapped=0;
        if(section.PointerToRawData && section.SizeOfRawData) {
            position=archive_member_offset+section.PointerToRawData;
            data=(BYTE *)map_file_range(current_input_file,position,bytes);
            mapped=(data!=NULL);
            if(!mapped) {
                data=(BYTE *)allocate_memory(bytes);
                error=0;
                if(seek_file(current_input_file,position,0,&error)==-1)
                    link_unresolved_error(NULL,1106,position);
                SECTION_READ(data,bytes);
            }
        }
        if(data) {
            start=section.VirtualAddress+(dump_file_type==1?0:dumper_context->optional_header.ImageBase);
            end=start+bytes;
            if(dumper_context->options.dump_disassembly && (section.Characteristics&0x20)) {
                fputc('\n',map_output);
                count=0;labels=NULL;
                if(state->fixup_count) {
                    image_base=dumper_context->optional_header.ImageBase;
                    gp=image_base+dumper_context->optional_header.DataDirectory[8].VirtualAddress;
                    count_labels=NULL;build_labels=NULL;
                    switch(dumper_context->file_header.Machine) {
                    case 0x14c:count_labels=count_x86_dumper_labels;build_labels=build_x86_dumper_labels;break;
                    case 0x162:case 0x166:case 0x168:
                        count_labels=count_mips_dumper_labels;build_labels=build_mips_dumper_labels;break;
                    case 0x184:count_labels=count_alpha_dumper_labels;build_labels=build_alpha_dumper_labels;break;
                    case 0x1a2:case 0x1a6:
                        count_labels=count_sh_dumper_labels;build_labels=build_sh_dumper_labels;break;
                    case 0x1c0:count_labels=count_arm_dumper_labels;build_labels=build_arm_dumper_labels;break;
                    case 0x1f0:count_labels=count_ppc_dumper_labels;build_labels=build_ppc_dumper_labels;break;
                    case 0x601:count_labels=count_mppc_dumper_labels;build_labels=build_mppc_dumper_labels;break;
                    }
                    if(count_labels)count=count_labels(image_base,gp,state->fixups,state->fixup_count,start,end);
                    if(count) {
                        labels=(DumperLabel *)allocate_memory(count*sizeof(DumperLabel));
                        build_labels(image_base,gp,state->fixup_count,state->fixups,start,end,section_index,labels);
                    }
                }
                if(!count && dump_file_type==2) {
                    count=count_pe_dumper_labels(start,end);
                    if(count) {
                        labels=(DumperLabel *)allocate_memory(count*sizeof(DumperLabel));
                        build_pe_dumper_labels(start,end,labels);
                    }
                }
                qsort(labels,count,sizeof(DumperLabel),compare_dumper_labels);
                state->label_count=count;state->labels=labels;
                switch(dumper_context->file_header.Machine) {
                case 0x14c:case 0x162:case 0x166:case 0x168:case 0x184:
                case 0x1a2:case 0x1a6:case 0x1c0:case 0x1f0:case 0x601:
                    dump_disassembly(state,NULL,dumper_context->file_header.Machine,&section,
                        section_index,dumper_context->optional_header.ImageBase,data,bytes);
                    break;
                default:
                    fputs("LINK : warning : Disassembly not supported for this target machine\n",map_output);
                    break;
                }
                release_memory(state->labels);
                state->label_count=0;state->labels=NULL;
            }
            if(dumper_context->options.dump_raw_data) {
                fprintf(map_output,"\nRAW DATA #%hX\n",section_index);
                dump_raw_data(start,data,bytes);
            }
            if((BYTE)archive && dumper_context->options.dump_exports &&
                !_strnicmp(name,".idata$5",strlen(".idata$5")))
                dump_import_addresses(data,bytes,section_index);
        }
        if(dump_file_type==1) {
            if(dumper_context->options.dump_fpo && !strcmp(name,".debug$F"))
                dump_fpo(state,archive_member_offset+section.PointerToRawData,section.SizeOfRawData);
            if(dumper_context->options.dump_pdata && !strcmp(name,".pdata")) {
                error=0;seek_file(current_input_file,archive_member_offset+section.PointerToRawData,0,&error);
                dump_object_pdata(dumper_context,&section,section_index);
            }
            if(dumper_context->options.dump_directives && !strcmp(name,".drectve"))
                dump_directives(archive_member_offset+section.PointerToRawData,section.SizeOfRawData);
        } else if(dump_file_type==3) {
            if(!(dumper_context->file_header.Characteristics&0x200) && !strcmp(name,".rdata")) {
                if(dumper_context->options.dump_headers)dump_debug_section_header(&section);
                dump_debug_directories(state,&section);
            }
        } else if(dump_file_type==2) {
            if(DIRECTORY_IN_SECTION(6)) {
                if(dumper_context->options.dump_headers)dump_debug_section_header(&section);
                dump_debug_directories(state,&section);
            }
            if(dumper_context->options.dump_architecture && DIRECTORY_IN_SECTION(7))
                dump_architecture(state,&section);
            if(dumper_context->options.dump_pdata && DIRECTORY_IN_SECTION(3))
                dump_image_pdata(state,dumper_context);
            if(dumper_context->options.dump_dependents) {
                if(DIRECTORY_IN_SECTION(1))dump_dependents(&section);
                if(DIRECTORY_IN_SECTION(13))dump_delay_dependents(&section);
            }
            if(dumper_context->options.dump_imports) {
                if(DIRECTORY_IN_SECTION(1))dump_imports(&section);
                if(DIRECTORY_IN_SECTION(13))dump_delay_imports(&section);
            }
            if(dumper_context->options.dump_exports && DIRECTORY_IN_SECTION(0) &&
                dumper_context->file_header.Machine!=0x601)dump_exports(&section);
            if(dumper_context->options.dump_base_relocations && DIRECTORY_IN_SECTION(5)) {
                fprintf(map_output,"\nBASE RELOCATIONS #%hX\n",section_index);
                position=dumper_context->optional_header.DataDirectory[5].VirtualAddress+
                    section.PointerToRawData-section.VirtualAddress;
                if(dumper_file_range_valid(position,0)) {
                    error=0;seek_file(current_input_file,position,0,&error);
                    remaining=dumper_context->optional_header.DataDirectory[5].Size;
                    while(remaining) {
                        SECTION_READ(&reloc,8);
                        if(!reloc.SizeOfBlock)break;
                        reloc_data=(BYTE *)allocate_memory(reloc.SizeOfBlock-8);
                        SECTION_READ(reloc_data,reloc.SizeOfBlock-8);
                        dump_base_reloc_block(&reloc,reloc_data);
                        remaining-=reloc.SizeOfBlock;
                        release_memory(reloc_data);
                    }
                } else link_warning(NULL,4030,position,"BASERELOCATIONS");
            }
        } else if(dump_file_type==7) {
            if(DIRECTORY_IN_SECTION(6)) {
                if(dumper_context->options.dump_headers)dump_debug_section_header(&section);
                dump_debug_directories(state,&section);
            }
            if(dumper_context->options.dump_pdata && DIRECTORY_IN_SECTION(3))
                dump_separate_pdata(data+dumper_context->optional_header.DataDirectory[3].VirtualAddress-
                    section.VirtualAddress,dumper_context->optional_header.DataDirectory[3].Size/20);
            if(!strcmp(name,".ppcldr") && data)dump_mppc_loader(data);
        }
        if(dumper_context->options.dump_relocations && section.NumberOfRelocations) {
            fprintf(map_output,"\nRELOCATIONS #%hX\n",section_index);
            position=archive_member_offset+section.PointerToRelocations;
            count=section.NumberOfRelocations;
            if(dumper_file_range_valid(position,count*(dump_file_type==3?8:10))) {
                error=0;seek_file(current_input_file,position,0,&error);
                if(dump_file_type==3)dump_rom_relocations(&section,count,data,bytes);
                else dump_coff_relocations(&section,count,data,bytes);
            } else link_warning(NULL,4030,position,"RELOCATIONS");
        }
        if(dumper_context->options.dump_line_numbers && section.NumberOfLinenumbers) {
            fprintf(map_output,"\nLINENUMBERS #%hX\n",section_index);
            position=archive_member_offset+section.PointerToLinenumbers;
            if(dumper_file_range_valid(position,section.NumberOfLinenumbers*6)) {
                error=0;seek_file(current_input_file,position,0,&error);
                dump_line_numbers(section.NumberOfLinenumbers);
            } else link_warning(NULL,4030,position,"LINENUMBERS");
        }
        if(!mapped)release_memory(data);
    }
}
#undef SECTION_READ
#undef DIRECTORY_IN_SECTION

/* Genuine unvisited children retain their named entry traps. */
extern "C" {

void __stdcall build_dumper_symbol_index(DumperSectionState *,DWORD,DWORD);
void __stdcall release_dumper_symbol_index(DumperSectionState *);
void __stdcall disassemble_section(DumperSectionState *,void *,DWORD,DWORD,DWORD,DWORD,
    const void *,DWORD,FILE *) { __asm { int 3 } }
void __stdcall dump_disassembly(DumperSectionState *state,void *context,DWORD raw_machine,
    IMAGE_SECTION_HEADER *section,DWORD index,DWORD image_base,const void *data,DWORD bytes)
{
    WORD machine=(WORD)raw_machine;
    DWORD address=section->VirtualAddress+(dump_file_type==1?0:image_base);
    union {DWORD raw;BYTE low;} mode;
#ifdef __linux__
    /* The original initializes only its low BYTE, then forwards the DWORD.
     * Native upper bytes are zero representation only; the pending consumer
     * traps before any unqualified upper-bit use. No raw equality is claimed. */
    mode.raw=0;
#endif
    mode.low=0;
    if(machine==IMAGE_FILE_MACHINE_I386) {
        if(section->Characteristics&0x20000) {
            address=(index&0xffff)<<16;
            mode.low=1;
        }
    } else if(machine==IMAGE_FILE_MACHINE_R4000 &&
        !strncmp((char *)section->Name,".text16",7))mode.low=1;
    if(dump_file_type!=2)build_dumper_symbol_index(state,index,image_base);
    disassemble_section(state,context,raw_machine,mode.raw,index,address,data,bytes,map_output);
    if(dump_file_type!=2)release_dumper_symbol_index(state);
}
void __stdcall dump_raw_data(DWORD address,const void *buffer,DWORD length)
{
    const BYTE *data=(const BYTE *)buffer,*end=data+length,*line;
    DWORD format=dumper_context->options.dump_raw_format,offset=0;
    WORD width=dumper_context->options.dump_raw_count,units;
    BYTE explicit_width=(width!=0);
    int columns,padding=0,i;
    signed char character;
    if(!explicit_width) {
        switch(format) {
        case 0:width=16;padding=61;break;
        case 1:width=8;padding=53;break;
        case 2:width=4;padding=49;break;
        }
    }
#ifdef __linux__
    /* The original reads its first WORD/DWORD before checking the tail.
     * Such an initial short unit is outside the supported native byte owner;
     * Win32 retains the authentic read and later byte-format fallback. */
    if(length && ((format==1 && length<2)||(format==2 && length<4))) { __asm { int 3 } }
#endif
    while(data<end) {
        columns=fprintf(map_output,"  %08lX: ",address+offset);
        line=data;
        for(units=width;units && data<end;--units) {
            switch(format) {
            case 2:
                columns+=fprintf(map_output,"%08lX ",*(const DWORD *)data);
                data+=4;offset+=4;
                if(data+4>end)format=0;
                break;
            case 1:
                columns+=fprintf(map_output,"%04hX ",*(const WORD *)data);
                data+=2;offset+=2;
                if(data+2>end)format=0;
                break;
            case 0:
                columns+=fprintf(map_output,"%02X ",*data++);
                ++offset;
                break;
            }
        }
        if(!explicit_width) {
            while(columns++<padding)fputc(' ',map_output);
            for(i=0;i<16 && line!=end;++i) {
                character=(signed char)*line++;
                if(!(isprint)((unsigned char)character))character='.';
                fputc(character,map_output);
            }
        }
        fputc('\n',map_output);
    }
}
void __stdcall dump_import_addresses(const void *,DWORD,DWORD) { __asm { int 3 } }
IMAGE_SECTION_HEADER *__stdcall find_dumper_section(DWORD,WORD *) { __asm { int 3 } return NULL; }
IMAGE_SYMBOL *__stdcall find_dumper_symbol(DumperSectionState *,DWORD,DWORD,DWORD *) { __asm { int 3 } return NULL; }
void __stdcall print_dumper_address(DumperSectionState *state,const char *prefix,DWORD address)
{
    IMAGE_SYMBOL *symbol;
    DWORD section=address,displacement,i;
    IMAGE_SECTION_HEADER *header;
    const BYTE *record;
    if(state->public_symbols) {
#ifdef __linux__
        /* This external debug-provider domain is not qualified by the current
         * COFF input. Preserve the real Win32 virtual call and stop natively
         * before invoking an unqualified receiver. */
        __asm { int 3 }
#endif
        header=find_dumper_section(address,(WORD *)&section);
        if(!header)return;
        record=state->public_symbols->NearestPublic(section,address-header->VirtualAddress,&displacement);
        if(record && *(const WORD *)(record+2)==0x1009 &&
            *(const WORD *)(record+12)==(WORD)section && !displacement)
            fprintf(map_output,"%s%.*s",prefix,(unsigned)record[14],record+15);
        return;
    }
    if(state->symbol_count) {
        symbol=find_dumper_symbol(state,state->symbol_base+address,0,&displacement);
        if(symbol && !displacement) {
            fprintf(map_output,"%s",prefix);
            dump_relocation_symbol(map_output,"%s",symbol);
        }
        return;
    }
    if(!dump_symbols || dump_file_type==1)return;
    for(i=0;i<dumper_context->file_header.NumberOfSymbols;i+=symbol->NumberOfAuxSymbols+1) {
        symbol=dump_symbols+i;
        if(symbol->Value==address &&
            (symbol->StorageClass==IMAGE_SYM_CLASS_EXTERNAL || symbol->StorageClass==IMAGE_SYM_CLASS_STATIC) &&
            (symbol->Type&0x30)==0x20) {
            fprintf(map_output,"%s",prefix);
            dump_relocation_symbol(map_output,"%s",symbol);
            return;
        }
    }
}
void __stdcall dump_fpo(DumperSectionState *state,DWORD position,DWORD bytes)
{
    static const char *frame_names[]={"fpo","trap","tss","std"};
    DWORD error=0,count=bytes/sizeof(FPO_DATA),i;
    FPO_DATA *records;
    FileRecord *record;
    seek_file(current_input_file,position,FILE_BEGIN,&error);
    fprintf(map_output,"\nFPO Data (%ld)\n",count);
    records=(FPO_DATA *)allocate_memory(bytes+16);
    if(file_access_statistics)record_file_access();
    record=file_slots[current_input_file];
    if(record->flags&8)mapped_read(record,records,bytes);
    else buffered_read(record,records,bytes);
    fputs("                                       Use Has  Frame\n Address  Proc Size   Locals  Regs   Prolog  BP  SEH  Type   Params\n",map_output);
    for(i=0;i<count;++i) {
        FPO_DATA *fpo=records+i;
        fprintf(map_output,"  %08X   %8u %8u     %1d %8u   %c   %c   %4s     %4u",
            fpo->ulOffStart,fpo->cbProcSize,fpo->cdwLocals*4,
            fpo->cbRegs,fpo->cbProlog,fpo->fUseBP?'Y':'N',fpo->fHasSEH?'Y':'N',
            frame_names[fpo->cbFrame],fpo->cdwParams*4);
        print_dumper_address(state,"  ",fpo->ulOffStart);
        fputc('\n',map_output);
    }
    release_memory(records);
}
void __stdcall dump_object_pdata(LinkContext *,IMAGE_SECTION_HEADER *,DWORD) { __asm { int 3 } }
void __stdcall dump_directives(DWORD offset,DWORD bytes)
{
    char *text=(char *)allocate_memory(bytes+1),*token;
    DWORD error=0,index;
    FileRecord *record;
    seek_file(current_input_file,offset,FILE_BEGIN,&error);
    if(file_access_statistics)record_file_access();
    record=file_slots[current_input_file];
    if(record->flags&8)mapped_read(record,text,bytes);
    else buffered_read(record,text,bytes);
    for(index=0;index<bytes;++index)if(!text[index])text[index]=' ';
    text[bytes]=0;
    fputs("\n   Linker Directives\n   -----------------\n",map_output);
    token=strtok(text," ");
    while(token) {
        fprintf(map_output,"   %s\n",token);
        token=strtok(NULL," ");
    }
    release_memory(text);
}
void __stdcall dump_debug_directory(IMAGE_DEBUG_DIRECTORY *directory)
{
    DWORD saved,error,bytes,remaining;
    BYTE *data,*item;
    char pdb_name[MAX_PATH];
    FileRecord *record;
    default_phase="DumpDebugDirectory";
    fputs("        ",map_output);
    switch(directory->Type) {
    case 1:fputs("coff   ",map_output);break;
    case 2:fputs("cv     ",map_output);break;
    case 3:fputs("fpo    ",map_output);break;
    case 4:fputs("misc   ",map_output);break;
    case 5:fputs("pdata  ",map_output);break;
    case 6:fputs("fixup  ",map_output);break;
    case 7:fputs("-> src ",map_output);break;
    case 8:fputs("src -> ",map_output);break;
    default:fprintf(map_output,"(%6lu)",directory->Type);break;
    }
    fprintf(map_output,"%8X    %08X %8X",directory->SizeOfData,
        directory->AddressOfRawData,directory->PointerToRawData);
    if(directory->PointerToRawData && directory->Type==4) {
        if(file_access_statistics)record_file_access();
        record=file_slots[current_input_file];
        saved=record->flags&8?record->file_size:((FileBufferHeader *)record->base)->cursor;
        error=0;seek_file(current_input_file,directory->PointerToRawData,FILE_BEGIN,&error);
        bytes=remaining=directory->SizeOfData;
        data=(BYTE *)allocate_memory(bytes);
        if(file_access_statistics)record_file_access();
        record=file_slots[current_input_file];
        if(record->flags&8)mapped_read(record,data,bytes);
        else buffered_read(record,data,bytes);
        item=data;
        for(;;) {
#ifdef __linux__
            if(remaining<8) { __asm { int 3 } }
#endif
            if(*(DWORD *)item==1) {
                DWORD name_offset=dumper_context->optional_header.MajorLinkerVersion==2 &&
                    dumper_context->optional_header.MinorLinkerVersion<37?9:12;
#ifdef __linux__
                if(remaining<=name_offset || !memchr(item+name_offset,0,remaining-name_offset)) { __asm { int 3 } }
#endif
                fprintf(map_output,"    Image Name: %s",item+name_offset);
                break;
            }
            bytes=*(DWORD *)(item+4);
#ifdef __linux__
            if(!bytes || bytes>remaining) { __asm { int 3 } }
#endif
            item+=bytes;
            remaining-=bytes;
            if(!remaining)break;
        }
        release_memory(data);
        error=0;seek_file(current_input_file,saved,FILE_BEGIN,&error);
    }
    if(directory->PointerToRawData && directory->Type==2) {
        if(file_access_statistics)record_file_access();
        record=file_slots[current_input_file];
        saved=record->flags&8?record->file_size:((FileBufferHeader *)record->base)->cursor;
        error=0;seek_file(current_input_file,directory->PointerToRawData,FILE_BEGIN,&error);
        bytes=directory->SizeOfData;
        if(file_access_statistics)record_file_access();
        record=file_slots[current_input_file];
        if(record->flags&8)mapped_read(record,&pdb_codeview_header,16);
        else buffered_read(record,&pdb_codeview_header,16);
        fprintf(map_output,"    Format: %4.4s",&pdb_codeview_header);
        if(pdb_codeview_header.signature==0x3031424e) {
            bytes-=16;
#ifdef __linux__
            if(bytes>sizeof(pdb_name)) { __asm { int 3 } }
#endif
            if(file_access_statistics)record_file_access();
            record=file_slots[current_input_file];
            if(record->flags&8)mapped_read(record,pdb_name,bytes);
            else buffered_read(record,pdb_name,bytes);
#ifdef __linux__
            if(!memchr(pdb_name,0,bytes)) { __asm { int 3 } }
#endif
            fprintf(map_output,", %x, %x, %s",pdb_current_signature,pdb_current_age,pdb_name);
        }
        error=0;seek_file(current_input_file,saved,FILE_BEGIN,&error);
    }
    fputc('\n',map_output);
}
void __stdcall dump_debug_section_header(IMAGE_SECTION_HEADER *section)
{
    DWORD position,count,error,signature;
    IMAGE_DEBUG_DIRECTORY directory;
    FileRecord *record;
    default_phase="DumpDebugDirectories";
    if(dump_file_type==3) {
        position=archive_member_offset+section->PointerToRawData;
        count=(DWORD)-1;
    } else {
        position=archive_member_offset+section->PointerToRawData-section->VirtualAddress+
            dumper_context->optional_header.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress;
        count=dumper_context->optional_header.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size/
            sizeof(IMAGE_DEBUG_DIRECTORY);
        if(dump_file_type==2) {
            error=0;seek_file(current_input_file,position,FILE_BEGIN,&error);
            if(file_access_statistics)record_file_access();
            record=file_slots[current_input_file];
            if(record->flags&8)mapped_read(record,&signature,sizeof(signature));
            else buffered_read(record,&signature,sizeof(signature));
            if(signature==0x3030424e)return;
        }
    }
    fprintf(map_output,"\n\nDebug Directories\n        Type       Size     Address  Pointer\n\n");
    error=0;seek_file(current_input_file,position,FILE_BEGIN,&error);
    while(count--) {
        if(file_access_statistics)record_file_access();
        record=file_slots[current_input_file];
        if(record->flags&8)mapped_read(record,&directory,sizeof(directory));
        else buffered_read(record,&directory,sizeof(directory));
        if(!directory.Type)break;
        dump_debug_directory(&directory);
    }
}
void __stdcall dump_debug_fixups(DumperSectionState *,DWORD,DWORD) { __asm { int 3 } }
void __stdcall dump_debug_omap(DumperSectionState *,DWORD,DWORD,DWORD) { __asm { int 3 } }
void __stdcall dump_debug_directories(DumperSectionState *state,IMAGE_SECTION_HEADER *section)
{
    DWORD position,count,error,signature;
    IMAGE_DEBUG_DIRECTORY directory;
    FileRecord *record;
    if(dump_file_type==3) {
        position=archive_member_offset+section->PointerToRawData;
        count=(DWORD)-1;
    } else {
        position=archive_member_offset+section->PointerToRawData-section->VirtualAddress+
            dumper_context->optional_header.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress;
        count=dumper_context->optional_header.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size/
            sizeof(IMAGE_DEBUG_DIRECTORY);
        if(dump_file_type==2) {
            error=0;seek_file(current_input_file,position,FILE_BEGIN,&error);
            if(file_access_statistics)record_file_access();
            record=file_slots[current_input_file];
            if(record->flags&8)mapped_read(record,&signature,sizeof(signature));
            else buffered_read(record,&signature,sizeof(signature));
            if(signature==0x3030424e)return;
        }
    }
    while(count--) {
        error=0;seek_file(current_input_file,position,FILE_BEGIN,&error);
        if(file_access_statistics)record_file_access();
        record=file_slots[current_input_file];
        if(record->flags&8)mapped_read(record,&directory,sizeof(directory));
        else buffered_read(record,&directory,sizeof(directory));
        if(!directory.Type)break;
        switch(directory.Type) {
        case 3:
            if(dumper_context->options.dump_fpo)
                dump_fpo(state,directory.PointerToRawData,directory.SizeOfData);
            break;
        case 6:
            if(dumper_context->options.dump_fixups)
                dump_debug_fixups(state,directory.PointerToRawData,directory.SizeOfData);
            break;
        case 7:
            if(dumper_context->options.dump_omap_to)
                dump_debug_omap(state,directory.PointerToRawData,directory.SizeOfData,1);
            break;
        case 8:
            if(dumper_context->options.dump_omap_from)
                dump_debug_omap(state,directory.PointerToRawData,directory.SizeOfData,0);
            break;
        }
        position+=sizeof(IMAGE_DEBUG_DIRECTORY);
    }
}
void __stdcall dump_architecture(DumperSectionState *,IMAGE_SECTION_HEADER *) { __asm { int 3 } }
void __stdcall dump_image_pdata(DumperSectionState *,LinkContext *) { __asm { int 3 } }
void __stdcall dump_dependents(IMAGE_SECTION_HEADER *section)
{
    IMAGE_IMPORT_DESCRIPTOR descriptor;
    FileRecord *record;
    DWORD position,next_position,error,index,name_position;
    WORD i;
    BYTE ch;
    default_phase="DumpDependencies";
    fputs("\n  Image has the following dependencies:\n",map_output);
    position=section->PointerToRawData-section->VirtualAddress+
        dumper_context->optional_header.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
    if(!dumper_file_range_valid(position,0)) {
        link_warning(NULL,4030,position,"IMPORTS");
        return;
    }
    for(;;) {
        error=0;seek_file(current_input_file,position,FILE_BEGIN,&error);
        if(file_access_statistics)record_file_access();
        record=file_slots[current_input_file];
        if(record->flags&8)mapped_read(record,&descriptor,sizeof(descriptor));
        else buffered_read(record,&descriptor,sizeof(descriptor));
        next_position=position+sizeof(descriptor);
        if(!descriptor.Characteristics && !descriptor.Name && !descriptor.FirstThunk)break;
        index=0;
        for(i=0;i<dumper_context->file_header.NumberOfSections;++i) {
            IMAGE_SECTION_HEADER *candidate=dump_section_headers+i;
            if(descriptor.Name>=candidate->VirtualAddress &&
                descriptor.Name<candidate->VirtualAddress+candidate->SizeOfRawData)index=i;
        }
        name_position=dump_section_headers[index].PointerToRawData-
            dump_section_headers[index].VirtualAddress+descriptor.Name;
        error=0;seek_file(current_input_file,name_position,FILE_BEGIN,&error);
        fputs("\n    ",map_output);
        for(;;) {
            if(file_access_statistics)record_file_access();
            record=file_slots[current_input_file];
            if(record->flags&8)mapped_read(record,&ch,1);
            else buffered_read(record,&ch,1);
            if(!ch)break;
            fputc((signed char)ch,map_output);
        }
        position=next_position;
    }
    fputc('\n',map_output);
}
void __stdcall dump_delay_dependents(IMAGE_SECTION_HEADER *) { __asm { int 3 } }
/* The two bound-import record kinds share this encountered eight-byte prefix. */
typedef struct DumperBoundImport {
    DWORD timestamp;
    WORD name_offset,forwarders;
} DumperBoundImport;
void __stdcall dump_imports(IMAGE_SECTION_HEADER *section)
{
    IMAGE_IMPORT_DESCRIPTOR descriptor;
    IMAGE_SECTION_HEADER *candidate;
    FileRecord *record;
    DWORD position,next_descriptor,error,index,iat_section;
    DWORD name_position,lookup_position,bound_position,thunk,bound_value;
    DWORD bound_rva,bytes,offset,i;
    WORD j,hint;
    BYTE ch;
    BOOL bound_header;
    time_t timestamp;
    const char *time_text;
    BYTE *data;
    DumperBoundImport *module,*forwarder;
#ifdef __linux__
    BOOL found_iat;
#endif
#define IMPORT_READ(destination,length) do { \
    if(file_access_statistics)record_file_access(); \
    record=file_slots[current_input_file]; \
    if(record->flags&8)mapped_read(record,(destination),(length)); \
    else buffered_read(record,(destination),(length)); \
} while(0)
    default_phase="DumpImports";
    bound_header=dumper_context->optional_header.DataDirectory[11] /* IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT */.VirtualAddress!=0;
    fputs("\n  Section contains the following imports:\n",map_output);
    position=section->PointerToRawData-section->VirtualAddress+
        dumper_context->optional_header.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
    if(!dumper_file_range_valid(position,0)) {
        link_warning(NULL,4030,position,"IMPORTS");
        return;
    }
    for(;;) {
        error=0;seek_file(current_input_file,position,FILE_BEGIN,&error);
        IMPORT_READ(&descriptor,sizeof(descriptor));
        next_descriptor=position+sizeof(descriptor);
        if(!descriptor.Characteristics && !descriptor.Name && !descriptor.FirstThunk)break;
        index=0;
#ifdef __linux__
        found_iat=FALSE;
#endif
        for(j=0;j<dumper_context->file_header.NumberOfSections;++j) {
            candidate=dump_section_headers+j;
            if(descriptor.Name>=candidate->VirtualAddress &&
                descriptor.Name<candidate->VirtualAddress+candidate->SizeOfRawData)index=j;
            if((DWORD)descriptor.FirstThunk>=candidate->VirtualAddress &&
                (DWORD)descriptor.FirstThunk<candidate->VirtualAddress+candidate->SizeOfRawData) {
                iat_section=j;
#ifdef __linux__
                found_iat=TRUE;
#endif
            }
        }
        name_position=dump_section_headers[index].PointerToRawData-
            dump_section_headers[index].VirtualAddress+descriptor.Name;
        error=0;seek_file(current_input_file,name_position,FILE_BEGIN,&error);
        fputs("\n    ",map_output);
        for(;;) {
            IMPORT_READ(&ch,1);
            if(!ch)break;
            fputc((signed char)ch,map_output);
        }
        fputc('\n',map_output);
        if(descriptor.TimeDateStamp && (!bound_header || descriptor.TimeDateStamp!=(DWORD)-1)) {
            fprintf(map_output,"\n      %8lX time date stamp",descriptor.TimeDateStamp);
            timestamp=descriptor.TimeDateStamp;
            time_text=ctime(&timestamp);
            if(time_text)fprintf(map_output," %s",time_text);
            else fputc('\n',map_output);
            if(descriptor.ForwarderChain!=(DWORD)-1)
                fprintf(map_output,"      %8lX index of first forwarder reference\n",descriptor.ForwarderChain);
        }
        fputc('\n',map_output);
#ifdef __linux__
        /* The original leaves this index unwritten when no IAT section
         * matches. Do not turn that malformed-input state into index zero. */
        if(!found_iat) { __asm { int 3 } }
#endif
        candidate=dump_section_headers+(WORD)iat_section;
        bound_position=candidate->PointerToRawData-candidate->VirtualAddress+(DWORD)descriptor.FirstThunk;
        if(!descriptor.Characteristics) {
            lookup_position=bound_position;
            bound_position=0;
        } else {
            lookup_position=section->PointerToRawData-section->VirtualAddress+descriptor.Characteristics;
            if(!descriptor.TimeDateStamp)bound_position=0;
        }
        for(;;) {
            error=0;seek_file(current_input_file,lookup_position,FILE_BEGIN,&error);
            IMPORT_READ(&thunk,4);
            lookup_position+=4;
            if(!thunk)break;
            if(bound_position) {
                error=0;seek_file(current_input_file,bound_position,FILE_BEGIN,&error);
                IMPORT_READ(&bound_value,4);
                bound_position+=4;
            }
            fputs("      ",map_output);
            if(bound_position)fprintf(map_output,"%8X  ",bound_value);
            else fprintf(map_output,"          ");
            if(thunk&0x80000000) {
                fprintf(map_output,"       Ordinal %5lu\n",thunk&0xffff);
                continue;
            }
            for(j=0;j<dumper_context->file_header.NumberOfSections;++j) {
                candidate=dump_section_headers+j;
                if(thunk>=candidate->VirtualAddress &&
                    thunk<candidate->VirtualAddress+candidate->SizeOfRawData)break;
            }
#ifdef __linux__
            if(j==dumper_context->file_header.NumberOfSections) { __asm { int 3 } }
#endif
            candidate=dump_section_headers+j;
            name_position=candidate->PointerToRawData-candidate->VirtualAddress+thunk;
            error=0;seek_file(current_input_file,name_position,FILE_BEGIN,&error);
            IMPORT_READ(&hint,2);
            fprintf(map_output,"% 4hX   ",hint);
            for(;;) {
                IMPORT_READ(&ch,1);
                if(!ch)break;
                fputc((signed char)ch,map_output);
            }
            fputc('\n',map_output);
        }
        position=next_descriptor;
    }
    bound_rva=dumper_context->optional_header.DataDirectory[11] /* IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT */.VirtualAddress;
    if(bound_rva) {
        fputs("\n  Header contains the following bound import information:\n",map_output);
        if(bound_rva>=dumper_context->optional_header.SizeOfHeaders) {
            fprintf(map_output,"        **** Invalid offset %x\n",bound_rva);
            return;
        }
        bytes=dumper_context->optional_header.DataDirectory[11] /* IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT */.Size;
        data=(BYTE *)allocate_memory(bytes);
        error=0;seek_file(current_input_file,bound_rva,FILE_BEGIN,&error);
        IMPORT_READ(data,bytes);
        module=(DumperBoundImport *)data;
        for(;;) {
#ifdef __linux__
            offset=(BYTE *)module-data;
            if(offset>bytes || bytes-offset<sizeof(*module)) { __asm { int 3 } }
#endif
            if(!module->name_offset)break;
#ifdef __linux__
            if(module->name_offset>=bytes || !memchr(data+module->name_offset,0,bytes-module->name_offset)) { __asm { int 3 } }
#endif
            fprintf(map_output,"    Bound to %s [%8lX]",data+module->name_offset,module->timestamp);
            timestamp=module->timestamp;
            time_text=ctime(&timestamp);
            if(time_text)fprintf(map_output," %s",time_text);
            else fputc('\n',map_output);
            forwarder=module+1;
            for(i=0;i<module->forwarders;++i,++forwarder) {
#ifdef __linux__
                offset=(BYTE *)forwarder-data;
                if(offset>bytes || bytes-offset<sizeof(*forwarder) || forwarder->name_offset>=bytes ||
                    !memchr(data+forwarder->name_offset,0,bytes-forwarder->name_offset)) { __asm { int 3 } }
#endif
                fprintf(map_output,"      Contained forwarders bound to %s [%8lX]",
                    data+forwarder->name_offset,forwarder->timestamp);
                timestamp=forwarder->timestamp;
                time_text=ctime(&timestamp);
                if(time_text)fprintf(map_output," %s",time_text);
                else fputc('\n',map_output);
            }
            module=forwarder;
        }
        release_memory(data);
    }
#undef IMPORT_READ
}
void __stdcall dump_delay_imports(IMAGE_SECTION_HEADER *) { __asm { int 3 } }
void __stdcall dump_exports(IMAGE_SECTION_HEADER *section)
{
    IMAGE_EXPORT_DIRECTORY directory;
    FileRecord *record;
    DWORD position,error,*functions,*names,i,rva,export_begin,export_end;
    WORD *ordinals,index;
    BYTE ch;
    BOOL forwarded;
    time_t timestamp;
    const char *time_text;
    char version[32];
#define EXPORT_READ(destination,length) do { \
    if(file_access_statistics)record_file_access(); \
    record=file_slots[current_input_file]; \
    if(record->flags&8)mapped_read(record,(destination),(length)); \
    else buffered_read(record,(destination),(length)); \
} while(0)
    default_phase="DumpExports";
    position=section->PointerToRawData-section->VirtualAddress+
        dumper_context->optional_header.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
    if(!dumper_file_range_valid(position,0)) {
        link_warning(NULL,4030,position,"EXPORTS");
        return;
    }
    error=0;seek_file(current_input_file,position,FILE_BEGIN,&error);
    EXPORT_READ(&directory,sizeof(directory));
    position=section->PointerToRawData-section->VirtualAddress+directory.Name;
    error=0;seek_file(current_input_file,position,FILE_BEGIN,&error);
    fputs("\n         Section contains the following exports for ",map_output);
    for(;;) {
        EXPORT_READ(&ch,1);
        if(!ch)break;
        fputc((signed char)ch,map_output);
    }
    fprintf(map_output,"\n\n            %8lX characteristics\n            %8lX time date stamp",directory.Characteristics,directory.TimeDateStamp);
    timestamp=directory.TimeDateStamp;
    time_text=ctime(&timestamp);
    if(time_text)fprintf(map_output," %s",time_text);
    else fputc('\n',map_output);
    sprintf(version,"%hu.%02hu",directory.MajorVersion,directory.MinorVersion);
    fprintf(map_output,"            %8s version\n            %8lu ordinal base\n            %8lu number of functions\n            %8lu number of names\n\n            ordinal hint RVA      name\n\n",version,directory.Base,directory.NumberOfFunctions,directory.NumberOfNames);
    functions=(DWORD *)allocate_memory(directory.NumberOfFunctions*4);
    names=(DWORD *)allocate_memory(directory.NumberOfNames*4);
    ordinals=(WORD *)allocate_memory(directory.NumberOfNames*2);
    error=0;seek_file(current_input_file,section->PointerToRawData-section->VirtualAddress+
        (DWORD)directory.AddressOfFunctions,FILE_BEGIN,&error);
    EXPORT_READ(functions,directory.NumberOfFunctions*4);
    error=0;seek_file(current_input_file,section->PointerToRawData-section->VirtualAddress+
        (DWORD)directory.AddressOfNames,FILE_BEGIN,&error);
    EXPORT_READ(names,directory.NumberOfNames*4);
    error=0;seek_file(current_input_file,section->PointerToRawData-section->VirtualAddress+
        (DWORD)directory.AddressOfNameOrdinals,FILE_BEGIN,&error);
    EXPORT_READ(ordinals,directory.NumberOfNames*2);
    export_begin=dumper_context->optional_header.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
    export_end=export_begin+dumper_context->optional_header.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;
    for(i=0;i<directory.NumberOfNames;++i) {
        index=ordinals[i];
#ifdef __linux__
        if(index>=directory.NumberOfFunctions) { __asm { int 3 } }
#endif
        rva=functions[index];
        functions[index]=0;
        fprintf(map_output,"              %5u %4X ",directory.Base+index,i);
        forwarded=rva>=export_begin && rva<export_end;
        if(forwarded)fputs("         ",map_output);
        else fprintf(map_output,"%08lX ",rva);
        error=0;seek_file(current_input_file,section->PointerToRawData-section->VirtualAddress+names[i],FILE_BEGIN,&error);
        for(;;) {
            EXPORT_READ(&ch,1);
            if(!ch)break;
            fputc((signed char)ch,map_output);
        }
        if(forwarded) {
            fputs(" (forwarded to ",map_output);
            error=0;seek_file(current_input_file,section->PointerToRawData-section->VirtualAddress+rva,FILE_BEGIN,&error);
            for(;;) {
                EXPORT_READ(&ch,1);
                if(!ch)break;
                fputc((signed char)ch,map_output);
            }
            fputc(')',map_output);
        }
        fputc('\n',map_output);
    }
    for(i=0;i<directory.NumberOfFunctions;++i) {
        rva=functions[i];
        if(!rva)continue;
        fprintf(map_output,"              %5u      ",directory.Base+i);
        forwarded=rva>=export_begin && rva<export_end;
        if(forwarded)fputs("         ",map_output);
        else fprintf(map_output,"%08lX ",rva);
        fputs("[NONAME]",map_output);
        if(forwarded) {
            fputs(" (forwarded to ",map_output);
            error=0;seek_file(current_input_file,section->PointerToRawData-section->VirtualAddress+rva,FILE_BEGIN,&error);
            for(;;) {
                EXPORT_READ(&ch,1);
                if(!ch)break;
                fputc((signed char)ch,map_output);
            }
            fputc(')',map_output);
        }
        fputc('\n',map_output);
    }
    release_memory(functions);
    release_memory(names);
    release_memory(ordinals);
#undef EXPORT_READ
}
void __stdcall dump_base_reloc_block(IMAGE_BASE_RELOCATION *block,const void *entries)
{
    const WORD *entry=(const WORD *)entries;
    const WORD *end=entry+(DWORD)(block->SizeOfBlock-8)/2;
    WORD type;
    const char *name;
    default_phase="DumpBaseRelocations";
    fprintf(map_output,"%8lX virtual address, %8lX SizeOfBlock\n",block->VirtualAddress,block->SizeOfBlock);
    while(entry<end) {
        type=*entry>>12;
        switch(type) {
        case 0:name="ABS";break;
        case 1:name="HIGH";break;
        case 2:name="LOW";break;
        case 3:name="HIGHLOW";break;
        case 4:name="HIGHADJ";break;
        case 5:name="JMPADDR";break;
        case 6:name="SECTION";break;
        case 7:name="REL32";break;
        default:
            fprintf(map_output,"0x%hx ",type);
            name="UNKNOWN BASED RELOCATION";
            break;
        }
        fprintf(map_output,"%8hX %s",*entry++&0xfff,name);
        if(type==4) {
#ifdef __linux__
            /* The native input domain requires the HIGHADJ payload word. */
            if(entry>=end) { __asm { int 3 } }
#endif
            fprintf(map_output," (%04hx)\n",*entry++);
        } else fputc('\n',map_output);
    }
}
void __stdcall dump_separate_pdata(const void *,DWORD) { __asm { int 3 } }
void __stdcall dump_mppc_loader(const void *) { __asm { int 3 } }
void __stdcall dump_rom_relocations(IMAGE_SECTION_HEADER *,DWORD,const void *,DWORD) { __asm { int 3 } }

void __stdcall dump_line_numbers(DWORD count)
{
    DWORD bytes=count*sizeof(IMAGE_LINENUMBER),index,columns=0;
    WORD baseline=0;
    IMAGE_LINENUMBER *lines,*line;
    BYTE *end;
    IMAGE_SYMBOL *symbol,*tag;
    FileRecord *record;
    default_phase="DumpLinenumbers";
    lines=(IMAGE_LINENUMBER *)allocate_memory(bytes);
    if(file_access_statistics)record_file_access();
    record=file_slots[current_input_file];
    if(record->flags&8)mapped_read(record,lines,bytes);
    else buffered_read(record,lines,bytes);
    end=(BYTE *)lines+bytes;
    for(line=lines;(BYTE *)line<end;++line) {
        if(columns==4) { fputc('\n',map_output);columns=0; }
        if(!line->Linenumber) {
            baseline=0;
            if(columns)fputc('\n',map_output);
            index=line->Type.SymbolTableIndex;
            fprintf(map_output,"\n Symbol index: %8lX",index);
            if(!dump_symbols)fputc('\n',map_output);
            else {
                symbol=index<dumper_context->file_header.NumberOfSymbols?dump_symbols+index:NULL;
                if(symbol && (symbol->StorageClass==IMAGE_SYM_CLASS_EXTERNAL ||
                    (symbol->StorageClass==IMAGE_SYM_CLASS_STATIC && (symbol->Type&0x30)==0x20)) &&
                    symbol->NumberOfAuxSymbols==1) {
                    DWORD tag_index=*(DWORD *)(symbol+1);
                    if(tag_index && dump_symbols[tag_index].NumberOfAuxSymbols>=1) {
                        tag=dump_symbols+tag_index;
                        baseline=*(WORD *)((BYTE *)(tag+1)+4);
                        fprintf(map_output," Base line number: %5hu\n",baseline);
                    }
                    dump_relocation_symbol(map_output," Symbol name = %s\n",symbol);
                } else fputs("(error: invalid symbol index)\n",map_output);
            }
            columns=0;
        } else {
            fprintf(map_output,"%s%08lX(%5hu)",columns?"  ":" ",
                line->Type.VirtualAddress,(unsigned)baseline+line->Linenumber);
            ++columns;
        }
    }
    fputc('\n',map_output);
    release_memory(lines);
}
BYTE __stdcall dumper_file_range_valid(DWORD position,DWORD bytes)
{
    return position+bytes<=dump_file_size;
}
DWORD __stdcall count_pe_dumper_labels(DWORD,DWORD) { __asm { int 3 } }
int __cdecl compare_dumper_labels(const void *,const void *) { __asm { int 3 } }
DWORD __stdcall count_x86_dumper_labels(DWORD,DWORD,const void *,DWORD,DWORD,DWORD) { __asm { int 3 } }
void __stdcall build_x86_dumper_labels(DWORD,DWORD,DWORD,const void *,DWORD,DWORD,DWORD,DumperLabel *) { __asm { int 3 } }
DWORD __stdcall count_mips_dumper_labels(DWORD,DWORD,const void *,DWORD,DWORD,DWORD) { __asm { int 3 } }
void __stdcall build_mips_dumper_labels(DWORD,DWORD,DWORD,const void *,DWORD,DWORD,DWORD,DumperLabel *) { __asm { int 3 } }
DWORD __stdcall count_alpha_dumper_labels(DWORD,DWORD,const void *,DWORD,DWORD,DWORD) { __asm { int 3 } }
void __stdcall build_alpha_dumper_labels(DWORD,DWORD,DWORD,const void *,DWORD,DWORD,DWORD,DumperLabel *) { __asm { int 3 } }
DWORD __stdcall count_sh_dumper_labels(DWORD,DWORD,const void *,DWORD,DWORD,DWORD) { __asm { int 3 } }
void __stdcall build_sh_dumper_labels(DWORD,DWORD,DWORD,const void *,DWORD,DWORD,DWORD,DumperLabel *) { __asm { int 3 } }
DWORD __stdcall count_mppc_dumper_labels(DWORD,DWORD,const void *,DWORD,DWORD,DWORD) { __asm { int 3 } }
void __stdcall build_mppc_dumper_labels(DWORD,DWORD,DWORD,const void *,DWORD,DWORD,DWORD,DumperLabel *) { __asm { int 3 } }
DWORD __stdcall count_ppc_dumper_labels(DWORD,DWORD,const void *,DWORD,DWORD,DWORD) { __asm { int 3 } }
void __stdcall build_ppc_dumper_labels(DWORD,DWORD,DWORD,const void *,DWORD,DWORD,DWORD,DumperLabel *) { __asm { int 3 } }
DWORD __stdcall count_arm_dumper_labels(DWORD,DWORD,const void *,DWORD,DWORD,DWORD) { __asm { int 3 } }
void __stdcall build_arm_dumper_labels(DWORD,DWORD,DWORD,const void *,DWORD,DWORD,DWORD,DumperLabel *) { __asm { int 3 } }
}

extern "C" {
extern DWORD dump_file_type;
extern char *dump_string_cursor;
extern char short_symbol_name[9];
char *__stdcall display_symbol_name(const char *,DWORD);
}
extern "C" void __stdcall dump_section_header(DWORD number,IMAGE_SECTION_HEADER *header)
{
    const char *name,*text,*raw_name;
    char *display;
    DWORD bits,bit,value,i;
    WORD access=0,kind;
    IMAGE_SYMBOL *symbol;
    fprintf(map_output,"\nSECTION HEADER #%hX\n%8.8s name",(WORD)number,header->Name);
    if(header->Name[0]=='/') {
        name=get_input_section_name(header,dump_string_table);
        fprintf(map_output," (%s)",name);
    }
    fprintf(map_output,"\n%8lX %s\n%8lX virtual address\n%8lX size of raw data\n%8lX file pointer to raw data\n%8lX file pointer to relocation table\n%8lX file pointer to line numbers\n%8hX number of relocations\n%8hX number of line numbers\n%8lX flags\n",
        header->Misc.PhysicalAddress,dump_file_type==1?"physical address":"virtual size",
        header->VirtualAddress,header->SizeOfRawData,header->PointerToRawData,
        header->PointerToRelocations,header->PointerToLinenumbers,
        header->NumberOfRelocations,header->NumberOfLinenumbers,header->Characteristics);
    if(dump_file_type==3) {
        for(bits=header->Characteristics,bit=0;bits;bits>>=1,++bit) {
            if(!(bits&1))continue;
            switch(1UL<<bit) {
            case 0:text="Regular";break;
            case 0x20:text="Text";access=1;break;
            case 0x40:text="Data";access=6;break;
            case 0x80:text="GP Uninit Data";access=6;break;
            case 0x100:text="Data";access=2;break;
            case 0x200:text="GP Init Data";access=6;break;
            case 0x400:text="Uninit Data";access=6;break;
            case 0x800:text="UCode";break;
            case 0x08000000:text="Literal 8";break;
            case 0x10000000:text="Literal 4";break;
            case 0x20000000:text="Non-Relocatable overlay";access=1;break;
            case 0x40000000:text="Library";break;
            case 0x80000000:text="Init Code";access=1;break;
            default:text="RESERVED - UNKNOWN";break;
            }
            fprintf(map_output,"         %s\n",text);
        }
    } else {
        for(bits=header->Characteristics&0xff8fffff,bit=0;bits;bits>>=1,++bit) {
            if(!(bits&1))continue;
            value=1UL<<bit;
            switch(value) {
            case 8:text="No Pad";break;
            case 0x20:text="Code";break;
            case 0x40:text="Initialized Data";break;
            case 0x80:text="Uninitialized Data";break;
            case 0x100:text="Other";break;
            case 0x200:text="Info";break;
            case 0x800:text="Remove";break;
            case 0x1000:text="Communal";break;
            case 0x8000:text="Far Data";break;
            case 0x20000:text="Purgeable or 16-Bit";break;
            case 0x40000:text="Locked";break;
            case 0x80000:text="Preload";break;
            case 0x1000000:text="Extended relocations";break;
            case 0x2000000:text="Discardable";break;
            case 0x4000000:text="Not Cached";break;
            case 0x8000000:text="Not Paged";break;
            case 0x10000000:text="Shared";break;
            case 0x20000000:text=NULL;access|=1;break;
            case 0x40000000:text=NULL;access|=2;break;
            case 0x80000000:text=NULL;access|=4;break;
            default:text="RESERVED - UNKNOWN";break;
            }
            if(!text)continue;
            fprintf(map_output,"         %s",text);
            if(value==0x1000 && dump_symbols) {
                symbol=NULL;
                for(i=0;i<dumper_context->file_header.NumberOfSymbols;) {
                    IMAGE_SYMBOL *candidate=dump_symbols+i;
                    if(candidate->SectionNumber==(LONG)(WORD)number &&
                        (candidate->StorageClass==IMAGE_SYM_CLASS_EXTERNAL ||
                         (candidate->StorageClass==IMAGE_SYM_CLASS_STATIC &&
                          (candidate->NumberOfAuxSymbols!=1 || (candidate->Type&0x30)==0x20)))) {
                        symbol=candidate;break;
                    }
                    i+=candidate->NumberOfAuxSymbols+1;
                }
                if(!symbol)fputs(" (no symbol)",map_output);
                else {
                    raw_name=get_symbol_name((StringNode *)symbol,dump_string_cursor,&kind,NULL);
                    if(kind==1)raw_name=strncpy(short_symbol_name,raw_name,8);
                    display=display_symbol_name(raw_name,1);
                    fprintf(map_output,"; sym= %s",display);
                    raw_name=get_symbol_name((StringNode *)symbol,dump_string_cursor,&kind,NULL);
                    if(kind==1)raw_name=strncpy(short_symbol_name,raw_name,8);
                    if(display!=raw_name)release_memory(display);
                }
            }
            fputc('\n',map_output);
        }
        if(dump_file_type==1) {
            switch(header->Characteristics&0x700000) {
            case 0x100000:text="1 byte align";break;
            case 0x200000:text="2 byte align";break;
            case 0x300000:text="4 byte align";break;
            case 0x400000:text="8 byte align";break;
            case 0x500000:text="16 byte align";break;
            case 0x600000:text="32 byte align";break;
            case 0x700000:text="64 byte align";break;
            default:text="(no align specified)";break;
            }
            fprintf(map_output,"         %s\n",text);
        }
    }
    if(access) {
        switch(access) {
        case 1:text="Execute Only";break;
        case 2:text="Read Only";break;
        case 3:text="Execute Read";break;
        case 4:text="Write Only";break;
        case 5:text="Execute Write";break;
        case 6:text="Read Write";break;
        case 7:text="Execute Read Write";break;
        }
        fprintf(map_output,"         %s\n",text);
    }
}


/* Physical first argument remains a DWORD; each machine child qualifies the
 * consumed type width when reached. The two result owners are WORD/BYTE. */
typedef const char *(__stdcall *DumperRelocationName)(DWORD,WORD *,BYTE *);
extern "C" {
void __stdcall dump_relocation_symbol(FILE *stream,const char *format,IMAGE_SYMBOL *symbol)
{
    char short_name[9];
    const char *name;
    char *display;
    unsigned i;
    if(!symbol->N.Name.Short)name=dump_string_table+symbol->N.Name.Long;
    else {
        for(i=0;i<8;++i) {
            unsigned char c=symbol->N.ShortName[i];
            short_name[i]=(c>0x1f && c<0x7f)?c:0;
        }
        short_name[8]=0;
        name=short_name;
    }
    display=display_symbol_name(name,0);
    fprintf(stream,format,display);
    if(display!=name)free(display);
}
const char *__stdcall x86_dumper_relocation_name(DWORD raw_type,WORD *bytes,BYTE *has_symbol)
{
    const char *name;
    WORD width;
    /* The physical first DWORD is consumed as an unsigned WORD. */
    switch((WORD)raw_type) {
    case IMAGE_REL_I386_ABSOLUTE:name="ABS";width=0;break;
    case IMAGE_REL_I386_DIR16:name="DIR16";width=2;break;
    case IMAGE_REL_I386_REL16:name="REL16";width=2;break;
    case IMAGE_REL_I386_DIR32:name="DIR32";width=4;break;
    case IMAGE_REL_I386_DIR32NB:name="DIR32NB";width=4;break;
    case IMAGE_REL_I386_SEG12:name="SEG12";width=2;break;
    case IMAGE_REL_I386_SECTION:name="SECTION";width=2;break;
    case IMAGE_REL_I386_SECREL:name="SECREL";width=4;break;
    case IMAGE_REL_I386_REL32:name="REL32";width=4;break;
    default:name=NULL;width=0;break;
    }
    *bytes=width;
    *has_symbol=(width!=0);
    return name;
}
const char *__stdcall mips_dumper_relocation_name(DWORD,WORD *,BYTE *) { __asm { int 3 } return NULL; }
const char *__stdcall alpha_dumper_relocation_name(DWORD,WORD *,BYTE *) { __asm { int 3 } return NULL; }
const char *__stdcall sh_dumper_relocation_name(DWORD,WORD *,BYTE *) { __asm { int 3 } return NULL; }
const char *__stdcall mppc_dumper_relocation_name(DWORD,WORD *,BYTE *) { __asm { int 3 } return NULL; }
const char *__stdcall ppc_dumper_relocation_name(DWORD,WORD *,BYTE *) { __asm { int 3 } return NULL; }
const char *__stdcall arm_dumper_relocation_name(DWORD,WORD *,BYTE *) { __asm { int 3 } return NULL; }
}
#define SECTION_RELOCATION_READ(data,count) do { \
    if(file_access_statistics)record_file_access(); \
    record=file_slots[current_input_file]; \
    if(record->flags&8)mapped_read(record,(data),(count)); \
    else buffered_read(record,(data),(count)); \
} while(0)
extern "C" void __stdcall dump_coff_relocations(IMAGE_SECTION_HEADER *header,DWORD count,
    const void *section_data,DWORD section_bytes)
{
    static const char digits[]="0123456789ABCDEF";
    DumperRelocationName get_type=NULL;
    IMAGE_RELOCATION *relocations,*relocation;
    FileRecord *record;
    const BYTE *payload,*end=(const BYTE *)section_data+section_bytes;
    char type_buffer[32],applied[18],*cursor;
    const char *type;
    DWORD error,i;
    WORD bytes,j;
    BYTE has_symbol,value;
    default_phase="DumpRelocations";
    if((header->Characteristics&0x1000000) && count==0xffff && dump_file_type==1) {
        SECTION_RELOCATION_READ(&count,4);
        if(count<0xffff)link_fatal(NULL,1183,count);
        error=0;seek_file(current_input_file,-4,FILE_CURRENT,&error);
    }
    fputs("                                                Symbol    Symbol\n Offset    Type              Applied To         Index     Name\n --------  ----------------  -----------------  --------  ------\n",map_output);
    switch(dumper_context->file_header.Machine) {
    case 0x14c:get_type=x86_dumper_relocation_name;break;
    case 0x162:case 0x166:case 0x168:get_type=mips_dumper_relocation_name;break;
    case 0x184:get_type=alpha_dumper_relocation_name;break;
    case 0x1a2:case 0x1a6:get_type=sh_dumper_relocation_name;break;
    case 0x1c0:get_type=arm_dumper_relocation_name;break;
    case 0x1f0:get_type=ppc_dumper_relocation_name;break;
    case 0x601:get_type=mppc_dumper_relocation_name;break;
    }
    relocations=(IMAGE_RELOCATION *)allocate_memory(count*sizeof(IMAGE_RELOCATION));
    SECTION_RELOCATION_READ(relocations,count*sizeof(IMAGE_RELOCATION));
    for(i=0;i<count;++i) {
        relocation=relocations+i;
        if(!get_type) {
            sprintf(type_buffer,"0x%04X",relocation->Type);
            type=type_buffer;bytes=0;has_symbol=0;
        } else {
            type=get_type(relocation->Type,&bytes,&has_symbol);
            if(!type) {
                sprintf(type_buffer,"Unknown (0x%04X)",relocation->Type);
                type=type_buffer;bytes=0;has_symbol=0;
            }
        }
        payload=(const BYTE *)section_data+relocation->VirtualAddress-header->VirtualAddress;
        cursor=applied+sizeof(applied)-1;
        *cursor=0;
#ifdef __linux__
        /* Text supports the encountered two/four/eight-byte relocation
         * payloads. Stop before an unqualified wider or preceding owner read;
         * the Win32 source keeps its ordinary unchecked backwards formatting. */
        if(bytes>8 || (bytes && payload<(const BYTE *)section_data)) { __asm { int 3 } }
#endif
        for(j=0;j<bytes;++j,++payload) {
            if(j==4)*--cursor=' ';
            if(payload>=end) {*--cursor='*';*--cursor='*';}
            else {
                value=*payload;
                *--cursor=digits[value&15];
                *--cursor=digits[value>>4];
            }
        }
        fprintf(map_output," %08lX  %-16s  %17s  %8lX",relocation->VirtualAddress,
            type,cursor,relocation->SymbolTableIndex);
        if(has_symbol)
            dump_relocation_symbol(map_output,"  %s",dump_symbols+relocation->SymbolTableIndex);
        fputc('\n',map_output);
    }
    release_memory(relocations);
}

#undef SECTION_RELOCATION_READ
