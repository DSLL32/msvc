#include <windows.h>
#include "sections.h"
#include "memory.h"
#include <recon.h>

OutputSection *import_section,*import_thunk_section,*import_descriptor_section;
OutputSection *delay_import_section,*architecture_section,*ppc_loader_section;
ContributionGroup *export_group,*debug_symbol_group,*debug_type_group;
ContributionGroup *debug_precompiled_group,*debug_fixup_group;

extern BYTE incremental_enabled;
char *__stdcall duplicate_link_string(const char *);
MemoryArena *output_section_arena;
WORD output_section_count;

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
        section->reserved3c=0;
        *sections->last=section;
        sections->last=&section->next;
        ++output_section_count;
    }
    return section;
}
ContributionGroup *__stdcall create_contribution_group(const char *name,OutputSection *section)
{
    __asm { int 3 }
    return NULL;
}

OutputSection *__stdcall find_matching_output_section(OutputModule *module,const char *name,
    DWORD flags,SectionList *sections,IMAGE_OPTIONAL_HEADER *header)
{
    __asm { int 3 }
    return NULL;
}
void __stdcall adjust_output_section_flags(DWORD *flags,const char *name,IMAGE_OPTIONAL_HEADER *header)
{
    __asm { int 3 }
}
