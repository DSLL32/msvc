/* Mac PowerPC COFF relocation bookkeeping; emitted PPC is never host code. */
#include <windows.h>
#include <string.h>
#include "input.h"
#include "initializers.h"
#include "comdat.h"
#include "sections.h"
#include "full-link-functions.h"
#include "debug-output.h"
#include "weak-symbols.h"
extern "C" __declspec(dllimport) DWORD __cdecl SigForPbCb(const void *,long,DWORD);
#include <recon.h>

#pragma intrinsic(strlen,strcpy,strcat)

extern "C" {
extern char *current_module_strings;
extern char short_symbol_name[9];
extern BYTE incremental_reuse,mac_file_padding;
extern DWORD mac_relocation_count,mac_toc_entry_count;
ContributionInfo *__stdcall get_contribution_info(Contribution *);
void __stdcall record_relocation_reference(LinkContext *,Contribution *,IMAGE_SYMBOL *,IMAGE_RELOCATION *);
StringNode *__stdcall record_incremental_entry(const char *,Contribution *,LinkContext *,DWORD);
void __stdcall prepare_mac_incremental_entry(StringNode *,LinkContext *);
void __stdcall append_mac_map_symbol(StringNode *);
BOOL __stdcall mark_symbol_bit(DWORD *,DWORD);
BOOL __stdcall test_symbol_bit(DWORD *,DWORD) { __asm { int 3 } return 0; }
BYTE __stdcall classify_mac_ppc_relocation(Contribution *contribution,WORD type,
    DWORD physical_flag,BYTE unused)
{
    switch(type) {
    case 1: /* LCALL: skipped by the reader, rejected on the later fixup pass. */
        if((BYTE)physical_flag)link_contribution_fatal(contribution,1187,"LCALL");
        return 1;
    case 0:case 2:case 3:case 4:case 5:case 6:
    case 10:case 11:case 12:case 13:case 14:case 15:case 19:
    case 34:case 37:case 40:case 41:case 42:
        return 0;
    default:
        link_contribution_fatal(contribution,1190,(DWORD)type);
        return 0;
    }
}
struct MacImportSymbolNode;
extern Contribution *incremental_code,*mac_jump_contribution;
extern ContributionGroup *mac_procedure_group;
extern StringNode *mac_toc_symbol;
extern DWORD mac_code_base;
extern BYTE incremental_enabled;
/*4944b8 is a DWORD import owner whose low byte is also tested/cleared by
 * the original incremental .pdata path. Clearing it does not clear its tail. */
DWORD mac_procedure_import_symbol;
void __stdcall report_incremental_addend(IMAGE_SYMBOL *,LinkContext *,Contribution *);
void __stdcall record_debug_fixup(WORD,WORD,DWORD,DWORD);
void __stdcall append_mac_relocation(DWORD,DWORD,DWORD);
DWORD __stdcall mac_section_relative_address(DWORD,BYTE *);
const char *__stdcall output_symbol_name(LinkContext *,IMAGE_SYMBOL *);
void __cdecl link_contribution_error(Contribution *,unsigned,...);
void __cdecl link_contribution_warning(Contribution *,unsigned,...);
void __stdcall count_relocation_error(LinkContext *);
OutputSection *__stdcall find_section_by_index(SHORT,SectionList *);
MacImportSymbolNode *__stdcall find_mac_import_symbol(LinkContext *,StringNode *);
IMAGE_SYMBOL *__stdcall create_incremental_symbol(LinkContext *,Contribution *,DWORD,IMAGE_SYMBOL *,BYTE);
void __stdcall write_mac_entry(const char *,StringNode *,DWORD,LinkContext *,DWORD);
StringNode *__stdcall lookup_mac_entry_symbol(const char *,LinkContext *,DWORD);
BYTE __stdcall is_mac_toc_symbol(LinkContext *,IMAGE_SYMBOL *) { __asm { int 3 } return 0; }
DWORD __stdcall get_mac_indirect_glue(DWORD,OutputModule *,IMAGE_SYMBOL *,LinkContext *)
{ __asm { int 3 } return 0; }
void __stdcall write_mac_toc_slot(LinkContext *,OutputModule *,IMAGE_SYMBOL *,DWORD,DWORD,StringNode *)
{ __asm { int 3 } }
void __stdcall emit_mac_import_glue(DWORD,LinkContext *);
extern Contribution *mac_glue_contribution;
extern DWORD mac_data_base;
extern BYTE pdb_enabled;
void __stdcall pdb_record_output_symbol(LinkContext *,const char *,WORD,StringNode *);
DWORD __stdcall resolve_mac_code_symbol(DWORD index,OutputModule *module,LinkContext *context)
{
    StringNode *symbol=(StringNode *)module->symbol_indices[index],*fallback=NULL,*glue;
    MacImportSymbolNode *imported;
    DWORD displacement;
    const char *name;
    char *glue_name;
    WORD kind;
    BYTE created;
    if(!symbol || !(symbol->attributes&2))return 0;
    if(!(symbol->attributes&8)) {
        imported=find_mac_import_symbol(context,symbol);
        if(!imported) {
            fallback=find_weak_default(symbol);
            if(fallback) {
                imported=find_mac_import_symbol(context,fallback);
                if(fallback->attributes&1)symbol->attributes|=1;
                if(fallback->attributes&0x200)symbol->attributes|=0x200;
                if(fallback->attributes&8) {
                    symbol->attributes|=8;
                    symbol->mac_glue_address=fallback->mac_glue_address;
                }
            }
        }
        displacement=(DWORD)(LONG)(SHORT)symbol->reserved36;
        name=get_symbol_name(fallback?fallback:symbol,context->symbols->hash.string_storage,&kind,NULL);
        if(kind==1)name=strncpy(short_symbol_name,name,8);
        glue_name=(char *)allocate_memory(strlen(name)+8);
        strcpy(glue_name,"__glue_");strcat(glue_name,name);
        created=0;
        glue=context->symbols->Lookup((const unsigned char *)glue_name,&created);
        if(created) {
            set_symbol_defined(glue,1,context->symbols);
            glue->contribution=mac_glue_contribution;
            glue->symbol.Value=context->mac_glue_bytes_used;
            emit_mac_import_glue(displacement,context);
        }
        glue->final_value=glue->symbol.Value+glue->contribution->rva;
        symbol->mac_glue_address=glue->final_value;
        symbol->attributes|=8;
        if(fallback) {
            fallback->attributes|=8;
            fallback->mac_glue_address=symbol->mac_glue_address;
        }
        if(pdb_enabled && context->options.debug_level)
            pdb_record_output_symbol(context,glue_name,
                (WORD)(context->file_header.NumberOfSections+1),glue);
        release_memory(glue_name);
        if((symbol->attributes&0x10) && !(symbol->attributes&1) &&
           !(symbol->attributes&0x200)) {
            if(!imported)link_fatal(NULL,1548,name);
            append_mac_relocation(mac_toc_symbol->final_value+displacement-mac_data_base,
                8,(DWORD)imported);
            symbol->attributes|=0x201;
            if(fallback)fallback->attributes|=0x201;
        }
    }
    return symbol->mac_glue_address;
}
}

