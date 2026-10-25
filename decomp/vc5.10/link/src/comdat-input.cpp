/* Genuine StringTable and Contribution member calls require the original
 * thiscall convention. The input reader itself remains a C-linkage stdcall. */
#include <windows.h>
#include <string.h>
#include "input.h"
#include "sections.h"
#include "memory.h"
#include "initializers.h"
#include "full-link-functions.h"
#include <recon.h>

extern "C" {
extern DWORD *section_symbol_ranges;
extern char *current_module_strings;
extern BYTE mac_target,incremental_enabled,incremental_reuse;
extern OutputSection *procedure_section,*architecture_section,*debug_section;
extern OutputModule *output_module;
OutputModule *comdat_symbol_module;
/* Short COFF names are copied here before string operations. Byte8 is the
 * shared zero terminator; the original object begins at47f128. */
char short_symbol_name[9];
/* The folding vector is the existing original buffer4 (494398/9c/a0),
 * initialized and freed by its already reconstructed application callbacks. */
#define fold_candidates ((ContributionInfo **)buffer4.base)
#define fold_candidate_count (buffer4.remaining)
#define fold_candidate_capacity (buffer4.capacity)
char *__stdcall format_contribution_module(Contribution *,char *);
DWORD *__stdcall build_section_symbol_ranges(Contribution *,IMAGE_SYMBOL *);
void __stdcall discard_associated_comdats(Contribution *);
void __stdcall report_duplicate_comdat(LinkOptions *,const char *,const char *,const char *);
void __stdcall record_undefined_comdat(StringNode *,const char *,OutputModule *);
BYTE __stdcall remove_module_symbol(OutputModule *,StringNode *);
ContributionInfo *__stdcall get_contribution_info(Contribution *);

#pragma intrinsic(strlen)
BYTE __stdcall read_comdat_section(LinkContext *context,Contribution *contribution,
    IMAGE_SYMBOL *symbols,DWORD index,char **name)
{
    StringTable *table=context->symbols;
    IMAGE_SYMBOL *section_symbol,*external;
    IMAGE_AUX_SYMBOL *auxiliary;
    StringNode *symbol;
    Contribution *associated=NULL,*previous;
    ContributionInfo *info,**grown;
    char module_name[520],previous_name[520];
    const char *symbol_name;
    DWORD checksum,flags,kind,count,capacity,i;
    WORD name_kind;
    BYTE selection,created=0;
    /* Both reads of the physical DWORD formal consume a signed low WORD.
     * The caller still pushes its complete original DWORD unchanged. */
    SHORT section_index=(SHORT)index;
    if (comdat_symbol_module!=contribution->module) {
        comdat_symbol_module=contribution->module;
        section_symbol_ranges=build_section_symbol_ranges(contribution,symbols);
    }
    if (section_symbol_ranges[(section_index-1)*2]==0xffffffff)
        link_contribution_fatal(contribution,1143,(int)section_index);
    section_symbol=symbols+section_symbol_ranges[(section_index-1)*2];
    if (section_symbol->StorageClass!=IMAGE_SYM_CLASS_STATIC)
        link_contribution_fatal(contribution,1143,(int)section_index);
    if (!section_symbol->NumberOfAuxSymbols)
        link_contribution_fatal(contribution,1162,(int)section_index);
    auxiliary=(IMAGE_AUX_SYMBOL *)(section_symbol+1);
    selection=auxiliary->Section.Selection;
    checksum=auxiliary->Section.CheckSum;
    if (selection==IMAGE_COMDAT_SELECT_ASSOCIATIVE) {
        contribution->comdat_selection=selection;
        previous=(Contribution *)comdat_symbol_module->contributions+
            auxiliary->Section.Number-1;
        contribution->associated_contribution=previous;
        if (previous->characteristics & 0x800) return 0;
        if (contribution->group->section==procedure_section ||
            contribution->group->section==architecture_section)
            associated=previous;
        goto accepted;
    }
    if (section_symbol_ranges[(section_index-1)*2+1]==0xffffffff)
        link_contribution_fatal(contribution,1143,(int)section_index);
    external=symbols+section_symbol_ranges[(section_index-1)*2+1];
    if (external->StorageClass!=IMAGE_SYM_CLASS_STATIC &&
        external->StorageClass!=IMAGE_SYM_CLASS_EXTERNAL &&
        external->StorageClass!=0x44 /* original far-external class */)
        link_contribution_fatal(contribution,1143,(int)section_index);
    if (!external->N.Name.Short) {
        /* get_symbol_name writes only WORD0; the original subsequently passes
         * the containing DWORD to a member that consumes only its low WORD. */
        symbol_name=get_symbol_name((StringNode *)external,current_module_strings,
            &name_kind,NULL);
        kind=name_kind;
    } else {
        symbol_name=get_symbol_name((StringNode *)external,NULL,&name_kind,NULL);
        if (name_kind==1) symbol_name=strncpy(short_symbol_name,symbol_name,8);
        kind=strlen(symbol_name)<=8;
    }
    *name=(char *)symbol_name;
    if (external->StorageClass==IMAGE_SYM_CLASS_STATIC) goto accepted;
    symbol=table->LookupSymbol((WORD)kind,symbol_name,&created);
    if (mac_target && (symbol->attributes & 0x800)) {
        created=1;
        symbol->attributes &= ~0x800;
    }
    if (incremental_reuse) {
        if (symbol->flags & 0x100) {
            created=1;
            symbol->reserved28=0; /* offset28: role not yet consumed. */
            symbol->undefined_previous=NULL;
            symbol->undefined_next=NULL;
            symbol->flags=1;
            set_symbol_defined(symbol,0,context->symbols);
            if (mac_target) symbol->attributes &= 0xffbe;
        }
        if (!created && !(symbol->flags & 0x60000)) {
            previous=symbol->contribution;
            if (symbol->flags & 1) {
                if (selection!=previous->comdat_selection) {
                    if (selection!=6 && previous->comdat_selection!=6) {
                        incremental_status_code=26;
                        return 0;
                    }
                } else {
                    if ((selection==3 || selection==4) &&
                        checksum!=previous->comdat_checksum) {
                        incremental_status_code=26;
                        return 0;
                    }
                    return 0;
                }
            } else if (symbol->flags & 0x20) {
                if (selection!=previous->comdat_selection) {
                    if (selection!=6 && previous->comdat_selection!=6) {
                        incremental_status_code=26;
                        return 0;
                    }
                } else {
                    if ((selection==3 || selection==4) &&
                        checksum!=previous->comdat_checksum) {
                        incremental_status_code=26;
                        return 0;
                    }
                    contribution->comdat_checksum=checksum;
                    contribution->comdat_selection=selection;
                    goto accepted;
                }
            }
        }
        if (created && (external->Type & 0x30)==0x20 &&
            (contribution->module->library->flags & 16)) symbol->flags |= 0x20000;
    }
    flags=symbol->flags;
    if (flags & 0x20) {
        if (!(flags & 1)) link_contribution_fatal(contribution,1179,symbol_name);
        previous=symbol->contribution;
        if (selection!=previous->comdat_selection && selection!=6 &&
            previous->comdat_selection!=6) goto duplicate;
        switch (previous->comdat_selection) {
        case 1 /* no duplicates */:
            goto duplicate;
        case 2 /* any */:
            if (selection!=6 /* largest */) return 0;
            break;
        case 3 /* same size */:
        case 4 /* exact match */:
            if (checksum==previous->comdat_checksum) return 0;
            goto duplicate;
        case IMAGE_COMDAT_SELECT_ASSOCIATIVE:
            return 0;
        case 6 /* largest */:
            if (contribution->raw_size-contribution->padding<=
                previous->raw_size-previous->padding) return 0;
            break;
        default:
            link_contribution_fatal(contribution,1163,(int)(SHORT)index);
            return 0;
        }
        if (incremental_enabled) {
            if (incremental_reuse) {
                incremental_status_code=26;
                return 0;
            }
            remove_module_symbol(previous->module,symbol);
        }
        symbol->contribution->characteristics |= 0x800;
        discard_associated_comdats(symbol->contribution);
        set_symbol_defined(symbol,0,table);
    } else if (flags & 2) {
        if (incremental_enabled) {
            symbol_name=get_symbol_name(symbol,context->symbols->hash.string_storage,
                &name_kind,NULL);
            if (name_kind==1) symbol_name=strncpy(short_symbol_name,symbol_name,8);
            record_undefined_comdat(symbol,symbol_name,comdat_symbol_module);
        }
        symbol->flags &= ~2U;
        set_symbol_defined(symbol,0,table);
    } else if (flags & 1) return 0;
    contribution->comdat_checksum=checksum;
    contribution->comdat_selection=selection;
    symbol->flags |= 0x20;
    if (incremental_enabled && (symbol->flags & 0x40000)) symbol->flags &= ~0x40000U;
accepted:
    if (!context->options.fold_identical_comdats) return 1;
    if (associated) {
        info=get_contribution_info(associated);
        info->fold_candidate=NULL;
        i=fold_candidate_count;
        while (i) {
            --i;
            if (fold_candidates[i]==info) {
                fold_candidates[i]=NULL;
                break;
            }
        }
    } else if (contribution->CanFold(*name)) {
        info=get_contribution_info(contribution);
        info->fold_candidate=contribution;
        count=fold_candidate_count+1;
        if (count>fold_candidate_capacity) {
            capacity=fold_candidate_capacity*3/2;
            if (capacity<count) capacity=count;
            grown=(ContributionInfo **)crt_operator_new(capacity*4);
            if (!grown) return 1;
            if (fold_candidates) {
                for (i=0;i<fold_candidate_count;++i)
                    grown[i]=fold_candidates[i];
                crt_operator_delete(fold_candidates);
            }
            buffer4.base=grown;
            fold_candidate_capacity=capacity;
        }
        fold_candidate_count=count;
        fold_candidates[count-1]=info;
    }
    return 1;
duplicate:
    format_module_name(comdat_symbol_module,module_name);
    format_contribution_module(symbol->contribution,previous_name);
    report_duplicate_comdat(&context->options,module_name,symbol_name,previous_name);
    return 0;
}
DWORD *__stdcall build_section_symbol_ranges(Contribution *contribution,IMAGE_SYMBOL *symbols)
{
    OutputModule *module=contribution->module;
    DWORD *ranges=(DWORD *)allocate_memory(module->section_count*8);
    DWORD index;
    SHORT section;
    for (index=0;index<module->section_count;++index) {
        ranges[index*2]=0xffffffff;
        ranges[index*2+1]=0xffffffff;
    }
    for (index=0;index<module->symbol_count;) {
        section=symbols->SectionNumber;
        if (section>0) {
            if (ranges[(section-1)*2]==0xffffffff) ranges[(section-1)*2]=index;
            else if (ranges[(section-1)*2+1]==0xffffffff) ranges[(section-1)*2+1]=index;
        }
        index+=symbols->NumberOfAuxSymbols+1;
        symbols+=symbols->NumberOfAuxSymbols+1;
    }
    return ranges;
}
void __stdcall discard_associated_comdats(Contribution *parent) { __asm { int 3 } }
void __stdcall report_duplicate_comdat(LinkOptions *options,const char *module,const char *symbol,const char *previous) { __asm { int 3 } }
void __stdcall record_undefined_comdat(StringNode *symbol,const char *name,OutputModule *module) { __asm { int 3 } }
BYTE __stdcall remove_module_symbol(OutputModule *module,StringNode *symbol) { __asm { int 3 } return 0; }
}
BYTE RECON_THISCALL Contribution::CanFold(const char *name)
{
    if (group->section==debug_section || (module->flags & 0x40) || !raw_size ||
        module==output_module || !(characteristics & IMAGE_SCN_MEM_EXECUTE)) return 0;
    if (name && !strncmp("_$E",name,3)) return 0;
    return !(characteristics & IMAGE_SCN_MEM_WRITE) ||
        (characteristics & IMAGE_SCN_CNT_CODE);
}
