#include <windows.h>
#include "memory.h"
#include "full-link-functions.h"
#include <recon.h>

/* Original494920 is a heap-block name pool, separate from the16-byte
 * VirtualAlloc arena headers used for relocation references at49492c. */
MemoryPool *comdat_names;
MemoryArena *comdat_reference_arena;
Contribution *last_comdat;
void __cdecl initialize_comdats(void)
{
    comdat_names=NULL;
}

extern OutputModule *output_module;
ContributionInfo *__stdcall get_contribution_info(Contribution *);
char *__stdcall duplicate_comdat_name(const char *);
void __stdcall attach_comdat(Contribution *contribution,const char *name,BYTE linker_defined)
{
    ContributionInfo *info=get_contribution_info(contribution);
    if (contribution->module!=output_module) {
        info->reference_count=contribution->relocation_count;
        if (info->reference_count)
            info->references=reserve_memory(&comdat_reference_arena,info->reference_count*12);
    }
    if (name) info->name=duplicate_comdat_name(name);
    if (linker_defined) contribution->characteristics |= 0x2000;
    info->fold_candidate=contribution;
    info->reserved24=0;
    info->reserved1c=0;
    info->reserved14=0;
    info->reserved18=0;
    if (last_comdat) info->previous_contribution=last_comdat;
    last_comdat=contribution;
}
char *__stdcall duplicate_comdat_name(const char *name) { __asm { int 3 } return NULL; }
