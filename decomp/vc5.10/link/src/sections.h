#ifndef LINK_SECTIONS_H
#define LINK_SECTIONS_H
#include <windows.h>
#include "full-link.h"
#include <recon.h>

typedef struct GroupEnumerator {
    BYTE (__stdcall *next)(struct GroupEnumerator *);
    void (__stdcall *end)(struct GroupEnumerator *);
    OutputSection *section;
    ContributionGroup *current;
} GroupEnumerator;
typedef char GroupEnumeratorSize[(sizeof(GroupEnumerator)==16)?1:-1];
typedef struct GroupContributionEnumerator {
    BYTE (__stdcall *next)(struct GroupContributionEnumerator *);
    void (__stdcall *end)(struct GroupContributionEnumerator *);
    ContributionGroup *group;
    Contribution *current;
} GroupContributionEnumerator;
typedef char GroupContributionEnumeratorSize[(sizeof(GroupContributionEnumerator)==16)?1:-1];

#ifdef __cplusplus
extern "C" {
#endif
void __stdcall initialize_group_enumerator(GroupEnumerator *,OutputSection *);
BYTE __stdcall next_contribution_group(GroupEnumerator *);

void __stdcall initialize_group_contributions(GroupContributionEnumerator *,ContributionGroup *);
BYTE __stdcall next_group_contribution(GroupContributionEnumerator *);
void __stdcall move_section_to_end(OutputSection *,SectionList *);
void __stdcall partition_sections(SectionList *,DWORD,DWORD);
extern OutputSection *bss_section,*small_data_section,*read_only_section,*data_section;
extern OutputSection *procedure_section,*exception_section,*export_section,*debug_section;
extern OutputSection *import_section,*import_thunk_section,*import_descriptor_section;
extern OutputSection *delay_import_section,*architecture_section,*ppc_loader_section;
extern ContributionGroup *export_group,*debug_symbol_group,*debug_type_group;
extern ContributionGroup *debug_precompiled_group,*debug_fixup_group;
char *__stdcall format_module_name(OutputModule *,char *);
OutputSection *__stdcall resolve_output_section(OutputSection *);
OutputSection *__stdcall find_matching_output_section(OutputModule *,const char *,DWORD,SectionList *,IMAGE_OPTIONAL_HEADER *);
void __stdcall adjust_output_section_flags(DWORD *,const char *,IMAGE_OPTIONAL_HEADER *);
OutputSection *__stdcall create_output_section(OutputModule *,const char *,DWORD,SectionList *,IMAGE_OPTIONAL_HEADER *);
ContributionGroup *__stdcall create_contribution_group(const char *,OutputSection *);
#ifdef __cplusplus
}
#endif
#endif
