/* Emit the already selected object modules in library/module list order. */
#include "pass-two.h"
#include "archive.h"
#include "machine.h"

void pass_two(Image *image)
{
    LibraryEnumerator libraries;
    ModuleEnumerator modules;
    Module *module;
    const char *name;
    if (verbose_link) printf("Start Pass2\n");
    initialize_library_enumerator(&libraries,image->libraries.first);
    while (next_library(&libraries)) {
        initialize_module_enumerator(&modules,libraries.current);
        while (next_module(&modules)) {
            module=modules.current;
            if (module->library->name) {
                archive_data_offset=module->name.member_offset;
                name=module->library->name;
            } else {
                archive_data_offset=0;
                name=module->name.display_name;
            }
            input_file=open_file(name,0x8000,0);
            process_module_output(image,module);
            close_file(input_file,(unsigned char)(module->library->name==NULL));
        }
    }
    if (verbose_link) printf("End Pass2\n");
}


/* COFF symbol conversion precedes relocation and contribution writes. */
extern char *coff_strings;
extern unsigned long section_header_offset,symbol_table_file_offset;
extern void **mac_object_symbols;
extern ModuleDebugInfo *module_debug_info;
extern unsigned char mac_image;
extern unsigned short mac_code_section_count;
unsigned long output_module_index;
Library *mac_segment_library;

void process_module_output(Image *image,Module *module)
{
    FunctionSymbolMap *function_symbols=NULL;
    PIMAGE_SYMBOL symbols,cursor,symbol;
    ContributionEnumerator contributions;
    Contribution *contribution;
    External *mac_external;
    Module *segment_module;
    ModuleDebugInfo *debug;
    unsigned long member_offset,string_bytes,index=0;
    unsigned short section_index=1,segment;
    const char *name;
    char display[MAX_PATH];
    if (verbose_link) printf("     %s\n",format_module_name(module,display));
    if (image->options.debug_level)
        function_symbols=allocate_function_symbol_maps((unsigned short)module->section_count);
    member_offset=module->library->name ? module->name.member_offset : 0;
    name=module->library->name ? module->library->name : module->name.display_name;
    coff_strings=read_coff_strings(name,module->symbol_offset+module->symbol_count*18+
                                 member_offset,&string_bytes);
    symbols=read_coff_symbols(module->symbol_offset+member_offset,module->symbol_count);
    if (mac_image) mac_object_symbols=calloc(module->symbol_count,sizeof(*mac_object_symbols));
    section_header_offset=module->optional_header_size+20;
    seek_file(output_file,symbol_record_size*symbol_count+symbol_table_file_offset,SEEK_SET);
    seek_file(input_file,module->symbol_offset+member_offset,SEEK_SET);
    cursor=symbols;
    while (index!=module->symbol_count) {
        symbol=next_coff_symbol(&cursor,0);
        ++index;
        process_output_symbol(image,module,symbol,&cursor,function_symbols,&index);
    }
    initialize_contribution_enumerator(&contributions,module);
    while (next_contribution(&contributions)) {
        contribution=contributions.current;
        if (!(image->options.eliminate_unreferenced && contribution_discarded(contribution))) {
            mac_external=mac_image ? find_mac_contribution_external(contribution) : NULL;
            if (!mac_external) write_contribution(image,contribution,symbols);
            else {
                if (mac_external->symbol.N.Name.Short)
                    name=strncpy(short_symbol_name,(const char *)&mac_external->symbol,8);
                else name=image->symbols->strings.data+mac_external->symbol.N.Name.Long;
                segment_module=find_module(mac_segment_library,name,0);
                for (segment=1;segment<mac_code_section_count;++segment)
                    write_contribution(image,&segment_module->contributions[segment-1],symbols);
            }
            if (contribution->raw_size)
                process_contribution_debug(image,contribution,section_index,function_symbols);
        }
        ++section_index;
    }
    if (image->options.debug_level && (image->options.debug_types&2)) {
        debug=&module_debug_info[output_module_index];
        debug->module=module;
        if (!debug->module_name) {
            debug->library_name=module->library->name;
            if (!debug->source_name) debug->source_name=module->path;
            debug->module_name=module->path;
        }
    }
    release_coff_strings(coff_strings);
    coff_strings=NULL;
    if (image->options.debug_level)
        release_function_symbol_maps(function_symbols,(unsigned short)module->section_count);
    release_coff_symbols(symbols);
    if (mac_image) free(mac_object_symbols);
    ++output_module_index;
}


