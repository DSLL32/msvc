#include <windows.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "link-driver.h"
#include "sections.h"
#include "files.h"
#include "memory.h"
#include "comdat.h"
#include "full-link-functions.h"
#include <recon.h>

extern "C" {
extern BYTE pdb_enabled,mac_target,incremental_enabled,small_section_alignment;
extern BYTE no_base_relocations,large_alpha_image,large_ppc_image;
extern OutputModule *output_module;
extern OutputSection *relocation_section;
extern ContributionGroup *mac_procedure_group;
extern Contribution *mac_jump_contribution,*mac_toc,*incremental_code;
/* Original 47ef88: indexed only by contributions belonging to output_module. */
Contribution **output_contribution_map;
DWORD __stdcall align_section_size(DWORD,DWORD);
BYTE __stdcall is_discardable_comdat(Contribution *);
DWORD __stdcall mips_code_padding(Contribution *);
void __stdcall add_alpha_branch_island(Contribution *,DWORD,DWORD);
void __stdcall add_ppc_branch_island(Contribution *,DWORD,DWORD);
void __stdcall initialize_relocation_padding(void *,DWORD,DWORD,DWORD);
}

#pragma intrinsic(strcmp)
#define ROUND_UP(n,a) ((((n)-1)/(a)+1)*(a))

