/* Alpha object fixups; host execution does not execute the emitted instructions. */
#include <windows.h>
#include "input.h"
#include "sections.h"
#include "options.h"
#include "full-link-functions.h"
#include <recon.h>

extern "C" {
extern BYTE incremental_enabled;
extern DWORD undefined_symbols,gp_base,gp_limit;
extern StringNode *gp_symbol;
extern Contribution *incremental_code;
extern BYTE *next_base_relocation;
BYTE __stdcall classify_alpha_relocation(Contribution *,WORD,DWORD,BYTE);
void __stdcall report_incremental_addend(IMAGE_SYMBOL *,LinkContext *,Contribution *);
void __stdcall record_debug_fixup(WORD,WORD,DWORD,DWORD);
void __stdcall record_relative_fixup(DWORD);
OutputSection *__stdcall find_section_by_index(SHORT,SectionList *);
const char *__stdcall output_symbol_name(LinkContext *,IMAGE_SYMBOL *);
void __cdecl link_contribution_warning(Contribution *,unsigned,...);
void __cdecl link_contribution_error(Contribution *contribution,unsigned message,...) { __asm { int 3 } }
void __stdcall count_relocation_error(LinkContext *context) { __asm { int 3 } }
DWORD __stdcall find_alpha_branch_target(LinkContext *context,DWORD site,
    DWORD absolute,DWORD section,DWORD value) { __asm { int 3 } return 0; }
}
BYTE RECON_THISCALL LinkAuxiliaryData::RecordFixup(DWORD value,WORD type,
    Contribution *contribution,DWORD input_rva,SHORT section)
{ __asm { int 3 } return 0; }

/* These stores were inlined at each real fixup site. */
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

