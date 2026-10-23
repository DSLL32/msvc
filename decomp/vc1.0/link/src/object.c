/* COFF sections and symbols read during pass one. */
#include "link-pass.h"
#include "comdat.h"
#include <recon.h>
#pragma intrinsic(strcmp,strcpy)
char *coff_strings;
unsigned long section_header_offset;
void **mac_object_symbols;
extern unsigned char mac_image;
extern unsigned long archive_data_offset,string_table_size;

void process_object(Image *image,unsigned char *new_externals,unsigned char *symbols_changed,Module *module,
    unsigned char explicit_input,unsigned short member)
{
    char *name;
    unsigned long string_bytes,index;
    PIMAGE_SYMBOL symbols;
    if (mac_image && tool_kind!=2) initialize_mac_object_symbols(module->symbol_count);
    name=module->library->name;
    if (!name) name=module->name.display_name;
    coff_strings=read_coff_strings(name,module->symbol_offset+module->symbol_count*18+
        archive_data_offset,&string_bytes);
    string_table_size+=string_bytes;
    symbols=read_coff_symbols(module->symbol_offset+archive_data_offset,
        module->symbol_count);
    if (mac_image) mac_object_symbols=calloc(module->symbol_count,sizeof(void *));
    section_header_offset=module->optional_header_size+20;
    for (index=0;index<module->section_count;++index)
        process_object_section(image,module,explicit_input,(unsigned short)(index+1),symbols);
    process_object_symbols(image,module,new_externals,symbols_changed,symbols,member);
    if (mac_image && tool_kind!=2) {
        finish_mac_object_symbols();
        process_mac_object_symbols(image->symbols,symbols,coff_strings);
    }
    release_coff_strings(coff_strings);
    release_coff_symbols(symbols);
    if (mac_image) free(mac_object_symbols);
    coff_strings=NULL;
}

void process_object_section(Image *image,Module *module,unsigned char explicit_input,
    unsigned short index,PIMAGE_SYMBOL symbols)
{
    IMAGE_SECTION_HEADER header;
    Contribution *contribution;
    char *comdat_name=NULL,*directives;
    char display[_MAX_PATH];
    unsigned long member_offset;
    unsigned char alignment;
    seek_file(input_file,section_header_offset+archive_data_offset,SEEK_SET);
    read_section_header(input_file,&header);
    section_header_offset+=sizeof(header);
    contribution=create_contribution(coff_section_name(header.Name,coff_strings),
        header.SizeOfRawData,header.NumberOfRelocations,header.NumberOfLinenumbers,
        header.PointerToRelocations,header.PointerToLinenumbers,header.PointerToRawData,
        header.Characteristics,header.Characteristics,header.VirtualAddress,module,0,
        &image->sections,&image->optional);
    filter_debug_contribution(contribution,&image->options);
    alignment=(unsigned char)align_contribution(1,header.Characteristics);
    if (alignment>=contribution->group->alignment)
        contribution->group->alignment=(unsigned char)align_contribution(1,header.Characteristics);
    if (contribution->flags&IMAGE_SCN_LNK_COMDAT) {
        if (!select_comdat(image->symbols,contribution,symbols,index,&comdat_name)) {
            contribution->raw_size=0;
            return;
        }
    }
    if (image->options.eliminate_unreferenced) attach_comdat_node(module,contribution,comdat_name);
    if (!(contribution->flags&IMAGE_SCN_LNK_COMDAT) && image->options.eliminate_unreferenced)
        add_comdat_root(NULL,NULL,contribution,&comdat_roots);
    if (contribution->flags&IMAGE_SCN_LNK_INFO) {
        if (strcmp(".drectve",contribution->group->name)) return;
        member_offset=module->library->name ? module->name.member_offset : 0;
        seek_file(input_file,contribution->data_offset+member_offset,SEEK_SET);
        directives=calloc(contribution->raw_size+1,1);
        if (!directives) out_of_memory();
        read_file(input_file,directives,contribution->raw_size);
        process_directives(directives,image,format_module_name(module,display),
            (unsigned char)(module->library->name!=NULL));
        free(directives);
        return;
    }
    if (contribution->flags&0x800) return;
    contribution->padding=0;
    classify_contribution(contribution,&image->options);
    if (strcmp(contribution->group->name,".debug$S") &&
        strcmp(contribution->group->name,".debug$T") &&
        strcmp(contribution->group->name,".debug$P") && tool_kind!=2)
        read_contribution_relocations(image,contribution,symbols);
}

