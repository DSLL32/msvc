/* SuperH COFF resource/code relocation records remain output data on the host. */
#include <windows.h>
#include "input.h"
#include "initializers.h"
#include "comdat.h"
#include "sections.h"
#include "full-link-functions.h"
extern "C" __declspec(dllimport) DWORD __cdecl SigForPbCb(const void *,long,DWORD);
#include <recon.h>

extern "C" {
ContributionInfo *__stdcall get_contribution_info(Contribution *);
void __stdcall record_relocation_reference(LinkContext *,Contribution *,IMAGE_SYMBOL *,IMAGE_RELOCATION *);
BYTE __stdcall classify_sh_relocation(Contribution *,WORD,DWORD,BYTE);
}
extern "C" DWORD __stdcall read_sh_relocations(LinkContext *context,Contribution *contribution,
    IMAGE_SYMBOL *symbols,IMAGE_RELOCATION *relocations,DWORD count,DWORD index)
{
    ContributionInfo *info=get_contribution_info(contribution);
    IMAGE_RELOCATION *relocation=relocations;
    DWORD checksum=0,total=0,i;
    BYTE folding=context->options.fold_identical_comdats && buffer4.remaining &&
        ((ContributionInfo **)buffer4.base)[buffer4.remaining-1]==info;
    for(i=0;i<count;++i,++relocation) {
        if(classify_sh_relocation(contribution,relocation->Type,0,0))continue;
        if(context->options.effective_reference_opt || context->options.fold_identical_comdats)
            record_relocation_reference(context,contribution,symbols,relocation);
        if(folding) {
            checksum=SigForPbCb(&relocation->Type,2,checksum);
            checksum=SigForPbCb(&relocation->VirtualAddress,4,checksum);
            checksum=SigForPbCb(&symbols[relocation->SymbolTableIndex].StorageClass,1,checksum);
        }
        if(!context->options.fixed_image && (relocation->Type==2 || relocation->Type==12))++total;
    }
    if(folding) {
        checksum=SigForPbCb(&contribution->characteristics,4,checksum);
        checksum=SigForPbCb(&contribution->comdat_selection,1,checksum);
        ((ContributionInfo **)buffer4.base)[buffer4.remaining-1]->fold_signature=checksum;
    }
    return total;
}

/* Names and values follow the encountered SuperH relocation-name table. */
enum ShRelocation {
    SH_DIRECT16=1,SH_DIRECT32,SH_DIRECT8,SH_DIRECT8_WORD,SH_DIRECT8_LONG,
    SH_DIRECT4,SH_DIRECT4_WORD,SH_DIRECT4_LONG,SH_PCREL8_WORD,SH_PCREL8_LONG,
    SH_PCREL12_WORD,SH_STARTOF_SECT,SH_SIZEOF_SECT,SH_SECTION,SH_SECREL,SH_DIRECT32_NB
};
extern "C" {
extern BYTE *next_base_relocation;
OutputSection *__stdcall find_section_by_index(SHORT,SectionList *);
const char *__stdcall output_symbol_name(LinkContext *,IMAGE_SYMBOL *);
void __cdecl link_contribution_error(Contribution *,unsigned,...);
void __stdcall count_relocation_error(LinkContext *);
void __stdcall record_debug_fixup(WORD,WORD,DWORD,DWORD);
OutputSection *__stdcall find_sh_symbol_section(LinkContext *context,const char *name)
{ __asm { int 3 } return NULL; }
}
#define APPEND_BASE_RELOCATION(kind,site,target,addend) do { \
    LinkBaseRelocation *record=(LinkBaseRelocation *)next_base_relocation; \
    record->type=(kind);record->address=(site);record->section=(target); \
    record->value=(addend);next_base_relocation+=12; \
} while(0)
#define RELOCATION_ERROR(message) do { \
    link_contribution_error(contribution,(message),output_symbol_name(context,symbol)); \
    count_relocation_error(context); \
} while(0)

