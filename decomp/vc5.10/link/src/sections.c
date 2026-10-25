#include <windows.h>
#include <string.h>
#include <stdlib.h>
#include "sections.h"
#include "memory.h"
#include "full-link-functions.h"
#include <recon.h>

OutputSection *import_section,*import_thunk_section,*import_descriptor_section;
OutputSection *delay_import_section,*architecture_section,*ppc_loader_section;
ContributionGroup *export_group,*debug_symbol_group,*debug_type_group;
ContributionGroup *debug_precompiled_group,*debug_fixup_group;

extern BYTE incremental_enabled;
BYTE initialize_bss;
char *__stdcall duplicate_link_string(const char *);
MemoryArena *output_section_arena;
MemoryArena *contribution_group_arena;
WORD output_section_count;

#pragma intrinsic(strcmp,strcat)

OutputSection *__stdcall create_output_section(OutputModule *module,const char *name,
    DWORD flags,SectionList *sections,IMAGE_OPTIONAL_HEADER *header)
{
    OutputSection *section=find_matching_output_section(module,name,flags,sections,header);
    if (!section) {
        adjust_output_section_flags(&flags,name,header);
        if (incremental_enabled)
            section=(OutputSection *)allocate_link_memory(1,sizeof(OutputSection));
        else section=(OutputSection *)reserve_memory(&output_section_arena,sizeof(OutputSection));
        section->name=duplicate_link_string(name);
        section->characteristics=flags;
        section->original_characteristics=flags;
        section->groups=NULL;
        section->merge_target=NULL;
        *sections->last=section;
        sections->last=&section->next;
        ++output_section_count;
    }
    return section;
}
ContributionGroup *__stdcall create_contribution_group(const char *name,OutputSection *section)
{
    ContributionGroup *group=find_contribution_group(section,name);
    ContributionGroup *current,**position;
    if (group) return group;
    if (incremental_enabled)
        group=(ContributionGroup *)allocate_link_memory(1,sizeof(ContributionGroup));
    else group=(ContributionGroup *)reserve_memory(&contribution_group_arena,sizeof(ContributionGroup));
    group->name=duplicate_link_string(name);
    group->section=section;
    group->alignment=1;
    position=&section->groups;
    current=*position;
    while (current && strcmp(group->name,current->name)>=0) {
        position=&current->next;
        current=current->next;
    }
    *position=group;
    group->next=current;
    return group;
}

ContributionGroup *__stdcall find_contribution_group(OutputSection *section,const char *name)
{
    GroupEnumerator enumerator;
    ContributionGroup *group=NULL;
    initialize_group_enumerator(&enumerator,resolve_output_section(section));
    while (next_contribution_group(&enumerator)) {
        if (!strcmp(enumerator.current->name,name)) {
            group=enumerator.current;
            break;
        }
    }
    end_section_enumerator((SectionEnumerator *)&enumerator);
    return group;
}

void __stdcall initialize_group_enumerator(GroupEnumerator *enumerator,OutputSection *section)
{
    enumerator->next=next_contribution_group;
    enumerator->end=(void (__stdcall *)(GroupEnumerator *))end_section_enumerator;
    enumerator->current=NULL;
    enumerator->section=section;
}

BYTE __stdcall next_contribution_group(GroupEnumerator *enumerator)
{
    if (enumerator->current) enumerator->current=enumerator->current->next;
    else enumerator->current=enumerator->section->groups;
    return enumerator->current!=NULL;
}

OutputSection *__stdcall find_matching_output_section(OutputModule *module,const char *name,
    DWORD flags,SectionList *sections,IMAGE_OPTIONAL_HEADER *header)
{
    DWORD normalized=flags;
    SectionEnumerator enumerator;
    BYTE mismatched=0;
    char module_name[512];
    adjust_output_section_flags(&normalized,name,header);
    initialize_section_enumerator(&enumerator,sections);
    while (next_output_section(&enumerator)) {
        if (!strcmp(enumerator.current->name,name)) {
            if (normalized==enumerator.current->original_characteristics) break;
            mismatched=1;
        }
    }
    end_section_enumerator(&enumerator);
    if (mismatched && !enumerator.current)
        link_warning(module?format_module_name(module,module_name):NULL,4078,name,flags);
    return resolve_output_section(enumerator.current);
}
void __stdcall adjust_output_section_flags(DWORD *flags,const char *name,IMAGE_OPTIONAL_HEADER *header)
{
    DWORD value=*flags & 0xfe0feff7;
    if (!strcmp(name,".debug")) value|=0x02000000;
    if (initialize_bss && (value & 0x80)) {
        value&=~0x80;
        value|=0x40;
    }
    if (header->Subsystem==IMAGE_SUBSYSTEM_NATIVE && !strcmp(name,".rsrc"))
        value|=0x02000000;
    if (!(value & 0x1e0)) value|=0x40;
    if (!(value & 0xe0000000)) {
        if (value & 0x20) value|=0x60000000;
        else value|=0xc0000000;
    }
    *flags=value;
}