void filter_debug_contribution(Contribution *contribution,ImageOptions *options)
{
    char *name=contribution->group->name;
    if ((!options->debug_level || !(options->debug_types&2)) &&
        (!strcmp(name,".debug$S") || !strcmp(name,".debug$T") ||
         !strcmp(name,".debug$P"))) contribution->flags |= 0x800;
    if ((!options->debug_level || !(options->debug_types&0x10)) && !strcmp(name,".debug$G"))
        contribution->flags |= 0x800;
    if (!strcmp(name,".debug$F") && (!options->debug_level || !(options->debug_types&8)))
        contribution->flags |= 0x800;
}

unsigned long align_contribution(unsigned long offset,unsigned long flags)
{
    unsigned long mask;
    if (flags&8) return offset;
    if (mac_image) mask=3;
    else switch (flags&0x700000) {
    case 0: case 0x500000: mask=15; break;
    case 0x200000: mask=1; break;
    case 0x300000: mask=3; break;
    case 0x400000: mask=7; break;
    case 0x600000: mask=31; break;
    case 0x700000: mask=63; break;
    default: return offset;
    }
    return (offset&mask) ? (offset&~mask)+mask+1 : offset;
}

/* Original [00022fd0,00023020). Reserve line and CodeView source information. */
void classify_contribution(Contribution *contribution,ImageOptions *options)
{
    Section *section;
    if (!options->rom && contribution->line_count &&
        (options->debug_level==2 || options->debug_level==3)) {
        section=contribution->group->section;
        section->line_count+=contribution->line_count;
        if ((options->debug_types&2) && (options->debug_types&4))
            section->source_name_capacity+=261;
    }
}

/* Macintosh fixup lists are independent state objects, not part of debug_flags. */
MacFixupList mac_data_references;
MacFixupList *mac_section_code_fixups,*mac_section_data_fixups;

