/* This method's genuine ECX receiver requires the original C++ ABI. */
#include <windows.h>
#include <string.h>
#include "link-driver.h"
#include "sections.h"
#include "full-link-functions.h"
#include "memory.h"
#include <recon.h>

void RECON_THISCALL LinkContext::InitializeSections(void)
{
    SectionList *list=&sections;
    IMAGE_OPTIONAL_HEADER *header=&optional_header;
    bss_section=create_output_section(NULL,".bss",0xc0000080,list,header);
    small_data_section=create_output_section(NULL,".sdata",0xc0000040,list,header);
    read_only_section=create_output_section(NULL,".rdata",0x40000040,list,header);
    data_section=create_output_section(NULL,".data",0xc0000040,list,header);
    procedure_section=create_output_section(NULL,".pdata",0x40000040,list,header);
    import_section=create_output_section(NULL,".idata",0xc0300040,list,header);
    import_thunk_section=import_section;
    import_descriptor_section=import_section;
    delay_import_section=create_output_section(NULL,".didat",0xc0300040,list,header);
    export_section=create_output_section(NULL,".edata",0x40000040,list,header);
    export_group=create_contribution_group(".edata",export_section);
    exception_section=create_output_section(NULL,".xdata",0x40000040,list,header);
    architecture_section=create_output_section(NULL,".arch",0x42400040,list,header);
    ppc_loader_section=create_output_section(NULL,".ppcldr",0x42000040,list,header);
    debug_section=create_output_section(NULL,".debug",0x42100040,list,header);
    debug_symbol_group=create_contribution_group(".debug$S",debug_section);
    debug_type_group=create_contribution_group(".debug$T",debug_section);
    debug_precompiled_group=create_contribution_group(".debug$P",debug_section);
    debug_fixup_group=create_contribution_group(".debug$F",debug_section);
}

extern "C" InputArguments section_merges;
BYTE RECON_THISCALL LinkContext::MergeNamedSections(const char *source_name,const char *destination_name)
{
    OutputSection *source,*destination;
    source=find_output_section(source_name,&sections);
    if(!source)return 0;
    destination=find_matching_output_section(NULL,destination_name,source->characteristics,
        &sections,&optional_header);
    if(!destination) {
        destination=find_output_section(destination_name,&sections);
        if(!destination)destination=create_output_section(NULL,destination_name,
            source->characteristics,&sections,&optional_header);
    }
    merge_section_groups(source,destination);
    return 1;
}
void RECON_THISCALL LinkContext::ApplyMerges(void)
{
    InputArgument *argument=section_merges.first;
    unsigned index;
    for(index=0;(WORD)index<section_merges.count;++index) {
        MergeNamedSections(argument->name,argument->original_name);
        argument=argument->next;
    }
}

extern "C" {
extern BYTE mac_target,incremental_enabled;
OutputSection *relocation_section;
BYTE no_base_relocations;
/* The producer reserves twelve bytes per relocation. Fields remain opaque
 * until the real relocation writers consume these same objects. */
BYTE *base_relocations,*next_base_relocation,*base_relocations_end;
}
void RECON_THISCALL LinkContext::FinalizeOrder(void)
{
    DWORD count;
    if(mac_target)return;
    if(!is_vxd) {
        relocation_section=create_output_section(NULL,".reloc",0x42000040,
            &sections,&optional_header);
        if(!options.fixed_image)relocation_section->raw_size=1;
    }
    if(!options.fixed_image) {
        if(!optional_header.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size) {
            no_base_relocations=1;
            optional_header.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size=1;
        }
        count=optional_header.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size;
        base_relocations=(BYTE *)allocate_memory(count*12);
        next_base_relocation=base_relocations;
        base_relocations_end=base_relocations+
            optional_header.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size*12;
    }
}

BYTE RECON_THISCALL LinkContext::CanMergeReadOnlyData(void)
{
    switch(optional_header.Subsystem) {
    case IMAGE_SUBSYSTEM_WINDOWS_GUI:
    case IMAGE_SUBSYSTEM_WINDOWS_CUI:
        if(optional_header.MajorSubsystemVersion<3 ||
           (optional_header.MajorSubsystemVersion==3 &&
            optional_header.MinorSubsystemVersion<51))return 0;
        break;
    case IMAGE_SUBSYSTEM_POSIX_CUI:
        return 0;
    }
    return 1;
}