char *__stdcall format_module_name(OutputModule *module,char *name)
{
    char basename[256],extension[256];
    *name=0;
    if (!module) return name;
    if (!(module->library->flags & 16)) {
        _splitpath(module->library->name,NULL,NULL,basename,extension);
        strcat(name,basename);
        strcat(name,extension);
        strcat(name,"(");
    }
    _splitpath(module->name,NULL,NULL,basename,extension);
    strcat(name,basename);
    strcat(name,extension);
    if (!(module->library->flags & 16)) strcat(name,")");
    return name;
}
OutputSection *__stdcall resolve_output_section(OutputSection *section)
{
    OutputSection *resolved=section;
    if (!section) return NULL;
    while (resolved->merge_target) {
        resolved=resolved->merge_target;
        if (resolved==section) link_fatal(NULL,1145,section->name);
    }
    return resolved;
}

void __stdcall initialize_section_enumerator(SectionEnumerator *enumerator,SectionList *sections)
{
    enumerator->next=next_output_section;
    enumerator->end=end_section_enumerator;
    enumerator->current=NULL;
    enumerator->first=sections->first;
}

BYTE __stdcall next_output_section(SectionEnumerator *enumerator)
{
    if (enumerator->current) enumerator->current=enumerator->current->next;
    else enumerator->current=enumerator->first;
    return enumerator->current!=NULL;
}

void __stdcall end_section_enumerator(SectionEnumerator *enumerator)
{
}

extern MemoryArena *contribution_arena;
extern BYTE incremental_reuse;
void __stdcall split_section_name(const char *,const char **);
Contribution *__stdcall create_contribution(const char *name,DWORD bytes,DWORD flags,
    DWORD section_flags,OutputModule *module,SectionList *sections,LinkContext *context)
{
    const char *section_name;
    OutputSection *section=NULL;
    ContributionGroup *group;
    Contribution *contribution;
    unsigned allocation_size;
    split_section_name(name,&section_name);
    if (context->is_vxd==1) section=find_output_section(section_name,sections);
    if (!section) section=create_output_section(module,section_name,section_flags,
        sections,&context->optional_header);
    group=create_contribution_group(name,section);
    if (module->contribution_count<module->section_count)
        contribution=(Contribution *)module->contributions+module->contribution_count;
    else {
        allocation_size=sizeof(Contribution);
        if (context->options.effective_reference_opt || context->options.fold_identical_comdats)
            allocation_size+=40;
        if (incremental_enabled) contribution=(Contribution *)allocate_link_memory(1,allocation_size);
        else contribution=(Contribution *)reserve_memory(&contribution_arena,allocation_size);
    }
    ++module->contribution_count;
    ++group->contribution_count;
    contribution->associated_contribution=NULL;
    contribution->raw_size=bytes;
    contribution->group=group;
    contribution->characteristics=flags;
    contribution->module=module;
    if (!incremental_reuse) {
        if (group->last) group->last->next=contribution;
        else group->first=contribution;
        group->last=contribution;
    }
    return contribution;
}
/* The observed storage prefix is the 32 bytes copied here; no larger original
   capacity is inferred. Current section names terminate within this prefix. */
char section_base_name[32];
void __stdcall split_section_name(const char *name,const char **base)
{
    char *separator;
    strncpy(section_base_name,name,32);
    *base=section_base_name;
    separator=strchr(section_base_name,'$');
    if (separator) *separator=0;
}

ContributionInfo *__stdcall get_contribution_info(Contribution *contribution)
{
    OutputModule *module=contribution->module;
    DWORD address=(DWORD)contribution;
    DWORD base=(DWORD)module->contributions;
    WORD index;
    /* Preserve the original inclusive upper comparison, including its
     * one-past-array behavior; do not silently tighten this to >=. */
    if (address<base || address>base+module->section_count*64)
        return (ContributionInfo *)((BYTE *)contribution+64);
    index=(WORD)((LONG)(address-base)>>6);
    return module->contribution_info+index;
}
