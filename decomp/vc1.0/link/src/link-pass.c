/* Output layout and emission around the two linker passes. */
#include "link-pass.h"
#include "dos-stub.h"
#pragma intrinsic(strlen)
SectionDefault readonly_section_defaults={".rdata",0x40000040UL};
SectionDefault bss_section_defaults={".bss",0xc0000080UL};
SectionDefault debug_section_defaults={".debug",0x42000048UL};
SectionDefault idata_directory_defaults={".idata$2",0xc0000048UL};
SectionDefault reloc_section_defaults={".reloc",0x42000040UL};
BaseFixup *base_fixup_next,*base_fixups;
unsigned long codeview_start,codeview_end,undefined_symbols;
External **sorted_externals;
unsigned long symbol_table_file_offset;
ComdatNode *comdat_nodes;
ComdatRoot *comdat_roots;
External *end_symbol,*header_symbol,*entry_symbol,*gp_symbol;
Contribution *absolute_contribution;
ModuleDebugInfo *module_debug_info;
unsigned long bss_size,symbol_count,weak_symbol_count,local_symbol_count,string_table_size;
Module *output_module;
Contribution *bss_contribution,*codeview_contribution,*coff_debug_contribution;
Contribution *fixup_contribution,*debug_directory_contribution;
Section *base_relocation_section,*debug_section;
FixupBlock *fixup_blocks;
unsigned long fixup_count,fixups_in_block;
unsigned long mac_near_bss_size,mac_near_data_size,mac_far_bss_size,mac_far_data_size;
unsigned long mac_far_bss_offset,mac_data_base,mac_data_size;
Contribution *mac_swap,*mac_jump_table,*mac_data_fixups,*mac_debug,*mac_resource_map;
unsigned char mac_image,mac_swappable,mac_loader_present,mac_pcode_present;
unsigned char mac_small_model=1;
unsigned short mac_section_count,mac_entry_section;
extern Contribution *mac_far_bss;
extern unsigned char debug_requested;
extern char *output_name,*dos_stub_template;
extern unsigned long dos_stub_size;
extern FILE *map_stream;


/* The ROM optional header used by the MIPS output format. */
typedef struct RomOptionalHeader {
    unsigned short magic;
    unsigned char linker_major,linker_minor;
    unsigned long code_size,data_size,bss_size,entry,code_base,data_base,bss_base;
    unsigned long gpr_mask,cpr_mask[4],gp_value;
} RomOptionalHeader;

