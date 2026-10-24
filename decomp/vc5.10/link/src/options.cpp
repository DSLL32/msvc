/* The /include branch makes a real StringTable member call. That is the only
 * C++ requirement here: keep the actual dispatcher vertex and thiscall ABI,
 * without inserting a C-to-member wrapper. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mbstring.h>
#include <io.h>
#include "link.h"
#include "crt-imports.h"
#include "options.h"
#include <recon.h>

extern "C" {
extern BYTE pdb_enabled, debug_option_present, incremental_enabled, mac_target;
extern BYTE reference_optimization_specified;
extern DWORD debug_formats, maximum_image_size;
extern char *repro_directory;
extern FILE *repro_response;

/* Globals first consumed by their corresponding option handlers. */
const char *definition_file = "", *vxd_module_name = "";
const char *import_library_name, *output_file_name, *pdb_file_name, *map_file_name;
const char *entry_point_name, *order_file_name;
BYTE reproducible_build, mapped_debug, separate_types, verbose_link, verbose_libraries;
BYTE mac_file_padding, incremental_requested, incremental_switch_set;
BYTE export_directive_changed, linker_test, linker_time, xoff;
WORD warning_level = 1;
DWORD edit_pad_size, icf_iterations = 2;
ByteBuffer image_comments;
InputArguments export_arguments, weak_mac_imports, weak_mac_libraries;
InputArguments mac_resources, mac_import_arguments, section_attributes, section_merges;
struct LibrarianContext { DWORD reserved[2]; void *library; } *librarian_context;
}

/* These macros expand inside this one original function. They add no helper
 * vertices and preserve its shared diagnostics and end-of-option cleanup. */
#define OPTION(keyword) (!_stricmp(info->name,(keyword)))
#define VALUE (info->values[(WORD)index].text)
#define KEY (info->values[(WORD)index].name)
#define FIRST (info->values[0].text)
#define REQUIRE_VALUE() do { if (!info->value_count || !*FIRST) goto missing_value; } while (0)
#define REQUIRE_UNNAMED() do { if (KEY) goto invalid_value; } while (0)