/* These name operations and fixup diagnostics were inline in the real owner.
 * Mac p-code entry names are resolved through the ordinary symbol table. */
#define MAC_SYMBOL_NAME(node) do { \
    name=get_symbol_name((node),context->symbols->hash.string_storage,&kind,NULL); \
    if(kind==1)name=strncpy(short_symbol_name,name,8); \
} while(0)
#define MAC_PCODE_TARGET(prefix,local_flag) do { \
    if(external) { \
        named=mapped->symbol.StorageClass==IMAGE_SYM_CLASS_WEAK_EXTERNAL ? \
            find_weak_default(mapped):mapped; \
        MAC_SYMBOL_NAME(named); \
        buffer=(char *)allocate_memory(strlen(name)+6); \
        strcpy(buffer,(prefix));strcat(buffer,name); \
        named=context->symbols->FindSymbol(buffer);release_memory(buffer); \
        value=named->symbol.Value+named->contribution->rva; \
    } else value=create_incremental_symbol(context,contribution,0,symbol,(local_flag))->Value; \
} while(0)
#define MAC_FIXUP_ERROR(message) do { \
    link_contribution_error(contribution,(message),output_symbol_name(context,symbol)); \
    count_relocation_error(context); \
} while(0)
#define MAC_IMPORT_OWNER() do { \
    imported=find_mac_import_symbol(context,mapped); \
    if(!imported) { \
        named=find_weak_default(mapped); \
        if(named)imported=find_mac_import_symbol(context,named); \
        if(!imported) { MAC_SYMBOL_NAME(mapped);link_fatal(NULL,1548,name); } \
    } \
} while(0)