int link_passes(Image *image,unsigned char *pack)
{
    IMAGE_OPTIONAL_HEADER *header=&image->optional;
    SectionList *sections=&image->sections;
    Section *readonly,*section;
    External *external;
    IncludedExternal *include_next;
    unsigned long directory_bytes=0,debug_reserved,debug_file_start,header_end,header_last;
    unsigned long file_cursor,code_rva,data_rva,code_start,data_start,bss_start;
    unsigned long rom_data_start,rom_bss_start,layout_state=0,line_count=0,relocation_count=0;
    unsigned long code_file_end,data_file_end,coff_lines=0,coff_lines_size=0;
    unsigned long relocation_cursor=0,coff_line_cursor=0;
    unsigned long symbol_start=0,string_start=0,string_size=0,coff_size=0,coff_prefix=0;
    unsigned long debug_end,saved_position,fpo_entries,padding,bytes,rounded;
    Contribution *local_imports;
    ExternalLink *import_aliases;
    unsigned long value,signature;
    unsigned char created,flags,failed;
    void *zeroes,*fpo;
    char *full_name;
    Group *fpo_group;
    IMAGE_SECTION_HEADER section_header;
    IMAGE_DEBUG_DIRECTORY directory;
    IMAGE_COFF_SYMBOLS_HEADER coff;
    RomOptionalHeader rom;
    initialize_modules((void (*)(char *,int,char *))fatal_message,&output_module);
    if (image->options.eliminate_unreferenced)
        initialize_comdat((void (*)(char *,int,char *))fatal_message);
    bss_contribution=create_contribution(bss_section_defaults.name,0,0,0,0,0,0,0,
        bss_section_defaults.flags,0,output_module,0,sections,header);
    bss_contribution->flags |= 0x2000;
    if (image->options.eliminate_unreferenced)
        bss_contribution->graph=create_comdat_node(bss_contribution,NULL,NULL,&comdat_nodes,1);
    debug_section=create_section(debug_section_defaults.name,debug_section_defaults.flags,sections,header);
    readonly=create_section(readonly_section_defaults.name,readonly_section_defaults.flags,sections,header);
    if (image->options.debug_level) {
        if (image->options.debug_types&8) directory_bytes=28;
        if (image->options.debug_types&0x10) directory_bytes+=28;
        if (image->options.debug_types&1) directory_bytes+=28;
        if (image->options.debug_types&2) directory_bytes+=28;
        debug_directory_contribution=create_contribution(readonly_section_defaults.name,
            directory_bytes,0,0,0,0,0,0,readonly_section_defaults.flags,0,output_module,1,sections,header);
        debug_directory_contribution->flags |= 0x2000;
        debug_directory_contribution->graph=create_comdat_node(debug_directory_contribution,NULL,NULL,&comdat_nodes,1);
    }
    readonly->raw_size+=directory_bytes;
    readonly->raw_size+=directory_bytes;
    abort_phase="Pass1";
    pass_one(image);
    if (image->options.map_format) {
        if (!map_filename) map_filename=replace_extension(output_name,".map");
        map_stream=fopen(map_filename,"wt");
        if (!map_stream) fatal_message(program_name,7,map_filename);
    }
    apply_section_merges(image);
    bss_contribution->raw_size+=bss_size;
    if (mac_image) mac_far_bss->raw_size+=mac_far_bss_offset;
    header_symbol=lookup_external(image->symbols,1,"header",&created,&flags);
    if (header_symbol->flags&1) fatal_message(program_name,4,"Special symbol \"header\" already defined");
    set_external_defined(header_symbol,1,image->symbols);
    header_symbol->value.final_value=header_symbol->symbol.Value=0;
    header_symbol->symbol.SectionNumber=-2;
    header_symbol->owner.contribution=absolute_contribution;
    end_symbol=lookup_external(image->symbols,1,"end",&created,&flags);
    if (end_symbol->flags&1) fatal_message(program_name,4,"Special symbol \"end\" already defined");
    set_external_defined(end_symbol,1,image->symbols);
    end_symbol->symbol.Value=end_symbol->value.final_value=0;
    end_symbol->symbol.SectionNumber=-2;
    end_symbol->owner.contribution=absolute_contribution;
    local_symbol_count+=2;
    if (*absolute_symbols_file) {
        sort_externals(image->symbols);
        read_absolute_symbols(absolute_symbols_file,sorted_externals,image->symbols);
    }
    if (entry_symbol && !(entry_symbol->flags&1)) resolve_entry_symbol(image->symbols);
    resolve_local_imports(image,&local_imports,&import_aliases);
    emit_externals(image->symbols,0,0,1,&image->options);
    if (undefined_symbols && !image->options.force) {
        fatal_message(output_name,0x17,undefined_symbols);
        return 0x17;
    }
    if (!mac_image) {
        base_relocation_section=create_section(reloc_section_defaults.name,
            reloc_section_defaults.flags,sections,header);
        if (!image->options.fixed) {
            if (!header->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size)
                ++header->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size;
            base_relocation_section->raw_size=1;
            base_fixups=calloc(header->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size,sizeof(*base_fixups));
            base_fixup_next=base_fixups;
            if (!base_fixups) out_of_memory();
        }
    }
    if (image->options.debug_level) {
        coff_debug_contribution=create_contribution(".debug$C",0,0,0,0,0,0,0,
            debug_section_defaults.flags,0,output_module,1,sections,header);
        coff_debug_contribution->flags |= 0x2000;
        coff_debug_contribution->graph=create_comdat_node(coff_debug_contribution,NULL,NULL,&comdat_nodes,1);
        fixup_contribution=create_contribution(".debug$G",0,0,0,0,0,0,0,
            debug_section_defaults.flags,0,output_module,1,sections,header);
        fixup_contribution->flags |= 0x2000;
        fixup_contribution->graph=create_comdat_node(fixup_contribution,NULL,NULL,&comdat_nodes,1);
        codeview_contribution=create_contribution(".debug$H",0,0,0,0,0,0,0,
            debug_section_defaults.flags,0,output_module,1,sections,header);
        codeview_contribution->flags |= 0x2000;
        codeview_contribution->graph=create_comdat_node(codeview_contribution,NULL,NULL,&comdat_nodes,1);
        if (image->options.debug_types&1) coff_debug_contribution->raw_size=32;
        if (image->options.debug_types&0x10) fixup_contribution->raw_size=16;
        if (image->options.debug_types&2) codeview_contribution->raw_size=8;
    }
    if (!image->options.debug_level || !find_section_by_name(".debug",sections))
        debug_section->characteristics |= 0x800;
    debug_reserved=debug_section->raw_size;
    debug_section->raw_size=0;
    if (mac_image) { initialize_mac_resources(); initialize_mac_exports(); }
    image->file.NumberOfSections+=count_output_sections(image);
    if (mac_image) {
        external=find_external(image->symbols,0,"___Init32BitLoadSeg");
        if (external && !(external->flags&0x40)) mac_loader_present=1;
        external=find_external(image->symbols,0,"___InitSwapper");
        if (external && !(external->flags&0x40)) {
            if (verbose_link && mac_image) printf("*** App is swappable ***\n");
            mac_swappable=1;
        }
        external=find_external(image->symbols,0,"___pcd_enter_pcode");
        if (external && !(external->flags&0x40)) {
            if (verbose_link && mac_image) printf("*** App contains pcode ***\n");
            mac_pcode_present=1;
        }
        dos_stub_template=(char *)&mac_dos_stub;
        if (mac_swappable) {
            ++image->file.NumberOfSections;
            mac_swap=create_contribution(".swap",0,0,0,0,0,0,0,0,0,output_module,1,sections,header);
            mac_swap->flags |= 0x2000;
            if (image->options.eliminate_unreferenced)
                mac_swap->graph=create_comdat_node(mac_swap,NULL,NULL,&comdat_nodes,1);
            mac_swap->raw_size=12;
        }
        if (!mac_small_model) {
            ++image->file.NumberOfSections;
            mac_jump_table=create_contribution(".jtable",0,0,0,0,0,0,0x1800020,0x1800020,0,
                output_module,1,sections,header);
            mac_jump_table->flags |= 0x2000;
            if (image->options.eliminate_unreferenced)
                mac_jump_table->graph=create_comdat_node(mac_jump_table,NULL,NULL,&comdat_nodes,1);
            move_section_last(mac_jump_table->group->section,sections);
            mac_jump_table->raw_size=size_mac_jump_table(image->symbols);
            ++image->file.NumberOfSections;
            mac_data_fixups=create_contribution(".dfix",0,0,0,0,0,0,0,0,0,output_module,1,sections,header);
            mac_data_fixups->flags |= 0x2000;
            if (image->options.eliminate_unreferenced)
                mac_data_fixups->graph=create_comdat_node(mac_data_fixups,NULL,NULL,&comdat_nodes,1);
        }
        if (image->options.debug_level) {
            ++image->file.NumberOfSections;
            mac_debug=create_contribution(".mscv",0,0,0,0,0,0,0,0,0,output_module,1,sections,header);
            mac_debug->flags |= 0x2000;
            if (image->options.eliminate_unreferenced)
                mac_debug->graph=create_comdat_node(mac_debug,NULL,NULL,&comdat_nodes,1);
        }
        ++image->file.NumberOfSections;
        mac_resource_map=create_contribution(".resmap",0,0,0,0,0,0,0,0,0,output_module,1,sections,header);
        mac_resource_map->flags |= 0x2000;
        if (image->options.eliminate_unreferenced)
            mac_resource_map->graph=create_comdat_node(mac_resource_map,NULL,NULL,&comdat_nodes,1);
        mac_resource_map->raw_size=image->file.NumberOfSections*6;
        move_section_last(mac_resource_map->group->section,sections);
        initialize_mac_section_numbers(image->file.NumberOfSections);
        readonly->characteristics=(readonly->characteristics&~0xe0UL)|0x100;
        section=mac_resource_map->group->section;
        section->characteristics=(section->characteristics&~0xe0UL)|0x100;
        if (!mac_small_model) {
            section=mac_data_fixups->group->section;
            section->characteristics=(section->characteristics&~0xe0UL)|0x100;
        }
        if (image->options.debug_level) {
            section=mac_debug->group->section;
            section->characteristics=(section->characteristics&~0xe0UL)|0x100;
        }
        if (mac_swappable) {
            section=mac_swap->group->section;
            section->characteristics=(section->characteristics&~0xe0UL)|0x100;
        }
    }
    __tzset();
    if (debug_requested) image->file.TimeDateStamp=0xffffffffUL;
    else time((time_t *)&image->file.TimeDateStamp);
    if (image->options.portable_image) pe_header_offset=dos_stub_size;
    header_end=20+pe_header_offset+image->file.SizeOfOptionalHeader+
        image->file.NumberOfSections*40+image_comments.length+(mapped_debug?40:0);
    header_last=header_end-1;
    header->SizeOfHeaders=(header_last/header->FileAlignment+1)*header->FileAlignment;
    if (!split_image && !image->options.rom) {
        rounded=(header->ImageBase+0xffff)&~0xffffUL;
        if (rounded!=header->ImageBase) warning_message(program_name,0x5e,header->ImageBase,rounded);
        header->ImageBase=rounded;
        header->BaseOfCode=((header->SizeOfHeaders-1)/header->SectionAlignment+1)*header->SectionAlignment;
    }
    if (!(image->file.Characteristics&IMAGE_FILE_DLL)) header->AddressOfEntryPoint=header->BaseOfCode;
    code_rva=header->BaseOfCode;
    file_cursor=(header_last/header->FileAlignment+1)*header->FileAlignment;
    order_sections(image);
    if (entry_symbol && image->options.eliminate_unreferenced)
        add_comdat_root(NULL,entry_symbol,NULL,&comdat_roots);
    while (included_externals) {
        include_next=included_externals->next;
        if (image->options.eliminate_unreferenced)
            add_comdat_root(NULL,included_externals->external,NULL,&comdat_roots);
        free(included_externals);
        included_externals=include_next;
    }
    if (image->options.eliminate_unreferenced) {
        resolve_comdat_edges(image->symbols,comdat_nodes);
        if (image->options.eliminate_unreferenced) visit_comdat_roots(comdat_roots,image->symbols);
    }
    if (verbose_link) print_comdat_graph(comdat_nodes);
    if (image->options.eliminate_unreferenced) release_comdat_storage();
    if (mac_image) {
        set_mac_export_mode(0);
        build_mac_resources(image);
        if (image->options.debug_level) {
            full_name=_fullpath(NULL,output_name,0);
            mac_debug->raw_size=strlen(full_name)+mac_section_count*12+13;
            free(full_name);
            finalize_mac_debug();
        }
        if (entry_symbol) mac_entry_section=entry_symbol->owner.contribution->group->section->index;
        else fatal_message("",0x3d,NULL);
        if (mac_loader_present) initialize_mac_loader();
    }
    layout_sections(image,0x20,&code_rva,&file_cursor,&relocation_count,&line_count,&layout_state);
    if (!base_pair) header->BaseOfData=code_rva;
    data_rva=header->BaseOfData;
    if (image->options.rom) {
        rom_data_start=data_rva;
        data_file_end=file_cursor;
        layout_sections(image,0x40,&data_rva,&data_file_end,&relocation_count,&line_count,&layout_state);
        rom_bss_start=bss_start=data_rva;
        code_file_end=file_cursor;
        layout_sections(image,0x80,&data_rva,&code_file_end,&relocation_count,&line_count,&layout_state);
    } else if (mac_image) {
        mac_data_base=data_rva;
        data_file_end=code_file_end=file_cursor;
        layout_sections(image,0x100040,&data_rva,&data_file_end,&relocation_count,&line_count,&layout_state);
        layout_sections(image,0x100080,&data_rva,&code_file_end,&relocation_count,&line_count,&layout_state);
        data_file_end=file_cursor;
        layout_sections(image,0x40,&data_rva,&data_file_end,&relocation_count,&line_count,&layout_state);
        layout_sections(image,0x80,&data_rva,&code_file_end,&relocation_count,&line_count,&layout_state);
        if (!mac_small_model) {
            bytes=size_mac_data_fixups(image,data_file_end);
            mac_data_fixups->file_offset=data_file_end;
            mac_data_fixups->rva=data_rva;
            section=mac_data_fixups->group->section;
            section->file_offset=data_file_end;
            section->rva=data_rva;
            section->raw_size=((bytes-1)/header->FileAlignment+1)*header->FileAlignment;
            section->characteristics &= ~0x100UL;
            data_rva+=section->raw_size;
            data_file_end+=section->raw_size;
        }
        layout_sections(image,0x100,&data_rva,&data_file_end,&relocation_count,&line_count,&layout_state);
        if (verbose_link && mac_image) {
            printf("\nNear bss :  0x%8.lx\n",mac_near_bss_size);
            printf("Near data:  0x%8.lx\n",mac_near_data_size);
            printf("Far bss :   0x%8.lx\n",mac_far_bss_size);
            printf("Far data:   0x%8.lx\n",mac_far_data_size);
        }
        mac_data_size=mac_near_data_size+mac_far_bss_size+mac_far_data_size+mac_near_bss_size;
        if (!mac_small_model) mac_data_fixups->group->section->characteristics |= 0x100;
    } else {
        bss_start=data_rva;
        code_file_end=file_cursor;
        layout_sections(image,0x80,&data_rva,&code_file_end,&relocation_count,&line_count,&layout_state);
        code_start=data_rva;
        data_file_end=file_cursor;
        layout_sections(image,0x40,&data_rva,&data_file_end,&relocation_count,&line_count,&layout_state);
    }
    if (!image->options.fixed && !image->options.rom && !mac_image) {
        base_relocation_section->raw_size=header->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size*2;
        bytes=((data_rva-header->BaseOfData+0x1000)>>12)+((code_rva-header->BaseOfCode+0x1000)>>12);
        base_relocation_section->raw_size+=bytes*10;
    }
    layout_sections(image,0x2000020,&data_rva,&data_file_end,&relocation_count,&line_count,&layout_state);
    layout_sections(image,0x2000040,&data_rva,&data_file_end,&relocation_count,&line_count,&layout_state);
    if (header->BaseOfCode>header->BaseOfData) {
        if (data_rva>header->BaseOfCode) warning_message(output_name,0x5b,data_rva,"");
    } else if (code_rva>header->BaseOfData) warning_message(output_name,0x5c,code_rva,"");
    sort_sections_by_rva(sections);
    if (mac_image) {
        mac_resource_map->group->section->raw_size=image->file.NumberOfSections*6;
        seek_file(output_file,0,SEEK_SET);
    }
    if (image->options.portable_image) write_file(output_file,dos_stub_template,dos_stub_size);
    seek_file(output_file,20,SEEK_CUR);
    seek_file(output_file,image->file.SizeOfOptionalHeader,SEEK_CUR);
    if (!mac_image) relocation_cursor=base_relocation_section->file_offset;
    debug_file_start=debug_section->file_offset;
    if (image->options.debug_level) {
        if (image->options.debug_types&1) {
            coff_prefix=32;
            coff_lines=coff_debug_contribution->file_offset+32;
            coff_line_cursor=coff_lines;
            coff_lines_size=line_count*6;
            symbol_start=coff_lines+coff_lines_size;
            symbol_table_file_offset=symbol_start;
            string_start=symbol_start+local_symbol_count*18;
            string_size=string_table_size;
            coff_debug_contribution->raw_size=32+coff_lines_size+local_symbol_count*18+string_size;
            pad_contribution(coff_debug_contribution);
        }
        if (image->options.debug_types&0x10) {
            fixup_contribution->raw_size=relocation_count*12;
            pad_contribution(fixup_contribution);
        }
    }
    write_section_headers(0x20,&relocation_cursor,&coff_line_cursor,image);
    if (image->options.rom) {
        write_section_headers(0x40,&relocation_cursor,&coff_line_cursor,image);
        write_section_headers(0x80,&relocation_cursor,&coff_line_cursor,image);
    } else {
        write_section_headers(0x80,&relocation_cursor,&coff_line_cursor,image);
        if (mac_image) write_section_headers(0x100080,&relocation_cursor,&coff_line_cursor,image);
        write_section_headers(0x40,&relocation_cursor,&coff_line_cursor,image);
        if (mac_image) {
            write_section_headers(0x100040,&relocation_cursor,&coff_line_cursor,image);
            write_section_headers(0x100,&relocation_cursor,&coff_line_cursor,image);
        }
    }
    write_section_headers(0x2000020,&relocation_cursor,&coff_line_cursor,image);
    write_section_headers(0x2000040,&relocation_cursor,&coff_line_cursor,image);
    if (image->options.debug_level) {
        saved_position=tell_file(output_file);
        debug_section->rva=data_rva;
        debug_section->raw_size=debug_reserved;
        if (image->options.debug_types&2) codeview_start=codeview_contribution->file_offset;
        else debug_section->file_offset=debug_file_start;
        debug_section->index=image->file.NumberOfSections+1;
        if (mapped_debug) {
            ++image->file.NumberOfSections;
            make_section_header(debug_section,&section_header);
            write_section_header(output_file,&section_header);
        }
    }
    if (image_comments.length) {
        write_file(output_file,image_comments.data,image_comments.length);
        release_text(&image_comments);
    }
    zeroes=calloc(header->FileAlignment,1);
    if (!zeroes) out_of_memory();
    padding=(header_last/header->FileAlignment+1)*header->FileAlignment-header_end;
    if (padding) write_file(output_file,zeroes,padding);
    write_local_imports(local_imports,&import_aliases,header,&image->options);
    if (end_symbol) {
        end_symbol->symbol.Value=data_rva-header->ImageBase;
        end_symbol->value.final_value=data_rva;
    }
    if (image->options.debug_level && (image->options.debug_types&2)) {
        module_debug_info=calloc(count_modules(image->libraries.first),sizeof(*module_debug_info));
        if (!module_debug_info) out_of_memory();
    }
    bss_size=bss_contribution->rva;
    if (mac_image) {
        mac_far_bss_offset=mac_far_bss->rva;
        if (!mac_small_model) finalize_mac_fixups();
        if (image->options.debug_level) write_mac_debug(image);
        write_mac_resources();
        if (mac_swappable) write_mac_swapper();
        debug_section->characteristics=0x42000040;
    }
    if (debug_enabled(24)) dump_symbols(image->symbols);
    abort_phase="Pass2";
    pass_two(image);
    abort_file[0]=0;
    external=find_external(image->symbols,1,"__tls_used");
    if (external && external->owner.contribution) {
        header->DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress=external->value.final_value;
        header->DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].Size=24;
    }
    if (!image->options.fixed && image->file.Machine!=0x268) {
        abort_phase="EmitRelocs";
        if (image->options.rom) emit_rom_relocations();
        else emit_base_relocations(header);
    }
    abort_phase="EmitExternals";
    seek_file(output_file,symbol_start+symbol_record_size*symbol_count,SEEK_SET);
    if (count_externals(image->symbols) &&
        (emit_symbol_class(2,&image->options) || emit_symbol_class(0x44,&image->options))) {
        if (weak_symbol_count) resolve_weak_externals(image->symbols,0x40c0);
        if (image->options.map_format==1) write_map(1,image,output_name);
        emit_externals(image->symbols,1,1,0,&image->options);
    }
    if (image->options.debug_level) {
        if (image->options.debug_types&1) {
            string_start=symbol_start+symbol_count*18;
            coff_size=coff_prefix+coff_lines_size+symbol_count*18+string_size;
        }
        debug_end=debug_section->file_end;
        if (image->options.debug_types&1) {
            abort_phase="EmitStrings";
            seek_file(output_file,string_start,SEEK_SET);
            emit_string_table(image->symbols);
            debug_end=tell_file(output_file);
        }
        if (image->options.debug_types&8) {
            fpo_group=find_group(debug_section,".debug$F");
            if (fpo_group) debug_end=fpo_group->file_offset+fpo_group->size;
        }
        if (image->options.debug_types&0x10)
            debug_end=fixup_contribution->raw_size+fixup_contribution->file_offset;
        if (image->options.debug_types&2) {
            abort_phase="EmitCodeView";
            seek_file(output_file,debug_section->file_end,SEEK_SET);
            emit_codeview(image);
            debug_end=tell_file(output_file);
        }
        debug_section->raw_size=debug_end-debug_section->file_offset;
        debug_section->file_end=debug_section->file_offset+debug_section->raw_size;
        debug_section->virtual_size=debug_section->raw_size;
        if (mapped_debug) {
            seek_file(output_file,saved_position,SEEK_SET);
            make_section_header(debug_section,&section_header);
            write_section_header(output_file,&section_header);
        }
        header->DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress=readonly->rva;
        fpo_group=find_group(debug_section,".debug$F");
        if ((image->options.debug_types&8) && fpo_group && fpo_group->size)
            header->DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size+=28;
        if (image->options.debug_types&0x10) header->DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size+=28;
        if (image->options.debug_types&1) header->DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size+=28;
        if (image->options.debug_types&2) header->DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size+=28;
        directory.Characteristics=0;
        directory.TimeDateStamp=image->file.TimeDateStamp;
        directory.MajorVersion=directory.MinorVersion=0;
        seek_file(output_file,debug_directory_contribution->file_offset,SEEK_SET);
        if (image->options.debug_types&1) {
            directory.AddressOfRawData=mapped_debug?coff_debug_contribution->rva:0;
            directory.PointerToRawData=coff_debug_contribution->file_offset;
            directory.SizeOfData=coff_size;
            directory.Type=1;
            write_file(output_file,&directory,28);
        }
        if (image->options.debug_types&2) {
            directory.AddressOfRawData=mapped_debug?codeview_contribution->rva:0;
            directory.PointerToRawData=codeview_contribution->file_offset;
            directory.SizeOfData=debug_section->file_end-codeview_contribution->file_offset;
            directory.Type=2;
            write_file(output_file,&directory,28);
        }
        if ((image->options.debug_types&8) && fpo_group && fpo_group->size) {
            directory.PointerToRawData=fpo_group->file_offset;
            directory.AddressOfRawData=mapped_debug?fpo_group->rva:0;
            directory.SizeOfData=fpo_group->size;
            directory.Type=3;
            write_file(output_file,&directory,28);
            value=tell_file(output_file);
            seek_file(output_file,directory.PointerToRawData,SEEK_SET);
            fpo=malloc(directory.SizeOfData+10);
            fpo_entries=directory.SizeOfData>>4;
            if (!fpo) out_of_memory();
            read_file(output_file,fpo,directory.SizeOfData);
            qsort(fpo,fpo_entries,16,compare_fpo_records);
            seek_file(output_file,directory.PointerToRawData,SEEK_SET);
            write_file(output_file,fpo,directory.SizeOfData);
            seek_file(output_file,value,SEEK_SET);
            free(fpo);
        }
        if (image->options.debug_types&0x10) {
            directory.AddressOfRawData=mapped_debug?fixup_contribution->rva:0;
            directory.PointerToRawData=fixup_contribution->file_offset;
            directory.Type=6;
            directory.SizeOfData=(fixup_count*341+fixups_in_block)*12;
            write_file(output_file,&directory,28);
        }
        if (image->options.debug_types&1) {
            seek_file(output_file,coff_debug_contribution->file_offset,SEEK_SET);
            coff.NumberOfSymbols=symbol_count;
            coff.LvaToFirstSymbol=symbol_start-debug_file_start;
            coff.NumberOfLinenumbers=line_count;
            coff.LvaToFirstLinenumber=coff_lines-debug_file_start;
            coff.RvaToFirstByteOfCode=header->BaseOfCode;
            coff.RvaToLastByteOfCode=header->BaseOfCode+header->SizeOfCode;
            coff.RvaToFirstByteOfData=header->BaseOfData;
            coff.RvaToLastByteOfData=header->BaseOfCode+header->SizeOfUninitializedData+header->SizeOfInitializedData;
            write_file(output_file,&coff,32);
        }
        if (image->options.debug_types&2) {
            signature=0x3530424e; /* NB05 */
            seek_file(output_file,codeview_contribution->file_offset,SEEK_SET);
            write_file(output_file,&signature,4);
            value=codeview_end-codeview_start;
            write_file(output_file,&value,4);
        }
        if (image->options.debug_types&0x10) {
            seek_file(output_file,fixup_contribution->file_offset,SEEK_SET);
            while (fixup_blocks) {
                FixupBlock *next;
                write_file(output_file,fixup_blocks->records,fixups_in_block*12);
                next=fixup_blocks->next;
                free(fixup_blocks);
                fixup_blocks=next;
                fixups_in_block=341;
            }
        }
    }
    if (!mac_image) finish_pe_image(image,zeroes);
    free(zeroes);
    abort_phase="FinalPhase";
    if (image->options.debug_level) {
        image->file.NumberOfSymbols=symbol_count;
        image->file.PointerToSymbolTable=symbol_start;
    }
    if (!line_count) image->file.Characteristics |= IMAGE_FILE_LINE_NUMS_STRIPPED;
    if (!undefined_symbols || image->options.force) image->file.Characteristics |= IMAGE_FILE_EXECUTABLE_IMAGE;
    if (image->options.debug_level!=3) image->file.Characteristics |= IMAGE_FILE_LOCAL_SYMS_STRIPPED;
    seek_file(output_file,pe_header_offset,SEEK_SET);
    write_coff_header(output_file,&image->file);
    if (image->options.debug_level && mapped_debug) {
        header->SizeOfImage=((debug_section->rva+debug_section->raw_size-1)/header->SectionAlignment+1)*header->SectionAlignment;
        header->SizeOfInitializedData+=debug_section->raw_size;
    } else header->SizeOfImage=data_rva;
    if (header->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress) {
        section=find_section(".idata",idata_directory_defaults.flags,sections,header);
        header->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size=section->file_end-section->file_offset;
    }
    if (entry_symbol && (entry_symbol->flags&1)) header->AddressOfEntryPoint=entry_symbol->value.final_value;
    failed=undefined_symbols && !image->options.force;
    *pack=!failed && image->options.debug_level==3 && (image->options.debug_types&2) && !image->options.no_pack;
    if (image->options.checksum && *pack) header->CheckSum=0xffffffffUL;
    if (!mac_image && header->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size)
        header->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress=base_relocation_section->rva;
    if (image->options.rom && (image->file.Machine==0x162 || image->file.Machine==0x166)) {
        rom.magic=0x107;
        rom.linker_major=header->MajorLinkerVersion; rom.linker_minor=header->MinorLinkerVersion;
        rom.code_size=header->SizeOfCode; rom.data_size=header->SizeOfInitializedData;
        rom.bss_size=header->SizeOfUninitializedData; rom.entry=header->AddressOfEntryPoint;
        rom.code_base=header->BaseOfCode; rom.data_base=rom_data_start; rom.bss_base=bss_start;
        rom.gpr_mask=rom.cpr_mask[0]=rom.cpr_mask[1]=rom.cpr_mask[2]=rom.cpr_mask[3]=0;
        rom.gp_value=gp_symbol?gp_symbol->value.final_value+rom.code_base:0;
        write_file(output_file,&rom,image->file.SizeOfOptionalHeader);
    } else write_optional_header(output_file,header,image->file.SizeOfOptionalHeader);
    if (failed) { fatal_message(output_name,0x17,undefined_symbols); return 0x17; }
    if (image->options.checksum && !*pack) update_checksum(output_file);
    return 0;
}

/* COFF-only debugging retains selected local and external symbol records. */
unsigned char emit_symbol_class(unsigned char storage_class,ImageOptions *options)
{
    if (!options->debug_level || !(options->debug_types&1)) return 0;
    switch (storage_class) {
    case IMAGE_SYM_CLASS_EXTERNAL:
    case LINK_SYM_CLASS_FAR_EXTERNAL:
    case IMAGE_SYM_CLASS_WEAK_EXTERNAL:
        return 1;
    case IMAGE_SYM_CLASS_STATIC:
    case IMAGE_SYM_CLASS_FILE:
        return !options->rom;
    case IMAGE_SYM_CLASS_UNDEFINED_LABEL:
    case IMAGE_SYM_CLASS_UNDEFINED_STATIC:
        return 0;
    default:
        return options->debug_level==3;
    }
}
