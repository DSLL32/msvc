#include <windows.h>
#include <string.h>
#include "input.h"
#include "initializers.h"
#include "comdat.h"
#include "sections.h"
#include "full-link-functions.h"
__declspec(dllimport) DWORD __cdecl SigForPbCb(const void *,long,DWORD);
#include <recon.h>

extern char *current_module_strings;
extern char short_symbol_name[9];
ContributionInfo *__stdcall get_contribution_info(Contribution *);
void __stdcall record_relocation_reference(LinkContext *,Contribution *,IMAGE_SYMBOL *,IMAGE_RELOCATION *);

DWORD __stdcall read_x86_relocations(LinkContext *context,Contribution *contribution,
    IMAGE_SYMBOL *symbols,IMAGE_RELOCATION *relocations,DWORD count,DWORD index)
{
    ContributionInfo *info=get_contribution_info(contribution);
    IMAGE_RELOCATION *relocation=relocations;
    IMAGE_SYMBOL *symbol;
    const char *name;
    DWORD checksum=0,total=0,i;
    WORD kind;
    BYTE folding=context->options.fold_identical_comdats && buffer4.remaining &&
        ((ContributionInfo **)buffer4.base)[buffer4.remaining-1]==info;
    BYTE vxd=context->is_vxd==1;
    for (i=0;i<count;++i,++relocation) {
        if (classify_x86_relocation(contribution,relocation->Type,0,vxd)) continue;
        symbol=symbols+relocation->SymbolTableIndex;
        if (symbol->StorageClass==IMAGE_SYM_CLASS_UNDEFINED_STATIC) {
            name=get_symbol_name((StringNode *)symbol,current_module_strings,&kind,NULL);
            if (kind==1) name=strncpy(short_symbol_name,name,8);
            link_contribution_fatal(contribution,1186,name);
        }
        if (context->options.effective_reference_opt || context->options.fold_identical_comdats)
            record_relocation_reference(context,contribution,symbols,relocation);
        if (folding) {
            checksum=SigForPbCb(&relocation->Type,2,checksum);
            checksum=SigForPbCb(&relocation->VirtualAddress,4,checksum);
            checksum=SigForPbCb(&symbol->StorageClass,1,checksum);
        }
        if (context->is_vxd==1) ++total;
        else if (!context->options.fixed_image) {
            if (relocation->Type==6) ++total; /* IMAGE_REL_I386_DIR32 */
            else if (relocation->Type==20 && context->options.new_relocations &&
                (WORD)index!=(WORD)symbols[relocations->SymbolTableIndex].SectionNumber)
                ++total;
        }
    }
    if (folding) {
        checksum=SigForPbCb(&contribution->characteristics,4,checksum);
        checksum=SigForPbCb(&contribution->comdat_selection,1,checksum);
        ((ContributionInfo **)buffer4.base)[buffer4.remaining-1]->fold_signature=checksum;
    }
    return total;
}
extern BYTE incremental_enabled;
IMAGE_SYMBOL *__stdcall create_incremental_symbol(LinkContext *,Contribution *,DWORD,IMAGE_SYMBOL *,BYTE);
void __stdcall redirect_comdat_reference(ComdatReference *,Contribution *);
void __stdcall record_relocation_reference(LinkContext *context,Contribution *contribution,
    IMAGE_SYMBOL *symbols,IMAGE_RELOCATION *relocation)
{
    IMAGE_SYMBOL *symbol=symbols+relocation->SymbolTableIndex,*redirected;
    OutputModule *module=contribution->module;
    Contribution *target,*redirected_target;
    ComdatReference *reference;
    if (symbol->StorageClass==IMAGE_SYM_CLASS_EXTERNAL || symbol->StorageClass==0x44 ||
        symbol->StorageClass==0x69 /* weak external */) {
        reference=(ComdatReference *)add_comdat_reference(relocation->SymbolTableIndex,contribution,NULL);
        if (incremental_enabled && relocation->Type!=0x28) reference->flag_byte=1;
        return;
    }
    if (symbol->SectionNumber<=0) return;
    target=(Contribution *)module->contributions+symbol->SectionNumber-1;
    if (incremental_enabled && (symbol->Type & 0x8000) && relocation->Type!=0x28) {
        redirected=create_incremental_symbol(context,contribution,1,symbol,0);
        redirected_target=(Contribution *)module->contributions+redirected->SectionNumber-1;
        reference=(ComdatReference *)add_comdat_reference(0,contribution,target);
        redirect_comdat_reference(reference,redirected_target);
        return;
    }
    if (contribution!=target) add_comdat_reference(0,contribution,target);
}
IMAGE_SYMBOL *__stdcall create_incremental_symbol(LinkContext *context,Contribution *contribution,
    DWORD kind,IMAGE_SYMBOL *symbol,BYTE handler) { __asm { int 3 } return NULL; }
