#ifndef LINK_SECTIONS_H
#define LINK_SECTIONS_H
#include <windows.h>
#include "full-link.h"
#include <recon.h>
#ifdef __cplusplus
extern "C" {
#endif
extern OutputSection *bss_section,*small_data_section,*read_only_section,*data_section;
extern OutputSection *procedure_section,*exception_section,*export_section,*debug_section;
extern OutputSection *import_section,*import_thunk_section,*import_descriptor_section;
extern OutputSection *delay_import_section,*architecture_section,*ppc_loader_section;
extern ContributionGroup *export_group,*debug_symbol_group,*debug_type_group;
extern ContributionGroup *debug_precompiled_group,*debug_fixup_group;
OutputSection *__stdcall find_matching_output_section(OutputModule *,const char *,DWORD,SectionList *,IMAGE_OPTIONAL_HEADER *);
void __stdcall adjust_output_section_flags(DWORD *,const char *,IMAGE_OPTIONAL_HEADER *);
OutputSection *__stdcall create_output_section(OutputModule *,const char *,DWORD,SectionList *,IMAGE_OPTIONAL_HEADER *);
ContributionGroup *__stdcall create_contribution_group(const char *,OutputSection *);
#ifdef __cplusplus
}
#endif
#endif
