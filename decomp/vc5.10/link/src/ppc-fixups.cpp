/* PowerPC fixups serialize target instructions; they are never host code. */
#include <windows.h>
#include "input.h"
#include "sections.h"
#include "full-link-functions.h"
#include <recon.h>

extern "C" {
extern DWORD undefined_symbols;
extern DWORD gp_base,gp_limit;
extern StringNode *gp_symbol;
extern BYTE *next_base_relocation;
BYTE __stdcall classify_ppc_relocation(Contribution *,WORD,DWORD,BYTE);
void __stdcall record_debug_fixup(WORD,WORD,DWORD,DWORD);
OutputSection *__stdcall find_section_by_index(SHORT,SectionList *);
const char *__stdcall output_symbol_name(LinkContext *,IMAGE_SYMBOL *);
void __cdecl link_contribution_error(Contribution *,unsigned,...);
void __stdcall count_relocation_error(LinkContext *);
DWORD __stdcall find_ppc_branch_target(LinkContext *context,DWORD site,
    DWORD absolute,DWORD section,DWORD value);
}

/* These record stores and diagnostics were inline at their actual call sites. */
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

extern "C" void __stdcall process_ppc_relocations(Contribution *contribution,
    IMAGE_RELOCATION *relocations,DWORD count,BYTE *contents,IMAGE_SYMBOL *symbols,
    LinkContext *context,DWORD *symbol_indices)
{
    BYTE fixed=context->options.fixed_image;
    BYTE small_data=contribution->group->section==small_data_section;
    BYTE save_fixups=(context->options.debug_formats & 8) &&
        contribution->group->section!=debug_section;
    DWORD contribution_rva=contribution->rva;
    while(count) {
        IMAGE_RELOCATION *relocation=relocations;
        IMAGE_SYMBOL *symbol;
        OutputSection *target;
        DWORD input_rva,site,value,absolute_value,word,offset,combined;
        BYTE *fixup;
        SHORT section;
        BYTE absolute,negative;
        if(classify_ppc_relocation(contribution,relocation->Type,1,0))goto next;
        input_rva=contribution->module->section_map[
            ((LONG)((DWORD)contribution-(DWORD)contribution->module->contributions)>>6)].virtual_address;
        site=contribution_rva-input_rva+relocation->VirtualAddress;
        fixup=contents-input_rva+relocation->VirtualAddress;
        symbol=symbols+relocation->SymbolTableIndex;
        section=symbol->SectionNumber;
        value=symbol->Value;
        absolute=section==IMAGE_SYM_ABSOLUTE;
        absolute_value=absolute ? value : value+context->optional_header.ImageBase;
        if(save_fixups && !absolute)record_debug_fixup(relocation->Type,0,site,value);
        negative=(relocation->Type & IMAGE_REL_PPC_NEG)!=0;
        if(negative && !absolute && !fixed) {RELOCATION_ERROR(2009);goto next;}
        switch((BYTE)relocation->Type) {
        case IMAGE_REL_PPC_ADDR32: /* ADDR32 */
            if(negative)*(DWORD *)fixup-=absolute_value;
            else *(DWORD *)fixup+=absolute_value;
            if(!absolute) {
                if(small_data)context->auxiliary->RecordFixup(relocation->VirtualAddress,3,
                    contribution,input_rva,section);
                else if(!fixed)APPEND_BASE_RELOCATION(3,site,section,0);
            }
            break;
        case IMAGE_REL_PPC_ADDR24: case IMAGE_REL_PPC_ADDR14: /* ADDR24 / ADDR14 */
            if(!absolute && !fixed) {RELOCATION_ERROR(2009);break;}
            if(absolute_value & 3) {RELOCATION_ERROR(2008);absolute_value&=~3;}
            word=*(DWORD *)fixup;
            if((BYTE)relocation->Type==IMAGE_REL_PPC_ADDR24) {
                offset=word & 0x3fffffc;
                if(offset & 0x2000000)offset|=0xfc000000;
                offset=negative ? offset-absolute_value : offset+absolute_value;
                combined=offset & 0xfe000000;
                if(combined && combined!=0xfe000000)RELOCATION_ERROR(2013);
                *(DWORD *)fixup=(word & ~0x3fffffc)|(offset & 0x3fffffc);
            } else {
                offset=(DWORD)(LONG)(SHORT)(word & 0xfffc);
                offset=negative ? offset-absolute_value : offset+absolute_value;
                if((LONG)offset>0x7fff || (LONG)offset< -0x8000)RELOCATION_ERROR(2013);
                if(relocation->Type & IMAGE_REL_PPC_BRTAKEN) {
                    if((LONG)offset<0)word&=~0x200000;
                    else word|=0x200000;
                } else if(relocation->Type & IMAGE_REL_PPC_BRNTAKEN) {
                    if((LONG)offset>=0)word&=~0x200000;
                    else word|=0x200000;
                }
                *(DWORD *)fixup=(word & ~0xfffc)|(offset & 0xfffc);
            }
            break;
        case IMAGE_REL_PPC_ADDR16: /* ADDR16 */
            if(negative)*(WORD *)fixup-=(WORD)absolute_value;
            else *(WORD *)fixup+=(WORD)absolute_value;
            if(!absolute && !fixed)APPEND_BASE_RELOCATION(2,site,section,0);
            break;
        case IMAGE_REL_PPC_REL24: case IMAGE_REL_PPC_REL14: /* REL24 / REL14 */
            if(value & 3) {RELOCATION_ERROR(2008);value&=~3;}
            word=*(DWORD *)fixup;
            value-=contribution_rva;
            if((BYTE)relocation->Type==IMAGE_REL_PPC_REL24) {
                offset=word & 0x3fffffc;
                if(offset & 0x2000000)offset|=0xfc000000;
                offset=negative ? offset-value : offset+value;
                if(!undefined_symbols && ((LONG)offset< -0x2000000 || (LONG)offset>=0x2000000)) {
                    /* Only the low BYTE of this physical absolute-state DWORD
                     * is consumed by the pending branch-target helper. */
                    offset=find_ppc_branch_target(context,site,absolute,section,absolute_value)-site;
                    if((LONG)offset< -0x2000000 || (LONG)offset>=0x2000000)RELOCATION_ERROR(2013);
                }
                *(DWORD *)fixup=(word & ~0x3fffffc)|(offset & 0x3fffffc);
            } else {
                offset=(DWORD)(LONG)(SHORT)(word & 0xfffc);
                offset=negative ? offset-value : offset+value;
                if((LONG)offset>0x7fff || (LONG)offset< -0x8000)RELOCATION_ERROR(2013);
                if(relocation->Type & IMAGE_REL_PPC_BRTAKEN) {
                    if((LONG)offset<0)word&=~0x200000;
                    else word|=0x200000;
                } else if(relocation->Type & IMAGE_REL_PPC_BRNTAKEN) {
                    if((LONG)offset>=0)word&=~0x200000;
                    else word|=0x200000;
                }
                *(DWORD *)fixup=(word & ~0xfffc)|(offset & 0xfffc);
            }
            break;
        case IMAGE_REL_PPC_ADDR32NB: /* ADDR32NB */
            if(negative)*(DWORD *)fixup-=value;
            else *(DWORD *)fixup+=value;
            break;
        case IMAGE_REL_PPC_SECREL: /* SECREL */
            if(!absolute) {
                target=find_section_by_index(section,&context->sections);
                if(target)value-=target->rva;
            }
            *(DWORD *)fixup+=value;
            break;
        case IMAGE_REL_PPC_SECTION: /* SECTION */
            if(section>0)*(WORD *)fixup+=(WORD)section;
            else if(absolute)*(WORD *)fixup+=(WORD)(context->file_header.NumberOfSections+1);
            break;
        case IMAGE_REL_PPC_SECREL16: /* SECREL16 */
            if(!absolute) {
                target=find_section_by_index(section,&context->sections);
                if(target)value-=target->rva;
            }
            if((DWORD)(LONG)(SHORT)value!=value)RELOCATION_ERROR(2014);
            *(WORD *)fixup+=(WORD)value;
            break;
        case IMAGE_REL_PPC_REFHI: /* REFHI */
            if(!count || (relocation+1)->Type!=IMAGE_REL_PPC_PAIR)link_contribution_fatal(contribution,1187,"REFHI");
            ++relocations;--count;relocation=relocations;
            PAIR_DEBUG_FIXUP();
            combined=value+((DWORD)(LONG)*(SHORT *)fixup<<16)+(LONG)(SHORT)relocation->SymbolTableIndex;
            if(!absolute) {
                if(!fixed)APPEND_BASE_RELOCATION(4,site,section,combined);
                combined+=context->optional_header.ImageBase;
            }
            *(WORD *)fixup=(WORD)((LONG)(combined+0x8000)>>16);
            break;
        case IMAGE_REL_PPC_REFLO: /* REFLO */
            *(WORD *)fixup+=(WORD)absolute_value;
            if(!absolute && !fixed)APPEND_BASE_RELOCATION(2,site,section,0);
            break;
        case IMAGE_REL_PPC_SECRELLO: case IMAGE_REL_PPC_SECRELHI: /* SECRELLO / SECRELHI */
            if(!absolute) {
                target=find_section_by_index(section,&context->sections);
                if(target)value-=target->rva;
            }
            if((BYTE)relocation->Type==IMAGE_REL_PPC_SECRELLO)*(WORD *)fixup+=(WORD)value;
            else {
                if(!count || (relocation+1)->Type!=IMAGE_REL_PPC_PAIR)link_contribution_fatal(contribution,1187,"SECRELHI");
                ++relocations;--count;relocation=relocations;
                PAIR_DEBUG_FIXUP();
                combined=value+((DWORD)(LONG)*(SHORT *)fixup<<16)+(LONG)(SHORT)relocation->SymbolTableIndex;
                *(WORD *)fixup=(WORD)((LONG)(combined+0x8000)>>16);
            }
            break;
        case 21: /* GPREL in the target's relocation-name table. */
            if(!gp_symbol) {RELOCATION_ERROR(2003);break;}
            if(absolute || value<gp_base || value>=gp_limit)RELOCATION_ERROR(2003);
            offset=(DWORD)(LONG)*(SHORT *)fixup-gp_symbol->symbol.Value+value;
            if((LONG)offset< -0x8000 || (LONG)offset>0x7fff)RELOCATION_ERROR(2004);
            *(WORD *)fixup=(WORD)offset;
            break;
        }
next:
        ++relocations;--count;
    }
}
#undef PAIR_DEBUG_FIXUP
#undef RELOCATION_ERROR
#undef APPEND_BASE_RELOCATION