void __stdcall redirect_comdat_reference(ComdatReference *reference,Contribution *target) { __asm { int 3 } }

extern BYTE *next_base_relocation;
extern Contribution *incremental_code;
OutputSection *__stdcall find_section_by_index(SHORT,SectionList *);
void __stdcall report_incremental_addend(IMAGE_SYMBOL *,LinkContext *,Contribution *);
void __stdcall record_debug_fixup(WORD,WORD,DWORD,DWORD);
void __stdcall record_relative_fixup(DWORD);
const char *__stdcall output_symbol_name(LinkContext *,IMAGE_SYMBOL *);
/* Expansion preserves the original inline stores, without a helper vertex. */
#define APPEND_BASE_RELOCATION(kind,site,target,addend) do { \
    LinkBaseRelocation *record=(LinkBaseRelocation *)next_base_relocation; \
    record->type=(kind); record->address=(site); record->section=(target); \
    record->value=(addend); next_base_relocation+=12; \
} while(0)

void __stdcall process_x86_relocations(Contribution *contribution,
    IMAGE_RELOCATION *relocations,DWORD count,BYTE *contents,IMAGE_SYMBOL *symbols,
    LinkContext *context,DWORD *symbol_indices)
{
    BYTE vxd=(context->is_vxd==1),fixed=context->options.fixed_image;
    BYTE debug=(contribution->group->section==debug_section);
    BYTE save_fixups=(context->options.debug_formats & 8) && !debug;
    DWORD contribution_rva=contribution->rva;
    while(count--) {
        IMAGE_RELOCATION *relocation=relocations++;
        IMAGE_SYMBOL *symbol;
        SHORT section;
        DWORD input_rva,site,value,absolute_value,target_offset,packed_site,packed_target;
        DWORD *fixup;
        OutputSection *target;
        BYTE absolute;
        if(classify_x86_relocation(contribution,relocation->Type,1,vxd))continue;
        input_rva=contribution->module->section_map[
            ((LONG)((DWORD)contribution-(DWORD)contribution->module->contributions)>>6)
            ].virtual_address;
        site=contribution_rva-input_rva+relocation->VirtualAddress;
        fixup=(DWORD *)(contents-input_rva+relocation->VirtualAddress);
        symbol=symbols+relocation->SymbolTableIndex;
        section=symbol->SectionNumber;
        value=symbol->Value;
        if(incremental_enabled && !debug && symbol_indices[relocation->SymbolTableIndex] &&
           (symbol->StorageClass==IMAGE_SYM_CLASS_EXTERNAL ||
            symbol->StorageClass==IMAGE_SYM_CLASS_WEAK_EXTERNAL ||
            symbol->StorageClass==IMAGE_SYM_CLASS_FAR_EXTERNAL)) {
            if(!*fixup)value=incremental_code->rva+symbol_indices[relocation->SymbolTableIndex];
            else report_incremental_addend(symbol,context,contribution);
        }
        absolute=(section==-1);
        absolute_value=absolute ? value : context->optional_header.ImageBase+value;
        if(save_fixups && !absolute)
            record_debug_fixup(relocation->Type,0,site,value);
        switch(relocation->Type) {
        case 1: /* IMAGE_REL_I386_DIR16, accepted only for VxD. */
            if(vxd) {
                target=find_section_by_index(section,&context->sections);
                if(target)value-=target->rva;
                *(WORD *)fixup+=(WORD)value;
            }
            break;
        case 2: /* IMAGE_REL_I386_REL16, accepted only for VxD. */
            if(vxd)*(WORD *)fixup+=(WORD)(value-2-site);
            break;
        case IMAGE_REL_I386_DIR32:
            if(debug) {
                if(absolute) {
                    *((WORD *)(fixup+1))+=(WORD)(context->file_header.NumberOfSections+1);
                    *fixup+=value;
                } else {
                    target=find_section_by_index(section,&context->sections);
                    if(target) {
                        *((WORD *)(fixup+1))+=target->index;
                        *fixup+=value-target->rva;
                    } else *fixup+=value;
                }
            } else if(!vxd || absolute) {
                *fixup+=absolute_value;
                if(!absolute && !fixed)APPEND_BASE_RELOCATION(3,site,section,0);
            } else {
                if(!section)link_contribution_fatal(contribution,1188,
                    output_symbol_name(context,symbol));
                target=find_section_by_index(section,&context->sections);
                target_offset=value-target->rva+*fixup;
                packed_site=((site-contribution->group->section->rva)&0xffffff)|
                    ((DWORD)(SHORT)contribution->group->section->index<<24);
                packed_target=(target_offset&0xffffff)|((DWORD)(SHORT)target->index<<24);
                /* Only the defined low24 bits participate in this signed check;
                 * the original temporary's untouched high BYTE is shifted out. */
                if((DWORD)((LONG)(target_offset<<8)>>8)!=target_offset)
                    link_contribution_fatal(contribution,1157,output_symbol_name(context,symbol));
                if(!fixed)APPEND_BASE_RELOCATION(3,packed_site,section,packed_target);
            }
            break;
        case IMAGE_REL_I386_SECTION:
            if(section>0)*(WORD *)fixup+=(WORD)section;
            else if(absolute)*(WORD *)fixup+=(WORD)(context->file_header.NumberOfSections+1);
            break;
        case IMAGE_REL_I386_SECREL:
            if(!absolute) {
                target=find_section_by_index(section,&context->sections);
                if(target)value-=target->rva;
            }
            *fixup+=value;
            break;
        case IMAGE_REL_I386_DIR32NB:
            *fixup+=value;
            break;
        case IMAGE_REL_I386_REL32:
            if(context->options.write_map)record_relative_fixup(site);
            if(vxd) {
                DWORD relative_site=site-contribution->group->section->rva;
                if(!section)link_contribution_fatal(contribution,1188,
                    output_symbol_name(context,symbol));
                target=find_section_by_index(section,&context->sections);
                target_offset=value-target->rva;
                if(contribution->group->section==target)
                    *fixup+=target_offset-relative_site-4;
                else {
                    packed_site=(relative_site&0xffffff)|
                        ((DWORD)(SHORT)contribution->group->section->index<<24);
                    packed_target=(target_offset&0xffffff)|((DWORD)(SHORT)target->index<<24);
                    if((DWORD)((LONG)(target_offset<<8)>>8)!=target_offset)
                        link_contribution_fatal(contribution,1157,output_symbol_name(context,symbol));
                    if(!fixed)APPEND_BASE_RELOCATION(8,packed_site,section,packed_target);
                }
            } else {
                value-=4+site;
                if(!absolute && context->options.new_relocations &&
                   section!=(SHORT)contribution->group->section->index && !fixed)
                    APPEND_BASE_RELOCATION(7,site,section,0);
                *fixup+=value;
            }
            break;
        }
    }
}
#undef APPEND_BASE_RELOCATION
OutputSection *__stdcall find_section_by_index(SHORT index,SectionList *sections) { __asm { int 3 } return NULL; }
void __stdcall report_incremental_addend(IMAGE_SYMBOL *symbol,LinkContext *context,Contribution *contribution) { __asm { int 3 } }
void __stdcall record_debug_fixup(WORD type,WORD flags,DWORD site,DWORD value) { __asm { int 3 } }
void __stdcall record_relative_fixup(DWORD site) { __asm { int 3 } }
const char *__stdcall output_symbol_name(LinkContext *context,IMAGE_SYMBOL *symbol) { __asm { int 3 } return NULL; }