/* Original code [000230c0,00023464), switch table [00023464,00023520). */
void read_contribution_relocations(Image *image,Contribution *contribution,PIMAGE_SYMBOL symbols)
{
    unsigned long count=contribution->relocation_count,index,offset;
    PIMAGE_RELOCATION records,relocation;
    unsigned char standalone_code;
    Section *section;
    if (!count) return;
    records=allocate_coff_relocations(count);
    seek_file(input_file,contribution->relocation_offset+archive_data_offset,SEEK_SET);
    read_coff_relocations(input_file,records,count);
    switch (image->file.Machine) {
    case IMAGE_FILE_MACHINE_I386:
        for (index=0,relocation=records;index<count;++index,++relocation) {
            if (image->options.eliminate_unreferenced)
                add_relocation_edge(image->symbols,contribution,symbols,relocation);
            if (!image->options.fixed && relocation->Type==IMAGE_REL_I386_DIR32)
                ++image->optional.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size;
        }
        break;
    case IMAGE_FILE_MACHINE_I860:
        for (index=0,relocation=records;index<count;++index,++relocation) {
            if (image->options.eliminate_unreferenced)
                add_relocation_edge(image->symbols,contribution,symbols,relocation);
            if (!image->options.fixed) {
                if (relocation->Type==IMAGE_REL_I860_HIGHADJ)
                    image->optional.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size+=2;
                else if (relocation->Type==IMAGE_REL_I860_DIR32 || relocation->Type==IMAGE_REL_I860_HIGH)
                    ++image->optional.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size;
            }
        }
        break;
    case 0x268: /* Macintosh 680x0 relocation records. */
        section=contribution->group->section;
        standalone_code=(section->characteristics&IMAGE_SCN_CNT_CODE) && section->mac_type!=0x45444f43;
        for (index=0,relocation=records;index<count;++index,++relocation) {
            if (standalone_code) validate_mac_code_relocation(relocation->Type);
            if (image->options.eliminate_unreferenced)
                add_relocation_edge(image->symbols,contribution,symbols,relocation);
            offset=relocation->VirtualAddress-contribution->input_rva;
            switch (relocation->Type) {
            case 1: case 8: case 25: case 28:
                if (!standalone_code) record_mac_symbol_reference(relocation->SymbolTableIndex,section,0xc0000000UL);
                break;
            case 3: case 26: case 47:
                if (!standalone_code) record_mac_symbol_reference(relocation->SymbolTableIndex,section,0x40000000UL);
                break;
            case 4:
                append_mac_fixup(&mac_data_references,contribution,offset);
                break;
            case 5: case 27:
                append_mac_fixup(&mac_data_references,contribution,offset);
                if (!standalone_code) record_mac_symbol_reference(relocation->SymbolTableIndex,section,0x40000000UL);
                break;
            case 7:
                if (!standalone_code) record_mac_symbol_reference(relocation->SymbolTableIndex,section,0x80000000UL);
                break;
            case 10:
                append_mac_fixup(&mac_section_data_fixups[(unsigned short)section->mac_resource_id],contribution,offset);
                break;
            case 11: case 29:
                if (!standalone_code) record_mac_symbol_reference(relocation->SymbolTableIndex,section,0);
                record_mac_data_reference(contribution,offset,relocation->SymbolTableIndex);
                break;
            case 12:
                if (!standalone_code) record_mac_symbol_reference(relocation->SymbolTableIndex,section,0x40000000UL);
                append_mac_fixup(&mac_section_data_fixups[(unsigned short)section->mac_resource_id],contribution,offset);
                break;
            case 20:
                if (!standalone_code) record_mac_symbol_reference(relocation->SymbolTableIndex,section,0);
                break;
            case 22:
                record_mac_data_reference(contribution,offset,relocation->SymbolTableIndex);
                break;
            case 35: record_mac_symbol_reference(relocation->SymbolTableIndex,section,0x81000000UL); break;
            case 36: record_mac_symbol_reference(relocation->SymbolTableIndex,section,0x82000000UL); break;
            case 37: record_mac_symbol_reference(relocation->SymbolTableIndex,section,0x84000000UL); break;
            case 38: record_mac_symbol_reference(relocation->SymbolTableIndex,section,0x01000000UL); break;
            case 39: record_mac_symbol_reference(relocation->SymbolTableIndex,section,0x02000000UL); break;
            case 40: record_mac_symbol_reference(relocation->SymbolTableIndex,section,0x04000000UL); break;
            case 41: record_mac_symbol_reference(relocation->SymbolTableIndex,NULL,0x00800000UL); break;
            case 42:
                append_mac_fixup(&mac_section_code_fixups[(unsigned short)section->mac_resource_id],contribution,offset);
                record_mac_symbol_reference(relocation->SymbolTableIndex,NULL,0x00800000UL);
                break;
            }
        }
        break;
    }
    release_coff_relocations(records);
}