extern "C" void __stdcall move_named_group(const char *,OutputSection *,OutputSection *);
BYTE RECON_THISCALL LinkContext::CanMergeImportSections(void)
{
    if(specified_switches & 0x200000)return 0;
    if(incremental_enabled)return 0;
    if(options.profile && file_header.Machine==0x14c)return 0;
    switch(optional_header.Subsystem) {
    case IMAGE_SUBSYSTEM_WINDOWS_GUI:
    case IMAGE_SUBSYSTEM_WINDOWS_CUI:
        if(optional_header.MajorSubsystemVersion<3 ||
           (optional_header.MajorSubsystemVersion==3 &&
            optional_header.MinorSubsystemVersion<51))return 0;
        break;
    case IMAGE_SUBSYSTEM_POSIX_CUI:
        return 0;
    case 9:
        if(optional_header.MajorSubsystemVersion<2)return 0;
        break;
    }
    return 1;
}
/* This formerly provisional boundary name covers the import-section merge. */
void RECON_THISCALL LinkContext::MergeExceptionSections(void)
{
    OutputSection *destination;
    ContributionGroup *group;
    if(!CanMergeImportSections())return;
    destination=optional_header.Subsystem==9 ? data_section : read_only_section;
    move_named_group(".idata$5",import_section,destination);
    group=find_contribution_group(destination,".idata$5");
    if(!group)return;
    move_group_to_end(group);
    import_thunk_section=destination;
    if(options.retain_relocations) {
        import_descriptor_section=create_output_section(NULL,"INIT",0xe2000020,
            &sections,&optional_header);
        merge_output_section(import_section,import_descriptor_section);
    } else if(!options.rom) {
        import_descriptor_section=read_only_section;
        merge_output_section(import_section,read_only_section);
    }
}

/* This BSS byte is set by the /ORDER enabling boundary at original 458150. */
extern "C" BYTE order_file_enabled;
BYTE order_file_enabled;
void RECON_THISCALL LinkContext::PrepareAuxiliarySections(void)
{
    if(order_file_enabled)ApplyOrderFile();
}

WORD RECON_THISCALL LinkContext::CountOutputSections(void)
{
    SectionEnumerator output;
    GroupEnumerator groups;
    GroupContributionEnumerator contributions;
    DWORD count=0;
    initialize_section_enumerator(&output,&sections);
    while(next_output_section(&output)) {
        OutputSection *section=output.current;
        BYTE empty=1;
        if(options.rom && (section==procedure_section || section==relocation_section))continue;
        if(section->characteristics & 0x800)continue;
        if(section->raw_size) {
            ++count;
            continue;
        }
        if(section==debug_section)continue;
        initialize_group_enumerator(&groups,section);
        while(next_contribution_group(&groups)) {
            initialize_group_contributions(&contributions,groups.current);
            while(next_group_contribution(&contributions)) {
                Contribution *contribution=contributions.current;
                if(!(contribution->characteristics & 0x800)) {
                    empty=contribution->raw_size==0;
                    if(!empty)break;
                }
            }
            end_section_enumerator((SectionEnumerator *)&contributions);
            if(!empty) {
                ++count;
                break;
            }
        }
        end_section_enumerator((SectionEnumerator *)&groups);
    }
    return (WORD)count;
}

extern "C" DWORD __stdcall align_image_base(DWORD base)
{
    DWORD aligned=base;
    if(base & 0xffff)aligned=(base & 0xffff0000)+0x10000;
    if(base!=aligned)link_warning(NULL,4023,base,aligned);
    return aligned;
}

extern "C" {
OutputSection *resource_section;
extern MacResource *mac_resource_list;
}
void RECON_THISCALL LinkContext::PrepareSectionLayout(void)
{
    OutputSection *section,*previous,*first_moved;
    MacResource *resource;
    resource_section=find_output_section(".rsrc",&sections);
    if(resource_section)move_section_to_end(resource_section,&sections);
    if(!options.rom && !mac_target) {
        partition_sections(&sections,0x40,0x40);
        partition_sections(&sections,0x80,0x80);
    } else {
        partition_sections(&sections,0x80,0x80);
        partition_sections(&sections,0x40,0x40);
    }
    partition_sections(&sections,0x20,0x20);
    if(!mac_target)partition_sections(&sections,0x08000000,0x08000000);
    partition_sections(&sections,0x02000000,0);
    if(relocation_section)move_section_to_end(relocation_section,&sections);
    if(is_vxd==1) {
        section=sections.first;
        first_moved=NULL;
        while(section!=*sections.last) {
            if(strstr(section->name,"vxdp")) {
                if(section==first_moved)break;
                if(!first_moved)first_moved=section;
                previous=section;
                section=section->next;
                move_section_to_end(previous,&sections);
            } else section=section->next;
        }
    } else {
        if(resource_section)move_section_to_end(resource_section,&sections);
        if(architecture_section && file_header.Machine==0x184)
            move_section_to_end(architecture_section,&sections);
        if(relocation_section)move_section_to_end(relocation_section,&sections);
    }
    if(mac_target) {
        for(resource=mac_resource_list;resource;resource=resource->next)
            move_section_to_end(resource->contribution->group->section,&sections);
    }
    move_section_to_end(debug_section,&sections);
}
