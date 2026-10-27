/* MIPS COFF relocation application; all output instruction bytes remain data. */
#include <windows.h>
#include "input.h"
#include "sections.h"
#include "full-link-functions.h"
#include <recon.h>

extern "C" {
extern BYTE incremental_enabled;
extern DWORD gp_base,gp_limit;
extern StringNode *gp_symbol;
extern Contribution *incremental_code;
extern BYTE *next_base_relocation;
BYTE __stdcall classify_mips_relocation(Contribution *,WORD,DWORD,BYTE);
void __stdcall report_incremental_addend(IMAGE_SYMBOL *,LinkContext *,Contribution *);
void __stdcall record_debug_fixup(WORD,WORD,DWORD,DWORD);
OutputSection *__stdcall find_section_by_index(SHORT,SectionList *);
const char *__stdcall output_symbol_name(LinkContext *,IMAGE_SYMBOL *);
void __cdecl link_contribution_error(Contribution *,unsigned,...);
void __stdcall count_relocation_error(LinkContext *);
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
#define PAIR_DEBUG_FIXUP() do { \
    if(save_fixups && !absolute)record_debug_fixup(relocation->Type,0, \
        contribution_rva+relocation->VirtualAddress-input_rva,relocation->SymbolTableIndex); \
} while(0)

