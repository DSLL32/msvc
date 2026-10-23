#include <windows.h>
#include <string.h>
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

#pragma intrinsic(strcmp)

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
    __asm { int 3 }
    return NULL;
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