void RECON_THISCALL LinkContext::LayoutSections(DWORD *rva,DWORD *file_offset,DWORD *line_count)
{
    SectionEnumerator sections_enum;
    GroupEnumerator groups;
    GroupContributionEnumerator contributions;
    ContributionGroup *xdata;
    DWORD branch_count=0,branch_base,mapping_index=0;
    BYTE base_data_set=0;
    if(pdb_enabled)
        output_contribution_map=(Contribution **)zero_allocate(output_module->contribution_count*4);
    branch_base=*rva;
    xdata=find_contribution_group(read_only_section,".xdata");
    initialize_section_enumerator(&sections_enum,&sections);
    while(next_output_section(&sections_enum)) {
        OutputSection *section=sections_enum.current;
        Contribution *previous=NULL,*last_branch=NULL;
        DWORD type,address,cursor,initialized_end,section_size=0;
        BYTE had_bytes=0;
        if(is_vxd==1 && section==debug_section)continue;
        type=section->characteristics & 0x1e0;
        address=initialized_end=*rva;
        cursor=*file_offset;
        if(!base_data_set && type!=0x20) {
            base_data_set=1;
            optional_header.BaseOfData=address;
        }
        if(section==relocation_section && !is_vxd && !mac_target &&
           !options.fixed_image && !options.rom) {
            DWORD pages=(address-optional_header.BaseOfCode+0xfff)>>12;
            DWORD relocation_count=optional_header.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size;
            relocation_section->raw_size=relocation_count*2;
            if(pages>relocation_count)pages=relocation_count;
            if(small_section_alignment && optional_header.BaseOfCode<0x1000)++pages;
            relocation_section->raw_size+=pages*10;
            if(options.new_relocations)
                relocation_section->raw_size+=file_header.NumberOfSections*pages*2;
            if(incremental_enabled && !mac_target && !no_base_relocations) {
                DWORD extra=relocation_section->raw_size*10/100+100;
                if(file_header.Machine==0x184)extra*=2;
                initialize_relocation_padding(reserved264,
                    (address-optional_header.BaseOfCode+0xfff)>>12,
                    optional_header.BaseOfCode,extra);
                relocation_section->raw_size+=extra;
            }
            address+=relocation_section->raw_size;
            initialized_end=address;
            cursor+=relocation_section->raw_size;
        }
        initialize_group_enumerator(&groups,section);
        while(next_contribution_group(&groups)) {
            ContributionGroup *group=groups.current;
            DWORD aligned,padding,existing_padding=0;
            if(pdb_enabled && section==debug_section &&
               (group==debug_symbol_group || group==debug_type_group ||
                group==debug_precompiled_group))continue;
            aligned=address & ~(group->alignment-1);
            if(aligned!=address)aligned+=group->alignment;
            padding=aligned-address;
            if(padding) {
                cursor+=padding;
                address+=padding;
                section_size+=padding;
                /* The first group must already be aligned. The original
                 * also requires a preceding contribution on this path. */
                previous->raw_size+=padding;
                previous->padding+=padding & 0xff;
            }
            group->rva=address;
            group->file_offset=cursor;
            initialize_group_contributions(&contributions,group);
            while(next_group_contribution(&contributions)) {
                Contribution *contribution=contributions.current;
                if(contribution->characteristics & 0x800)continue;
                if(contribution->raw_size)had_bytes=1;
                if(contribution->characteristics & 0x4000)continue;
                if(options.effective_reference_opt && is_discardable_comdat(contribution))continue;
                padding=align_section_size(address,contribution->characteristics)-address;
                existing_padding+=contribution->padding;
                if(type==0x20) {
                    if(file_header.Machine==0x166 || file_header.Machine==0x168) {
                        if(options.default144) {
                            contribution->rva=address+padding;
                            DWORD extra=mips_code_padding(contribution);
                            padding+=extra;
                            if(extra && !previous)group->reserved0c=extra;
                        }
                    } else if(file_header.Machine==0x184 && large_alpha_image) {
                        if(contribution->raw_size+address+padding-branch_base>=
                           0x400000-branch_count*16) {
                            branch_base=address+padding;
                            add_alpha_branch_island(last_branch,branch_base,branch_count);
                            padding+=branch_count*16;
                            branch_count=0;
                            previous=last_branch;
                        }
                        /* Before assignment below, +0ch carries the branch
                         * reservation count, rather than a file offset. */
                        branch_count+=contribution->file_offset;
                    } else if(file_header.Machine==0x1f0 && large_ppc_image) {
                        DWORD target=address+padding;
                        if(target+0x40+contribution->raw_size+branch_count*16-branch_base>0x1ffffff) {
                            add_ppc_branch_island(last_branch,target,branch_count);
                            padding+=branch_count*16;
                            branch_count=0;
                            branch_base=target;
                        }
                        branch_count+=contribution->file_offset;
                        previous=last_branch;
                    }
                }
                if(padding) {
                    WORD machine=file_header.Machine;
                    if((machine!=0x166 && machine!=0x168 && machine!=0x184 &&
                        machine!=0x601 && machine!=0x1a2 && machine!=0x1a6 && machine!=0x1c0) ||
                       (previous && (!mac_target || !incremental_enabled ||
                                     previous->group!=mac_procedure_group))) {
                        previous->raw_size+=padding;
                        previous->padding+=padding;
                    }
                    address+=padding;
                    cursor+=padding;
                    section_size+=padding;
                }
                contribution->file_offset=cursor;
                contribution->rva=address;
                cursor+=contribution->raw_size;
                address+=contribution->raw_size;
                section_size+=contribution->raw_size;
                if(small_section_alignment || (contribution->characteristics & 0x1e0)!=0x80)
                    initialized_end=address;
                if(options.debug_level==2 || options.debug_level==3) {
                    WORD lines=contribution->module==output_module ? 0 : contribution->line_number_count;
                    if(lines)*line_count+=contribution->module==output_module ? 0 : contribution->line_number_count;
                }
                previous=last_branch=contribution;
                if(pdb_enabled && contribution->module==output_module)
                    output_contribution_map[mapping_index++]=contribution;
            }
            if(branch_count) {
                padding=align_section_size(address,0x500000)-address;
                if(file_header.Machine==0x184)
                    add_alpha_branch_island(previous,address+padding,branch_count);
                else add_ppc_branch_island(previous,address+padding,branch_count);
                padding+=branch_count*16;
                branch_count=0;
                cursor+=padding;
                address+=padding;
                section_size+=padding;
                previous->padding+=padding;
                previous->raw_size+=padding;
                initialized_end=address;
            }
            group->size=address-group->rva;
            if(group==debug_fixup_group) {
                if(pdb_enabled)pdb_module_count=group->size>>4;
                if(incremental_enabled && group->size) {
                    DWORD extra=pdb_module_count/10+25;
                    if(extra>0xfff)extra=0xfff;
                    pdb_module_count+=extra;
                    padding=extra*16;
                    previous->raw_size+=padding;
                    cursor+=padding;
                    previous->padding+=padding;
                    address+=padding;
                    section_size+=padding;
                    group->size+=padding;
                    initialized_end=address;
                }
            }
            if(mac_target) {
                if(!incremental_enabled)continue;
                if(group==mac_procedure_group) {
                    mac_module_count=group->size/20;
                    if(group->size) {
                        DWORD extra=mac_module_count/10+25;
                        if(extra>0xccc)extra=0xccc;
                        mac_module_count+=extra;
                        padding=extra*20;
                        previous->padding+=padding;
                        previous->raw_size+=padding;
                        cursor+=padding;
                        group->size+=padding;
                        address+=padding;
                        section_size+=padding;
                        initialized_end=address;
                    }
                }
            }
            if(incremental_enabled && group->size && section!=debug_section &&
               (!mac_target ? section!=procedure_section : group!=mac_procedure_group) &&
               strcmp(group->name,".bss") && strcmp(group->name,".textbss") &&
               strcmp(section->name,".idata")) {
                DWORD size=group->size,requested;
                if(type==0x20)requested=(size-existing_padding)*25/100+0x1000;
                else if(group==xdata)requested=(size-existing_padding)*10/100+0x200;
                else {
                    if(mac_target && !strcmp(group->name,".data")) {
                        size-=mac_jump_contribution->raw_size+mac_toc->raw_size;
                        existing_padding-=mac_toc->padding+mac_jump_contribution->padding;
                    }
                    requested=(size-existing_padding)*20/100+0x100;
                }
                padding=existing_padding<requested ? requested-existing_padding : 0;
                previous->raw_size+=padding;
                cursor+=padding;
                previous->padding+=padding;
                address+=padding;
                group->size+=padding;
                section_size+=padding;
                initialized_end=address;
            }
        }
        if(!mac_target && section==procedure_section) {
            auxiliary->SetProcedureSize(file_header.Machine,section_size);
            if(incremental_enabled && section_size) {
                DWORD record_size=auxiliary->procedure_record_size;
                DWORD padding=section_size/10+record_size*25;
                if(padding>0xffff)padding=0xffff;
                padding=(padding/record_size)*record_size;
                section_size+=padding;
                auxiliary->SetProcedureSize(file_header.Machine,section_size);
                previous->padding+=padding & 0xffff;
                initialized_end=address+padding;
                previous->raw_size+=padding;
            }
        }
        if(!section_size && had_bytes)--file_header.NumberOfSections;
        section->rva=*rva;
        section->file_offset=*file_offset;
        if(section->raw_size || section_size) {
            DWORD initialized=initialized_end-section->rva,bytes;
            section->reserved08=section->file_offset+initialized;
            section->virtual_size=section->raw_size+section_size;
            if(is_vxd==1)
                bytes=ROUND_UP(section->virtual_size,
                    ((BYTE *)section)[0x39] ? 4096 : optional_header.SectionAlignment);
            else bytes=ROUND_UP(section->virtual_size,optional_header.FileAlignment);
            section->raw_size=bytes;
            if(type==0x20)optional_header.SizeOfCode+=bytes;
            else if(type==0x40) {
                if(section!=debug_section)optional_header.SizeOfInitializedData+=bytes;
            } else if(type==0x80) {
                optional_header.SizeOfUninitializedData+=bytes;
                bytes=0;
                section->file_offset=0;
            }
            if(section!=debug_section)*rva+=ROUND_UP(section->raw_size,optional_header.SectionAlignment);
            section->initialized_file_size=initialized==section->virtual_size ? bytes :
                ROUND_UP(initialized,optional_header.FileAlignment);
            *file_offset+=section->initialized_file_size;
        }
    }
}