void process_output_symbol(Image *image,Module *module,PIMAGE_SYMBOL symbol,
                           PIMAGE_SYMBOL *cursor,FunctionSymbolMap *maps,
                           unsigned long *index)
{
    External *external=NULL;
    Contribution *contribution;
    Group *group;
    short original_section=symbol->SectionNumber;
    unsigned char special=0,emitted=0,discarded=0;
    switch (symbol->StorageClass) {
    case IMAGE_SYM_CLASS_EXTERNAL:
    case LINK_SYM_CLASS_FAR_EXTERNAL:
    case IMAGE_SYM_CLASS_WEAK_EXTERNAL:
        external=update_external_symbol(image,symbol,module);
        if (mac_image) mac_object_symbols[*index-1]=external;
        break;
    case IMAGE_SYM_CLASS_STATIC:
    case IMAGE_SYM_CLASS_LABEL:
    case IMAGE_SYM_CLASS_UNDEFINED_LABEL:
    case IMAGE_SYM_CLASS_UNDEFINED_STATIC:
        update_static_symbol(image,symbol,module);
        break;
    case IMAGE_SYM_CLASS_FILE: update_file_symbol(image,symbol,&special); break;
    case IMAGE_SYM_CLASS_SECTION: update_section_symbol(image,symbol,module); break;
    default: update_debug_symbol(image,symbol); break;
    }
    if (original_section>0) {
        contribution=&module->contributions[original_section-1];
        if (!contribution->raw_size ||
            (image->options.eliminate_unreferenced && contribution_discarded(contribution)))
            discarded=1;
    }
    if (original_section && !discarded && emit_symbol_class(symbol->StorageClass,&image->options)) {
        if (symbol->StorageClass==IMAGE_SYM_CLASS_EXTERNAL ||
            symbol->StorageClass==LINK_SYM_CLASS_FAR_EXTERNAL) {
            if (image->options.debug_level!=3) {
                process_symbol_auxiliaries(image,symbol,original_section,cursor,0,special,maps,index);
                return;
            }
            external->flags |= EXTERNAL_EMITTED;
        }
        /* COFF .lf is identified by its four-byte short-name discriminator. */
        if (mac_image && original_section>0 &&
            symbol->N.Name.Short!=*(const unsigned long *)".lf") {
            group=external ? external->owner.contribution->group :
                             module->contributions[original_section-1].group;
            symbol->Value+=group->section->rva;
        }
        write_coff_symbol(output_file,symbol);
        if (mac_image && original_section>0 &&
            symbol->N.Name.Short!=*(const unsigned long *)".lf") {
            group=external ? external->owner.contribution->group :
                             module->contributions[original_section-1].group;
            symbol->Value-=group->section->rva;
        }
        emitted=1;
        ++symbol_count;
    }
    if (symbol->NumberOfAuxSymbols)
        process_symbol_auxiliaries(image,symbol,original_section,cursor,emitted,special,maps,index);
}


unsigned long file_symbol_previous_offset,file_symbol_first_index;
void update_file_symbol(Image *image,PIMAGE_SYMBOL symbol,unsigned char *special)
{
    unsigned long previous=file_symbol_previous_offset;
    IMAGE_SYMBOL saved;
    ModuleDebugInfo *debug;
    if (emit_symbol_class(IMAGE_SYM_CLASS_FILE,&image->options)) {
        if (previous) {
            file_symbol_previous_offset=tell_file(output_file);
            seek_file(output_file,previous,SEEK_SET);
            read_coff_symbol(output_file,&saved);
            saved.Value=symbol_count;
            seek_file(output_file,previous,SEEK_SET);
            write_coff_symbol(output_file,&saved);
            seek_file(output_file,file_symbol_previous_offset,SEEK_SET);
        } else {
            file_symbol_previous_offset=tell_file(output_file);
            file_symbol_first_index=symbol_count;
        }
        symbol->Value=file_symbol_first_index;
    }
    if (image->options.debug_level && (image->options.debug_types&2) &&
        symbol->NumberOfAuxSymbols) {
        debug=&module_debug_info[output_module_index];
        if (!debug->source_name) {
            debug->source_name=calloc(symbol->NumberOfAuxSymbols+1,18);
            if (!debug->source_name) out_of_memory();
            *special=1;
            return;
        }
    }
    *special=0;
}


