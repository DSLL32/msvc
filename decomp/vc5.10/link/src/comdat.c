#include <windows.h>
#include <string.h>
#include "memory.h"
#include "comdat.h"
#include "full-link-functions.h"
#include <recon.h>

/* Original494920 is a heap-block name pool, separate from the16-byte
 * VirtualAlloc arena headers used for relocation references at49492c. */
MemoryPool *comdat_names;
MemoryArena *comdat_reference_arena;
MemoryArena *comdat_overflow_arena;
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
    info->fold_signature=0;
    info->reserved18=0;
    if (last_comdat) info->previous_contribution=last_comdat;
    last_comdat=contribution;
}
void *__stdcall allocate_pool_memory(MemoryPool **,unsigned);
#pragma intrinsic(strlen,memcpy)
char *__stdcall duplicate_comdat_name(const char *name)
{
    char *copy=(char *)allocate_pool_memory(&comdat_names,strlen(name)+1);
    memcpy(copy,name,strlen(name)+1);
    return copy;
}
void *__stdcall add_comdat_reference(DWORD symbol_index,Contribution *parent,Contribution *child)
{
    ContributionInfo *info=get_contribution_info(parent);
    ComdatReference *reference;
    WORD index;
    while (info->references_used==info->reference_count) {
        if (!info->reference_count) {
            info->reference_count=8;
            info->references=reserve_memory(&comdat_reference_arena,96);
            break;
        }
        if (!info->reference_overflow)
            info->reference_overflow=(ContributionInfo *)reserve_memory(&comdat_overflow_arena,40);
        info=info->reference_overflow;
    }
    index=info->references_used++;
    reference=(ComdatReference *)info->references+index;
    if (child) {
        reference->contribution=child;
        reference->flags=0;
    } else {
        reference->flags=0;
        reference->symbol_index=symbol_index;
    }
    return reference;
}

extern StringNode **current_module_symbols;
void __stdcall record_module_comdats(OutputModule *module)
{
    ModuleContributionEnumerator contributions;
    ComdatReferenceEnumerator references;
    ComdatReference *reference;
    initialize_module_contribution_enumerator(&contributions,module);
    while (next_module_contribution(&contributions)) {
        if (contributions.current->characteristics & 0x800) continue;
        initialize_comdat_reference_enumerator(&references,
            get_contribution_info(contributions.current));
        while (next_comdat_reference(&references)) {
            reference=references.current;
            if (!reference->contribution)
                reference->symbol=current_module_symbols[reference->symbol_index];
        }
    }
}
void __stdcall initialize_module_contribution_enumerator(ModuleContributionEnumerator *enumerator,
    OutputModule *module)
{
    enumerator->next=next_module_contribution;
    enumerator->end=(void (__stdcall *)(void *))end_section_enumerator;
    enumerator->current=NULL;
    enumerator->module=module;
    enumerator->index=0;
}
BYTE __stdcall next_module_contribution(ModuleContributionEnumerator *enumerator)
{ __asm { int 3 } return 0; }
void __stdcall initialize_comdat_reference_enumerator(ComdatReferenceEnumerator *enumerator,
    ContributionInfo *block) { __asm { int 3 } }
BYTE __stdcall next_comdat_reference(ComdatReferenceEnumerator *enumerator)
{ __asm { int 3 } return 0; }