extern "C" BYTE __stdcall compact_procedure_entries(WORD machine);
void RECON_THISCALL LinkAuxiliaryData::SetProcedureSize(WORD machine,DWORD bytes)
{
    procedure_record_size=compact_procedure_entries(machine) ? 8 : 20;
    procedure_count=bytes/procedure_record_size;
}

extern "C" BYTE __stdcall compact_procedure_entries(WORD machine)
{
    return machine==0x1f0 || machine==0x1a2 || machine==0x1a6 || machine==0x1c0;
}

extern "C" {
void __stdcall count_section_lines(LinkOptions *,OutputSection *);
void __stdcall build_section_header(OutputSection *,IMAGE_SECTION_HEADER *);
void __stdcall adjust_rom_section_header(OutputSection *,IMAGE_SECTION_HEADER *,LinkContext *);
extern int output_file;
extern BYTE file_access_statistics;
extern const char *default_phase;
void __cdecl record_file_access(void);
void __cdecl link_contribution_warning(Contribution *,unsigned,...);
char *__stdcall format_contribution_module(Contribution *,char *);
}
extern "C" void __stdcall finish_image_layout(LinkContext *context,void *padding)
{
    SectionEnumerator enumerator;
    char module_name[512];
    default_phase="ZeroPad";
    initialize_section_enumerator(&enumerator,&context->sections);
    while(next_output_section(&enumerator)) {
        OutputSection *section=enumerator.current;
        DWORD extra,error=0;
        if((section->characteristics & IMAGE_SCN_LNK_REMOVE) || !section->raw_size ||
           (section->characteristics & 0x1e0)==IMAGE_SCN_CNT_UNINITIALIZED_DATA)
            continue;
        if(context->is_vxd==1 && section->index==context->file_header.NumberOfSections)
            continue;
        extra=ROUND_UP(section->reserved08,context->optional_header.FileAlignment)-section->reserved08;
        if(extra) {
            FileRecord *file;
            seek_file(output_file,section->reserved08,FILE_BEGIN,&error);
            if(file_access_statistics)record_file_access();
            file=file_slots[output_file];
            if(file->flags & 8)mapped_write(file,padding,extra);
            else buffered_write(file,padding,extra);
        }
        if(!strcmp(section->name,".rsrc")) {
            ContributionGroup *group=find_contribution_group(section,".rsrc$01");
            if(group->contribution_count>1)
                link_contribution_warning(group->last,4059,
                    format_contribution_module(group->first,module_name));
            context->optional_header.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress=section->rva;
            context->optional_header.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].Size=section->virtual_size;
        }
    }
}
void RECON_THISCALL LinkContext::WriteSections(DWORD *line_offset)
{
    SectionEnumerator enumerator;
    WORD index=1;
    initialize_section_enumerator(&enumerator,&sections);
    while(next_output_section(&enumerator)) {
        OutputSection *section=enumerator.current;
        IMAGE_SECTION_HEADER header;
        if(!section->raw_size)continue;
        count_section_lines(&options,section);
        if(section->line_number_count) {
            section->line_number_offset=*line_offset;
            *line_offset+=section->line_number_count*6;
        }
        build_section_header(section,&header);
        if(options.rom) {
            adjust_rom_section_header(section,&header,this);
            if(options.rom && !header.Characteristics)continue;
        }
        if(section!=debug_section) {
            (this->*image_write_section)(output_file,section,&header);
            section->index=index++;
        }
    }
    end_section_enumerator(&enumerator);
}

