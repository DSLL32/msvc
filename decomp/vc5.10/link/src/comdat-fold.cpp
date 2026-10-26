/* Identical COMDAT candidates are grouped by size and relocation count. */
#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include "link-driver.h"
#include "input.h"
#include "comdat.h"
#include "sections.h"
#include "memory.h"
#include "initializers.h"
#include "files.h"
#include "full-link-functions.h"
#include <recon.h>

extern "C" {
extern DWORD icf_iterations;
extern OutputModule *output_module;
MemoryPool *fold_changes;
int __cdecl compare_fold_candidates(const void *left,const void *right)
{
    const ContributionInfo *first=*(const ContributionInfo *const *)left;
    const ContributionInfo *second=*(const ContributionInfo *const *)right;
    Contribution *a,*b;
    DWORD first_size,second_size,first_relocations,second_relocations;
    if(!first)return second!=NULL;
    if(!second)return -1;
    a=first->fold_candidate;
    b=second->fold_candidate;
    if(!a)return b!=NULL;
    if(!b)return -1;
    first_size=a->raw_size-a->padding;
    second_size=b->raw_size-b->padding;
    first_relocations=a->module==output_module ? 0 : a->relocation_count;
    second_relocations=b->module==output_module ? 0 : b->relocation_count;
    if(first_size==second_size)return (LONG)(first_relocations-second_relocations);
    return (LONG)(first_size-second_size);
}
BYTE __stdcall compare_and_fold_comdats(ContributionInfo *,ContributionInfo *,LinkContext *,DWORD);
void __stdcall fold_comdat_range(DWORD first,DWORD limit,LinkContext *context,BYTE *changed)
{
    ContributionInfo **candidates=(ContributionInfo **)buffer4.base;
    ContributionInfo *info,*other;
    Contribution *contribution,*candidate;
    BYTE reference_opt=context->options.effective_reference_opt;
    DWORD current,next;
    for(current=first;current<limit;++current) {
        info=candidates[current];
        contribution=info->fold_candidate;
        if(!contribution || (contribution->characteristics & 0x4000) ||
            (reference_opt && !(contribution->characteristics & 0x2000)))continue;
        for(next=current+1;next<limit;++next) {
            other=candidates[next];
            candidate=other->fold_candidate;
            if(!candidate || (candidate->characteristics & 0x4000) ||
                contribution==candidate ||
                (reference_opt && !(candidate->characteristics & 0x2000)))continue;
            if(info->fold_signature!=other->fold_signature)continue;
            if(compare_and_fold_comdats(candidates[current],other,context,1)) {
                candidate->characteristics |= 0x4000;
                *changed=1;
                candidate->associated_contribution=contribution;
            }
            candidates=(ContributionInfo **)buffer4.base;
        }
    }
}
static BYTE *fold_first_bytes,*fold_second_bytes;
static LONG fold_buffer_size=1024;
extern BYTE file_access_statistics;
__declspec(dllimport) DWORD __cdecl SigForPbCb(const void *,long,DWORD);
void *__stdcall reallocate_memory(void *,unsigned);
ContributionInfo *__stdcall get_contribution_info(Contribution *);
BYTE __stdcall compare_special_fold_references(ContributionInfo *first,ContributionInfo *second,
    LinkContext *context,DWORD depth)
{
    ComdatReferenceEnumerator first_refs,second_refs;
    Contribution *a,*b;
    ContributionInfo *first_info,*second_info;
    BYTE equal=1;
    if(first->reference_count!=second->reference_count)return 0;
    initialize_comdat_reference_enumerator(&first_refs,first);
    initialize_comdat_reference_enumerator(&second_refs,second);
    while(next_comdat_reference(&first_refs) && next_comdat_reference(&second_refs)) {
        a=first_refs.current->contribution;
        b=second_refs.current->contribution;
        if(!a || !b) { equal=0; break; }
        if(a->group->section==b->group->section && a->group->section==debug_section)
            equal=1;
        else {
            while(b && (b->characteristics & 0x4000))b=b->associated_contribution;
            while(a && (a->characteristics & 0x4000))a=a->associated_contribution;
            second_info=get_contribution_info(b);
            first_info=get_contribution_info(a);
            equal=compare_and_fold_comdats(first_info,second_info,context,depth);
        }
        ++depth;
        if(!equal)break;
    }
    end_section_enumerator((SectionEnumerator *)&first_refs);
    end_section_enumerator((SectionEnumerator *)&second_refs);
    return equal;
}
void *__stdcall allocate_pool_memory(MemoryPool **,unsigned);
IMAGE_SYMBOL *__stdcall read_module_symbols(DWORD,DWORD,BYTE);
void __stdcall release_module_symbols(IMAGE_SYMBOL *);
char *__stdcall read_module_string_table(const char *,DWORD,DWORD *);
void __stdcall release_module_strings(char *);
void __stdcall release_relocations(IMAGE_RELOCATION *);
#pragma intrinsic(memcpy)
void __stdcall read_fold_inputs(IMAGE_SYMBOL **symbols,const char **strings,
    IMAGE_RELOCATION **relocations,DWORD *count,ContributionInfo *info)
{
    Contribution *contribution=info->fold_candidate;
    OutputModule *module=contribution->module;
    ModuleIndexRecord *cache=(ModuleIndexRecord *)buffer8.base+module->module_index;
    IMAGE_SYMBOL *input_symbols;
    char *input_strings;
    IMAGE_RELOCATION *input_relocations;
    DWORD offset;
    /* At a zero table offset the original leaves this size uninitialized.
     * Use zero for that defect, as in the ordinary object loader. */
    DWORD strings_size=0;
    const char *name;
    if(!cache->symbols && !cache->strings) {
        if(!(module->library->flags & 16))offset=(module->flags & 0x40) ?
            *(DWORD *)((BYTE *)module->optional_data+12) : (DWORD)module->identity;
        else offset=0;
        input_symbols=read_module_symbols(offset+module->symbol_offset,module->symbol_count,0);
        cache->symbols=(IMAGE_SYMBOL *)allocate_pool_memory(&fold_changes,module->symbol_count*18);
        memcpy(cache->symbols,input_symbols,module->symbol_count*18);
        release_module_symbols(input_symbols);
        if(!(module->library->flags & 16))offset=(module->flags & 0x40) ?
            *(DWORD *)((BYTE *)module->optional_data+12) : (DWORD)module->identity;
        else offset=0;
        name=(module->library->flags & 16) ? (const char *)module->identity : module->library->name;
        input_strings=read_module_string_table(name,
            offset+module->symbol_count*18+module->symbol_offset,&strings_size);
        cache->strings=(const char *)allocate_pool_memory(&fold_changes,strings_size);
        memcpy((void *)cache->strings,input_strings,strings_size);
        release_module_strings(input_strings);
    }
    if(!info->fold_relocations) {
        input_relocations=contribution->ReadRelocations(count);
        info->fold_relocations=(IMAGE_RELOCATION *)allocate_pool_memory(&fold_changes,*count*10);
        memcpy(info->fold_relocations,input_relocations,*count*10);
        release_relocations(input_relocations);
    }
    *relocations=info->fold_relocations;
    *symbols=cache->symbols;
    *strings=cache->strings;
}
BYTE __stdcall compare_absolute_fold_relocation(LinkContext *context,
    IMAGE_RELOCATION *first,IMAGE_RELOCATION *second)
{ __asm { int 3 } return 0; }
extern char short_symbol_name[9];
BYTE __stdcall compare_fold_symbols(IMAGE_SYMBOL *first,const char *first_strings,
    IMAGE_SYMBOL *second,const char *second_strings,LinkContext *context)
{
    WORD kind;
    const char *name;
    char *first_name,*second_name;
    StringNode *first_symbol,*second_symbol;
    Contribution *a,*b;
    if((first->StorageClass!=2 && first->StorageClass!=105) ||
        (second->StorageClass!=2 && second->StorageClass!=105) ||
        first->StorageClass!=second->StorageClass)return 0;
    name=get_symbol_name((StringNode *)first,first_strings,&kind,NULL);
    if(kind==1)name=strncpy(short_symbol_name,name,8);
    if(!name)return 0;
    first_name=duplicate_argument(name);
    name=get_symbol_name((StringNode *)second,second_strings,&kind,NULL);
    if(kind==1)name=strncpy(short_symbol_name,name,8);
    if(!name)return 0;
    second_name=duplicate_argument(name);
    if(!first_name || !second_name)return 0;
    first_symbol=context->symbols->FindSymbol(first_name);
    second_symbol=context->symbols->FindSymbol(second_name);
    free(first_name);
    free(second_name);
    a=first_symbol->contribution;
    b=second_symbol->contribution;
    while(a && (a->characteristics & 0x4000))a=a->associated_contribution;
    while(b && (b->characteristics & 0x4000))b=b->associated_contribution;
    return a==b && first_symbol->symbol.Value==second_symbol->symbol.Value;
}
BYTE __stdcall compare_fold_section_symbols(Contribution *first,IMAGE_SYMBOL *first_symbol,
    Contribution *second,IMAGE_SYMBOL *second_symbol)
{
    if(first_symbol->StorageClass!=3 || second_symbol->StorageClass!=3)return 0;
    if(((LONG)((DWORD)first-(DWORD)first->module->contributions)>>6)+1!=
        first_symbol->SectionNumber)return 0;
    if(((LONG)((DWORD)second-(DWORD)second->module->contributions)>>6)+1!=
        second_symbol->SectionNumber)return 0;
    return first_symbol->Value==second_symbol->Value;
}
BYTE __stdcall compare_fold_references(ContributionInfo *first,ContributionInfo *second,
    LinkContext *context,DWORD depth)
{
    Contribution *a=first->fold_candidate,*b=second->fold_candidate;
    DWORD first_count,second_count,index;
    IMAGE_RELOCATION *first_relocations=NULL,*second_relocations=NULL;
    IMAGE_SYMBOL *first_symbols,*second_symbols,*first_symbol,*second_symbol;
    const char *first_strings,*second_strings;
    int first_file,second_file,saved_file;
    BYTE equal,vxd;
    while(a && (a->characteristics & 0x4000))a=a->associated_contribution;
    while(b && (b->characteristics & 0x4000))b=b->associated_contribution;
    /* The original uses the resulting owners without adding NULL guards. */
    first_count=a->module==output_module ? 0 : a->relocation_count;
    second_count=b->module==output_module ? 0 : b->relocation_count;
    if(first_count!=second_count)return 0;
    if(!first_count)return 1;
    equal=compare_special_fold_references(first,second,context,depth);
    if(!equal)return equal;
    first_file=open_module_file(a->module,0x8000,0);
    second_file=open_module_file(b->module,0x8000,0);
    saved_file=current_input_file;
    current_input_file=first_file;
    read_fold_inputs(&first_symbols,&first_strings,&first_relocations,&first_count,first);
    current_input_file=second_file;
    read_fold_inputs(&second_symbols,&second_strings,&second_relocations,&second_count,second);
    current_input_file=saved_file;
    vxd=context->is_vxd==1;
    for(index=0;index<first_count;++index) {
        IMAGE_RELOCATION *left=first_relocations+index,*right=second_relocations+index;
        if(left->Type!=right->Type)equal=0;
        /* Only the classifier's actual WORD type and BYTE VxD formals consume
         * these slots; upper physical DWORD transport bits are not semantics. */
        else if(classify_relocation(a,left->Type,0,vxd))
            equal=compare_absolute_fold_relocation(context,left,right);
        else {
            first_symbol=first_symbols+left->SymbolTableIndex;
            second_symbol=second_symbols+right->SymbolTableIndex;
            equal=left->VirtualAddress==right->VirtualAddress &&
                (compare_fold_symbols(first_symbol,first_strings,
                    second_symbol,second_strings,context) ||
                compare_fold_section_symbols(a,first_symbol,b,second_symbol));
        }
        if(!equal)break;
    }
    close_argument_file(first_file,0);
    close_argument_file(second_file,0);
    return equal;
}
#pragma intrinsic(memcmp)
BYTE __stdcall compare_and_fold_comdats(ContributionInfo *first,ContributionInfo *second,
    LinkContext *context,DWORD depth)
{
    Contribution *a=first->fold_candidate,*b=second->fold_candidate;
    DWORD first_signature,second_signature,first_depth,second_depth,error,offset;
    LONG bytes;
    int file;
    FileRecord *record;
    OutputModule *module;
    BYTE compatible;
    while(a && (a->characteristics & 0x4000))a=a->associated_contribution;
    while(b && (b->characteristics & 0x4000))b=b->associated_contribution;
    first_signature=first->content_signature;
    second_signature=second->content_signature;
    if(first_signature && second_signature && first_signature!=second_signature)return 0;
    first_depth=first->comparison_depth;
    second_depth=second->comparison_depth;
    if((first_depth || second_depth) && first_depth!=second_depth)return 0;
    if(!a || !b)return 0;
    if(a==b)return 1;
    if(a->module==output_module || b->module==output_module)return 0;
    if((a->module->flags & 0x40) || (b->module->flags & 0x40))return 0;
    if(first_depth && first_depth==second_depth)compatible=1;
    else {
        first->comparison_depth=second->comparison_depth=depth;
        compatible=a->group==b->group && a->characteristics==b->characteristics &&
            a->comdat_selection==b->comdat_selection &&
            compare_fold_references(first,second,context,depth+1);
        second->comparison_depth=first->comparison_depth=0;
    }
    if(!compatible)return 0;
    bytes=(LONG)(a->raw_size-a->padding);
    if(!fold_first_bytes) {
        if(fold_buffer_size<=bytes)fold_buffer_size=bytes;
        fold_first_bytes=(BYTE *)allocate_memory(fold_buffer_size);
        fold_second_bytes=(BYTE *)allocate_memory(fold_buffer_size);
    }
    if(fold_buffer_size<bytes) {
        fold_buffer_size=bytes;
        fold_first_bytes=(BYTE *)reallocate_memory(fold_first_bytes,fold_buffer_size);
        fold_second_bytes=(BYTE *)reallocate_memory(fold_second_bytes,fold_buffer_size);
    }
    module=a->module;
    file=open_argument_file((module->library->flags & 16) ?
        module->name : module->library->name,0x8000,0);
    module=a->module;
    if(!(module->library->flags & 16))offset=(module->flags & 0x40) ?
        *(DWORD *)((BYTE *)module->optional_data+12) : (DWORD)module->identity;
    else offset=0;
    error=0;
    seek_file(file,a->input_raw_offset+offset,0,&error);
    if(file_access_statistics)record_file_access();
    record=file_slots[file];
    if(record->flags & 8)mapped_read(record,fold_first_bytes,bytes);
    else buffered_read(record,fold_first_bytes,bytes);
    close_argument_file(file,0);
    if(!first_signature) {
        first_signature=SigForPbCb(fold_first_bytes,bytes,0);
        first->content_signature=first_signature;
    }
    module=b->module;
    file=open_argument_file((module->library->flags & 16) ?
        module->name : module->library->name,0x8000,0);
    module=b->module;
    if(!(module->library->flags & 16))offset=(module->flags & 0x40) ?
        *(DWORD *)((BYTE *)module->optional_data+12) : (DWORD)module->identity;
    else offset=0;
    error=0;
    seek_file(file,b->input_raw_offset+offset,0,&error);
    if(file_access_statistics)record_file_access();
    record=file_slots[file];
    if(record->flags & 8)mapped_read(record,fold_second_bytes,bytes);
    else buffered_read(record,fold_second_bytes,bytes);
    close_argument_file(file,0);
    if(!second_signature) {
        second_signature=SigForPbCb(fold_second_bytes,bytes,0);
        second->content_signature=second_signature;
    }
    if(first_signature!=second_signature)return 0;
    return memcmp(fold_first_bytes,fold_second_bytes,(DWORD)bytes)==0;
}
}
void RECON_THISCALL LinkContext::FoldIdenticalComdats(void)
{
    DWORD iteration,first,limit,count,size,relocations;
    ContributionInfo **candidates;
    Contribution *contribution;
    BYTE changed=1;
    if(!options.fold_identical_comdats || !buffer4.remaining)return;
    qsort(buffer4.base,buffer4.remaining,4,compare_fold_candidates);
    fold_changes=NULL;
    count=buffer4.remaining;
    for(iteration=0;iteration<icf_iterations && changed;++iteration) {
        changed=0;
        first=0;
        while(first<count) {
            candidates=(ContributionInfo **)buffer4.base;
            if(!candidates[first] || !candidates[first]->fold_candidate)break;
            contribution=candidates[first]->fold_candidate;
            size=contribution->raw_size-contribution->padding;
            relocations=contribution->module==output_module ? 0 : contribution->relocation_count;
            for(limit=first+1;limit<count;++limit) {
                if(!candidates[limit] || !candidates[limit]->fold_candidate)break;
                contribution=candidates[limit]->fold_candidate;
                if(contribution->raw_size-contribution->padding!=size ||
                    (contribution->module==output_module ? 0 : contribution->relocation_count)!=relocations)break;
            }
            if(limit-first>1) {
                fold_comdat_range(first,limit,this,&changed);
                count=buffer4.remaining;
            }
            first=limit;
        }
    }
    release_memory_pool(&fold_changes);
}