#pragma intrinsic(strlen,strcmp,strcpy,strcat,memset)
extern "C" void __stdcall configure_link_options(LinkContext *context,
                                                  DirectiveModule *module,
                                                  const char *file)
{
    StringTable *symbols = context->symbols;
    InputArgument *argument = option_arguments.first;
    SwitchInfo *info;
    char *repro_option, *path, *text, *separator, *copy;
    char basename[512], extension[256], line[MAX_PATH];
    char drive[4], directory[256], stub_path[MAX_PATH];
    char import_basename[256], import_extension[256];
    const char *symbol_name, *library, *current_version, *old_code_version, *old_api_version;
    unsigned index, option_index, bytes, padded, length;
    DWORD value, major, minor, iterations;
    WORD used, machine;
    int found, number, upper;
    BYTE warnings_as_errors = 0, weak, glue;
    FILE *input;
    IncludeSymbol *include;
    StringNode *symbol;
    IMAGE_DOS_HEADER *dos_header;
    BYTE *dos_bytes;

    for (option_index=0; option_index<option_arguments.count; ++option_index) {
        info=parse_switch(file,argument->name);
        used=0;
        index=0;
        repro_option=duplicate_argument(argument->name);
        if (!strcmp(info->name,"?")) linker_usage();

        if (OPTION("align")) {
            REQUIRE_VALUE(); index=1;
            if (!switch_number(info,0,&value)) goto bad_number;
            if (!value || (value & (value-1))) link_warning(file,4043,value);
            else {
                context->specified_switches |= 1;
                context->optional_header.SectionAlignment=value;
            }
            used=1;
            goto check_extra;
        } else if (OPTION("base")) {
            REQUIRE_VALUE();
            if (context->options.image_base_set && module) goto next_option;
            if (*FIRST=='@') {
                if (info->value_count<2 || !*info->values[1].text) goto missing_value;
                path=canonicalize_argument(FIRST+1,".txt");
                if (repro_directory) {
                    copy_repro_file(path,0);
                    _splitpath(path,NULL,NULL,basename,extension);
                    release_memory(repro_option);
                    repro_option=(char *)allocate_memory(strlen(basename)+strlen(extension)+
                        strlen("base:@\".\\%s%s,%s\"")+strlen(info->values[1].text));
                    sprintf(repro_option,"base:@\".\\%s%s,%s\"",basename,extension,info->values[1].text);
                }
                input=fopen(path,"rt");
                if (!input) link_fatal(file,1104,path);
                found=0;
                do {
                    if (!fgets(line,MAX_PATH,input)) break;
                    line[strlen(line)-1]=0;
                    separator=strchr(line,';');
                    if (separator) *separator=0;
                    text=strtok(line," \t");
                    if (text && !_stricmp(text,info->values[1].text)) {
                        text=strtok(NULL," \t");
                        if (!text || (found=sscanf(text,"%li",&value))!=1)
                            link_fatal(path,1111,text);
                        context->optional_header.ImageBase=value;
                        text=strtok(NULL," \t");
                        if (!text || sscanf(text,"%li",&maximum_image_size)!=1)
                            link_fatal(path,1111,text);
                    }
                } while (!found);
                fclose(input);
                if (!found) link_warning(path,4198,info->values[1].text);
                release_memory(path);
                used=2;
            } else if (info->value_count==1) {
                if (!switch_number(info,0,&context->optional_header.ImageBase)) goto bad_number;
                used=1;
            } else if (info->value_count==2) {
                if (!switch_number(info,0,&context->optional_header.ImageBase)) goto bad_number;
                index=1;
                if (!switch_number(info,1,&maximum_image_size)) goto bad_number;
                used=2;
            } else link_fatal(file,1111,argument->name+5);
            context->options.image_base_set=1;
            goto check_extra;
        } else if (OPTION("brepro")) {
            reproducible_build=1;
            goto check_extra;
        } else if (OPTION("comment")) {
            if (!info->value_count) goto missing_value;
            append_bytes(&image_comments,argument->name+8,strlen(argument->name+8)+1);
            context->specified_switches |= 2;
        } else if (OPTION("dbgimplib")) {
            context->options.debug_import_library=1;
            goto check_extra;
        } else if (OPTION("debug")) {
            context->options.debug_level=3;
            for (index=0;(WORD)index<info->value_count;++index) {
                REQUIRE_UNNAMED();
                if (!_stricmp(VALUE,"mapped")) mapped_debug=1;
                else if (!_stricmp(VALUE,"notmapped")) mapped_debug=0;
                else if (!_stricmp(VALUE,"full")) context->options.debug_level=3;
                else if (!_stricmp(VALUE,"partial")) context->options.debug_level=2;
                else if (!_stricmp(VALUE,"minimal")) context->options.debug_level=1;
                else if (!_stricmp(VALUE,"none")) context->options.debug_level=0;
                else goto invalid_value;
            }
        } else if (OPTION("debugtype")) {
            REQUIRE_VALUE();
            for (index=0;(WORD)index<info->value_count;++index) {
                REQUIRE_UNNAMED();
                if (!_stricmp(VALUE,"coff")) debug_formats |= 1;
                else if (!_stricmp(VALUE,"cv")) debug_formats |= 2;
                else if (!_stricmp(VALUE,"both")) debug_formats |= 3;
                else if (!_stricmp(VALUE,"fpo")) debug_formats |= 4;
                else if (!_stricmp(VALUE,"fixup")) debug_formats |= 8;
                else if (!_stricmp(VALUE,"map")) {
                    context->options.write_map=1;
                    context->options.map_lines=1;
                    context->specified_switches |= 0x20;
                } else goto invalid_value;
            }
        } else if (OPTION("def")) {
            if (!info->value_count) goto missing_value;
            definition_file=argument->name+4;
            if (repro_directory) {
                copy_repro_file(definition_file,0);
                _splitpath(definition_file,NULL,NULL,basename,extension);
                release_memory(repro_option);
                repro_option=(char *)allocate_memory(strlen("def:\".\\%s%s\"")+strlen(basename)+strlen(extension));
                sprintf(repro_option,"def:\".\\%s%s\"",basename,extension);
            }
        } else if (OPTION("defaultlib")) {
            if (!info->value_count) goto missing_value;
            if (!context->options.no_default_libraries)
                add_default_library(argument->name+11,&context->pointer_list);
        } else if (OPTION("delay")) {
            REQUIRE_VALUE();
            for (index=0;(WORD)index<info->value_count;++index) {
                REQUIRE_UNNAMED();
                if (!_stricmp(VALUE,"nobind")) context->options.bind_delay_imports=0;
                else if (!_stricmp(VALUE,"unload")) context->options.unload_delay_imports=1;
                else goto invalid_value;
                used=(WORD)(index+1);
            }
            goto check_extra;
        } else if (OPTION("delayload")) {
            if (!info->value_count) goto missing_value;
            if (has_delay_library(argument->name+10)) link_warning(NULL,4194);
            else add_delay_library(argument->name+10);
        } else if (OPTION("disallowlib")) {
            if (!info->value_count) goto missing_value;
            if (!context->options.no_default_libraries)
                disallow_library(argument->name+12,&context->pointer_list,module?module->owner:NULL);
        } else if (OPTION("dll")) {
            context->file_header.Characteristics |= 0x2000;
            for (index=0;(WORD)index<info->value_count;++index) {
                REQUIRE_UNNAMED();
                if (_stricmp(VALUE,"system")) goto invalid_value;
                context->file_header.Characteristics |= 0x1000;
            }
        } else if (OPTION("dllchar")) {
            REQUIRE_VALUE();
            for (index=0;(WORD)index<info->value_count;++index) {
                REQUIRE_UNNAMED();
                if (_stricmp(VALUE,"x86thunk")) goto invalid_value;
                context->optional_header.DllCharacteristics=0x1000;
            }
        } else if (OPTION("driver")) {
            context->options.retain_relocations=1;
            for (index=0;(WORD)index<info->value_count;++index) {
                REQUIRE_UNNAMED();
                if (!_stricmp(VALUE,"uponly")) context->file_header.Characteristics |= 0x4000;
                else if (!_stricmp(VALUE,"wdm")) context->optional_header.DllCharacteristics |= 0x2000;
                else goto invalid_value;
            }
        } else if (OPTION("editandcontinue")) {
            if (!module) {
                context->options.edit_and_continue=1;
                incremental_requested=1;
            } else if (incremental_enabled && pdb_enabled && context->options.debug_level)
                context->options.edit_and_continue=1;
        } else if (OPTION("encpadsize")) {
            REQUIRE_VALUE(); index=1;
            if (!switch_number(info,0,&value)) goto bad_number;
            edit_pad_size=value;
            context->specified_switches |= 0x800000;
        } else if (OPTION("entry")) {
            if (!info->value_count) goto missing_value;
            entry_point_name=duplicate_argument(argument->name+6);
            context->entry_name=entry_point_name;
            context->specified_switches |= 4;
        } else if (OPTION("exetype")) {
            REQUIRE_VALUE();
            for (index=0;(WORD)index<info->value_count;++index) {
                REQUIRE_UNNAMED();
                if (!_stricmp(VALUE,"dynamic")) context->dynamic_vxd=1;
                else if (_stricmp(VALUE,"dev386")) goto invalid_value;
            }
        } else if (OPTION("export")) {
            if (!info->value_count) goto missing_value;
            if (selected_tool_kind==2)
                add_librarian_export(argument->name+7,librarian_context->library,1,file);
            else if (selected_tool_kind==1) {
                if (module && context->is_vxd!=1) export_directive_changed=1;
                else append_argument_record(&export_arguments,argument->name+7,NULL);
            }
        } else if (OPTION("fixed")) {
            context->options.fixed_image=1;
            for (index=0;(WORD)index<info->value_count;++index) {
                REQUIRE_UNNAMED();
                if (_stricmp(VALUE,"no")) goto invalid_value;
                context->options.fixed_image=0;
            }
            context->specified_switches |= 0x80000;
        } else if (OPTION("force")) {
            for (index=0;(WORD)index<info->value_count;++index) {
                REQUIRE_UNNAMED();
                if (!_stricmp(VALUE,"multiple")) context->options.force |= 2;
                else if (!_stricmp(VALUE,"unresolved")) context->options.force |= 1;
                else goto invalid_value;
            }
            if (!context->options.force) context->options.force=3;
        } else if (OPTION("fullbuild")) {
            context->options.option142=0;
            goto check_extra;
        } else if (OPTION("gpsize")) {
            REQUIRE_VALUE(); index=1;
            if (!switch_number(info,0,&value)) goto bad_number;
            if (context->options.gp_size<value) context->options.gp_size=value;
            context->specified_switches |= 8; used=1;
            goto check_extra;
        } else if (OPTION("hdrsize")) {
            REQUIRE_VALUE(); index=1;
            if (!switch_number(info,0,&value)) goto bad_number;
            context->options.header_size=value; used=1;
            goto check_extra;
        } else if (OPTION("heap")) {
            REQUIRE_VALUE();
            if (!context->options.heap_set || !module) {
                if (*FIRST && !switch_number(info,0,&context->optional_header.SizeOfHeapReserve)) goto bad_number;
                index=1;
                /* The original heap test reads the second byte, unlike stack. */
                if (info->value_count>=2 && info->values[1].text[1] &&
                    !switch_number(info,1,&context->optional_header.SizeOfHeapCommit)) goto bad_number;
                context->options.heap_set=1;
                context->optional_header.SizeOfHeapCommit=(context->optional_header.SizeOfHeapCommit+3)&~3;
                context->optional_header.SizeOfHeapReserve=(context->optional_header.SizeOfHeapReserve+3)&~3;
                if (context->optional_header.SizeOfHeapReserve<context->optional_header.SizeOfHeapCommit)
                    context->optional_header.SizeOfHeapReserve=context->optional_header.SizeOfHeapCommit;
            }
            used=2; goto check_extra;
        } else if (OPTION("ignore")) {
            REQUIRE_VALUE();
            for (index=0;(WORD)index<info->value_count;++index) {
                REQUIRE_UNNAMED();
                if (switch_number(info,(WORD)index,&value)) disable_warning(value);
                else {
                    text=VALUE; separator=strchr(text,'-');
                    if (!separator) goto invalid_value;
                    *separator=0; number=atoi(text); upper=atoi(separator+1);
                    if (upper<number) goto invalid_value;
                    do { disable_warning(number++); } while (number<=upper);
                }
            }
        } else if (OPTION("implib")) {
            if (!info->value_count) goto missing_value;
            import_library_name=argument->name+7;
            if (repro_directory) {
                release_memory(repro_option);
                _splitpath(import_library_name,NULL,NULL,import_basename,import_extension);
                repro_option=(char *)allocate_memory(strlen(import_basename)+strlen(import_extension)+12);
                strcpy(repro_option,"implib:"); strcat(repro_option,"\".\\");
                strcat(repro_option,import_basename); strcat(repro_option,import_extension);
                strcat(repro_option,"\"");
            }
        } else if (OPTION("import")) {
            REQUIRE_VALUE(); weak=glue=0;
            symbol_name=library=current_version=old_api_version=old_code_version=NULL;
            for (index=0;(WORD)index<info->value_count;++index) {
                if (!KEY) {
                    if (symbol_name) goto invalid_value;
                    symbol_name=VALUE;
                } else if (!_stricmp(KEY,"glue")) glue=!strcmp(VALUE,"1");
                else if (!_stricmp(KEY,"lib")) {
                    if (library) goto invalid_value;
                    library=VALUE;
                } else if (!_stricmp(KEY,"weak")) weak=!strcmp(VALUE,"1");
                else if (!_stricmp(KEY,"currentver")) {
                    if (current_version) goto invalid_value;
                    if (!switch_number(info,(WORD)index,&value)) goto bad_number;
                    current_version=VALUE;
                } else if (!_stricmp(KEY,"oldapiver")) {
                    if (old_api_version) goto invalid_value;
                    if (!switch_number(info,(WORD)index,&value)) goto bad_number;
                    old_api_version=VALUE;
                } else if (!_stricmp(KEY,"oldcodever")) {
                    if (old_code_version) goto invalid_value;
                    if (!switch_number(info,(WORD)index,&value)) goto bad_number;
                    old_code_version=VALUE;
                } else goto invalid_value;
            }
            if (current_version || old_code_version || old_api_version)
                set_mac_import_versions(library,current_version,old_code_version,old_api_version);
            if (weak) {
                if (symbol_name) append_argument(&weak_mac_imports,symbol_name);
                else if (library) append_argument(&weak_mac_libraries,library);
                else link_fatal(file,1117,info->name);
            }
            if (symbol_name && (library || glue)) {
                copy=(char *)allocate_memory(strlen(symbol_name)+2);
                if (*symbol_name=='?') strcpy(copy,symbol_name);
                else { strcpy(copy,"_"); strcat(copy,symbol_name); }
                if (!module) append_argument_record(&mac_import_arguments,copy,library);
                else if (!add_mac_import((unsigned char *)copy,(const unsigned char *)library,module,context))
                    link_warning(file,4544,symbol_name);
            }
            if (!current_version && !old_code_version && !old_api_version && !weak &&
                (!symbol_name || (!library && !glue))) link_fatal(file,1117,info->name);
        } else if (OPTION("include")) {
            if (!info->value_count) goto missing_value;
            symbol=symbols->Lookup((const unsigned char *)argument->name+8,NULL);
            if (!incremental_enabled || !module) {
                for (include=context->includes;include;include=include->next)
                    if (include->symbol==symbol) break;
                if (!include) {
                    include=(IncludeSymbol *)allocate_memory(sizeof(IncludeSymbol));
                    include->symbol=symbol; include->next=context->includes; context->includes=include;
                }
            }
            if (module) {
                record_symbol_reference(symbol,module->owner);
                if (incremental_enabled) record_incremental_include(module->owner,symbol);
            }
            context->specified_switches |= 0x10;
        } else if (OPTION("incremental")) {
            REQUIRE_VALUE();
            for (index=0;(WORD)index<info->value_count;++index) {
                REQUIRE_UNNAMED();
                if (!_stricmp(VALUE,"yes")) incremental_requested=1;
                else if (!_stricmp(VALUE,"no")) incremental_requested=0;
                else goto invalid_value;
            }
            incremental_switch_set=1;
        } else if (OPTION("largeaddressaware")) {
            context->file_header.Characteristics |= 0x20;
            for (index=0;(WORD)index<info->value_count;++index) {
                REQUIRE_UNNAMED();
                if (_stricmp(VALUE,"no")) goto invalid_value;
                context->file_header.Characteristics &= ~0x20;
            }
        } else if (OPTION("libpath")) {
            /* Already handled by the preliminary argument pass. */
        } else if (OPTION("link50compat")) {
            context->options.legacy_import_library=1;
        } else if (OPTION("mac")) {
            REQUIRE_VALUE();
            for (index=0;(WORD)index<info->value_count;++index) {
                if (!KEY) {
                    if (!_stricmp(VALUE,"bundle")) context->options.mac_bundle=1;
                    else if (!_stricmp(VALUE,"nobundle")) context->options.mac_bundle=0;
                    else {
                        if (!_stricmp(VALUE,"mfilepad")) mac_file_padding=1;
                        else if (!_stricmp(VALUE,"nomfilepad")) mac_file_padding=0;
                        else goto invalid_value;
                        context->specified_switches |= 0x40000;
                    }
                } else if (!_stricmp(KEY,"type")) context->options.mac_type=VALUE;
                else if (!_stricmp(KEY,"creator")) context->options.mac_creator=VALUE;
                else if (!_stricmp(KEY,"init")) {
                    set_mac_initializer(context,VALUE);
                    context->specified_switches |= module?0xc000:0x4000;
                } else if (!_stricmp(KEY,"term")) {
                    set_mac_terminator(context,VALUE);
                    context->specified_switches |= module?0x30000:0x10000;
                } else goto invalid_value;
            }
        } else if (OPTION("macdata")) {
            if (!info->value_count) goto missing_value;
            add_mac_data((const unsigned char *)argument->name+8,1,0xffffffff);
        } else if (OPTION("machine")) {
            REQUIRE_VALUE();
            machine=parse_machine(context,FIRST);
            context->file_header.Machine=machine;
            if (machine==0x168) context->options.default144=0;
            else if (machine==0x601) mac_target=1;
            else if (!machine) link_warning(file,4012,FIRST,"IX86, ALPHA, ARM, MIPS, MIPSR41XX, PPC, SH3, or SH4");
            context->specified_switches |= 0x1000; used=1;
            goto check_extra;
        } else if (OPTION("macres")) {
            if (!info->value_count) goto missing_value;
            if (!find_argument(argument->name+7,&mac_resources)) {
                append_argument_record(&mac_resources,argument->name+7,"resntBinaryResource");
                context->specified_switches |= 0x2000;
            }
        } else if (OPTION("map")) {
            context->options.write_map=1;
            if (info->value_count) {
                map_file_name=argument->name+4;
                if (repro_directory) {
                    _splitpath(map_file_name,NULL,NULL,basename,extension);
                    release_memory(repro_option);
                    repro_option=(char *)allocate_memory(strlen("map:\".\\%s%s\"")+strlen(basename)+strlen(extension));
                    sprintf(repro_option,"map:\".\\%s%s\"",basename,extension);
                }
            }
            context->specified_switches |= 0x20;
        } else if (OPTION("mapinfo")) {
            REQUIRE_VALUE(); context->specified_switches |= 0x20;
            context->options.write_map=1;
            for (index=0;(WORD)index<info->value_count;++index) {
                REQUIRE_UNNAMED();
                if (!_stricmp(VALUE,"exports")) context->options.map_exports=1;
                else if (!_stricmp(VALUE,"lines")) context->options.map_lines=1;
                else if (!_stricmp(VALUE,"fixups")) context->options.map_fixups=1;
                else goto invalid_value;
                used=(WORD)(index+1);
            }
            goto check_extra;
        } else if (OPTION("merge")) {
            if (!info->value_count) goto missing_value;
            text=argument->name+6;
            separator=(char *)_mbschr((unsigned char *)text,'=');
            if (!separator || separator==text || !separator[1]) goto invalid_value;
            *separator=0;
            copy=separator+1;
            if (!valid_section_name((unsigned char *)text)) {
                path=duplicate_argument(text); *separator='=';
                link_fatal(file,1184,path,argument->name);
                copy=separator;
            }
            if (!valid_section_name((unsigned char *)copy)) {
                copy[-1]='='; link_fatal(file,1184,copy,argument->name);
            }
            append_argument_record(&section_merges,text,copy);
            context->specified_switches |= 0x100000;
        } else if (OPTION("miscrdata")) {
            context->options.misc_rdata=1; goto check_extra;
        } else if (OPTION("mppc32bittypes")) {
            context->options.mppc32_types=1; goto check_extra;
        } else if (OPTION("name")) {
            if (!info->value_count) goto missing_value;
            if (module && context->is_vxd==1) vxd_module_name=argument->name+5;
            else link_warning(file,4044,argument->name);
        } else if (OPTION("newglue")) {
            context->options.new_glue=1; goto check_extra;
        } else if (OPTION("newimplib")) {
            context->options.legacy_import_library=0; goto check_extra;
        } else if (OPTION("newrelocs")) {
            context->options.new_relocations=1; goto check_extra;
        } else if (OPTION("nodefaultlib") || OPTION("nod")) {
            if (!info->value_count) { context->options.no_default_libraries=1; text=NULL; }
            else text=argument->name+(OPTION("nod")?4:13);
            exclude_default_library(text,&context->pointer_list);
        } else if (OPTION("nodelayload")) {
            if (!info->value_count) goto missing_value;
            remove_delay_library(argument->name+12);
        } else if (OPTION("noentry")) {
            context->options.no_entry=1; goto check_extra;
        } else if (OPTION("nologo")) {
            logo_pending=0; goto check_extra;
        } else if (OPTION("nooptidata")) {
            context->specified_switches |= 0x200000; goto check_extra;
        } else if (OPTION("nopack")) {
            context->options.no_pack=1; goto check_extra;
        } else if (OPTION("opt")) {
            REQUIRE_VALUE();
            for (index=0;(WORD)index<info->value_count;++index) {
                REQUIRE_UNNAMED();
                if (!_stricmp(VALUE,"icf")) {
                    if ((unsigned)(WORD)index+1<info->value_count) {
                        ++index;
                        if (!*VALUE || switch_number(info,(WORD)index,&iterations)) icf_iterations=iterations;
                        else index+=0xffff; /* Resume at the unconsumed next option after loop increment. */
                    }
                    context->options.fold_identical_comdats=1;
                    context->specified_switches |= 0x400000;
                } else if (!_stricmp(VALUE,"noicf")) {
                    context->options.fold_identical_comdats=0;
                    context->specified_switches |= 0x400000;
                } else {
                    if (!_stricmp(VALUE,"noref")) context->options.effective_reference_opt=0;
                    else if (!_stricmp(VALUE,"ref")) context->options.effective_reference_opt=1;
                    else goto invalid_value;
                    reference_optimization_specified=1;
                }
            }
        } else if (OPTION("optidata")) goto check_extra;
        else if (OPTION("osversion")) {
            REQUIRE_VALUE(); REQUIRE_UNNAMED(); index=1; minor=0;
            text=strchr(FIRST,'.');
            if ((text && (sscanf(text+1,"%li",&minor)!=1 || minor>0xffff)) ||
                sscanf(FIRST,"%li",&major)!=1 || major>0xffff) goto bad_number;
            context->optional_header.MajorOperatingSystemVersion=(WORD)major;
            context->optional_header.MinorOperatingSystemVersion=(WORD)minor;
            context->specified_switches |= 0x400; used=1; goto check_extra;
        } else if (OPTION("out")) {
            if (!info->value_count) goto missing_value;
            text=argument->name+4;
            if (!context->options.output_name_set || !module) {
                output_file_name=text; context->options.output_name_set=1;
                if (repro_directory) {
                    _splitpath(output_file_name,NULL,NULL,basename,extension);
                    release_memory(repro_option);
                    repro_option=(char *)allocate_memory(strlen("out:\".\\%s%s\"")+strlen(basename)+strlen(extension));
                    sprintf(repro_option,"out:\".\\%s%s\"",basename,extension);
                }
            } else if (context->file_header.Machine!=0x601) {
                _splitpath(output_file_name,NULL,NULL,basename,extension);
                strcat(basename,extension);
                if (_mbsicmp((unsigned char *)basename,(unsigned char *)text)) link_warning(file,4070,text,output_file_name);
            }
        } else if (OPTION("pdb")) {
            if (!info->value_count) goto missing_value;
            text=argument->name+4;
            if (!_stricmp(text,"none")) pdb_enabled=0;
            else {
                pdb_file_name=text;
                if (repro_directory) {
                    _splitpath(text,NULL,NULL,basename,extension);
                    if (*basename && *extension) {
                        release_memory(repro_option);
                        repro_option=(char *)allocate_memory(strlen("pdb:\".\\%s%s\"")+strlen(basename)+strlen(extension));
                        sprintf(repro_option,"pdb:\".\\%s%s\"",basename,extension);
                    }
                }
            }
        } else if (OPTION("pdbtype")) {
            REQUIRE_VALUE(); REQUIRE_UNNAMED(); index=1;
            if (!_stricmp(FIRST,"con") || !_stricmp(FIRST,"consolidate")) separate_types=0;
            else if (!_stricmp(FIRST,"sept") || !_stricmp(FIRST,"septypes")) separate_types=1;
            else goto invalid_value;
            used=1; goto check_extra;
        } else if (OPTION("profile")) {
            context->options.profile=1; context->options.map_fixups=1; goto check_extra;
        } else if (OPTION("release")) {
            context->options.checksum_enabled=1; goto check_extra;
        } else if (OPTION("re")) {
            context->options.order=1; goto check_extra;
        } else if (OPTION("rom")) {
            context->options.rom=1; goto check_extra;
        } else if (OPTION("section")) {
            if (!info->value_count) goto missing_value;
            text=argument->name+8;
            separator=(char *)_mbschr((unsigned char *)text,',');
            if (separator) {
                length=separator-text; copy=(char *)allocate_memory(length+1);
                _mbsnbcpy((unsigned char *)copy,(unsigned char *)text,length);
                copy[length]=0;
                if (!valid_section_name((unsigned char *)copy)) link_fatal(file,1184,copy,argument->name);
                release_memory(copy);
            }
            append_argument(&section_attributes,text);
            if (!module) context->specified_switches |= 0x40;
        } else if (OPTION("stack")) {
            REQUIRE_VALUE();
            if (!context->options.stack_set || !module) {
                if (*FIRST && !switch_number(info,0,&context->optional_header.SizeOfStackReserve)) goto bad_number;
                index=1;
                if (info->value_count>=2 && *info->values[1].text &&
                    !switch_number(info,1,&context->optional_header.SizeOfStackCommit)) goto bad_number;
                context->options.stack_set=1;
                context->optional_header.SizeOfStackCommit=(context->optional_header.SizeOfStackCommit+3)&~3;
                context->optional_header.SizeOfStackReserve=(context->optional_header.SizeOfStackReserve+3)&~3;
            }
            used=2; goto check_extra;
        } else if (OPTION("stub")) {
            if (!info->value_count) goto missing_value;
            path=argument->name+5; input=fopen(path,"rb");
            if (!input) {
                _splitpath(path,drive,directory,basename,extension);
                if (!*drive && !*directory) {
                    _splitpath(_pgmptr,drive,directory,NULL,NULL);
                    _makepath(stub_path,drive,directory,basename,extension);
                    input=fopen(stub_path,"rb");
                }
                if (!input) link_fatal(path,1104,path);
            }
            bytes=_filelength(input->_file);
            if ((int)bytes<0) link_fatal(path,1107,path);
            if ((int)bytes<64) link_fatal(path,1132,path);
            padded=(bytes+7)&~7;
            dos_bytes=(BYTE *)allocate_memory(padded);
            if (fread(dos_bytes,1,bytes,input)!=bytes) link_fatal(path,1107,path);
            fclose(input);
            if ((int)bytes<(int)padded) memset(dos_bytes+bytes,0,padded-bytes);
            dos_header=(IMAGE_DOS_HEADER *)dos_bytes;
            if (dos_header->e_magic!=0x5a4d) link_fatal(path,1132,path);
            if (dos_header->e_lfarlc<64) link_warning(path,4060);
            context->dos_image=(DefaultDosImage *)dos_bytes;
            context->dos_image_size=padded;
            context->specified_switches |= 0x80;
        } else if (OPTION("subsystem")) {
            REQUIRE_VALUE();
            for (index=0;(WORD)index<info->value_count;++index) {
                REQUIRE_UNNAMED();
                if (!(WORD)index) {
                    if (!_stricmp(VALUE,"console")) context->optional_header.Subsystem=3;
                    else if (!_stricmp(VALUE,"native")) {
                        context->optional_header.Subsystem=1; context->options.checksum_enabled=1;
                    } else if (!_stricmp(VALUE,"posix")) context->optional_header.Subsystem=7;
                    else if (!_stricmp(VALUE,"windows")) context->optional_header.Subsystem=2;
                    else if (!_stricmp(VALUE,"windowsce")) {
                        context->optional_header.Subsystem=9; context->options.reference_opt_specified=1;
                    } else goto invalid_value;
                    context->specified_switches |= 0x100;
                } else {
                    if ((WORD)index!=1) goto invalid_value;
                    minor=0; text=strchr(VALUE,'.');
                    if ((text && (sscanf(text+1,"%li",&minor)!=1 || minor>0xffff)) ||
                        sscanf(VALUE,"%li",&major)!=1 || major>0xffff) goto bad_number;
                    machine=context->optional_header.Subsystem;
                    if ((machine>=2 && machine<4 && (major<=2 || (major==3 && minor<=9))) ||
                        (machine==9 && !major)) link_warning(file,4010,VALUE);
                    else {
                        context->optional_header.MajorSubsystemVersion=(WORD)major;
                        context->optional_header.MinorSubsystemVersion=(WORD)minor;
                        context->specified_switches |= 0x800;
                    }
                }
            }
        } else if (OPTION("swaprun")) {
            REQUIRE_VALUE();
            for (index=0;(WORD)index<info->value_count;++index) {
                REQUIRE_UNNAMED();
                if (!_stricmp(VALUE,"cd")) context->file_header.Characteristics |= 0x400;
                else if (!_stricmp(VALUE,"net")) context->file_header.Characteristics |= 0x800;
                else goto invalid_value;
            }
        } else if (OPTION("test")) {
            linker_test=1; goto check_extra;
        } else if (OPTION("time")) {
            linker_time=1; goto check_extra;
        } else if (OPTION("vc6compat")) {
            context->options.legacy_import_library=0; goto check_extra;
        } else if (OPTION("verbose")) {
            if (info->value_count && *FIRST) {
                for (index=0;(WORD)index<info->value_count;++index) {
                    REQUIRE_UNNAMED();
                    if (_stricmp(VALUE,"lib")) goto invalid_value;
                    verbose_libraries=1;
                }
            } else { verbose_link=1; verbose_libraries=1; goto check_extra; }
        } else if (OPTION("version")) {
            REQUIRE_VALUE();
            if (info->values[0].name) {
                if (_stricmp(info->values[0].name,"liborder") || _stricmp(FIRST,"before")) goto invalid_value;
                for (index=1;(WORD)index<info->value_count;++index) {
                    REQUIRE_UNNAMED();
                    if (find_library((unsigned char *)VALUE,context->pointer_list.first,1))
                        link_warning(file,4057,file,VALUE);
                }
            } else {
                index=1; minor=0; text=strchr(FIRST,'.');
                if ((text && (sscanf(text+1,"%li",&minor)!=1 || minor>0xffff)) ||
                    sscanf(FIRST,"%li",&major)!=1 || major>0xffff) goto bad_number;
                context->optional_header.MajorImageVersion=(WORD)major;
                context->optional_header.MinorImageVersion=(WORD)minor;
                context->specified_switches |= 0x200; used=1; goto check_extra;
            }
        } else if (OPTION("vxd")) {
            if (module && context->is_vxd!=1) link_fatal(file,1153);
            context->file_header.Machine=0x14c; goto check_extra;
        } else if (OPTION("warn")) {
            if (info->value_count && *FIRST) {
                REQUIRE_UNNAMED();
                if (!switch_number(info,0,&value)) goto bad_number;
                warning_level=(WORD)value; used=1;
            } else warning_level=2;
            goto check_extra;
        } else if (OPTION("WX")) {
            warnings_as_errors=1; goto check_extra;
        } else if (OPTION("windowsce")) {
            REQUIRE_VALUE();
            for (index=0;(WORD)index<info->value_count;++index) {
                REQUIRE_UNNAMED();
                if (!_stricmp(VALUE,"emulation")) context->options.windowsce_emulation=1;
                else if (!_stricmp(VALUE,"noconvert")) context->options.windowsce_noconvert=1;
                else goto invalid_value;
            }
        } else if (OPTION("ws")) {
            REQUIRE_VALUE();
            for (index=0;(WORD)index<info->value_count;++index) {
                REQUIRE_UNNAMED();
                if (_stricmp(VALUE,"aggressive")) goto invalid_value;
                context->file_header.Characteristics |= 0x10;
            }
        } else if (OPTION("xoff")) {
            xoff=1; goto check_extra;
        } else if (!_strnicmp(argument->name,"order:@",7)) {
            context->options.order_file=1; order_file_name=argument->name+7;
            enable_order_file();
            if (repro_directory) {
                copy_repro_file(order_file_name,0);
                _splitpath(order_file_name,NULL,NULL,basename,extension);
                release_memory(repro_option);
                repro_option=(char *)allocate_memory(strlen("order:@\".\\%s%s\"")+strlen(basename)+strlen(extension));
                sprintf(repro_option,"order:@\".\\%s%s\"",basename,extension);
            }
        } else link_warning(file,4044,argument->name);
        goto next_option;

missing_value:
        link_fatal(file,1146,argument->name);
        goto next_option;
invalid_value:
        link_fatal(file,1117,argument->name);
bad_number:
        link_fatal(file,1147,argument->name);
        used=(WORD)index;
check_extra:
        if (used<info->value_count) link_warning(file,4056,argument->name);
next_option:
        argument=argument->next;
        if (!module && repro_directory) fprintf(repro_response,"/%s\n",repro_option);
        release_memory(repro_option);
    }
    if (warnings_as_errors) enable_warning_errors();
}

StringNode *RECON_THISCALL StringTable::Lookup(const unsigned char *name,BYTE *created)
{
    __asm { int 3 }
    return NULL;
}
#undef OPTION
#undef VALUE
#undef KEY
#undef FIRST
#undef REQUIRE_VALUE
#undef REQUIRE_UNNAMED