DWORD RECON_THISCALL LinkContext::RangeCodeBytes(DWORD start,DWORD limit,Contribution **last)
{
    SectionEnumerator sections_enum;
    GroupEnumerator groups;
    GroupContributionEnumerator contributions;
    Contribution *previous=NULL,*boundary=NULL;
    DWORD result=0;
    if(last)*last=NULL;
    initialize_section_enumerator(&sections_enum,&sections);
    while(next_output_section(&sections_enum)) {
        OutputSection *section=sections_enum.current;
        DWORD address,bytes,aligned,padding;
        if((section->characteristics & 0x1e0)!=IMAGE_SCN_CNT_CODE ||
           strcmp(section->name,".text"))continue;
        address=start;
        bytes=0;
        initialize_group_enumerator(&groups,section);
        while(next_contribution_group(&groups)) {
            ContributionGroup *group=groups.current;
            aligned=address & ~(DWORD)(group->alignment-1);
            if(aligned!=address)aligned+=group->alignment;
            padding=aligned-address;
            address+=padding;
            bytes+=padding;
            initialize_group_contributions(&contributions,group);
            while(next_group_contribution(&contributions)) {
                Contribution *contribution=contributions.current;
                Contribution *before=previous;
                previous=contribution;
                padding=contribution->raw_size ?
                    align_section_size(address,contribution->characteristics)-address : 0;
                address+=padding+contribution->raw_size;
                bytes+=padding+contribution->raw_size;
                if(limit && !boundary && incremental_code &&
                   limit>=incremental_code->raw_size && bytes>=limit-incremental_code->raw_size)
                    boundary=before;
            }
        }
        result=ROUND_UP(section->raw_size+bytes,optional_header.FileAlignment);
    }
    if(last && boundary)*last=boundary;
    return result;
}