unsigned long function_aux_offset,block_aux_offset,previous_aux_offset;
void process_symbol_auxiliaries(Image *image,PIMAGE_SYMBOL symbol,short original_section,
                                PIMAGE_SYMBOL *cursor,unsigned char emitted,
                                unsigned char special,FunctionSymbolMap *maps,
                                unsigned long *index)
{
    unsigned remaining=symbol->NumberOfAuxSymbols;
    PIMAGE_AUX_SYMBOL auxiliary;
    while (remaining) {
        auxiliary=(PIMAGE_AUX_SYMBOL)next_coff_symbol(cursor,1);
        ++(*index);
        if (emitted) {
            switch (symbol->StorageClass) {
            case IMAGE_SYM_CLASS_STATIC:
                if (is_section_symbol(image,symbol)) break;
                /* Other STATIC auxiliaries use the external/function path. */
            case IMAGE_SYM_CLASS_EXTERNAL:
            case LINK_SYM_CLASS_FAR_EXTERNAL:
                update_function_auxiliary(symbol,original_section,auxiliary,
                    &previous_aux_offset,&function_aux_offset,maps,*index,image);
                break;
            case IMAGE_SYM_CLASS_FUNCTION:
                update_block_auxiliary(symbol,auxiliary,&previous_aux_offset,
                    &function_aux_offset,&block_aux_offset,remaining);
                break;
            }
            write_coff_auxiliary(output_file,auxiliary);
            ++symbol_count;
        }
        if (symbol->StorageClass==IMAGE_SYM_CLASS_FILE && special)
            strncat(module_debug_info[output_module_index].source_name,(const char *)auxiliary,18);
        --remaining;
    }
}


extern unsigned long mac_data_base,mac_data_size;
void update_static_symbol(Image *image,PIMAGE_SYMBOL symbol,Module *module)
{
    short section=symbol->SectionNumber;
    unsigned char mapped=(section>0 && image->options.map_format &&
        symbol->StorageClass==IMAGE_SYM_CLASS_STATIC && (symbol->Type&0x30)==0x20);
    const char *name;
    Contribution *contribution;
    if (mapped) {
        if (symbol->N.Name.Short)
            name=strncpy(short_symbol_name,(const char *)symbol,8);
        else name=coff_strings+symbol->N.Name.Long;
    }
    if (!symbol->N.Name.Short && emit_symbol_class(symbol->StorageClass,&image->options))
        symbol->N.Name.Long=intern_symbol_name(image->symbols,coff_strings+symbol->N.Name.Long);
    if (section<=0) { symbol->Value=0; return; }
    contribution=&module->contributions[section-1];
    symbol->SectionNumber=contribution->group->section->index;
    if (mapped && contribution->raw_size)
        record_map_symbol(name,contribution,symbol->Value,(unsigned char)((symbol->Type&0x30)==0x20));
    if (mac_image) {
        if (contribution->flags&0xc0) {
            symbol->Value+=contribution->rva-mac_data_base-mac_data_size;
            if (!strncmp((const char *)symbol,".debug",6)) return;
            strncmp((const char *)symbol,".rdata",6);
        } else symbol->Value+=contribution->rva-contribution->group->section->rva;
    } else symbol->Value+=contribution->rva;
}


