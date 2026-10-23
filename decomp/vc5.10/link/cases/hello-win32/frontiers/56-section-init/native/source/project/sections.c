#include <windows.h>
#include "sections.h"
#include <recon.h>

OutputSection *import_section,*import_thunk_section,*import_descriptor_section;
OutputSection *delay_import_section,*architecture_section,*ppc_loader_section;
ContributionGroup *export_group,*debug_symbol_group,*debug_type_group;
ContributionGroup *debug_precompiled_group,*debug_fixup_group;

OutputSection *__stdcall create_output_section(OutputModule *module,const char *name,
    DWORD flags,SectionList *sections,IMAGE_OPTIONAL_HEADER *header)
{
    __asm { int 3 }
    return NULL;
}
ContributionGroup *__stdcall create_contribution_group(const char *name,OutputSection *section)
{
    __asm { int 3 }
    return NULL;
}