extern "C" DWORD __stdcall section_data_size(OutputSection *section)
{
    GroupEnumerator groups;
    GroupContributionEnumerator contributions;
    DWORD size=0;
    initialize_group_enumerator(&groups,section);
    while(next_contribution_group(&groups)) {
        initialize_group_contributions(&contributions,groups.current);
        while(next_group_contribution(&contributions))
            size+=contributions.current->raw_size;
    }
    return size;
}

extern "C" void __stdcall count_section_lines(LinkOptions *options,OutputSection *section)
{
    GroupEnumerator groups;
    GroupContributionEnumerator contributions;
    section->line_number_count=0;
    if(!(options->debug_formats & 1) || !options->debug_level || options->debug_level==1)return;
    initialize_group_enumerator(&groups,section);
    while(next_contribution_group(&groups)) {
        initialize_group_contributions(&contributions,groups.current);
        while(next_group_contribution(&contributions)) {
            Contribution *contribution=contributions.current;
            if(contribution && (contribution->characteristics & 0x4000))continue;
            if(contribution->characteristics & 0x800)continue;
            if(options->effective_reference_opt && is_discardable_comdat(contribution))continue;
            section->line_number_count+=contribution->module==output_module ? 0 :
                contribution->line_number_count;
        }
    }
}

extern "C" void __stdcall build_section_header(OutputSection *section,IMAGE_SECTION_HEADER *header)
{
    strncpy((char *)header->Name,section->name,8);
    header->Misc.VirtualSize=section->virtual_size;
    header->VirtualAddress=section->rva;
    header->SizeOfRawData=section->initialized_file_size;
    header->PointerToRawData=section->file_offset;
    header->PointerToRelocations=0;
    header->PointerToLinenumbers=section->line_number_offset;
    header->NumberOfRelocations=0;
    header->NumberOfLinenumbers=(WORD)section->line_number_count;
    header->Characteristics=section->characteristics;
    if(!header->SizeOfRawData)header->PointerToRawData=0;
}

#pragma intrinsic(memset)
BYTE RECON_THISCALL LinkAuxiliaryData::PrepareOutput(void)
{
    if(!procedure_count)return 1;
    procedure_capacity=procedure_count;
    auxiliary_capacity=procedure_count*5;
    if(incremental_enabled) {
        procedure_data=(BYTE *)allocate_link_memory(1,procedure_count*procedure_record_size);
        contribution_records=allocate_link_memory(1,procedure_capacity*24);
        auxiliary_records=allocate_link_memory(1,auxiliary_capacity*8);
    } else {
        procedure_data=(BYTE *)allocate_memory(procedure_count*procedure_record_size);
        contribution_records=allocate_memory(procedure_capacity*24);
        auxiliary_records=allocate_memory(auxiliary_capacity*8);
    }
    memset(procedure_data,0,procedure_count*procedure_record_size);
    memset(contribution_records,0,procedure_capacity*24);
    memset(auxiliary_records,0,auxiliary_capacity*8);
    return 1;
}