/* Original [00023d00,00024330). Merge defined, common, and weak COFF externals. */
void process_object_symbols(Image *image,Module *module,unsigned char *new_externals,
    unsigned char *import_symbols,PIMAGE_SYMBOL records,unsigned short member)
{
    PIMAGE_SYMBOL cursor=records,symbol;
    unsigned long index=0,old_bss=bss_size,old_far_bss=mac_far_bss_offset;
    unsigned long member_offset,module_bss,value,*total;
    unsigned char imports=import_symbols?*import_symbols:0,created,define,auxiliary;
    short section_number;
    Contribution *contribution,*previous;
    External *external;
    char display[_MAX_PATH],previous_display[_MAX_PATH],local_name[9]={0};
    char *name;
    format_module_name(module,display);
    member_offset=module->library->name ? module->name.member_offset : 0;
    seek_file(input_file,module->symbol_offset+member_offset,SEEK_SET);
    while (index!=module->symbol_count) {
        symbol=next_coff_symbol(&cursor,0);
        if (symbol->StorageClass==IMAGE_SYM_CLASS_EXTERNAL ||
            symbol->StorageClass==IMAGE_SYM_CLASS_WEAK_EXTERNAL ||
            symbol->StorageClass==LINK_SYM_CLASS_FAR_EXTERNAL) {
            created=define=0;
            if (!symbol->N.Name.Short) name=coff_strings+symbol->N.Name.Long;
            else {
                strncpy(local_name,(char *)symbol,8);
                name=local_name;
            }
            external=lookup_external(image->symbols,(unsigned short)(symbol->N.Name.Short!=0),
                name,&created,&imports);
            if (mac_image) mac_object_symbols[index]=external;
            if (!created && (external->flags&(EXTERNAL_WEAK|EXTERNAL_LAZY)) &&
                symbol->StorageClass!=IMAGE_SYM_CLASS_WEAK_EXTERNAL) {
                external->flags &= ~(EXTERNAL_WEAK|EXTERNAL_LAZY);
                external->weak_default=NULL;
                --weak_symbol_count;
            }
            if (created) {
                external->symbol.Type=symbol->Type;
                if (new_externals) *new_externals=1;
            }
            if (mac_image && tool_kind!=2) record_mac_external(external,index);
            if (external->symbol.N.Name.Short)
                name=strncpy(short_symbol_name,(char *)external,8);
            else name=image->symbols->strings.data+external->symbol.N.Name.Long;
            section_number=symbol->SectionNumber;
            contribution=NULL;
            value=0;
            if (section_number>0) {
                define=1;
                contribution=&module->contributions[section_number-1];
                if (external->flags&EXTERNAL_DEFINED) {
                    if (external->flags&EXTERNAL_COMMON) {
                        external->flags &= ~EXTERNAL_COMMON;
                        bss_size-=external->symbol.Value;
                    } else {
                        define=0;
                        previous=external->owner.contribution;
                        if (!((contribution->flags&IMAGE_SCN_LNK_COMDAT) &&
                              (previous->flags&IMAGE_SCN_LNK_COMDAT))) {
                            if (!previous) strcpy(previous_display,"a previous module");
                            else format_input_reference(previous_display,previous->module->library->name,
                                previous->module->path);
                            warning_message(display,0x4d,name,previous_display);
                        }
                    }
                }
                value=symbol->Value;
            } else if (section_number==IMAGE_SYM_ABSOLUTE) {
                value=symbol->Value;
                define=1;
                if ((external->flags&EXTERNAL_DEFINED) && external->symbol.Value!=value) {
                    previous=external->owner.contribution;
                    format_input_reference(previous_display,previous->module->library->name,previous->module->path);
                    warning_message(display,0x4d,name,previous_display);
                    define=0;
                }
            } else if (!section_number && symbol->Value) {
                contribution=bss_contribution;
                total=&bss_size;
                if (mac_image && symbol->StorageClass==LINK_SYM_CLASS_FAR_EXTERNAL) {
                    contribution=mac_far_bss;
                    total=&mac_far_bss_offset;
                }
                if (!(external->flags&EXTERNAL_DEFINED) || (external->flags&EXTERNAL_COMMON)) {
                    if (!(external->flags&EXTERNAL_COMMON)) prepend_external(&module->common_symbols,external);
                    external->flags |= EXTERNAL_COMMON;
                    if (mac_image) {
                        if (!(external->flags&EXTERNAL_DEFINED)) external->symbol.StorageClass=symbol->StorageClass;
                        if ((symbol->StorageClass==LINK_SYM_CLASS_FAR_EXTERNAL && external->symbol.StorageClass==IMAGE_SYM_CLASS_EXTERNAL) ||
                            (symbol->StorageClass==IMAGE_SYM_CLASS_EXTERNAL && external->symbol.StorageClass==LINK_SYM_CLASS_FAR_EXTERNAL)) {
                            warning_message(module->path,0x85,name,NULL);
                            if (symbol->StorageClass==LINK_SYM_CLASS_FAR_EXTERNAL) total=&bss_size;
                            else {
                                external->symbol.StorageClass=IMAGE_SYM_CLASS_EXTERNAL;
                                contribution=bss_contribution;
                                external->owner.contribution=contribution;
                                mac_far_bss_offset-=external->symbol.Value;
                                bss_size+=external->symbol.Value;
                            }
                        }
                    }
                    if (external->symbol.Value<symbol->Value) {
                        if ((unsigned short)module->machine!=IMAGE_FILE_MACHINE_I386 && !mac_image && symbol->Value>=8)
                            *total=(*total+7)&~7UL;
                        symbol->Value=(symbol->Value+3)&~3UL;
                        *total+=symbol->Value-external->symbol.Value;
                        value=symbol->Value;
                        define=1;
                    }
                }
            } else if (!section_number && !(external->flags&EXTERNAL_DEFINED)) {
                record_undefined_reference(external,module);
                if (symbol->StorageClass==IMAGE_SYM_CLASS_WEAK_EXTERNAL) {
                    ++index;
                    process_weak_external(image->symbols,&cursor,symbol,external,module,
                        new_externals,created,&imports,member);
                }
            }
            if (define) define_external(external,contribution,value,section_number,symbol->Type,member,image->symbols);
        } else if (mac_image && tool_kind!=2 && symbol->SectionNumber>0)
            process_mac_static_symbol(&module->contributions[symbol->SectionNumber-1],symbol,index);
        ++index;
        if (emit_symbol_class(symbol->StorageClass,&image->options))
            local_symbol_count+=1+symbol->NumberOfAuxSymbols;
        for (auxiliary=0;auxiliary<symbol->NumberOfAuxSymbols;++auxiliary) {
            next_coff_symbol(&cursor,1);
            ++index;
        }
    }
    if (mac_image && verbose_link) {
        module_bss=bss_size-old_bss;
        value=mac_far_bss_offset-old_far_bss;
        if (module_bss) printf("NEARBSS :  %8lx bytes  (%s)\n",module_bss,display);
        if (value) printf("FARBSS  :  %8lx bytes  (%s)\n",value,display);
    }
    if (imports) mark_import_contributions(module);
    if (import_symbols) *import_symbols=imports;
}

