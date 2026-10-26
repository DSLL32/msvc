/* The full-link driver is one original function, including its cold section
 * merge continuation. C++ is used only for its actual thiscall boundaries. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <io.h>
#include <sys/stat.h>
#include "link.h"
#include "initializers.h"
#include "../crt/crt-imports.h"
#include "full-link-functions.h"
#include "memory.h"
#include <recon.h>

extern "C" {
extern BYTE pdb_enabled, incremental_enabled, incremental_reuse;
extern BYTE reproducible_build, mapped_debug, verbose_link;
extern const char *output_file_name, *pdb_file_name, *map_file_name;
extern const char *default_phase;
extern char current_phase[36];
extern FILE *map_output;
extern ByteBuffer image_comments;
extern InputArguments section_merges, section_attributes, weak_mac_imports, weak_mac_libraries;

/* Defined as each phase first consumes them, rather than copying an image's
 * unclassified data. Other passes will populate these named objects. */
OutputModule *output_module;
OutputSection *small_data_section, *read_only_section, *data_section;
OutputSection *bss_section, *exception_section, *procedure_section;
OutputSection *export_section, *debug_section;
Contribution *debug_directory, *coff_debug, *misc_debug, *fixup_debug, *codeview_debug;
Contribution *incremental_code, *mac_toc, *mac_function_info, *mac_data;
ContributionGroup *mac_procedure_group;
StringNode *entry_symbol, *gp_symbol, *mac_toc_symbol, *mac_function_symbol;
ComdatRoot *comdat_roots;
MacResource *mac_resource_list;
DWORD undefined_symbols, local_symbol_count, coff_string_size, fixup_count;
DWORD emitted_symbol_count, coff_symbol_offset, codeview_offset;
DWORD gp_base, gp_limit, incremental_weak_symbols;
BYTE duplicate_symbols, mac_target, large_alpha_image, large_ppc_image;
BYTE file_access_statistics;
int output_file;
void *module_debug_table;
char *old_image_name;
extern char short_symbol_name[9];
}

/* These were inline file-cache operations in this driver. Keep their expansion
 * here so there is no extra reconstructed function in the call graph. */
#define FILE_RECORD(fd) (file_slots[(fd)])
#define WRITE_FILE(fd,data,bytes) do { \
    if (file_access_statistics) record_file_access(); \
    file_record=FILE_RECORD(fd); \
    if (file_record->flags & 8) mapped_write(file_record,(data),(bytes)); \
    else buffered_write(file_record,(data),(bytes)); \
} while (0)
#define READ_FILE(fd,data,bytes) do { \
    if (file_access_statistics) record_file_access(); \
    file_record=FILE_RECORD(fd); \
    if (file_record->flags & 8) mapped_read(file_record,(data),(bytes)); \
    else buffered_read(file_record,(data),(bytes)); \
} while (0)
/* The buffered descriptor's position is the one consumed by this driver. Its
 * other fields remain opaque until the buffered I/O implementation is reached. */
typedef struct BufferedPosition { DWORD reserved[3], position; } BufferedPosition;
#define TELL_FILE(fd,destination) do { \
    if (file_access_statistics) record_file_access(); \
    file_record=FILE_RECORD(fd); \
    destination=(file_record->flags & 8) ? file_record->file_size : \
        ((BufferedPosition *)file_record->base)->position; \
} while (0)
#define ROUND_UP(value,alignment) ((((value)-1)/(alignment)+1)*(alignment))
#define COMDATS_ENABLED (options.effective_reference_opt || options.fold_identical_comdats)
#define ATTACH_DEBUG(contribution) do { \
    if (COMDATS_ENABLED) attach_comdat((contribution),NULL,1); \
} while (0)
#define FINISH_INTERVAL(label) do { \
    interval_start=timing.end_tick; \
    timing.end_tick=GetTickCount(); \
    elapsed=timing.end_tick-interval_start; \
    if (timing.display) { \
        fprintf(timing.output,"%s: ",(label)); \
        ++timing.options; \
        fprintf(timing.output,"Interval #%d, time = %d.%03ds\n", \
            timing.options,elapsed/1000,elapsed%1000); \
        fflush(timing.output); \
    } \
} while (0)

