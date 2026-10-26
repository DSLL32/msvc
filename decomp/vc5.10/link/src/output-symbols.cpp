#include <windows.h>
#include <string.h>
#include "input.h"
#include "full-link-functions.h"
#include "sections.h"
#include "debug-output.h"
#include <recon.h>

extern "C" {
extern BYTE incremental_enabled,pdb_enabled,mapped_debug;
extern char short_symbol_name[9],*current_module_strings;
extern int output_file;
extern OutputModule *output_module;
extern DWORD emitted_symbol_count;
void __stdcall process_file_output_symbol(LinkContext *,IMAGE_SYMBOL *,DWORD,DWORD *);
void __stdcall process_local_output_symbol(LinkContext *,IMAGE_SYMBOL *,OutputModule *);
StringNode *__stdcall process_external_output_symbol(LinkContext *,IMAGE_SYMBOL *);
void __stdcall process_section_output_symbol(LinkContext *,IMAGE_SYMBOL *,OutputModule *);
void __stdcall process_other_output_symbol(LinkContext *,IMAGE_SYMBOL *);
void __stdcall process_aux_output_symbols(IMAGE_SYMBOL *,IMAGE_SYMBOL **,BYTE,DWORD *);
void __stdcall pdb_record_output_symbol(LinkContext *,const char *,WORD,StringNode *);
void __stdcall write_coff_symbol(int,IMAGE_SYMBOL *);
BYTE __stdcall is_discardable_comdat(Contribution *);

void __stdcall process_output_symbol(LinkContext *context,OutputModule *module,
    IMAGE_SYMBOL *raw,IMAGE_SYMBOL **cursor,DWORD *index,DWORD *incremental_index,
    DWORD output_state)
{
    Contribution *contribution=NULL;
    StringNode *external=NULL;
    DWORD replacement_value=0xffffffff;
    SHORT section=raw->SectionNumber;
    BYTE discard=0,emitted=0;
    if(section>0) {
        contribution=(Contribution *)module->contributions+section-1;
        if(contribution && (contribution->characteristics & 0x4000)) {
            discard=1;
            while(contribution && (contribution->characteristics & 0x4000))
                contribution=contribution->associated_contribution;
        }
        if((contribution->characteristics & 0x800) ||
           (context->options.effective_reference_opt && is_discardable_comdat(contribution)))
            discard=1;
    }
    switch(raw->StorageClass) {
    case IMAGE_SYM_CLASS_FILE:
        process_file_output_symbol(context,raw,*index,&replacement_value);
        break;
    case IMAGE_SYM_CLASS_STATIC:
    case IMAGE_SYM_CLASS_LABEL:
    case IMAGE_SYM_CLASS_UNDEFINED_LABEL:
    case IMAGE_SYM_CLASS_UNDEFINED_STATIC:
        process_local_output_symbol(context,raw,module);
        break;
    case IMAGE_SYM_CLASS_EXTERNAL:
    case IMAGE_SYM_CLASS_FAR_EXTERNAL:
    case IMAGE_SYM_CLASS_WEAK_EXTERNAL:
        external=process_external_output_symbol(context,raw);
        if(incremental_enabled)*incremental_index=external->reserved28;
        if((BYTE)output_state && pdb_enabled) {
            WORD kind;
            const char *name=get_symbol_name((StringNode *)raw,
                context->symbols->hash.string_storage,&kind,NULL);
            if(kind==1)name=strncpy(short_symbol_name,name,8);
            pdb_record_output_symbol(context,name,
                (WORD)(context->file_header.NumberOfSections+1),external);
        }
        break;
    case IMAGE_SYM_CLASS_SECTION:
        process_section_output_symbol(context,raw,module);
        break;
    default:
        process_other_output_symbol(context,raw);
        break;
    }
    if(section && !discard && emit_symbol_class(raw->StorageClass,&context->options) &&
       !external) {
        if(raw->StorageClass==IMAGE_SYM_CLASS_STATIC && section>0 &&
           (contribution->module==output_module || !contribution->line_number_count) &&
           raw->NumberOfAuxSymbols)goto auxiliary;
        if(section>0 && contribution->group->section==debug_section && !mapped_debug)
            goto auxiliary;
        write_coff_symbol(output_file,raw);
        ++emitted_symbol_count;
        if(replacement_value!=0xffffffff)raw->Value=replacement_value;
        emitted=1;
    }
auxiliary:
    /* The helper's complete body reads only this BYTE, although the original
     * reuses and physically pushes the incoming DWORD transport slot. */
    if(raw->NumberOfAuxSymbols)process_aux_output_symbols(raw,cursor,emitted,index);
}

/* These two offsets chain the emitted FILE symbols; they are not module
 * indexes or a second copy of the COFF output cursor. */
DWORD last_file_symbol_offset,first_file_symbol_index;
extern void *module_debug_table;
extern DWORD module_debug_index;
extern BYTE file_access_statistics;
void __stdcall read_file_symbol(int file,IMAGE_SYMBOL *symbol)
{
    FileRecord *record;
    if(file_access_statistics)record_file_access();
    record=file_slots[file];
    if(record->flags&8)mapped_read(record,symbol,sizeof(*symbol));
    else buffered_read(record,symbol,sizeof(*symbol));
}
typedef struct BufferedPosition {
    BYTE reserved00[12];
    LONG position;
} BufferedPosition;
void __stdcall process_file_output_symbol(LinkContext *context,IMAGE_SYMBOL *symbol,
    DWORD index,DWORD *value)
{
    if(emit_symbol_class(IMAGE_SYM_CLASS_FILE,&context->options)) {
        FileRecord *file;
        if(last_file_symbol_offset) {
            DWORD previous=last_file_symbol_offset,error=0;
            IMAGE_SYMBOL old;
            int file_number=output_file;
            if(file_access_statistics)record_file_access();
            file=file_slots[file_number];
            last_file_symbol_offset=(file->flags & 8) ? file->file_size :
                ((BufferedPosition *)file->base)->position;
            seek_file(output_file,previous,FILE_BEGIN,&error);
            read_file_symbol(output_file,&old);
            old.Value=emitted_symbol_count;
            error=0;
            seek_file(output_file,previous,FILE_BEGIN,&error);
            write_coff_symbol(output_file,&old);
            error=0;
            seek_file(output_file,last_file_symbol_offset,FILE_BEGIN,&error);
        } else {
            int file_number=output_file;
            if(file_access_statistics)record_file_access();
            file=file_slots[file_number];
            last_file_symbol_offset=(file->flags & 8) ? file->file_size :
                ((BufferedPosition *)file->base)->position;
            first_file_symbol_index=emitted_symbol_count;
        }
        *value=symbol->Value;
        symbol->Value=first_file_symbol_index;
    }
    if(module_debug_table) {
        OutputModule *module=((DebugModuleRecord *)module_debug_table)[module_debug_index].module;
        if(module->debug_symbol_index==0xffffffff)module->debug_symbol_index=index;
    }
}
DWORD __stdcall intern_symbol_string(StringTable *,const char *);
void __stdcall record_map_symbol(const char *,Contribution *,DWORD,BYTE);
void __stdcall process_local_output_symbol(LinkContext *context,IMAGE_SYMBOL *symbol,
    OutputModule *module)
{
    SHORT section=symbol->SectionNumber;
    BYTE map_function=context->options.write_map && section>0 &&
        symbol->StorageClass==IMAGE_SYM_CLASS_STATIC && (symbol->Type & 0x30)==0x20;
    const char *name;
    WORD kind;
    if(map_function) {
        name=get_symbol_name((StringNode *)symbol,current_module_strings,&kind,NULL);
        if(kind==1)name=strncpy(short_symbol_name,name,8);
    }
    if(!symbol->N.Name.Short && emit_symbol_class(symbol->StorageClass,&context->options)) {
        const char *long_name=get_symbol_name((StringNode *)symbol,
            current_module_strings,&kind,NULL);
        if(kind==1)long_name=strncpy(short_symbol_name,long_name,8);
        symbol->N.Name.Long=intern_symbol_string(context->symbols,long_name);
    }
    if(section>0) {
        Contribution *contribution=(Contribution *)module->contributions+section-1;
        while(contribution && (contribution->characteristics & 0x4000))
            contribution=contribution->associated_contribution;
        symbol->SectionNumber=contribution->group->section->index;
        if(map_function && contribution->raw_size)
            record_map_symbol(name,contribution,symbol->Value,1);
        symbol->Value+=contribution->rva;
    } else symbol->Value=0;
}
StringNode *__stdcall process_external_output_symbol(LinkContext *context,IMAGE_SYMBOL *symbol)
{
    WORD kind;
    const char *name=get_symbol_name((StringNode *)symbol,current_module_strings,&kind,NULL);
    StringNode *external;
    Contribution *contribution;
    DWORD base=0;
    external=context->symbols->LookupSymbol(kind,name,NULL);
    if(!kind) {
        symbol->N.Name.Short=external->symbol.N.Name.Short;
        symbol->N.Name.Long=external->symbol.N.Name.Long;
    }
    contribution=external->contribution;
    while(contribution && (contribution->characteristics & 0x4000))
        contribution=contribution->associated_contribution;
    if(external->flags & 1) {
        if(!contribution)symbol->SectionNumber=external->symbol.SectionNumber;
        else if(!(contribution->characteristics & 0x800) &&
                (!context->options.effective_reference_opt || !is_discardable_comdat(contribution))) {
            symbol->SectionNumber=contribution->group->section->index;
            base=contribution->rva;
        } else symbol->SectionNumber=0;
    } else symbol->SectionNumber=0;
    symbol->Value=external->symbol.Value+base;
    external->final_value=symbol->Value;
    external->symbol.SectionNumber=symbol->SectionNumber;
    return external;
}
OutputSection *__stdcall find_input_output_section(OutputModule *,const char *,DWORD,SectionList *,IMAGE_OPTIONAL_HEADER *);
#pragma intrinsic(strcmp)
void __stdcall process_section_output_symbol(LinkContext *context,IMAGE_SYMBOL *symbol,
    OutputModule *module)
{
    WORD kind;
    char module_name[520];
    const char *name=get_symbol_name((StringNode *)symbol,
        context->symbols->hash.string_storage,&kind,NULL);
    OutputSection *section;
    if(kind==1)name=strncpy(short_symbol_name,name,8);
    section=find_input_output_section(module,name,symbol->Value,&context->sections,
        &context->optional_header);
    if(!section)link_fatal(format_module_name(module,module_name),2001,name);
    if(strncmp(name,".idata$",7) && strncmp(name,".didat$",7))symbol->Value=section->rva;
    else {
        GroupContributionEnumerator enumerator;
        symbol->Value=0;
        initialize_group_contributions(&enumerator,find_contribution_group(section,name));
        while(next_group_contribution(&enumerator)) {
            Contribution *contribution=enumerator.current;
            if(contribution->characteristics & 0x800)continue;
            if(context->options.effective_reference_opt && is_discardable_comdat(contribution))continue;
            if(!strcmp(contribution->module->name,module->name)) {
                symbol->Value=contribution->rva;
                break;
            }
        }
        end_section_enumerator((SectionEnumerator *)&enumerator);
    }
    symbol->SectionNumber=section->index;
}

void __stdcall process_other_output_symbol(LinkContext *context,IMAGE_SYMBOL *symbol)
{
    if(!symbol->N.Name.Short && emit_symbol_class(symbol->StorageClass,&context->options) &&
       symbol->N.Name.Long) {
        const char *name=get_symbol_name((StringNode *)symbol,current_module_strings,NULL,NULL);
        symbol->N.Name.Long=intern_symbol_string(context->symbols,name);
    }
}
/* Only their physical output offsets are installed here; the function-aux
 * updater retains its own pending identity and will recover their uses. */
DWORD previous_function_aux_offset,previous_function_symbol_index;
IMAGE_SYMBOL *__stdcall next_coff_symbol(IMAGE_SYMBOL **);
void __stdcall update_function_aux_symbols(IMAGE_SYMBOL *,DWORD *,DWORD *,BYTE);
void __stdcall write_coff_aux_symbol(int,const IMAGE_AUX_SYMBOL *);
void __stdcall process_aux_output_symbols(IMAGE_SYMBOL *symbol,IMAGE_SYMBOL **cursor,
    BYTE emit,DWORD *index)
{
    unsigned count=symbol->NumberOfAuxSymbols;
    while(count) {
        IMAGE_SYMBOL *auxiliary=next_coff_symbol(cursor);
        ++*index;
        if(emit) {
            if(symbol->StorageClass==IMAGE_SYM_CLASS_FUNCTION)
                update_function_aux_symbols(symbol,&previous_function_aux_offset,
                    &previous_function_symbol_index,(BYTE)count);
            write_coff_aux_symbol(output_file,(const IMAGE_AUX_SYMBOL *)auxiliary);
            ++emitted_symbol_count;
        }
        --count;
    }
}
void __stdcall update_function_aux_symbols(IMAGE_SYMBOL *symbol,DWORD *offset,DWORD *index,BYTE count) { __asm { int 3 } }
void __stdcall write_coff_aux_symbol(int file,const IMAGE_AUX_SYMBOL *symbol)
{
    FileRecord *record;
    if(file_access_statistics)record_file_access();
    record=file_slots[file];
    if(record->flags&8)mapped_write(record,symbol,18);
    else buffered_write(record,symbol,18);
}
void __stdcall pdb_record_output_symbol(LinkContext *context,const char *name,WORD section,StringNode *symbol) { __asm { int 3 } }
/* Original494578 is the existing Mac-target BYTE, written by the existing /machine option. */
extern BYTE mac_target;
void __stdcall write_coff_symbol(int file,IMAGE_SYMBOL *symbol)
{
    FileRecord *record;
    BYTE cleared=0;
    if(mac_target && (symbol->Type & 0x8000)) {
        symbol->Type&=0x7fff;
        cleared=1;
    }
    if(file_access_statistics)record_file_access();
    record=file_slots[file];
    if(record->flags & 8)mapped_write(record,symbol,18);
    else buffered_write(record,symbol,18);
    if(cleared)symbol->Type|=0x8000;
}
extern DWORD local_symbol_count;
extern const char *default_phase;
void __stdcall write_coff_symbols(LinkContext *context,DWORD image_rva)
{
    StringNode **symbols;
    DWORD count,index;
    IMAGE_SYMBOL raw;
    default_phase="EmitExternals";
    if(!mac_target) {
        IMAGE_SYMBOL header={0};
        strncpy((char *)header.N.ShortName,"header",8);
        header.Value=0;
        header.SectionNumber=IMAGE_SYM_DEBUG;
        header.StorageClass=IMAGE_SYM_CLASS_EXTERNAL;
        write_coff_symbol(output_file,&header);
        ++local_symbol_count;
        ++emitted_symbol_count;
    }
    symbols=context->symbols->GetSymbolsByValue();
    count=count_hash_symbols(context->symbols->hash.state);
    for(index=0;index<count;++index) {
        StringNode *symbol=symbols[index];
        Contribution *contribution;
        if((symbol->flags&0x08000100) || !(symbol->flags&1) ||
           (symbol->flags&0x200))continue;
        contribution=symbol->contribution;
        while(contribution && (contribution->characteristics&0x4000))
            contribution=contribution->associated_contribution;
        if(contribution) {
            if(contribution->characteristics&0x800)continue;
            if(context->options.effective_reference_opt &&
               is_discardable_comdat(contribution))continue;
        }
        if(!emit_symbol_class(symbol->symbol.StorageClass,&context->options))continue;
        raw=symbol->symbol;
        if(contribution)raw.SectionNumber=contribution->group->section->index;
        raw.Value=symbol->final_value;
        write_coff_symbol(output_file,&raw);
        ++emitted_symbol_count;
        symbol->flags|=0x08000000;
    }
    if(!mac_target) {
        IMAGE_SYMBOL end={0};
        strncpy((char *)end.N.ShortName,"end",8);
        end.Value=image_rva;
        end.SectionNumber=IMAGE_SYM_DEBUG;
        end.StorageClass=IMAGE_SYM_CLASS_EXTERNAL;
        write_coff_symbol(output_file,&end);
        ++local_symbol_count;
        ++emitted_symbol_count;
    }
}

}
