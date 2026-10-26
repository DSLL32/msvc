/* Import subsection ordering and incremental padding. */
#include <windows.h>
#include <string.h>
#include <stdlib.h>
#include "sections.h"
#include "full-link-functions.h"
#include "memory.h"
#include <recon.h>

#pragma intrinsic(strcmp)

typedef struct ImportSubsections {
    OutputSection *section;
    const char *prefix,*padding_group,*ordered[3];
} ImportSubsections;
extern BYTE incremental_enabled;
BYTE __stdcall next_group_contribution(GroupContributionEnumerator *);
void __stdcall initialize_group_contributions(GroupContributionEnumerator *enumerator,
                                             ContributionGroup *group)
{
    enumerator->next=next_group_contribution;
    enumerator->end=(void (__stdcall *)(GroupContributionEnumerator *))end_section_enumerator;
    enumerator->current=NULL;
    enumerator->group=group;
}
BYTE __stdcall next_group_contribution(GroupContributionEnumerator *enumerator)
{
    if(!enumerator->current)enumerator->current=enumerator->group->first;
    else enumerator->current=enumerator->current->next;
    return enumerator->current!=NULL;
}
int __cdecl compare_import_modules(const void *left,const void *right)
{
    const Contribution *a=*(const Contribution *const *)left;
    const Contribution *b=*(const Contribution *const *)right;
    return strcmp(a->module->name,b->module->name);
}
void __stdcall sort_import_group(ContributionGroup *group)
{
    GroupContributionEnumerator enumerator;
    Contribution **array,**next;
    unsigned index;
    array=(Contribution **)allocate_memory(group->contribution_count*4);
    initialize_group_contributions(&enumerator,group);
    next=array;
    while(next_group_contribution(&enumerator))*next++=enumerator.current;
    qsort(array,group->contribution_count,4,compare_import_modules);
    for(index=0;index<group->contribution_count-1;++index)
        array[index]->next=array[index+1];
    group->first=array[0];
    array[group->contribution_count-1]->next=NULL;
    release_memory(array);
}
/* Put this contribution after the contiguous run from the same module. */
BYTE __stdcall sort_import_tail(Contribution *contribution)
{
    Contribution **previous=&contribution->group->first;
    Contribution *current=contribution,*last=contribution;
    while(*previous!=contribution)previous=&(*previous)->next;
    while(current) {
        if(strcmp(current->module->name,last->module->name))break;
        last=current;
        current=current->next;
    }
    if(last==contribution)return 0;
    *previous=contribution->next;
    contribution->next=last->next;
    last->next=contribution;
    return 1;
}
void __stdcall pad_import_contribution(Contribution *contribution,Contribution *first)
{ __asm { int 3 } }

void __cdecl prepare_import_sections(void)
{
    ImportSubsections rules[2],*rule;
    GroupEnumerator groups;
    GroupContributionEnumerator contributions;
    ContributionGroup *group;
    Contribution *contribution,*first;
    /* The original loads an unwritten local before the first iteration. If
     * an empty incremental padding group is encountered before any producer,
     * its last-contribution pointer is undefined. Keep those bytes zero. */
    Contribution *last=NULL;
    DWORD total,padding;
    unsigned index;
    group=find_contribution_group(import_descriptor_section,".idata$2");
    if(group) {
        group->alignment=4;
        group=find_contribution_group(import_descriptor_section,".idata$3");
        if(group)group->alignment=4;
    }
    rules[0].section=import_section;
    rules[0].prefix=".idata$";
    rules[0].padding_group=".idata$6";
    rules[0].ordered[0]=".idata$4";
    rules[0].ordered[1]=".idata$5";
    rules[0].ordered[2]="";
    rules[1].section=delay_import_section;
    rules[1].prefix=".didat$";
    rules[1].padding_group=".didat$6";
    rules[1].ordered[0]=".didat$4";
    rules[1].ordered[1]=".didat$5";
    rules[1].ordered[2]=".didat$8";
    for(index=0;index<2;++index) {
        rule=&rules[index];
        if(!rule->section)continue;
        initialize_group_enumerator(&groups,rule->section);
        while(next_contribution_group(&groups)) {
            group=groups.current;
            if(strncmp(group->name,rule->prefix,7))continue;
            if(!strcmp(group->name,rule->ordered[0]) ||
               !strcmp(group->name,rule->ordered[1]) ||
               !strcmp(group->name,rule->ordered[2])) {
                sort_import_group(group);
                first=NULL;
                initialize_group_contributions(&contributions,group);
                while(next_group_contribution(&contributions)) {
                    contribution=contributions.current;
                    last=contribution;
                    if(!first)first=contribution;
                    if(contribution->rva) {
                        if(first==contribution && incremental_enabled)
                            first=contribution->next;
                        sort_import_tail(contribution);
                        if(incremental_enabled)pad_import_contribution(contribution,first);
                        first=contribution->next;
                        contribution->rva=0;
                    }
                }
            } else if(incremental_enabled && !strcmp(group->name,rule->padding_group)) {
                total=0;
                initialize_group_contributions(&contributions,group);
                while(next_group_contribution(&contributions)) {
                    last=contributions.current;
                    total+=last->raw_size;
                }
                padding=(total*10)/100+256;
                last->padding=padding;
                last->raw_size+=padding;
            }
        }
        end_section_enumerator((SectionEnumerator *)&groups);
    }
}

extern InputArguments delay_libraries;
void __cdecl prepare_delay_imports(void)
{
    if(delay_libraries.count) {
        move_named_group(".didat$5",delay_import_section,data_section);
        merge_output_section(delay_import_section,read_only_section);
        delay_import_section=read_only_section;
    }
}