extern "C" void __stdcall process_mac_ppc_relocations(Contribution *contribution,
    IMAGE_RELOCATION *relocations,DWORD count,BYTE *contents,IMAGE_SYMBOL *symbols,
    LinkContext *context,DWORD *symbol_indices)
{
    DWORD contribution_rva=contribution->rva;
    BYTE debug=contribution->group->section==debug_section;
    BYTE save_fixups=(context->options.debug_formats&8) && !debug;
    BYTE procedure=contribution->group==mac_procedure_group;
    OutputModule *module=contribution->module;
    while(count) {
        IMAGE_RELOCATION *relocation=relocations;
        IMAGE_SYMBOL *symbol;
        StringNode *mapped,*named,*descriptor;
        MacImportSymbolNode *imported;
        OutputSection *target;
        DWORD input_rva,index,site,value,absolute_value,special,word,offset,mask,glue,relative;
        BYTE *fixup;
        BYTE absolute,external,pcode,import_descriptor,created_descriptor,record_descriptor,is_code;
        SHORT section;
        WORD kind;
        const char *name;
        char *buffer;
        if(classify_mac_ppc_relocation(contribution,relocation->Type,1,0))goto next;
        input_rva=contribution->module->section_map[
            (LONG)((DWORD)contribution-(DWORD)contribution->module->contributions)>>6].virtual_address;
        site=contribution_rva-input_rva+relocation->VirtualAddress;
        fixup=contents-input_rva+relocation->VirtualAddress;
        index=relocation->SymbolTableIndex;
        symbol=symbols+index;
        section=symbol->SectionNumber;
        value=symbol->Value;
        if(incremental_enabled && !debug && symbol_indices[index] &&
           (symbol->StorageClass==IMAGE_SYM_CLASS_EXTERNAL ||
            symbol->StorageClass==IMAGE_SYM_CLASS_WEAK_EXTERNAL ||
            symbol->StorageClass==IMAGE_SYM_CLASS_FAR_EXTERNAL)) {
            word=*(DWORD *)fixup;
            if(!procedure && word && word!=0x1000048 && word!=0x48 && relocation->Type!=4) {
                if(!mac_procedure_group)report_incremental_addend(symbol,context,contribution);
            } else if(!procedure || relocation->Type==5)
                value=incremental_code->rva+symbol_indices[index];
        }
        absolute=section==IMAGE_SYM_ABSOLUTE;
        absolute_value=absolute?value:value+context->optional_header.ImageBase;
        special=0;
        if(save_fixups && !absolute) {
            if(value==mac_toc_symbol->final_value && is_mac_toc_symbol(context,symbol))special=1;
            switch(relocation->Type) {
            case 3:case 12:case 13:case 19:case 42:
                record_debug_fixup(relocation->Type,(WORD)special,site,value);
            }
        }
        mapped=(StringNode *)module->symbol_indices[index];
        external=symbol->StorageClass==IMAGE_SYM_CLASS_EXTERNAL ||
            symbol->StorageClass==IMAGE_SYM_CLASS_WEAK_EXTERNAL ||
            symbol->StorageClass==IMAGE_SYM_CLASS_FAR_EXTERNAL;
        pcode=external?(mapped && (mapped->symbol.Type&0x8000))!=0:
            (symbol->Type&0x8000)!=0;
        import_descriptor=created_descriptor=record_descriptor=0;
        switch(relocation->Type) {
        case 2: /* DATAREL */
            import_descriptor=symbol->StorageClass==IMAGE_SYM_CLASS_EXTERNAL &&
                mapped && (mapped->attributes&2);
            if(!import_descriptor) {
                if(save_fixups && !absolute)record_debug_fixup(2,(WORD)special,site,value);
                relative=mac_section_relative_address(value,&is_code);
                *(DWORD *)fixup=byte_swap_dword(byte_swap_dword(*(DWORD *)fixup)+relative);
            }
            if(incremental_reuse && (procedure || (module->flags&4)))break;
            relative=mac_section_relative_address(site,NULL);
            if(import_descriptor) {
                MAC_IMPORT_OWNER();
                append_mac_relocation(relative,8,(DWORD)imported);
            } else append_mac_relocation(relative,1+(is_code!=0),0);
            break;
        case 3: /* JMPADDR */
            *(DWORD *)fixup=byte_swap_dword(byte_swap_dword(*(DWORD *)fixup)+value-mac_code_base);
            break;
        case 5: /* DATADESCRREL */
            MAC_SYMBOL_NAME(mapped);
            if(pcode)MAC_PCODE_TARGET("__nep",0);
            value-=mac_code_base;
            MAC_SYMBOL_NAME(mapped);
            if(mapped->attributes&0x80) {
                write_mac_entry(name,mapped,value,context,1);descriptor=mapped;
            } else {
                write_mac_entry(name,mapped,value,context,0);
                if(mapped->attributes&2)import_descriptor=1;
                else {
                    MAC_SYMBOL_NAME(mapped);
                    descriptor=lookup_mac_entry_symbol(name,context,1);
                }
            }
            if(import_descriptor) {
                MAC_IMPORT_OWNER();
                if(incremental_reuse && (procedure || (module->flags&4))) {
                    if(procedure && !(BYTE)mac_procedure_import_symbol)
                        mac_procedure_import_symbol=(DWORD)imported;
                    break;
                }
                relative=mac_section_relative_address(site,NULL);
                append_mac_relocation(relative,8,(DWORD)imported);
                break;
            }
            value=descriptor->symbol.Value+mac_jump_contribution->rva;
            if(save_fixups && !absolute)record_debug_fixup(5,0,site,value);
            relative=mac_section_relative_address(value,NULL);
            *(DWORD *)fixup=byte_swap_dword(byte_swap_dword(*(DWORD *)fixup)+relative);
            if(incremental_reuse && (procedure || (module->flags&4))) {
                if(procedure)mac_procedure_import_symbol&=0xffffff00;
                break;
            }
            relative=mac_section_relative_address(site,&is_code);
            append_mac_relocation(relative,1+(is_code!=0),0);
            break;
        case 4: /* CREATEDESCRREL, followed by TOCREL */
            if(pcode)MAC_PCODE_TARGET("__nep",0);
            if(!(mapped->attributes&2)) {
                record_descriptor=(mapped->attributes&0x80)!=0;
                MAC_SYMBOL_NAME(mapped);
                value-=mac_code_base;
                write_mac_entry(name,mapped,value,context,record_descriptor);
            }
            if(!count || relocation[1].Type!=6)
                link_contribution_fatal(contribution,1187,"IMAGE_REL_MPPC_CREATEDESCRREL");
            created_descriptor=1;
            /* The target shares this TOC path with standalone kind6. */
        case 6:
            if(test_symbol_bit(module->symbol_bits[0],index))offset=(DWORD)(LONG)(SHORT)mapped->reserved36;
            else {
                word=module->symbol_indices[index]*4;
                offset=(DWORD)(LONG)(SHORT)((LONG)word<0x8000?word:0x7ffc-word);
            }
            fixup[0]=(BYTE)((LONG)offset>>8);fixup[1]=(BYTE)offset;
            if(save_fixups && !absolute)
                record_debug_fixup(6,0,site,mac_toc_symbol->final_value+offset);
            if(created_descriptor && !record_descriptor) {
                ++relocations;--count;
                index=relocations->SymbolTableIndex;
                write_mac_toc_slot(context,module,symbols,index,module->symbol_indices[index],mapped);
            } else {
                write_mac_toc_slot(context,module,symbols,index,(DWORD)mapped,mapped);
                if(created_descriptor) { ++relocations;--count; }
            }
            break;
        case 12: /* ADDR24: the original reads the raw word before its final swap. */
            if(!absolute) { MAC_FIXUP_ERROR(2009);break; }
            if(absolute_value&3) { MAC_FIXUP_ERROR(2008);absolute_value&=~3; }
            word=*(DWORD *)fixup;
            offset=word&0x3fffffc;
            if(offset&0x2000000)offset|=0xfc000000;
            offset+=absolute_value;
            mask=offset&0xfe000000;
            if(mask && mask!=0xfe000000)MAC_FIXUP_ERROR(2013);
            *(DWORD *)fixup=byte_swap_dword((word&~0x3fffffc)|(offset&0x3fffffc));
            break;
        case 13: /* ADDR14 */
            if(!absolute) { MAC_FIXUP_ERROR(2009);break; }
            if(absolute_value&3) { MAC_FIXUP_ERROR(2008);absolute_value&=~3; }
            word=byte_swap_dword(*(DWORD *)fixup);
            offset=(DWORD)(LONG)(SHORT)(word&0xfffc);
            offset+=absolute_value;
            if((LONG)offset>0x7fff || (LONG)offset< -0x8000)MAC_FIXUP_ERROR(2013);
            *(DWORD *)fixup=byte_swap_dword((word&~0xfffc)|(offset&0xfffc));
            break;
        case 14:case 15: /* REL24 / REL14 */
            if(pcode)MAC_PCODE_TARGET("__nep",0);
            else {
                glue=resolve_mac_code_symbol(index,module,context);
                if(glue) {
                    link_contribution_warning(contribution,4100,output_symbol_name(context,symbol));
                    value=glue;
                }
            }
            if(save_fixups && !absolute)record_debug_fixup(relocation->Type,0,site,value);
            if(value&3) { MAC_FIXUP_ERROR(2008);value&=~3; }
            value-=site;
            word=byte_swap_dword(*(DWORD *)fixup);
            if(relocation->Type==14) {
                offset=word&0x3fffffc;
                if(offset&0x2000000)offset|=0xfc000000;
                offset+=value;
                mask=offset&0xfe000000;
                if(mask && mask!=0xfe000000)MAC_FIXUP_ERROR(2013);
                word=(word&~0x3fffffc)|(offset&0x3fffffc);
            } else {
                offset=(DWORD)(LONG)(SHORT)(word&0xfffc);
                offset+=value;
                if((LONG)offset>0x7fff || (LONG)offset< -0x8000)MAC_FIXUP_ERROR(2013);
                word=(word&~0xfffc)|(offset&0xfffc);
            }
            *(DWORD *)fixup=byte_swap_dword(word);
            break;
        case 19: /* CV records retain little-endian DWORD/WORD fields. */
            if(absolute) {
                *(WORD *)(fixup+4)+=context->file_header.NumberOfSections+1;
                *(DWORD *)fixup+=value;
            } else {
                target=find_section_by_index(symbol->SectionNumber,&context->sections);
                if(target) {
                    value-=target->rva;
                    *(WORD *)(fixup+4)+=target->index;
                }
                *(DWORD *)fixup+=value;
            }
            break;
        case 34: /* TOCINDIRCALL */
            glue=get_mac_indirect_glue(index,module,symbols,context);
            if(save_fixups && !absolute)record_debug_fixup(34,0,site,glue);
            offset=(glue-site)&~3;
            if((LONG)offset!=((LONG)(offset<<6)>>6))MAC_FIXUP_ERROR(2013);
            word=byte_swap_dword(*(DWORD *)fixup);
            *(DWORD *)fixup=byte_swap_dword(word|(offset&0x3fffffc));
            if(*(DWORD *)(fixup+4)!=0x60) {
                link_contribution_error(contribution,2012,"TOCINDIRCALL",output_symbol_name(context,symbol));
                count_relocation_error(context);break;
            }
            *(DWORD *)(fixup+4)=byte_swap_dword(0x80410014);
            break;
        case 37: /* TOCCALLREL, followed by its LCALL marker */
            if(pcode) { MAC_PCODE_TARGET("__nep",0);glue=0; }
            else glue=resolve_mac_code_symbol(index,module,context);
            if(save_fixups && !absolute)record_debug_fixup(37,0,site,glue?glue:value);
            if(!glue) {
                if(pcode)mask=0x3ffffff;
                else {
                    if(value&3) { MAC_FIXUP_ERROR(2008);value&=~3; }
                    mask=0x3fffffc;
                }
                offset=value-site;
                word=byte_swap_dword(*(DWORD *)fixup);
                if((word&0xfc000000)==0x48000000) {
                    if((LONG)offset!=((LONG)(offset<<6)>>6))MAC_FIXUP_ERROR(2013);
                    word|=offset&mask;
                } else {
                    if((LONG)offset!=(LONG)(SHORT)offset)MAC_FIXUP_ERROR(2013);
                    word|=offset&0xfffc;
                }
                *(DWORD *)fixup=byte_swap_dword(word);
            } else {
                offset=glue-site;
                word=byte_swap_dword(*(DWORD *)fixup);
                if((word&0xfc000000)==0x48000000) {
                    if((LONG)offset!=((LONG)(offset<<6)>>6))MAC_FIXUP_ERROR(2013);
                    *(DWORD *)fixup=byte_swap_dword(word|(offset&0x3fffffc));
                    if(*(DWORD *)(fixup+4)!=0x60) {
                        link_contribution_error(contribution,2012,"TOCCALLREL",output_symbol_name(context,symbol));
                        count_relocation_error(context);
                    } else *(DWORD *)(fixup+4)=byte_swap_dword(0x80410014);
                } else if((LONG)offset!=(LONG)(SHORT)offset)MAC_FIXUP_ERROR(2013);
            }
            if(!count || relocation[1].Type!=1)
                link_contribution_fatal(contribution,1187,"TOCCALLREL");
            ++relocations;--count;
            break;
        case 40:case 41: /* PCODECALL / PCODECALLTONATIVE */
            word=byte_swap_dword(*(DWORD *)fixup);
            record_descriptor=(BYTE)((word>>28)&1);
            if(pcode) {
                if(record_descriptor)MAC_PCODE_TARGET("__nep",0);
                else MAC_PCODE_TARGET("__fh",1);
            } else {
                word|=0x10000000;
                if(external) {
                    value=resolve_mac_code_symbol(index,module,context);
                    if(!value)value=mapped->symbol.Value+mapped->contribution->rva;
                } else value=symbol->Value;
            }
            if(save_fixups && !absolute)record_debug_fixup(relocation->Type,0,site,value);
            offset=value-site;
            if((LONG)offset!=((LONG)(offset<<6)>>6))MAC_FIXUP_ERROR(2013);
            *(DWORD *)fixup=byte_swap_dword(word|(offset&0x3ffffff));
            break;
        case 42: /* PCODENEPE */
            if(site-(DWORD)fixup[0]*4-symbol->Value+8==0)fixup[0]-=2;
            break;
        }
next:
        ++relocations;--count;
    }
}
#undef MAC_IMPORT_OWNER
#undef MAC_FIXUP_ERROR
#undef MAC_PCODE_TARGET
#undef MAC_SYMBOL_NAME

