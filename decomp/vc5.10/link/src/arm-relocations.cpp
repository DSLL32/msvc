/* ARM COFF resource/code relocation records remain output data on the host. */
#include <windows.h>
#include "input.h"
#include "initializers.h"
#include "comdat.h"
#include "sections.h"
#include "full-link-functions.h"
#include "arm-relocations.h"
extern "C" __declspec(dllimport) DWORD __cdecl SigForPbCb(const void *,long,DWORD);
#include <recon.h>

extern "C" {
ContributionInfo *__stdcall get_contribution_info(Contribution *);
void __stdcall record_relocation_reference(LinkContext *,Contribution *,IMAGE_SYMBOL *,IMAGE_RELOCATION *);
BYTE __stdcall classify_arm_relocation(Contribution *,WORD,DWORD,BYTE);
}
extern "C" DWORD __stdcall read_arm_relocations(LinkContext *context,Contribution *contribution,
    IMAGE_SYMBOL *symbols,IMAGE_RELOCATION *relocations,DWORD count,DWORD index)
{
    ContributionInfo *info=get_contribution_info(contribution);
    IMAGE_RELOCATION *relocation=relocations;
    DWORD checksum=0,total=0,i;
    BYTE folding=context->options.fold_identical_comdats && buffer4.remaining &&
        ((ContributionInfo **)buffer4.base)[buffer4.remaining-1]==info;
    for(i=0;i<count;++i,++relocation) {
        if(classify_arm_relocation(contribution,relocation->Type,0,0))continue;
        if(context->options.effective_reference_opt || context->options.fold_identical_comdats)
            record_relocation_reference(context,contribution,symbols,relocation);
        if(folding) {
            checksum=SigForPbCb(&relocation->Type,2,checksum);
            checksum=SigForPbCb(&relocation->VirtualAddress,4,checksum);
            checksum=SigForPbCb(&symbols[relocation->SymbolTableIndex].StorageClass,1,checksum);
        }
        if(!context->options.fixed_image && relocation->Type==ARM_REL_ADDR32)++total;
    }
    if(folding) {
        checksum=SigForPbCb(&contribution->characteristics,4,checksum);
        checksum=SigForPbCb(&contribution->comdat_selection,1,checksum);
        ((ContributionInfo **)buffer4.base)[buffer4.remaining-1]->fold_signature=checksum;
    }
    return total;
}

extern "C" {
extern BYTE *next_base_relocation;
OutputSection *__stdcall find_section_by_index(SHORT,SectionList *);
void __stdcall record_debug_fixup(WORD,WORD,DWORD,DWORD);
}
#define APPEND_BASE_RELOCATION(kind,site,target,addend) do { \
    LinkBaseRelocation *record=(LinkBaseRelocation *)next_base_relocation; \
    record->type=(kind);record->address=(site);record->section=(target); \
    record->value=(addend);next_base_relocation+=12; \
} while(0)
extern "C" void __stdcall process_arm_relocations(Contribution *contribution,
    IMAGE_RELOCATION *relocations,DWORD count,BYTE *contents,IMAGE_SYMBOL *symbols,
    LinkContext *context,DWORD *symbol_indices)
{
    BYTE fixed=context->options.fixed_image;
    BYTE save_fixups=(context->options.debug_formats & 8) &&
        contribution->group->section!=debug_section;
    BYTE small_data=contribution->group->section==small_data_section;
    DWORD contribution_rva=contribution->rva;
    while(count--) {
        IMAGE_RELOCATION *relocation=relocations++;
        IMAGE_SYMBOL *symbol;
        OutputSection *target;
        DWORD input_rva,site,value,absolute_value,word,offset;
        DWORD *fixup;
        SHORT section;
        BYTE absolute;
        if(classify_arm_relocation(contribution,relocation->Type,1,0))continue;
        input_rva=contribution->module->section_map[
            ((LONG)((DWORD)contribution-(DWORD)contribution->module->contributions)>>6)].virtual_address;
        site=contribution_rva-input_rva+relocation->VirtualAddress;
        fixup=(DWORD *)(contents-input_rva+relocation->VirtualAddress);
        symbol=symbols+relocation->SymbolTableIndex;
        section=symbol->SectionNumber;
        value=symbol->Value;
        absolute=section==IMAGE_SYM_ABSOLUTE;
        absolute_value=absolute ? value : value+context->optional_header.ImageBase;
        switch(relocation->Type) {
        case ARM_REL_ADDR32:
            *fixup+=absolute_value;
            if(!absolute) {
                if(small_data)context->auxiliary->RecordFixup(relocation->VirtualAddress,3,
                    contribution,input_rva,section);
                else if(!fixed)APPEND_BASE_RELOCATION(3,site,section,0);
            }
            break;
        case ARM_REL_BRANCH24:
            if(!absolute)value-=site;
            word=*fixup;
            offset=word & 0xffffff;
            if(offset & 0x800000)offset|=0xff000000;
            *fixup=(word & 0xff000000)^(((value+(offset<<2)-8)>>2)&0xffffff);
            break;
        case ARM_REL_SECREL:
            if(!absolute) {
                target=find_section_by_index(section,&context->sections);
                if(target)value-=target->rva;
            }
            *fixup+=value;
            break;
        case ARM_REL_SECTION:
            if(section>0)*(WORD *)fixup+=(WORD)section;
            else if(absolute)*(WORD *)fixup+=(WORD)(context->file_header.NumberOfSections+1);
            break;
        case ARM_REL_ADDR32NB:
            *fixup+=value;
            break;
        /* The original accepts BRANCH11 but has no operand rewrite here. */
        }
        if(save_fixups && !absolute)record_debug_fixup(relocation->Type,0,site,value);
    }
}
#undef APPEND_BASE_RELOCATION