void define_external(External *external,Contribution *contribution,unsigned long value,
                     short section,unsigned short type,unsigned short member,SymbolTable *table)
{
    set_external_defined(external,1,table);
    if (external->flags&(EXTERNAL_WEAK|EXTERNAL_LAZY|EXTERNAL_ALIAS)) {
        external->flags &= ~(EXTERNAL_WEAK|EXTERNAL_LAZY|EXTERNAL_ALIAS);
        external->symbol.StorageClass=IMAGE_SYM_CLASS_EXTERNAL;
        --weak_symbol_count;
    }
    external->owner.contribution=contribution;
    external->symbol.Value=value;
    external->symbol.SectionNumber=section;
    external->symbol.Type=type;
    external->archive_member=member;
}

void add_relocation_edge(SymbolTable *table,Contribution *contribution,
                         PIMAGE_SYMBOL symbols,PIMAGE_RELOCATION relocation)
{
    PIMAGE_SYMBOL symbol=&symbols[relocation->SymbolTableIndex];
    Module *module=contribution->module;
    unsigned char linker_defined=module==output_module;
    char *name;
    switch (symbol->StorageClass) {
    case IMAGE_SYM_CLASS_EXTERNAL:
    case LINK_SYM_CLASS_FAR_EXTERNAL:
    case IMAGE_SYM_CLASS_WEAK_EXTERNAL:
        if (!symbol->N.Name.Short) name=coff_strings+symbol->N.Name.Long;
        else name=strncpy(short_symbol_name,(char *)symbol,8);
        add_comdat_edge(name,contribution->graph,NULL,linker_defined);
        break;
    default:
        if (symbol->SectionNumber>0)
            add_comdat_edge(NULL,contribution->graph,
                &module->contributions[symbol->SectionNumber-1],linker_defined);
        break;
    }
}