extern "C" {

DWORD __stdcall read_mac_ppc_relocations(LinkContext *context,OutputModule *module,
    Contribution *contribution,IMAGE_SYMBOL *symbols,IMAGE_RELOCATION *relocations,
    DWORD count,DWORD index)
{
    ContributionInfo *info=get_contribution_info(contribution);
    BYTE folding=context->options.fold_identical_comdats && buffer4.remaining &&
        ((ContributionInfo **)buffer4.base)[buffer4.remaining-1]==info;
    DWORD checksum=0,symbol_index,bytes;
    IMAGE_RELOCATION *relocation=relocations;
    IMAGE_SYMBOL *raw;
    StringNode *symbol;
    const char *name;
    WORD kind;
    BYTE descriptor,skip_next;
    for(;count;--count,++relocation) {
        descriptor=skip_next=0;
        if(classify_mac_ppc_relocation(contribution,relocation->Type,0,0))continue;
        raw=symbols+relocation->SymbolTableIndex;
        if(raw->StorageClass==IMAGE_SYM_CLASS_UNDEFINED_STATIC) {
            name=get_symbol_name((StringNode *)raw,current_module_strings,&kind,NULL);
            if(kind==1)name=strncpy(short_symbol_name,name,8);
            link_contribution_fatal(contribution,1186,name);
        }
        if(context->options.effective_reference_opt || context->options.fold_identical_comdats)
            record_relocation_reference(context,contribution,symbols,relocation);
        if(folding) {
            checksum=SigForPbCb(&relocation->Type,2,checksum);
            checksum=SigForPbCb(&relocation->VirtualAddress,4,checksum);
            checksum=SigForPbCb(&raw->StorageClass,1,checksum);
        }
        switch((BYTE)relocation->Type) {
        case 2:
            ++mac_relocation_count;
            break;
        case 5:
            ++mac_relocation_count;
            descriptor=1;
            /* The target shares this function-entry path with kind4. */
        case 4:
            symbol_index=relocation->SymbolTableIndex;
            symbol=(StringNode *)module->symbol_indices[symbol_index];
            if(symbol) {
                if(!(symbol->attributes&0x100)) {
                    name=get_symbol_name(symbol,context->symbols->hash.string_storage,&kind,NULL);
                    if(kind==1)name=strncpy(short_symbol_name,name,8);
                    record_incremental_entry(name,contribution,context,0);
                    symbol->attributes|=0x100;
                }
                if(incremental_reuse) {
                    symbol->attributes&=~0x40;
                    if(descriptor)prepare_mac_incremental_entry(symbol,context);
                }
            } else {
                name=get_symbol_name((StringNode *)(symbols+symbol_index),
                    current_module_strings,&kind,NULL);
                if(kind==1)name=strncpy(short_symbol_name,name,8);
                symbol=record_incremental_entry(name,contribution,context,1);
                module->symbol_indices[symbol_index]=(DWORD)symbol;
                symbol->attributes|=0x80;
                mark_symbol_bit(module->symbol_bits[0],symbol_index);
                if(incremental_reuse)symbol->attributes&=~0x40;
            }
            if(descriptor)break;
            skip_next=1;
            /* Non-descriptor kind4 also enters the TOC reference path. */
        case 6:
            symbol_index=relocation->SymbolTableIndex;
            if(test_symbol_bit(module->symbol_bits[0],symbol_index)) {
                symbol=(StringNode *)module->symbol_indices[symbol_index];
                if(!(symbol->attributes&0x10)) {
                    bytes=mac_toc_entry_count*4;
                    symbol->reserved36=(WORD)(bytes<0x8000?bytes:0x7ffc-bytes);
                    ++mac_toc_entry_count;
                    ++mac_relocation_count;
                    symbol->attributes|=0x10;
                    if(context->options.write_map)append_mac_map_symbol(symbol);
                }
            } else if(!mark_symbol_bit(module->symbol_bits[1],symbol_index)) {
                module->symbol_indices[symbol_index]=mac_toc_entry_count;
                ++mac_toc_entry_count;
                ++mac_relocation_count;
            }
            if(skip_next) { ++relocation;--count; }
            break;
        case 40:case 41:case 42:
            mac_file_padding=0;
            break;
        }
    }
    if(folding) {
        checksum=SigForPbCb(&contribution->characteristics,4,checksum);
        checksum=SigForPbCb(&contribution->comdat_selection,1,checksum);
        ((ContributionInfo **)buffer4.base)[buffer4.remaining-1]->fold_signature=checksum;
    }
    return 0;
}
}