External *update_external_symbol(Image *image,PIMAGE_SYMBOL symbol,Module *module)
{
    short input_section=symbol->SectionNumber,section;
    unsigned char created,imports;
    char name[9];
    External *external;
    Contribution *contribution;
    unsigned long displacement,size,*common_cursor;
    if (!symbol->N.Name.Short) {
        external=lookup_external(image->symbols,0,coff_strings+symbol->N.Name.Long,&created,&imports);
        symbol->N=external->symbol.N;
    } else {
        name[8]=0;
        strncpy(name,(const char *)symbol,8);
        external=lookup_external(image->symbols,1,name,&created,&imports);
    }
    if (input_section>0) {
        contribution=external->owner.contribution;
        symbol->SectionNumber=contribution->group->section->index;
        if (!mac_image) displacement=contribution->rva;
        else if ((contribution->flags&0xc0) &&
                 strncmp(contribution->group->name,".debug",6))
            displacement=contribution->rva-mac_data_base-mac_data_size;
        else displacement=contribution->rva-contribution->group->section->rva;
        external->value.final_value=external->symbol.Value+displacement;
    } else if (input_section==-1) {
        external->value.final_value=external->symbol.Value;
        displacement=0;
    } else if (!input_section) {
        section=external->symbol.SectionNumber;
        if (!section) {
            size=external->symbol.Value;
            if (size) {
                common_cursor=&bss_size;
                if (mac_image && external->symbol.StorageClass==LINK_SYM_CLASS_FAR_EXTERNAL)
                    common_cursor=&mac_far_bss_offset;
                external->symbol.Value=*common_cursor;
                external->symbol.SectionNumber=external->owner.contribution->group->section->index;
                displacement=mac_image ? 0-(mac_data_base+mac_data_size) : 0;
                external->value.final_value=external->symbol.Value+displacement;
                *common_cursor+=size;
            }
        } else if (section>0) {
            contribution=external->owner.contribution;
            if (!mac_image) displacement=contribution->rva;
            else if (contribution->flags&IMAGE_SCN_CNT_CODE)
                displacement=contribution->rva-contribution->group->section->rva;
            else displacement=contribution->rva-mac_data_base-mac_data_size;
            if (external->flags&EXTERNAL_COMMON) {
                displacement=mac_image ? 0-(mac_data_base+mac_data_size) : 0;
                external->value.final_value=external->symbol.Value+displacement;
            }
            symbol->SectionNumber=contribution->group->section->index;
        } else {
            displacement=0;
            symbol->SectionNumber=section;
        }
    }
    symbol->Value=external->symbol.Value+displacement;
    if (mac_image)
        symbol->SectionNumber=external->owner.contribution->group->section->index;
    return external;
}


/* Sixteen-byte templates for serialized contribution alignment. */
const unsigned char output_zero_padding[16]={0};
const unsigned char i386_code_padding[16]={
    0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,
    0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90
};
void write_contribution(Image *image,Contribution *contribution,PIMAGE_SYMBOL symbols)
{
    Module *module=contribution->module;
    ModuleDebugInfo *debug;
    unsigned long kind,member_offset,size,done,chunk;
    const unsigned char *fill;
    void *data;
    unsigned char allocated;
    section_header_offset=module->optional_header_size+20;
    if (contribution->flags&0x800) return;
    if (image->options.debug_level && (image->options.debug_types&2)) {
        debug=&module_debug_info[output_module_index];
        debug->library_name=module->library->name;
        if (!debug->source_name) debug->source_name=module->path;
        debug->module_name=module->path;
    }
    if (!contribution->raw_size) return;
    kind=contribution->flags&0x1e0;
    if (mac_image && (contribution->flags&IMAGE_SCN_CNT_CODE) &&
        contribution->group->section->groups->rva==contribution->rva)
        write_mac_loader_prologue(contribution);
    if (kind!=IMAGE_SCN_CNT_UNINITIALIZED_DATA) {
        member_offset=module->library->name ? module->name.member_offset : 0;
        seek_file(input_file,contribution->data_offset+member_offset,SEEK_SET);
        size=contribution->raw_size-contribution->padding;
        data=mapped_output_pointer(output_file,contribution->file_offset,size);
        allocated=data==NULL;
        if (allocated) {
            data=calloc(contribution->raw_size,1);
            if (!data) out_of_memory();
        }
        read_file(input_file,data,size);
        if (contribution->relocation_count) {
            contribution->relocations=allocate_coff_relocations(contribution->relocation_count);
            seek_file(input_file,contribution->relocation_offset+member_offset,SEEK_SET);
            read_coff_relocations(input_file,contribution->relocations,contribution->relocation_count);
            prepare_relocations();
            apply_relocations(contribution,data,symbols,image);
            release_coff_relocations(contribution->relocations);
            contribution->relocations=NULL;
        }
        if (allocated) {
            seek_file(output_file,contribution->file_offset,SEEK_SET);
            write_file(output_file,data,size);
            free(data);
        } else seek_file(output_file,contribution->file_offset+size,SEEK_SET);
    }
    if (contribution->padding && kind!=IMAGE_SCN_CNT_UNINITIALIZED_DATA) {
        fill=(kind==IMAGE_SCN_CNT_CODE && image->file.Machine==IMAGE_FILE_MACHINE_I386) ?
             i386_code_padding : output_zero_padding;
        for (done=0;done<contribution->padding;done+=16) {
            chunk=contribution->padding-done;
            if (chunk>16) chunk=16;
            write_file(output_file,fill,chunk);
        }
    }
    if (kind!=IMAGE_SCN_CNT_UNINITIALIZED_DATA && image->options.debug_level &&
        (image->options.debug_types&2) && kind!=IMAGE_SCN_CNT_CODE) {
        debug=&module_debug_info[output_module_index];
        if (!strncmp(contribution->group->name,".debug$S",8)) {
            if (!debug->symbol_offset) debug->symbol_offset=contribution->file_offset;
            debug->symbol_size+=contribution->raw_size;
        } else if (!strncmp(contribution->group->name,".debug$T",8)) {
            if (!debug->type_offset) {
                debug->type_offset=contribution->file_offset;
                debug->precompiled_types=0;
            }
            debug->type_size+=contribution->raw_size;
        } else if (!strncmp(contribution->group->name,".debug$P",8)) {
            if (!debug->type_offset) {
                debug->type_offset=contribution->file_offset;
                debug->precompiled_types=1;
            }
            debug->type_size+=contribution->raw_size;
        }
    }
}


