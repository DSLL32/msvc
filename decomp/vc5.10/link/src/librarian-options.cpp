#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "link.h"
#include "librarian.h"
#include "options.h"
#include <recon.h>

extern "C" {
extern const char *default_phase,*definition_file,*output_file_name;
extern char *repro_directory;
extern FILE *repro_response;
extern BYTE reproducible_build,verbose_link,xoff,mac_target;
extern InputArguments export_arguments;
}

#pragma intrinsic(strlen,strcpy,strcat,strcmp)
extern "C" void __stdcall configure_librarian_options(LinkContext *context)
{
    const char *saved_phase=default_phase;
    InputArgument *argument=option_arguments.first;
    SwitchInfo *info;
    WORD index,value_index;
    BYTE warning_errors=0;
    DWORD number;
    const char *current,*old_code,*old_api;
    char *text,*part,*dash;
    char repro_option[280],basename[256],extension[256],name[512];
    default_phase="ProcessLibrarianSwitches";
    for(index=0;index<option_arguments.count;++index) {
        text=argument->name;
        strcpy(repro_option,text);
        if(!strcmp(text,"?"))librarian_usage();
        if(!_stricmp(text,"Brepro"))reproducible_build=1;
        else if(!_stricmp(text,"convert"))context->options.librarian_convert=1;
        else if(!_strnicmp(text,"debugtype:",10)) {
            if(!_strnicmp(text+10,"coff",4))context->options.debug_formats=5;
            else if(!_strnicmp(text+10,"cv",2))context->options.debug_formats=6;
            else if(!_strnicmp(text+10,"both",4))context->options.debug_formats=7;
            else goto invalid_option;
        } else if(!_strnicmp(text,"def:",4)) {
            context->options.librarian_skip_directive_symbols=1;
            if(!text[4])goto next_option;
            definition_file=text+4;
            if(repro_directory) {
                copy_repro_file(definition_file,0);
                _splitpath(definition_file,NULL,NULL,basename,extension);
                sprintf(repro_option,"def:\".\\%s%s\"",basename,extension);
            }
            if(!output_file_name) {
                _splitpath(definition_file,NULL,NULL,name,NULL);
                strcat(name,".lib");
                output_file_name=duplicate_argument(name);
            }
        } else if(!_strnicmp(text,"export:",7)) {
            append_argument_record(&export_arguments,text+7,NULL);
        } else if(!_strnicmp(text,"extract:",8)) {
            if(!library_files.first)link_fatal(NULL,1131);
            librarian_extract_member=text+8;
        } else if(!_strnicmp(text,"ignore:",7)) {
            part=strtok(text+7,",");
            if(!part)goto next_option;
            do {
                dash=strchr(part,'-');
                if(dash) {
                    *dash++=0;
                    int last=atoi(dash),first=atoi(part);
                    while(first<=last)disable_warning(first++);
                } else disable_warning(atoi(part));
                part=strtok(NULL,",");
            } while(part);
        } else if(!_strnicmp(text,"import:",7)) {
            info=argument->switch_info=parse_switch(NULL,text);
            if(!info->value_count || !*info->values[0].text)
                link_fatal(NULL,1146,text);
            current=old_code=old_api=NULL;
            for(value_index=0;value_index<info->value_count;++value_index) {
                const char *key=info->values[value_index].name;
                if(!key)goto invalid_option;
                if(!_stricmp(key,"currentver")) {
                    if(current)goto invalid_option;
                    if(!switch_number(info,value_index,&number))goto bad_number;
                    current=info->values[value_index].text;
                } else if(!_stricmp(key,"oldapiver")) {
                    if(old_api)goto invalid_option;
                    if(!switch_number(info,value_index,&number))goto bad_number;
                    old_api=info->values[value_index].text;
                } else if(!_stricmp(key,"oldcodever")) {
                    if(old_code)goto invalid_option;
                    if(!switch_number(info,value_index,&number))goto bad_number;
                    old_code=info->values[value_index].text;
                } else goto invalid_option;
            }
            if(current || old_code || old_api)
                set_mac_import_versions(NULL,current,old_code,old_api);
        } else if(!_strnicmp(text,"include:",8)) {
            context->symbols->Lookup((const unsigned char *)text+8,NULL);
        } else if(!_strnicmp(text,"libpath:",8)) {
            goto next_option;
        } else if(!_stricmp(text,"link50compat")) {
            context->options.legacy_import_library=1;
        } else if(!_stricmp(text,"list")) {
            context->options.librarian_list=1;
        } else if(!_strnicmp(text,"list:",5)) {
            context->options.librarian_list=1;
            if(!text[5])goto next_option;
            librarian_list_output=text+5;
            if(repro_directory) {
                _splitpath(librarian_list_output,NULL,NULL,basename,extension);
                sprintf(repro_option,"list:.\\%s%s",basename,extension);
            }
        } else if(!_strnicmp(text,"mac:",4)) {
            info=argument->switch_info=parse_switch(NULL,text);
            if(!info->value_count || !*info->values[0].text)goto invalid_option;
            for(value_index=0;value_index<info->value_count;++value_index) {
                if(!info->values[value_index].name)goto invalid_option;
                if(!_stricmp(info->values[value_index].name,"init")) {
                    set_mac_initializer(context,info->values[value_index].text);
                    context->specified_switches|=0x4000;
                } else if(!_stricmp(info->values[value_index].name,"term")) {
                    set_mac_terminator(context,info->values[value_index].text);
                    context->specified_switches|=0x10000;
                } else goto invalid_option;
            }
        } else if(!_strnicmp(text,"machine:",8)) {
            context->file_header.Machine=parse_machine(context,text+8);
            if(context->file_header.Machine==0x601)mac_target=1;
            else if(!context->file_header.Machine)
                link_warning(NULL,4012,text+8,
                    "IX86, ALPHA, ARM, MIPS, MIPSR41XX, PPC, SH3, or SH4");
        } else if(!_strnicmp(text,"name:",4)) {
            _splitpath(text+5,NULL,NULL,basename,extension);
            if(*basename)context->options.library_basename=duplicate_argument(basename);
            if(*extension)context->options.default_extension=duplicate_argument(extension);
        } else if(!_stricmp(text,"newimplib")) {
            context->options.legacy_import_library=0;
        } else if(!_stricmp(text,"nodefaultlib")) {
            context->options.no_default_libraries=1;
            exclude_default_library(NULL,&context->pointer_list);
        } else if(!_strnicmp(text,"nodefaultlib:",13)) {
            exclude_default_library(text+13,&context->pointer_list);
        } else if(!_stricmp(text,"nologo"))logo_pending=0;
        else if(!_strnicmp(text,"out:",4)) {
            output_file_name=text+4;
            if(repro_directory) {
                _splitpath(output_file_name,NULL,NULL,basename,extension);
                sprintf(repro_option,"out:.\\%s%s",basename,extension);
            }
        } else if(!_strnicmp(text,"remove:",7))librarian_remove_options=1;
        else if(!_strnicmp(text,"subsystem:",10)) {
            if(!_strnicmp(text+10,"native",6))context->optional_header.Subsystem=1;
            else if(!_strnicmp(text+10,"windows",7))context->optional_header.Subsystem=2;
            else if(!_strnicmp(text+10,"console",7))context->optional_header.Subsystem=3;
            else if(!_strnicmp(text+10,"windowsce",9))context->optional_header.Subsystem=9;
            else if(!_strnicmp(text+10,"posix",5))context->optional_header.Subsystem=7;
            else goto invalid_option;
        } else if(!_stricmp(text,"vc6compat"))context->options.legacy_import_library=0;
        else if(!_stricmp(text,"verbose"))verbose_link=1;
        else if(!_stricmp(text,"vxd"))context->file_header.Machine=0x14c;
        else if(!_stricmp(text,"WX"))warning_errors=1;
        else if(!_stricmp(text,"xoff"))xoff=1;
        else link_warning(NULL,4044,text);
        goto next_option;
invalid_option:
        link_fatal(NULL,1117,text);
bad_number:
        link_fatal(NULL,1147,text);
next_option:
        argument=argument->next;
        if(repro_directory)fprintf(repro_response,"/%s\n",repro_option);
    }
    if(warning_errors)enable_warning_errors();
    default_phase=saved_phase;
}