extern "C" void __stdcall process_mips_relocations(Contribution *contribution,
    IMAGE_RELOCATION *relocations,DWORD count,BYTE *contents,IMAGE_SYMBOL *symbols,
    LinkContext *context,DWORD *symbol_indices)
{
    BYTE fixed=context->options.fixed_image;
    BYTE debug=contribution->group->section==debug_section;
    BYTE small_data=contribution->group->section==small_data_section;
    BYTE save_fixups=(context->options.debug_formats & 8) && !debug;
    DWORD contribution_rva=contribution->rva;
    while(count) {
        IMAGE_RELOCATION *relocation=relocations;
        IMAGE_SYMBOL *symbol;
        OutputSection *target;
        DWORD input_rva,site,value,absolute_value,symbol_index,word,combined;
        BYTE *fixup;
        SHORT section;
        BYTE absolute,has_addend;
        if(classify_mips_relocation(contribution,relocation->Type,1,0)) {
            ++relocations;--count;continue;
        }
        input_rva=contribution->module->section_map[
            ((LONG)((DWORD)contribution-(DWORD)contribution->module->contributions)>>6)].virtual_address;
        site=contribution_rva-input_rva+relocation->VirtualAddress;
        fixup=contents-input_rva+relocation->VirtualAddress;
        symbol_index=relocation->SymbolTableIndex;
        symbol=symbols+symbol_index;
        section=symbol->SectionNumber;
        value=symbol->Value;
        if(incremental_enabled && !debug && symbol_indices[symbol_index] &&
           (symbol->StorageClass==IMAGE_SYM_CLASS_EXTERNAL ||
            symbol->StorageClass==IMAGE_SYM_CLASS_WEAK_EXTERNAL ||
            symbol->StorageClass==IMAGE_SYM_CLASS_FAR_EXTERNAL) &&
           (!small_data || (site-contribution_rva)%20==8)) {
            switch(relocation->Type) {
            case IMAGE_REL_MIPS_JMPADDR: /* JMPADDR */
                has_addend=(*(DWORD *)fixup & 0x3ffffff)!=0;break;
            case IMAGE_REL_MIPS_REFWORD: case IMAGE_REL_MIPS_REFWORDNB: /* REFWORD / REFWORDNB */
                has_addend=*(DWORD *)fixup!=0;break;
            case IMAGE_REL_MIPS_REFHI: /* REFHI */
                has_addend=*(WORD *)fixup!=0 || (relocation+1)->SymbolTableIndex!=0;break;
            case IMAGE_REL_MIPS_REFLO: /* REFLO */
                has_addend=*(WORD *)fixup!=0;break;
            default:
                link_contribution_fatal(contribution,1190,(unsigned)relocation->Type);
            }
            if(has_addend)report_incremental_addend(symbol,context,contribution);
            else value=incremental_code->rva+symbol_indices[symbol_index];
        }
        absolute=section==IMAGE_SYM_ABSOLUTE;
        absolute_value=absolute ? value : value+context->optional_header.ImageBase;
        if(save_fixups && !absolute)record_debug_fixup(relocation->Type,0,site,value);
        switch(relocation->Type) {
        case IMAGE_REL_MIPS_REFHALF: /* REFHALF */
            *(WORD *)fixup+=(WORD)(absolute_value>>16);
            if(!absolute && !fixed)APPEND_BASE_RELOCATION(1,site,section,0);
            break;
        case IMAGE_REL_MIPS_REFWORD: /* REFWORD */
            *(DWORD *)fixup+=absolute_value;
            if(!absolute) {
                if(small_data)context->auxiliary->RecordFixup(relocation->VirtualAddress,3,contribution,input_rva,section);
                else if(!fixed)APPEND_BASE_RELOCATION(3,site,section,0);
            }
            break;
        case IMAGE_REL_MIPS_JMPADDR: /* JMPADDR */
            if(absolute_value & 3) {RELOCATION_ERROR(2008);absolute_value&=~3u;}
            word=*(DWORD *)fixup;
            *(DWORD *)fixup=word^((((absolute_value>>2)+word)^word)&0x3ffffff);
            if(!absolute && !fixed)APPEND_BASE_RELOCATION(5,site,section,0);
            break;
        case IMAGE_REL_MIPS_REFHI: /* REFHI */
            if(!count || (relocation+1)->Type!=IMAGE_REL_MIPS_PAIR /* PAIR */)
                link_contribution_fatal(contribution,1187,"REFHI");
            ++relocation;--count;relocations=relocation;
            PAIR_DEBUG_FIXUP();
            combined=value+(DWORD)(LONG)(SHORT)relocation->SymbolTableIndex+
                ((DWORD)(LONG)*(SHORT *)fixup<<16);
            if(!absolute) {
                if(!fixed)APPEND_BASE_RELOCATION(4,site,section,combined);
                combined+=context->optional_header.ImageBase;
            }
            *(WORD *)fixup=(WORD)((LONG)(combined+0x8000)>>16);
            break;
        case IMAGE_REL_MIPS_REFLO: /* REFLO */
            *(WORD *)fixup+=(WORD)absolute_value;
            if(!absolute && !fixed)APPEND_BASE_RELOCATION(2,site,section,0);
            break;
        case IMAGE_REL_MIPS_GPREL: case IMAGE_REL_MIPS_LITERAL: /* GPREL / LITERAL */
            if(!gp_symbol) {RELOCATION_ERROR(2003);break;}
            if(absolute || value<gp_base || value>=gp_limit)RELOCATION_ERROR(2003);
            value=value-gp_symbol->symbol.Value+(DWORD)(LONG)*(SHORT *)fixup;
            if((LONG)value< -32768 || (LONG)value>32767)RELOCATION_ERROR(2004);
            *(WORD *)fixup=(WORD)value;
            break;
        case IMAGE_REL_MIPS_SECTION: /* SECTION */
            if(section>0)*(WORD *)fixup+=(WORD)section;
            else if(absolute)*(WORD *)fixup+=(WORD)(context->file_header.NumberOfSections+1);
            break;
        case IMAGE_REL_MIPS_SECREL: /* SECREL */
            if(!absolute) {
                target=find_section_by_index(section,&context->sections);
                if(target)value-=target->rva;
            }
            if(!debug && value>=0x8000)RELOCATION_ERROR(2004);
            *(DWORD *)fixup+=value;
            break;
        case IMAGE_REL_MIPS_SECRELHI: /* SECRELHI */
            if(!absolute) {
                target=find_section_by_index(section,&context->sections);
                if(target)value-=target->rva;
            }
            if(!count || (relocation+1)->Type!=IMAGE_REL_MIPS_PAIR /* PAIR */)
                link_contribution_fatal(contribution,1187,"SECRELHI");
            ++relocation;--count;relocations=relocation;
            PAIR_DEBUG_FIXUP();
            combined=value+(DWORD)(LONG)(SHORT)relocation->SymbolTableIndex+
                ((DWORD)(LONG)*(SHORT *)fixup<<16);
            *(WORD *)fixup=(WORD)((LONG)(combined+0x8000)>>16);
            break;
        case IMAGE_REL_MIPS_SECRELLO: /* SECRELLO */
            if(!absolute) {
                target=find_section_by_index(section,&context->sections);
                if(target)value-=target->rva;
            }
            *(WORD *)fixup+=(WORD)value;
            break;
        case IMAGE_REL_MIPS_REFWORDNB: /* REFWORDNB */
            *(DWORD *)fixup+=value;
            break;
        }
        ++relocations;--count;
    }
}
#undef PAIR_DEBUG_FIXUP
#undef RELOCATION_ERROR
#undef APPEND_BASE_RELOCATION