void mark_import_contributions(Module *module)
{
    ContributionEnumerator contributions;
    initialize_contribution_enumerator(&contributions,module);
    while (next_contribution(&contributions))
        if (!strncmp(contributions.current->group->name,".idata",6))
            contributions.current->rva=1;
}

/* Original [00023660,00023803). Consume the single weak-external auxiliary. */
void process_weak_external(SymbolTable *table,PIMAGE_SYMBOL *cursor,PIMAGE_SYMBOL symbol,
    External *external,Module *module,unsigned char *new_externals,unsigned char created,
    unsigned char *imports,unsigned short member)
{
    PIMAGE_AUX_SYMBOL auxiliary;
    IMAGE_SYMBOL fallback;
    unsigned long position,search;
    const char *name;
    char display[264];
    if (symbol->NumberOfAuxSymbols!=1) {
        format_module_name(module,display);
        if (external->symbol.N.Name.Short)
            name=strncpy(short_symbol_name,(char *)&external->symbol,8);
        else name=table->strings.data+external->symbol.N.Name.Long;
        fatal_message(display,0x20,name);
    }
    auxiliary=(PIMAGE_AUX_SYMBOL)next_coff_symbol(cursor,1);
    --symbol->NumberOfAuxSymbols;
    position=tell_file(input_file);
    seek_file(input_file,module->symbol_offset+auxiliary->Sym.TagIndex*18+
        archive_data_offset,SEEK_SET);
    read_coff_symbol(input_file,&fallback);
    /* The ordinary COFF auxiliary union stores weak search characteristics at4. */
    search=auxiliary->Sym.Misc.TotalSize;
    if (created || (!(external->flags&EXTERNAL_DEFINED) && search==3)) {
        ++weak_symbol_count;
        external->symbol.StorageClass=IMAGE_SYM_CLASS_WEAK_EXTERNAL;
        name=fallback.N.Name.Short?(char *)&fallback:coff_strings+fallback.N.Name.Long;
        external->weak_default=lookup_external(table,(unsigned short)(fallback.N.Name.Short!=0),
            name,new_externals,imports);
        external->archive_member=member;
        if (search==1) external->flags|=EXTERNAL_WEAK;
        else if (search==2) external->flags|=EXTERNAL_LAZY;
        else if (search==3) external->flags|=EXTERNAL_ALIAS;
    }
    seek_file(input_file,position,SEEK_SET);
}

/* Original [00025300,00025400). The exception context shares the display name. */
extern char abort_file[256];
#pragma intrinsic(strcat,strcpy)
void format_input_reference(char *output,const char *library,const char *file)
{
    *output=0;
    if (library) {
        strcat(output,library);
        strcat(output," (");
        strcat(output,file);
        strcat(output,")");
    } else strcat(output,file);
    strcpy(abort_file,output);
}
#pragma function(strcat,strcpy)

/* Original [00025ad0,00025ba0). The terminator belongs to the buffer length. */
#pragma intrinsic(strlen)
void format_undefined_references(TextBuffer *buffer,External *external)
{
    ReferenceEnumerator references;
    char display[260];
    unsigned char first=1;
    buffer->data=NULL;
    buffer->capacity=0;
    buffer->length=0;
    initialize_reference_enumerator(&references,external);
    while (next_reference(&references)) {
        append_text(buffer,first?" in ":", ",first?4:2);
        format_module_name(references.current,display);
        append_text(buffer,display,strlen(display));
        first=0;
    }
    append_text(buffer,"",1);
}
#pragma function(strlen)