/* These views consume the already allocated24B/8B procedure tables. Other
 * fields retain opaque roles until their writer is reconstructed. */
typedef struct ProcedureContribution {
    Contribution *contribution;
    DWORD record_count,first_record,first_relocation,reserved10,original_rva;
} ProcedureContribution;
typedef struct ProcedureRelocation {
    DWORD address;
    WORD reserved04;
    SHORT section;
} ProcedureRelocation;
typedef char ProcedureContributionSize[(sizeof(ProcedureContribution)==24)?1:-1];
typedef char ProcedureRelocationSize[(sizeof(ProcedureRelocation)==8)?1:-1];
extern "C" {
extern BYTE file_access_statistics;
extern int output_file;
extern BYTE *next_base_relocation;
int __cdecl compare_procedure_contributions(const void *,const void *);
}
void RECON_THISCALL LinkAuxiliaryData::FinishOutput(LinkContext *context)
{
    DWORD index,relocation,limit,cursor,error=0;
    BYTE fixed;
    ProcedureContribution *records=(ProcedureContribution *)contribution_records;
    OutputSection *section;
    if(!procedure_contribution_count)return;
    qsort(records,procedure_contribution_count,sizeof(*records),compare_procedure_contributions);
    records[procedure_contribution_count-1].contribution->padding=procedure_padding;
    records[procedure_contribution_count-1].contribution->raw_size+=procedure_padding;
    section=find_output_section(".pdata",&context->sections);
    fixed=context->options.fixed_image;
    cursor=section->rva;
    seek_file(output_file,section->file_offset,FILE_BEGIN,&error);
    for(index=0;index<procedure_contribution_count;++index) {
        ProcedureContribution *record=(ProcedureContribution *)contribution_records+index;
        Contribution *contribution=record->contribution;
        contribution->rva=cursor;
        cursor+=record->record_count*procedure_record_size;
        limit=record->first_relocation+contribution->relocation_count;
        for(relocation=record->first_relocation;relocation<limit;++relocation) {
            if(!fixed) {
                ProcedureRelocation *input=(ProcedureRelocation *)auxiliary_records+relocation;
                LinkBaseRelocation *output=(LinkBaseRelocation *)next_base_relocation;
                output->type=3;
                output->address=input->address-record->original_rva+contribution->rva;
                output->section=input->section;
                output->value=0;
                next_base_relocation+=12;
            }
        }
        BYTE *data=procedure_data+record->first_record*procedure_record_size;
        DWORD bytes=record->record_count*procedure_record_size;
        int file_number=output_file;
        if(file_access_statistics)record_file_access();
        FileRecord *file=file_slots[file_number];
        if(file->flags & 8)mapped_write(file,data,bytes);
        else buffered_write(file,data,bytes);
    }
}
extern "C" int __cdecl compare_procedure_contributions(const void *,const void *)
{ __asm { int 3 } return 0; }

extern "C" void __stdcall pad_debug_contribution(Contribution *contribution)
{
    ContributionGroup *owner=contribution->group,*group;
    GroupEnumerator groups;
    GroupContributionEnumerator contributions;
    DWORD aligned_end=(contribution->file_offset+contribution->raw_size+3)&~3U;
    /* The original reuses its incoming pointer slot for the Boolean and loads
     * its cleared-low-byte DWORD as the initial delta. It is defined carried
     * state, replaced when the first following debug group is visited. */
    DWORD delta=(DWORD)contribution&~255U;
    BYTE following=0,first=0;
#ifdef __linux__
    BYTE have_delta=0;
#endif
    owner->size=contribution->raw_size;
    initialize_group_enumerator(&groups,debug_section);
    while(next_contribution_group(&groups)) {
        group=groups.current;
        if(following) {
            if(first) {
                delta=aligned_end-group->file_offset;
                first=0;
#ifdef __linux__
                have_delta=1;
#endif
            }
            group->rva+=delta;
            group->file_offset+=delta;
            initialize_group_contributions(&contributions,group);
            while(next_group_contribution(&contributions)) {
                contributions.current->file_offset+=delta;
                contributions.current->rva+=delta;
            }
        } else if(group==owner) {
            following=1;
            first=1;
        }
    }
#ifdef __linux__
    /* No cross-image equality is claimed for observable pointer-derived data. */
    if(!have_delta)exit(125);
#endif
    debug_section->reserved08+=delta;
    debug_section->raw_size=debug_section->reserved08-debug_section->file_offset;
    debug_section->virtual_size=debug_section->reserved08-debug_section->file_offset;
}

