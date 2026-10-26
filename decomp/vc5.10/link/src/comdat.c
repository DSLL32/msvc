#include <windows.h>
#include <stdio.h>
#include "../crt/crt-imports.h"
#include <string.h>
#include <mbstring.h>
#include "memory.h"
#include "comdat.h"
#include "sections.h"
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
    info->comparison_depth=0;
    info->fold_relocations=NULL;
    info->fold_signature=0;
    info->content_signature=0;
    if (last_comdat) info->previous_contribution=last_comdat;
    last_comdat=contribution;
}
void *__stdcall allocate_pool_memory(MemoryPool **,unsigned);
void __stdcall add_comdat_root(const char *name,StringNode *symbol,
    Contribution *owner,ComdatRoot **roots)
{
    ComdatRoot *root=(ComdatRoot *)allocate_pool_memory(&comdat_names,12);
    if (name) {
        root->name=name;
        root->kind=3;
    } else if (symbol) {
        root->symbol=symbol;
        root->kind=2;
    } else if (owner) {
        root->owner=owner;
        root->kind=1;
    }
    if (*roots) root->next=*roots;
    *roots=root;
}
#pragma intrinsic(strlen,memcpy,strcmp)
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
{
    if (enumerator->index<enumerator->module->section_count) {
        enumerator->current=(Contribution *)enumerator->module->contributions+enumerator->index;
        ++enumerator->index;
        return 1;
    }
    enumerator->current=NULL;
    return 0;
}
void __stdcall initialize_comdat_reference_enumerator(ComdatReferenceEnumerator *enumerator,
    ContributionInfo *block)
{
    enumerator->block=block;
    enumerator->next=next_comdat_reference;
    enumerator->end=(void (__stdcall *)(void *))end_section_enumerator;
    enumerator->current=NULL;
    enumerator->index=0;
}
BYTE __stdcall next_comdat_reference(ComdatReferenceEnumerator *enumerator)
{
    ContributionInfo *block=enumerator->block;
    enumerator->current=NULL;
    if (enumerator->index==block->references_used) {
        block=block->reference_overflow;
        if (!block) return 0;
        enumerator->block=block;
        enumerator->index=0;
    }
    block=enumerator->block;
    if (enumerator->index<block->references_used) {
        enumerator->current=(ComdatReference *)block->references+enumerator->index;
        ++enumerator->index;
    }
    return enumerator->current!=NULL;
}

/* Import address/name entries are marked before final RVAs are assigned. */
void __stdcall prepare_module_imports(OutputModule *module)
{
    ModuleContributionEnumerator enumerator;
    Contribution *contribution;
    const char *name;
    initialize_module_contribution_enumerator(&enumerator,module);
    while (next_module_contribution(&enumerator)) {
        contribution=enumerator.current;
        name=contribution->group->name;
        if (!strcmp(name,".idata$4") || !strcmp(name,".idata$5"))
            contribution->rva=1;
    }
    end_section_enumerator((SectionEnumerator *)&enumerator);
}

/* This boundary resolves reference symbols; its inherited name predates the
 * recovered traversal. The list is the same one populated by attach_comdat. */
typedef struct ComdatEnumerator {
    BYTE (__stdcall *next)(struct ComdatEnumerator *);
    void (__stdcall *end)(void *);
    Contribution *next_contribution;
    ContributionInfo *current;
} ComdatEnumerator;
typedef char ComdatEnumeratorSize[(sizeof(ComdatEnumerator)==16)?1:-1];
void __stdcall initialize_comdat_enumerator(ComdatEnumerator *,Contribution *);
BYTE __stdcall next_comdat(ComdatEnumerator *);
void __stdcall initialize_comdat_enumerator(ComdatEnumerator *enumerator,Contribution *first)
{
    enumerator->next=next_comdat;
    enumerator->end=(void (__stdcall *)(void *))end_section_enumerator;
    enumerator->next_contribution=first;
    enumerator->current=NULL;
}
BYTE __stdcall next_comdat(ComdatEnumerator *enumerator)
{
    if(enumerator->current)
        enumerator->next_contribution=enumerator->current->previous_contribution;
    if(!enumerator->next_contribution)return 0;
    enumerator->current=get_contribution_info(enumerator->next_contribution);
    return 1;
}
StringNode *__stdcall find_import_symbol(StringNode *,StringTable *,BYTE);
StringNode *__stdcall find_weak_default(StringNode *);
StringNode *__stdcall find_import_symbol(StringNode *symbol,StringTable *table,BYTE prefix)
{ __asm { int 3 } return NULL; }
StringNode *__stdcall find_weak_default(StringNode *symbol)
{ __asm { int 3 } return NULL; }
void __stdcall redirect_comdat_reference(ComdatReference *,Contribution *);
extern BYTE mac_target;
void __stdcall add_special_comdat_roots(StringTable *symbols)
{
    ComdatEnumerator comdats;
    ComdatReferenceEnumerator references;
    ComdatReference *reference;
    StringNode *symbol,*import_symbol;
    initialize_comdat_enumerator(&comdats,last_comdat);
    while(next_comdat(&comdats)) {
        initialize_comdat_reference_enumerator(&references,comdats.current);
        while(next_comdat_reference(&references)) {
            reference=references.current;
            if(!reference->contribution) {
                symbol=reference->symbol;
                if(!(symbol->flags & 1))reference->contribution=NULL;
                else {
                    reference->contribution=symbol->contribution;
                    if(mac_target && reference->flag_byte) {
                        if(symbol->symbol.Type & 0x8000) {
                            import_symbol=find_import_symbol(symbol,symbols,0);
                            if(!import_symbol)import_symbol=find_import_symbol(
                                find_weak_default(symbol),symbols,0);
                            redirect_comdat_reference(reference,import_symbol->contribution);
                        } else reference->flag_byte=0;
                    }
                }
            }
        }
    }
}