#pragma intrinsic(strlen,strcmp,memcpy)
int RECON_THISCALL LinkContext::FullLink(BYTE *pack_requested)
{
    IncludeSymbol *include=includes, *next_include, *aliases;
    Contribution *local_imports, *previous_incremental;
    DWORD first_relocation_count, second_relocation_count;
    void *first_relocations, *second_relocations;
    DWORD directory_size, index, extra_size, header_size, section_headers_offset;
    DWORD image_rva, file_offset, line_count, line_offset, first_line_offset;
    DWORD original_debug_offset, debug_start, debug_end, upper;
    DWORD interval_start, elapsed;
    void *extra_dos, *padding, *resource_data, *mapped_output;
    BYTE borrowed_resource, failed;
    int resource_file;
    char resource_name[36], *comma;
    const char *name;
    WORD name_kind;
    StringNode *symbol;
    OutputSection *section;
    SectionEnumerator section_enumerator;
    InputArgument *merge;
    MacResource *resource;
    FileRecord *file_record;
    LinkContext old_image;
    struct _stat old_information;

    initialize_output_modules(&output_module);
    initialize_contribution_pools();
    if (COMDATS_ENABLED) initialize_comdats();
    InitializeSections();
    if (options.debug_level) {
        directory_size=0;
        if (options.debug_formats & 4) directory_size+=28;
        if (options.debug_formats & 8) directory_size+=28;
        if (options.debug_formats & 1) directory_size+=28;
        if (options.debug_formats & 2) directory_size+=28;
        if (options.debug_formats & 16) directory_size+=28;
        if (directory_size && options.rom) directory_size+=28;
        debug_directory=create_contribution(is_vxd==1 ? ".debug" : ".rdata",
            directory_size,0x40000040,0x40000040,output_module,&sections,this);
        ATTACH_DEBUG(debug_directory);
    }
    default_phase="Pass1";
    PassOne();
    if (pdb_enabled) {
        pdb_file_name=derive_pdb_name(output_file_name,pdb_file_name);
        initialize_pdb(options.mppc32_types,pdb_file_name);
        if (!incremental_reuse) initialize_pdb_image(output_file_name);
    }
    if (!incremental_enabled) {
        old_image_name=derive_old_image_name(this);
        if (old_image_name) {
            if (read_old_image(old_image_name,0,&old_image,&old_information))
                _unlink(old_image_name);
            release_memory(old_image_name);
        }
    }
    if (options.write_map) {
        if (!map_file_name) map_file_name=replace_extension(output_file_name,".map");
        map_output=fopen(map_file_name,"wt");
        if (!map_output) link_fatal(NULL,1104,map_file_name);
    }
    ApplySectionAttributes();
    if (mac_target) PrepareMacResources();
    if (file_header.Machine==0x166 || file_header.Machine==0x168 ||
        file_header.Machine==0x1f0 || file_header.Machine==0x184) {
        section=find_output_section(".sbss",&sections);
        if (section && section_data_size(section)) link_fatal(NULL,1156);
        if (section_data_size(small_data_section)) {
            if (file_header.Characteristics & IMAGE_FILE_DLL) link_warning(NULL,4066);
            gp_symbol=symbols->LookupSymbol(1,"_gp",NULL);
            if (gp_symbol->flag_byte & 1)
                link_contribution_fatal(gp_symbol->contribution,1155,"_gp");
            set_symbol_defined(gp_symbol,1,symbols);
            gp_symbol->symbol.SectionNumber=-2;
            ++local_symbol_count;
        }
    }
    if (mac_target) {
        mac_toc_symbol=symbols->LookupSymbol(1,"__TocTb",NULL);
        if (mac_toc_symbol->flag_byte & 1)
            link_contribution_fatal(mac_toc_symbol->contribution,1155,"__TocTb");
        set_symbol_defined(mac_toc_symbol,1,symbols);
        mac_toc_symbol->contribution=NULL;
        mac_toc_symbol->symbol.Value=0;
        mac_toc_symbol->final_value=0;
        mac_toc_symbol->symbol.SectionNumber=-2;
        ++local_symbol_count;
        mac_function_symbol=symbols->LookupSymbol(1,"__FTInfo",NULL);
        if (mac_function_symbol->flag_byte & 1)
            link_contribution_fatal(mac_function_symbol->contribution,1155,"__FTInfo");
        set_symbol_defined(mac_function_symbol,1,symbols);
        mac_function_symbol->contribution=NULL;
        mac_function_symbol->symbol.Value=0;
        mac_function_symbol->final_value=0;
        mac_function_symbol->symbol.SectionNumber=-2;
        ++local_symbol_count;
    }
    if (entry_symbol) {
        if (!(entry_symbol->flag_byte & 1)) ResolveEntryPoint();
        if ((file_header.Characteristics & IMAGE_FILE_DLL) &&
            (optional_header.Subsystem==2 || optional_header.Subsystem==3) &&
            file_header.Machine==0x14c) {
            name=get_symbol_name(entry_symbol,symbols->hash.string_storage,
                                 &name_kind,NULL);
            if (name_kind==1) name=strncpy(short_symbol_name,name,8);
            if (*name!='?' && !(*name=='_' && strlen(name)>4 &&
                               !strcmp(name+strlen(name)-3,"@12")))
                link_warning(output_file_name,4086,name);
        }
    }
    if (mac_target) {
        PrepareMacSymbols();
        ResolveMacImports();
        ResolveMacExports();
    }
    ResolveLocalImports(&local_imports,&aliases);
    ResolveUndefined();
    if (options.retain_relocations)
        CollectRelocationSymbols(&first_relocation_count,&first_relocations,
                                 &second_relocation_count,&second_relocations);
    symbols->SortSymbols();
    if (undefined_symbols && !(options.force & 1))
        link_fatal(output_file_name,1120,undefined_symbols);
    if (duplicate_symbols) {
        if (!(options.force & 2)) link_fatal(output_file_name,1169);
        if (incremental_enabled) link_warning(output_file_name,4071);
    }
    if (duplicate_symbols || undefined_symbols) link_warning(output_file_name,4088);
    ApplyMerges();
    if (section_attributes.count) {
        initialize_section_enumerator(&section_enumerator,&sections);
        while (next_output_section(&section_enumerator))
            apply_section_merge(section_enumerator.current);
        end_section_enumerator(&section_enumerator);
    }
    merge=section_attributes.first;
    for (index=0;(WORD)index<section_attributes.count;++index,merge=merge->next) {
        if (!(merge->flags & 0x100)) {
            comma=strchr(merge->name,',');
            if (comma) *comma=0;
            link_warning(NULL,4039,merge->name);
            if (comma) *comma=',';
        }
    }
    if (!mac_target) {
        prepare_import_sections();
        if (mac_target) goto mac_sections;
        if (!options.rom && is_vxd!=1) {
            if (data_section && small_data_section->characteristics==data_section->characteristics)
                merge_section_groups(small_data_section,data_section);
            if (exception_section->characteristics==read_only_section->characteristics)
                merge_section_groups(exception_section,read_only_section);
            FinalizeOrder();
            if (optional_header.Subsystem==1 && options.retain_relocations)
                PrepareNativeRelocations();
            prepare_delay_imports();
            if (CanMergeReadOnlyData() && data_section &&
                (bss_section->characteristics | 0xc0)==(data_section->characteristics | 0xc0))
                merge_output_section(bss_section,data_section);
            MergeExceptionSections();
            if (optional_header.Subsystem!=1 &&
                export_section->characteristics==read_only_section->characteristics)
                merge_output_section(export_section,read_only_section);
            goto sections_ready;
        }
    } else {
mac_sections:
        PrepareMacSections();
        PrepareMacJumpTables();
        PrepareMacDebugSections();
        PrepareMacRuntimeSections();
        merge_section_groups(read_only_section,data_section);
        merge_section_groups(procedure_section,data_section);
        mac_procedure_group=find_contribution_group(data_section,".pdata");
        merge_output_section(bss_section,data_section);
    }
    FinalizeOrder();
sections_ready:
    PrepareAuxiliarySections();
    if (!options.debug_level) debug_section->characteristics |= 0x800;
    else {
        if (options.debug_formats & 1) {
            coff_debug=create_contribution(".debug$C",32,0x42100040,0x42100040,
                                            output_module,&sections,this);
            ATTACH_DEBUG(coff_debug);
        }
        if (options.debug_formats & 16) {
            misc_debug=create_contribution(options.misc_rdata ? ".rdata$A" : ".debug$E",0,
                options.misc_rdata ? 0x40000040 : 0x42100040,
                options.misc_rdata ? 0x40000040 : 0x42100040,output_module,&sections,this);
            ATTACH_DEBUG(misc_debug);
            misc_debug->raw_size=0x110;
            misc_debug->raw_size=(misc_debug->raw_size+3)&~3;
        }
        if (options.debug_formats & 8) {
            fixup_debug=create_contribution(".debug$G",fixup_count*12,
                0x42100040,0x42100040,output_module,&sections,this);
            ATTACH_DEBUG(fixup_debug);
        }
        if (options.debug_formats & 2) {
            codeview_debug=create_contribution(".debug$H",0,
                0x42300040,0x42300040,output_module,&sections,this);
            ATTACH_DEBUG(codeview_debug);
            if (codeview_debug->group->alignment<=4) codeview_debug->group->alignment=4;
            codeview_debug->raw_size=pdb_enabled ? strlen(pdb_file_name)+17 : 8;
        }
    }
    debug_section->raw_size=0;
    if (mac_target) {
        for (resource=mac_resource_list,index=0;resource;resource=resource->next,++index) {
            switch (resource->kind) {
            case 1: name=";;dat%d"; break;
            case 2: name=";;afp%d"; break;
            default: name=";;res%d";
            }
            sprintf(resource_name,name,(WORD)index);
            resource->contribution=create_contribution(resource_name,resource->size,
                0x2000040,0x2000040,output_module,&sections,this);
            ATTACH_DEBUG(resource->contribution);
        }
    }
    file_header.NumberOfSections+=CountOutputSections();
    if (incremental_enabled) incremental_code=CreateIncrementalContribution();
    if (mac_target) {
        move_contribution_to_end(mac_function_info);
        move_contribution_to_end(mac_data);
        move_contribution_to_end(mac_toc);
        move_group_to_end(mac_toc->group);
    }
    extra_size=BuildDosExtension(&extra_dos);
    pe_header_offset=dos_image_size+extra_size;
    _tzset();
    if (reproducible_build) file_header.TimeDateStamp=0xffffffff;
    else time((time_t *)&file_header.TimeDateStamp);
    header_size=(this->*image_header_size)(&section_headers_offset);
    if (is_vxd==1 && header_size<options.header_size) header_size=options.header_size;
    optional_header.SizeOfHeaders=ROUND_UP(header_size,optional_header.FileAlignment);
    if (options.checksum_enabled && is_vxd==0 && !options.rom && !options.retain_relocations)
        optional_header.SizeOfHeaders+=optional_header.FileAlignment;
    file_offset=optional_header.SizeOfHeaders;
    if (!options.image_base_set) {
        if (file_header.Characteristics & IMAGE_FILE_DLL) optional_header.ImageBase=0x10000000;
        else if (optional_header.Subsystem==9) optional_header.ImageBase=0x10000;
        else optional_header.ImageBase=0x400000;
    }
    if (optional_header.ImageBase<0x400000 && file_header.Machine==0x14c &&
        (optional_header.Subsystem==2 || optional_header.Subsystem==3))
        link_warning(NULL,4096,optional_header.ImageBase);
    if (!options.rom) {
        optional_header.ImageBase=align_image_base(optional_header.ImageBase);
        optional_header.BaseOfCode=ROUND_UP(optional_header.SizeOfHeaders,
                                           optional_header.SectionAlignment);
    }
    image_rva=optional_header.BaseOfCode;
    line_count=0;
    while (include) {
        next_include=include->next;
        if (COMDATS_ENABLED) add_comdat_root(NULL,include->symbol,NULL,&comdat_roots);
        if (!incremental_enabled) release_memory(include);
        include=next_include;
    }
    if (COMDATS_ENABLED) {
        if (entry_symbol) add_comdat_root(NULL,entry_symbol,NULL,&comdat_roots);
        if (is_vxd==0) {
            name=(file_header.Machine==0x14c || file_header.Machine==0x1a2 ||
                  file_header.Machine==0x1a6) ? "__tls_used" : "_tls_used";
            symbol=symbols->FindSymbol(name);
            if (symbol) add_comdat_root(NULL,symbol,NULL,&comdat_roots);
            name=(file_header.Machine==0x14c || file_header.Machine==0x1a2 ||
                  file_header.Machine==0x1a6) ? "__load_config_used" : "_load_config_used";
            symbol=symbols->FindSymbol(name);
            if (symbol) add_comdat_root(NULL,symbol,NULL,&comdat_roots);
        }
        add_special_comdat_roots(symbols);
    }
    if (options.effective_reference_opt) {
        eliminate_unused_comdats(comdat_roots,symbols);
        finish_comdat_elimination();
        if (verbose_link) print_removed_comdats();
    }
    if (options.fold_identical_comdats) {
        FoldIdenticalComdats();
        if (verbose_link) PrintFoldedComdats();
    }
    PrepareSectionLayout();
    if (file_header.Machine==0x184 &&
        RangeCodeBytes(image_rva,0x400000,&previous_incremental)>=0x400000) {
        large_alpha_image=1;
        if (incremental_enabled && previous_incremental) {
            /* Detach the incremental contribution from its old group, then
             * place it immediately after the range selected for branch thunks. */
            incremental_code->group->first=incremental_code->next;
            incremental_code->next=previous_incremental->next;
            previous_incremental->next=incremental_code;
            incremental_code->group=previous_incremental->group;
        }
    }
    incremental_contribution=incremental_code;
    if (file_header.Machine==0x1f0 && RangeCodeBytes(image_rva,0,NULL)>=0x2000000)
        large_ppc_image=1;
    LayoutSections(&image_rva,&file_offset,&line_count);
    if (gp_symbol) {
        update_gp_bounds();
        gp_symbol->symbol.Value=(gp_base+0x8000)&~7;
        gp_symbol->final_value=gp_symbol->symbol.Value;
        if (file_header.Machine==0x1f0 && gp_limit-(gp_base&~7)<0x8000) {
            gp_symbol->symbol.Value=gp_base&~7;
            gp_symbol->final_value=gp_symbol->symbol.Value;
        }
    }
    if (mac_target) {
        mac_toc_symbol->final_value=mac_toc_symbol->symbol.Value+mac_toc->rva;
        mac_function_symbol->final_value=mac_function_symbol->symbol.Value+mac_function_info->rva;
        set_mac_symbol_addresses();
        set_mac_section_addresses(&sections);
    }
    if (!options.rom) {
        upper=0;
        seek_file(output_file,0,0,&upper);
        /* The original contains two consecutive instrumentation hooks before
         * this first write. They are separate calls, even though neither has
         * a C argument. */
        if (file_access_statistics) record_file_access();
        WRITE_FILE(output_file,dos_image,dos_image_size);
        WRITE_FILE(output_file,extra_dos,extra_size);
        upper=0;
        seek_file(output_file,0x3c,0,&upper);
        WRITE_FILE(output_file,&pe_header_offset,4);
    }
    release_memory(extra_dos);
    original_debug_offset=debug_section->file_offset;
    if (options.debug_formats & 1) {
        line_offset=coff_debug->file_offset+32;
        coff_symbol_offset=line_offset+line_count*6;
        coff_debug->raw_size=coff_string_size+local_symbol_count*18+32+line_count*6;
        first_line_offset=line_offset;
        pad_debug_contribution(coff_debug);
    }
    upper=0;
    seek_file(output_file,section_headers_offset,0,&upper);
    WriteSections(&line_offset);
    if (is_vxd==1) {
        WriteVxdModuleTable(output_file);
        WriteVxdNames(output_file);
    }
    if (options.debug_level && is_vxd!=1) {
        TELL_FILE(output_file,debug_start);
        debug_section->rva=image_rva;
        if (options.debug_formats & 2) codeview_offset=codeview_debug->file_offset;
        else debug_section->file_offset=original_debug_offset;
        debug_section->index=file_header.NumberOfSections+1;
        if (mapped_debug) ++file_header.NumberOfSections;
    }
    padding=zero_allocate(optional_header.FileAlignment);
    if (is_vxd==0) {
        if (image_comments.used) {
            comment_size=image_comments.used;
            WRITE_FILE(output_file,image_comments.data,image_comments.used);
            clear_buffer(&image_comments);
        }
        directory_size=ROUND_UP(header_size,optional_header.FileAlignment)-header_size;
        if (directory_size) WRITE_FILE(output_file,padding,directory_size);
        if (options.checksum_enabled && !options.rom && !options.retain_relocations)
            WRITE_FILE(output_file,padding,optional_header.FileAlignment);
    }
    if (options.debug_formats & 2)
        module_debug_table=zero_allocate(count_debug_modules(pointer_list.first)*68);
    resources=mac_resource_list;
    if (mac_target) {
        for (resource=mac_resource_list;resource;resource=resource->next) {
            resource_file=0;
            borrowed_resource=1;
            if (!resource->filename) resource_data=resource->data;
            else {
                resource_file=open_argument_file(resource->filename,0x8000,0);
                resource_data=map_file_range(resource_file,0,resource->size);
                if (!resource_data) {
                    resource_data=allocate_memory(resource->size);
                    upper=0;
                    seek_file(resource_file,0,0,&upper);
                    READ_FILE(resource_file,resource_data,resource->size);
                    borrowed_resource=0;
                }
            }
            mapped_output=map_file_range(output_file,resource->contribution->file_offset,
                                         resource->size);
            if (mapped_output) memcpy(mapped_output,resource_data,resource->size);
            else {
                upper=0;
                seek_file(output_file,resource->contribution->file_offset,0,&upper);
                WRITE_FILE(output_file,resource_data,resource->size);
            }
            if (!borrowed_resource) release_memory(resource_data);
            if (resource->filename) close_argument_file(resource_file,0);
        }
    }
    if (COMDATS_ENABLED) release_comdat_tables();
    if (pdb_enabled && pdb_module_count) {
        pdb_modules=allocate_link_memory(pdb_module_count,4);
        allocate_pdb_module_table(pdb_module_count);
    }
    if (!mac_target) auxiliary->PrepareOutput();
    else if (incremental_enabled && mac_module_count) {
        mac_modules=allocate_link_memory(mac_module_count,4);
        allocate_mac_incremental_table(mac_module_count,options.fixed_image);
    }
    if (mac_target) {
        prepare_mac_resource_map();
        set_mac_entry_point(entry_symbol,this);
        prepare_mac_data_headers(this);
        prepare_mac_debug_information(this);
    }
    FINISH_INTERVAL("Pass 1");
    default_phase="Pass2";
    PassTwo();
    WriteLocalImports(local_imports,&aliases);
    if (options.retain_relocations)
        WriteRelocationSymbols(first_relocation_count,&first_relocations,
                               second_relocation_count,&second_relocations);
    current_phase[0]=0;
    auxiliary->FinishOutput(this);
    if (!mac_target) FinishNativeSections();
    if (mac_target) {
        if (mac_module_count) write_mac_incremental_table(this,mac_procedure_group->file_offset);
        if (mac_target) {
            finish_mac_headers(this);
            finish_mac_entry_point(entry_symbol,this);
            clear_argument_list(&weak_mac_imports);
            clear_argument_list(&weak_mac_libraries);
        }
    }
    clear_argument_list(&input_files);
    if (options.debug_formats & 1) {
        upper=0;
        seek_file(output_file,coff_symbol_offset+emitted_symbol_count*18,0,&upper);
        write_coff_symbols(this,image_rva);
        debug_end=coff_symbol_offset+emitted_symbol_count*18;
    } else debug_end=debug_start;
    EmitDebugDirectories(debug_start,original_debug_offset,line_count,debug_end,first_line_offset);
    finish_image_layout(this,padding);
    release_memory(padding);
    default_phase="FinalPhase";
    if (options.debug_formats & 1) {
        file_header.NumberOfSymbols=emitted_symbol_count;
        file_header.PointerToSymbolTable=coff_symbol_offset;
    } else file_header.Characteristics |= IMAGE_FILE_LOCAL_SYMS_STRIPPED;
    if (!line_count) file_header.Characteristics |= IMAGE_FILE_LINE_NUMS_STRIPPED;
    if (!undefined_symbols || (options.force & 1)) file_header.Characteristics |= IMAGE_FILE_EXECUTABLE_IMAGE;
    if (!options.debug_level || !mapped_debug) optional_header.SizeOfImage=image_rva;
    else {
        optional_header.SizeOfImage=ROUND_UP(debug_section->raw_size+debug_section->rva,
                                            optional_header.SectionAlignment);
        optional_header.SizeOfInitializedData+=ROUND_UP(debug_section->raw_size,
                                                       optional_header.FileAlignment);
    }
    FinishImage();
    failed=undefined_symbols && !(options.force & 1);
    *pack_requested=!failed && !pdb_enabled && (options.debug_formats & 2) && !options.no_pack;
    if (options.checksum_enabled && *pack_requested) optional_header.CheckSum=0xffffffff;
    (this->*image_write_headers)(output_file);
    if (large_alpha_image) finish_alpha_branches();
    if (large_ppc_image) finish_ppc_branches();
    if (failed) link_fatal(output_file_name,1120,undefined_symbols);
    if (options.checksum_enabled && !*pack_requested) UpdateChecksum();
    if (incremental_enabled && count_hash_symbols(symbols->hash.state) &&
        (emit_symbol_class(2,&options) || emit_symbol_class(0x44,&options)) &&
        incremental_weak_symbols) adjust_incremental_symbols(this);
    finish_symbol_tables();
    close_argument_file(output_file,1);
    if (pdb_enabled) {
        commit_pdb();
        close_pdb();
    }
    FINISH_INTERVAL("Pass 2");
    timing.end_tick=GetTickCount();
    if (timing.display) {
        fprintf(timing.output,"Final: Total time = %d.%03ds\n",
            (timing.end_tick-timing.start_tick)/1000,(timing.end_tick-timing.start_tick)%1000);
        fflush(timing.output);
    }
    timing.display=0;
    return 0;
}