extern "C" void __stdcall process_sh_relocations(Contribution *contribution,
    IMAGE_RELOCATION *relocations,DWORD count,BYTE *contents,IMAGE_SYMBOL *symbols,
    LinkContext *context,DWORD *symbol_indices)
{
    BYTE fixed=context->options.fixed_image;
    BYTE small_data=contribution->group->section==small_data_section;
    BYTE save_fixups=(context->options.debug_formats & 8) &&
        contribution->group->section!=debug_section;
    DWORD contribution_rva=contribution->rva;
    while(count--) {
        IMAGE_RELOCATION *relocation=relocations++;
        IMAGE_SYMBOL *symbol;
        OutputSection *target;
        BYTE *fixup;
        DWORD input_rva,site,value,absolute_value,offset;
        WORD word;
        SHORT section;
        BYTE absolute;
        if(classify_sh_relocation(contribution,relocation->Type,1,0))continue;
        input_rva=contribution->module->section_map[
            ((LONG)((DWORD)contribution-(DWORD)contribution->module->contributions)>>6)].virtual_address;
        site=contribution_rva-input_rva+relocation->VirtualAddress;
        fixup=contents-input_rva+relocation->VirtualAddress;
        symbol=symbols+relocation->SymbolTableIndex;
        section=symbol->SectionNumber;
        value=symbol->Value;
        absolute=section==IMAGE_SYM_ABSOLUTE;
        absolute_value=absolute ? value : value+context->optional_header.ImageBase;
        switch(relocation->Type) {
        case SH_DIRECT16: *(WORD *)fixup+=(WORD)absolute_value;break;
        case SH_DIRECT32:
            *(DWORD *)fixup+=absolute_value;
            if(!absolute) {
                if(small_data)context->auxiliary->RecordFixup(relocation->VirtualAddress,3,
                    contribution,input_rva,section);
                else if(!fixed)APPEND_BASE_RELOCATION(3,site,section,0);
            }
            break;
        case SH_DIRECT8: *fixup+=(BYTE)absolute_value;break;
        case SH_DIRECT8_WORD:
            if(absolute_value & 1) {RELOCATION_ERROR(2008);absolute_value&=~1;}
            *fixup=(BYTE)((WORD)((*fixup<<1)+(WORD)absolute_value)>>1);
            break;
        case SH_DIRECT8_LONG:
            if(absolute_value & 3) {RELOCATION_ERROR(2008);absolute_value&=~3;}
            *fixup=(BYTE)((WORD)((*fixup<<2)+(WORD)absolute_value)>>2);
            break;
        case SH_DIRECT4:
            *fixup=(BYTE)((*fixup & 0xf0)|((BYTE)(*fixup & 15)+(BYTE)absolute_value));
            break;
        case SH_DIRECT4_WORD:
            if(absolute_value & 1) {RELOCATION_ERROR(2008);absolute_value&=~1;}
            *fixup=(BYTE)((*fixup & 0xf0)|((absolute_value+(*fixup & 15)*2)>>1));
            break;
        case SH_DIRECT4_LONG:
            if(absolute_value & 3) {RELOCATION_ERROR(2008);absolute_value&=~3;}
            *fixup=(BYTE)((*fixup & 0xf0)|((absolute_value+(*fixup & 15)*4)>>2));
            break;
        case SH_PCREL8_WORD: case SH_PCREL8_LONG: case SH_PCREL12_WORD:
            if(!absolute)value-=site+4;
            if(relocation->Type==SH_PCREL8_LONG) {
                if(value & 3) {RELOCATION_ERROR(2008);value&=~3;}
                value+=*fixup*4;
                *fixup=(BYTE)(value>>2);
            } else {
                if(value & 1) {RELOCATION_ERROR(2008);value&=~1;}
                if(relocation->Type==SH_PCREL8_WORD) {
                    value+=*fixup*2;
                    *fixup=(BYTE)(value>>1);
                } else {
                    word=*(WORD *)fixup;
                    offset=word & 0xfff;
                    if(offset & 0x800)offset|=0xfffff000;
                    *(WORD *)fixup=(WORD)((word & 0xf000)|(((value+offset*2)>>1)&0xfff));
                }
            }
            break;
        case SH_STARTOF_SECT: case SH_SIZEOF_SECT:
            target=find_sh_symbol_section(context,output_symbol_name(context,symbol));
            if(!target)link_contribution_fatal(contribution,1188,output_symbol_name(context,symbol));
            value=target->rva;
            if(relocation->Type==SH_STARTOF_SECT) {
                *(DWORD *)fixup+=value+context->optional_header.ImageBase;
                if(!absolute && !fixed)APPEND_BASE_RELOCATION(3,site,target->index,0);
            } else *(DWORD *)fixup+=target->virtual_size;
            break;
        case SH_SECTION:
            if(section>0)*(WORD *)fixup+=(WORD)section;
            else if(absolute)*(WORD *)fixup+=(WORD)(context->file_header.NumberOfSections+1);
            break;
        case SH_SECREL:
            if(!absolute) {
                target=find_section_by_index(section,&context->sections);
                if(target)value-=target->rva;
            }
            *(DWORD *)fixup+=value;
            break;
        case SH_DIRECT32_NB: *(DWORD *)fixup+=value;break;
        }
        if(save_fixups && !absolute)record_debug_fixup(relocation->Type,0,site,value);
    }
}
#undef RELOCATION_ERROR
#undef APPEND_BASE_RELOCATION
