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
        section->merge_target=NULL;
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

#pragma intrinsic(strcmp)
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
    __asm { int 3 }
}

char *__stdcall format_module_name(OutputModule *module,char *name)
{
    __asm { int 3 }
    return NULL;
}
OutputSection *__stdcall resolve_output_section(OutputSection *section)
{
    __asm { int 3 }
    return NULL;
}