void __stdcall initialize_comdat_root_enumerator(ComdatRootEnumerator *,ComdatRoot *);
BYTE __stdcall next_comdat_root(ComdatRootEnumerator *);
void __stdcall initialize_comdat_root_enumerator(ComdatRootEnumerator *enumerator,ComdatRoot *first)
{
    enumerator->next=next_comdat_root;
    enumerator->end=(void (__stdcall *)(void *))end_section_enumerator;
    enumerator->first=first;
    enumerator->current=NULL;
}
BYTE __stdcall next_comdat_root(ComdatRootEnumerator *enumerator)
{
    if(enumerator->current)enumerator->current=enumerator->current->next;
    else enumerator->current=enumerator->first;
    return enumerator->current!=NULL;
}
void __stdcall mark_live_comdat(StringTable *symbols,Contribution *contribution)
{
    ComdatReferenceEnumerator references;
    ComdatReference *reference;
    if(contribution->characteristics & 0x2000)return;
    contribution->characteristics |= 0x2000;
    initialize_comdat_reference_enumerator(&references,get_contribution_info(contribution));
    while(next_comdat_reference(&references)) {
        reference=references.current;
        if(reference->contribution)mark_live_comdat(symbols,reference->contribution);
        if(mac_target && reference->redirected)mark_live_comdat(symbols,reference->redirected);
    }
}

/* Retained between import-library groups, as in the original local static. */
static BYTE import_modules_initialized;
static OutputLibrary **import_libraries;
static int import_library_capacity=5;
BYTE __stdcall is_discardable_comdat(Contribution *contribution)
{
    return !(contribution->characteristics & 0x2000);
}
void *__stdcall reallocate_memory(void *,unsigned);
void __stdcall append_import_module(OutputLibrary *library,OutputLibrary ***libraries,int *capacity,int *count)
{
    int current;
    for(current=0;current<*count;++current)
        if((*libraries)[current]==library)return;
    if(*count>*capacity-1) {
        *capacity*=2;
        *libraries=(OutputLibrary **)reallocate_memory(*libraries,*capacity*4);
    }
    (*libraries)[*count]=library;
    ++*count;
}
void __stdcall reset_import_modules(OutputLibrary **libraries,int *count)
{
    int index;
    for(index=0;index<*count;++index)libraries[index]=NULL;
    *count=0;
}
void __stdcall discard_module_imports(OutputLibrary *library,OutputSection *section,const char *name)
{ __asm { int 3 } }
void __cdecl finish_comdat_elimination(void)
{
    ContributionGroup *group;
    Contribution *current,*previous;
    OutputLibrary *library;
    int count,live,index;
    if(!import_section)return;
    group=find_contribution_group(import_section,".idata$4");
    if(!group)return;
    if(!(import_modules_initialized & 1)) {
        import_modules_initialized |= 1;
        import_libraries=(OutputLibrary **)allocate_memory(import_library_capacity*4);
    }
    count=0;
    previous=current=group->first;
    /* The original dereferences the first module before its empty-list test. */
    library=current->module->library;
    if(!current)return;
    while(current->next) {
        live=0;
        while(current && !_mbscmp((const unsigned char *)current->module->name,
                                (const unsigned char *)previous->module->name)) {
            if(!is_discardable_comdat(current))++live;
            append_import_module(library,&import_libraries,
                &import_library_capacity,&count);
            previous=current;
            current=current->next;
            library=current ? current->module->library : NULL;
        }
        if(live==1) {
            link_warning(NULL,4089,previous->module->name);
            previous->characteristics |= 0x800;
            for(index=0;index<count;++index) {
                discard_module_imports(import_libraries[index],import_descriptor_section,".idata$2");
                discard_module_imports(import_libraries[index],import_thunk_section,".idata$5");
            }
        }
        reset_import_modules(import_libraries,&count);
        previous=current;
        if(!current)break;
    }
}

void __cdecl release_comdat_tables(void)
{
    release_memory_pool(&comdat_names);
    release_memory_arena(comdat_overflow_arena);
    release_memory_arena(comdat_reference_arena);
}

extern OutputSection *debug_section;
extern void __cdecl link_message(unsigned,...);
char *__stdcall display_symbol_name(const char *,DWORD);
char *__stdcall format_contribution_module(Contribution *,char *);
void __stdcall print_discarded_comdat(Contribution *contribution,ContributionInfo *info)
{
    const char *name;
    char *display;
    unsigned message;
    char module_name[520];
    if(!info)info=get_contribution_info(contribution);
    name=info->name;
    if(name) {
        message=9018;
        display=display_symbol_name(name,1);
    } else if(contribution->group->section!=debug_section) {
        name=contribution->group->name;
        display=(char *)name;
        message=9020;
    } else display=NULL;
    if(display) {
        fputs("    ",stdout);
        if(contribution->module!=output_module)
            link_message(message,display,format_contribution_module(contribution,module_name));
        else link_message(9019,name);
        if(display!=name)release_memory(display);
    }
}
void __cdecl print_removed_comdats(void)
{
    ComdatEnumerator comdats;
    Contribution *contribution;
    ContributionInfo *info;
    initialize_comdat_enumerator(&comdats,last_comdat);
    while(next_comdat(&comdats)) {
        contribution=comdats.next_contribution;
        info=comdats.current;
        if(is_discardable_comdat(contribution))print_discarded_comdat(contribution,info);
    }
}