#pragma intrinsic(strlen)
unsigned long cv_line_count,cv_line_count_file_offset;
void process_contribution_debug(Image *image,Contribution *contribution,
                                 unsigned short section_index,FunctionSymbolMap *maps)
{
    Section *section=contribution->group->section;
    Module *module=contribution->module;
    ModuleDebugInfo *debug;
    PIMAGE_LINENUMBER lines,line;
    IMAGE_AUX_SYMBOL auxiliary;
    unsigned long count=contribution->line_count,remaining,member_offset;
    unsigned long auxiliary_offset,output_symbol;
    unsigned short name_length;
    const char *name;
    char display[MAX_PATH];
    if (!count || !section->line_count ||
        (image->options.debug_level!=2 && image->options.debug_level!=3) ||
        image->options.rom) return;
    lines=calloc(count,sizeof(*lines));
    if (!lines) out_of_memory();
    member_offset=module->library->name ? module->name.member_offset : 0;
    seek_file(input_file,contribution->line_offset+member_offset,SEEK_SET);
    read_coff_lines(input_file,lines,count);
    for (line=lines,remaining=count;remaining;--remaining,++line) {
        if (line->Linenumber) {
            line->Type.VirtualAddress-=contribution->input_rva;
            line->Type.VirtualAddress+=contribution->rva;
            line->Type.VirtualAddress-=section->rva;
            if (!(image->options.debug_types&2)) line->Type.VirtualAddress+=contribution->rva;
        } else {
            if (line->Type.SymbolTableIndex>module->symbol_count)
                fatal_message(program_name,0x29,module->path);
            if ((image->options.debug_types&1) && image->options.debug_level==3) {
                if (!lookup_function_symbol(maps,section_index,line->Type.SymbolTableIndex,
                                            &auxiliary_offset,&output_symbol))
                    fatal_message(format_module_name(module,display),0x2a,section_index);
                line->Type.SymbolTableIndex=output_symbol;
                seek_file(output_file,auxiliary_offset,SEEK_SET);
                read_coff_symbol(output_file,(PIMAGE_SYMBOL)&auxiliary);
                auxiliary.Sym.FcnAry.Function.PointerToLinenumber=
                    (unsigned long)(line-lines)*sizeof(*lines)+section->line_offset;
                seek_file(output_file,auxiliary_offset,SEEK_SET);
                write_coff_auxiliary(output_file,&auxiliary);
            }
        }
    }
    seek_file(output_file,section->line_offset,SEEK_SET);
    if ((image->options.debug_types&2) && (image->options.debug_types&4)) {
        debug=&module_debug_info[output_module_index];
        if (!debug->line_offset) {
            name=debug->source_name;
            name_length=(unsigned short)strlen(name);
            debug->line_offset=section->line_offset;
            write_file(output_file,&name_length,1);
            write_file(output_file,name,name_length);
            write_file(output_file,&section->index,2);
            cv_line_count_file_offset=tell_file(output_file);
            write_file(output_file,&count,2);
            debug->line_size=name_length+count*sizeof(*lines)+5;
            section->line_offset+=name_length+5;
            cv_line_count=count;
        } else {
            seek_file(output_file,cv_line_count_file_offset,SEEK_SET);
            cv_line_count+=count;
            write_file(output_file,&cv_line_count,2);
            debug->line_size+=count*sizeof(*lines);
            seek_file(output_file,section->line_offset,SEEK_SET);
        }
    }
    write_coff_lines(output_file,lines,count,&image->options);
    section->line_offset+=count*sizeof(*lines);
    free(lines);
}


/* Retained original graph vertex; this target's relocation prelude is empty. */
void prepare_relocations(void)
{
}