extern "C" void __stdcall process_alpha_relocations(Contribution *contribution,
    IMAGE_RELOCATION *relocations,DWORD count,BYTE *contents,IMAGE_SYMBOL *symbols,
    LinkContext *context,DWORD *symbol_indices)
{
    BYTE fixed=context->options.fixed_image;
    BYTE debug=contribution->group->section==debug_section;
    BYTE small_data=contribution->group->section==small_data_section;
    BYTE save_fixups=(context->options.debug_formats & 8) && !debug;
    DWORD contribution_rva=contribution->rva,rom_base=0;
    if(context->options.rom) {
        DWORD headers=context->file_header.NumberOfSections*40+0x4c;
        rom_base=context->optional_header.BaseOfCode-
            ((headers-1)/context->optional_header.FileAlignment+1)*context->optional_header.FileAlignment;
    }
    while(count) {
        IMAGE_RELOCATION *relocation=relocations;
        IMAGE_SYMBOL *symbol;
        OutputSection *target;
        DWORD input_rva,site,value,absolute_value,symbol_index,word,offset,combined;
        BYTE *fixup;
        SHORT section;
        BYTE absolute,has_addend;
        if(classify_alpha_relocation(contribution,relocation->Type,1,0)) {
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
            case IMAGE_REL_ALPHA_REFLONG: case IMAGE_REL_ALPHA_BRADDR:
                has_addend=(*(DWORD *)fixup & 0x1fffff)!=0;break;
            case IMAGE_REL_ALPHA_HINT:
                has_addend=(*(DWORD *)fixup & 0x3fff)!=0;break;
            case IMAGE_REL_ALPHA_REFHI:
                has_addend=*(WORD *)fixup!=0 || (relocation+1)->SymbolTableIndex!=0;break;
            case IMAGE_REL_ALPHA_REFLO:
                has_addend=*(WORD *)fixup!=0;break;
            case IMAGE_REL_ALPHA_REFLONGNB:
                has_addend=*(DWORD *)fixup!=0;break;
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
        case IMAGE_REL_ALPHA_REFLONG:
            *(DWORD *)fixup+=absolute_value;
            if(!absolute) {
                if(small_data)context->auxiliary->RecordFixup(*(DWORD *)fixup,3,contribution,input_rva,section);
                else if(!fixed)APPEND_BASE_RELOCATION(3,site,section,0);
            }
            break;
        case IMAGE_REL_ALPHA_REFQUAD:
            *(DWORD *)fixup+=absolute_value;
            ((DWORD *)fixup)[1]=(*(DWORD *)fixup & 0x80000000) ? 0xffffffff : 0;
            if(!absolute && !fixed)APPEND_BASE_RELOCATION(3,site,section,0);
            break;
        case IMAGE_REL_ALPHA_GPREL32:
            if(!gp_symbol) {RELOCATION_ERROR(2003);break;}
            if(absolute || value<gp_base || value>=gp_limit)RELOCATION_ERROR(2003);
            *(DWORD *)fixup+=value-gp_symbol->symbol.Value;
            break;
        case IMAGE_REL_ALPHA_LITERAL:
            if(!gp_symbol) {RELOCATION_ERROR(2003);break;}
            if(absolute || value<gp_base || value>=gp_limit)RELOCATION_ERROR(2003);
            value=value-gp_symbol->symbol.Value+(DWORD)(LONG)*(SHORT *)fixup;
            if((LONG)value< -32768 || (LONG)value>32767)RELOCATION_ERROR(2004);
            *(WORD *)fixup=(WORD)value;
            break;
        case IMAGE_REL_ALPHA_BRADDR:
            if(absolute_value & 3) {
                RELOCATION_ERROR(2008);
                value&=~3u;absolute_value&=~3u;
            }
            word=*(DWORD *)fixup;
            offset=word & 0x1fffff;
            if(offset & 0x100000)offset|=0xffe00000;
            value+=(offset<<2)-site-4;
            if(!undefined_symbols && (absolute || (LONG)value>=0x400000 || (LONG)value< -0x400000)) {
                if(incremental_enabled)
                    link_contribution_warning(contribution,2013,output_symbol_name(context,symbol));
                /* The original passes its whole scratch DWORD, but only its
                 * initialized low BYTE is consumed by the pending helper
                 * (4097a7). No equality of its unwritten upper bytes is claimed. */
                absolute_value=find_alpha_branch_target(context,site,absolute,section,absolute_value);
                if(!absolute_value)
                    link_contribution_fatal(contribution,2013,output_symbol_name(context,symbol));
                value=absolute_value-site-4;
            }
            *(DWORD *)fixup=(word & 0xffe00000)^(((LONG)value>>2)&0x1fffff);
            if(context->options.map_fixups)record_relative_fixup(site);
            break;
        case IMAGE_REL_ALPHA_HINT:
            if(absolute_value & 3) {RELOCATION_ERROR(2008);absolute_value&=~3u;}
            word=*(DWORD *)fixup;
            *(DWORD *)fixup=word^((((absolute_value>>2)+word)^word)&0x3fff);
            break;
        case IMAGE_REL_ALPHA_INLINE_REFLONG:
            if(!count || ((relocation+1)->Type!=IMAGE_REL_ALPHA_MATCH &&
                          (relocation+1)->Type!=IMAGE_REL_ALPHA_ABSOLUTE))
                link_contribution_fatal(contribution,1187,"INLINE_REFLONG");
            ++relocation;--count;relocations=relocation;
            PAIR_DEBUG_FIXUP();
            offset=relocation->Type==IMAGE_REL_ALPHA_ABSOLUTE ? 4 : relocation->SymbolTableIndex;
            combined=absolute_value+((DWORD)*(WORD *)fixup<<16)+*(WORD *)(fixup+offset);
            *(WORD *)fixup=(WORD)(combined>>16);
            *(WORD *)(fixup+offset)=(WORD)combined;
            if(combined & 0x8000)++*(WORD *)fixup;
            if(!absolute && !fixed) {
                APPEND_BASE_RELOCATION(4,site,section,combined-context->optional_header.ImageBase);
                APPEND_BASE_RELOCATION(2,site+offset,section,0);
            }
            break;
        case IMAGE_REL_ALPHA_REFHI:
            if(!count || (relocation+1)->Type!=IMAGE_REL_ALPHA_PAIR)
                link_contribution_fatal(contribution,1187,"REFHI");
            ++relocation;--count;relocations=relocation;
            PAIR_DEBUG_FIXUP();
            combined=value+((DWORD)(LONG)*(SHORT *)fixup<<16)+(DWORD)(LONG)(SHORT)relocation->SymbolTableIndex;
            if(!absolute) {
                if(!fixed)APPEND_BASE_RELOCATION(4,site,section,combined);
                combined+=context->optional_header.ImageBase;
            }
            *(WORD *)fixup=(WORD)((LONG)(combined+0x8000)>>16);
            break;
        case IMAGE_REL_ALPHA_REFLO:
            *(WORD *)fixup+=(WORD)absolute_value;
            if(!absolute && !fixed)APPEND_BASE_RELOCATION(2,site,section,0);
            break;
        case IMAGE_REL_ALPHA_SECTION:
            if(section>0)*(WORD *)fixup+=(WORD)section;
            else if(absolute)*(WORD *)fixup+=(WORD)(context->file_header.NumberOfSections+1);
            break;
        case IMAGE_REL_ALPHA_SECREL:
            if(!absolute) {
                target=find_section_by_index(section,&context->sections);
                if(target)value-=target->rva;
            }
            *(DWORD *)fixup+=value;
            if(!debug && (*(WORD *)fixup>0x7fff || value>=0x7fff))RELOCATION_ERROR(2004);
            break;
        case IMAGE_REL_ALPHA_REFLONGNB:
            *(DWORD *)fixup+=value-rom_base;
            break;
        case IMAGE_REL_ALPHA_SECRELHI:
            if(!absolute) {
                target=find_section_by_index(section,&context->sections);
                if(target)value-=target->rva;
            }
            if(!count || (relocation+1)->Type!=IMAGE_REL_ALPHA_PAIR)
                link_contribution_fatal(contribution,1187,"SECRELHI");
            ++relocation;--count;relocations=relocation;
            PAIR_DEBUG_FIXUP();
            combined=value+((DWORD)(LONG)*(SHORT *)fixup<<16)+(DWORD)(LONG)(SHORT)relocation->SymbolTableIndex;
            *(WORD *)fixup=(WORD)((LONG)(combined+0x8000)>>16);
            break;
        case IMAGE_REL_ALPHA_SECRELLO:
            if(!absolute) {
                target=find_section_by_index(section,&context->sections);
                if(target)value-=target->rva;
            }
            *(WORD *)fixup+=(WORD)value;
            break;
        }
        ++relocations;--count;
    }
}
#undef PAIR_DEBUG_FIXUP
#undef RELOCATION_ERROR
#undef APPEND_BASE_RELOCATION