/* This provisional entry name belongs to /SECTION attribute application,
 * not the separate group/section merge operation. Each matching argument is
 * marked in its existing flags and can update the same physical section. */
extern "C" void __stdcall apply_section_merge(OutputSection *section)
{
    extern InputArguments section_attributes;
    InputArgument *argument=section_attributes.first;
    const char *name=section->name;
    WORD index;
    for(index=0;index<section_attributes.count;++index,argument=argument->next) {
        char *attributes=strchr(argument->name,',');
        if(!attributes)continue;
        DWORD length=attributes-argument->name;
        ++attributes;
        if(strncmp(argument->name,name,length) || name[length])continue;
        argument->flags|=0x100;
        if(*attributes==',')continue;
        DWORD affected=0,bits=0;
        BYTE enabled=1;
        while(attributes && *attributes && *attributes!=',') {
            DWORD bit=0;
            BYTE inverse=0;
            switch(toupper((signed char)*attributes)) {
            case '!':case 'N':enabled=(BYTE)!enabled;break;
            case 'D':bit=0x02000000;break;
            case 'E':if(enabled)affected|=0xc0000000;bit=0x20000000;break;
            case 'K':inverse=1;bit=0x04000000;break;
            case 'P':inverse=1;bit=0x08000000;break;
            case 'R':if(enabled)affected|=0xa0000000;bit=0x40000000;break;
            case 'S':bit=0x10000000;break;
            case 'W':if(enabled)affected|=0x60000000;bit=0x80000000;break;
            case 'L':section->vxd_flag40=enabled;break;
            case 'X':bit=0x40000;break;
            case 'I':section->vxd_flag8000=enabled;break;
            case 'C':section->vxd_flag4000=enabled;break;
            default:link_fatal(NULL,1137,argument->name);break;
            }
            affected|=bit;
            if(inverse^enabled)bits|=bit;
            else bits&=~bit;
            ++attributes;
        }
        section->characteristics=(section->characteristics&~affected)|bits;
        section->vxd_flag10=(BYTE)((section->characteristics>>25)&1);
    }
}

extern "C" void __stdcall adjust_rom_section_header(OutputSection *section,
    IMAGE_SECTION_HEADER *header,LinkContext *context)
{
    switch(context->file_header.Machine) {
    case IMAGE_FILE_MACHINE_R4000:case IMAGE_FILE_MACHINE_R10000:
    case IMAGE_FILE_MACHINE_ALPHA:case 0x1a2:case 0x1a6:case 0x1c0:
    case IMAGE_FILE_MACHINE_POWERPC:
        break;
    default:return;
    }
    /* ROM section types are distinct from PE section characteristics. */
    switch(section->characteristics&0x1e0) {
    case IMAGE_SCN_CNT_CODE:
        header->Characteristics=0x20;
        break;
    case IMAGE_SCN_CNT_INITIALIZED_DATA:
        if(section==procedure_section || section==relocation_section)
            header->Characteristics=0;
        else header->Characteristics=(section->characteristics&IMAGE_SCN_MEM_WRITE)?
            0x40:0x100;
        break;
    case IMAGE_SCN_CNT_UNINITIALIZED_DATA:
        header->Characteristics=0x400;
        context->reserved1e8=section->rva;
        break;
    default:
        header->Characteristics=0;
        break;
    }
}
