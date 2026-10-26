/* Library-manager driver, including the inline specialized export-library
 * construction and cleanup. C++ is needed only for its genuine thiscall. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <mbstring.h>
#include <new.h>
#include "link.h"
#include "initializers.h"
#include "librarian.h"
#include "librarian-construct.h"
#include "input.h"
#include "files.h"
#include "pass-one.h"
#include "sections.h"
#include "full-link-functions.h"
#include "options.h"
#include <recon.h>

extern "C" {
extern const char *default_phase,*definition_file,*output_file_name;
extern char *repro_directory;
extern BYTE verbose_link;
extern OutputModule *output_module;
extern int output_file;
extern DWORD archive_member_offset;
BYTE librarian_remove_options;
const char *librarian_extract_member;
const char *librarian_list_output;
BYTE __stdcall has_switch(const char *);
void *__stdcall allocate_link_memory(unsigned,unsigned);
void __cdecl initialize_repro(void);
void __cdecl close_repro(void);
void __cdecl close_files(void);
void __cdecl cleanup_pdb(void);
void __stdcall convert_library_member(LinkContext *,OutputLibrary *,IMAGE_FILE_HEADER *,
    void *,int,DWORD,DWORD,const char *,const char *,IMAGE_SECTION_HEADER *,BYTE *,
    void *,void **,DWORD,DWORD);
}

#pragma intrinsic(strlen,strcpy,strcat)
extern "C" int __stdcall run_librarian(int argc,char **argv)
{
    const char *saved_phase=default_phase;
    LinkContext *context;
    LibrarianContext *special;
    InputArgument *argument;
    OutputLibrary *objects;
    LibraryEnumerator enumerator;
    IMAGE_FILE_HEADER header;
    IMAGE_SECTION_HEADER sections[30];
    BYTE section_flags[32],import_header[20],section_data[60],converted;
    /* The original writes only the low byte of this physical DWORD transport.
     * Its upper bytes are not defined on this path. The converter is pending;
     * no full-width import-conversion equivalence is claimed here. */
    union { DWORD value; BYTE low; } is_import;
    /* The driver initializes/tests this flag as BYTE but passes its physical
     * DWORD to the still-pending output owner. Preserve that transport width. */
    union { DWORD value; BYTE low; } temporary;
    void *conversion;
    DWORD format;
    WORD index;
    int is_vxd,result;
    char drive[4],directory[256],basename[256],list_path[260],output_name[520];
    char temp_name[52];
    char *absolute_output;
    const char *destination;

#ifdef __linux__
    temporary.value=0;
#endif
    temporary.low=0;
    default_phase="LibrarianMain";
    if(argc<2)librarian_usage();
    initialize_repro();
    collect_file_arguments(argc,argv,NULL,NULL);
    parse_command_line(argc,argv,NULL,NULL);
    is_vxd=has_switch("vxd")?1:0;
    context=(LinkContext *)allocate_link_memory(1,sizeof(LinkContext));
    context->Initialize(is_vxd);
    context->optional_header.Subsystem=0;
    context->file_header.Machine=0;
    configure_librarian_options(context);
    if(logo_pending)print_logo();
    if(context->options.librarian_skip_directive_symbols) {
        if(ordinary_files.count)read_librarian_objects(context);
        if(!context->file_header.Machine) {
            context->file_header.Machine=0x14c;
            link_warning(NULL,4068,"IX86");
        }
        BYTE old_library=context->options.legacy_import_library;
        switch(context->file_header.Machine) {
        case 0x1c0: case 0x1f0: case 0x200: old_library=0; break;
        case 0x601: old_library=1; break;
        }
        special=(LibrarianContext *)crt_operator_new(sizeof(LibrarianContext));
        if(special) {
            if(old_library)new(special) ExportLibrarianContext(context);
            else new(special) CoffLibrarianContext(context);
        }
        librarian_context=special;
        result=special->Process();
        special=librarian_context;
        if(special) {
            special->~LibrarianContext();
            crt_operator_delete(special);
        }
        librarian_context=NULL;
        return result;
    }
    context->options.debug_level=3;
    initialize_output_modules(&output_module);
    if(!ordinary_files.count && !library_files.count)goto no_inputs;
    argument=library_files.first;
    for(index=0;index<library_files.count;++index) {
        if(repro_directory)copy_repro_file(argument->name,1);
        load_librarian_library(context,argument->name);
        argument=argument->next;
    }
    current_phase[0]=0;
    if(context->options.librarian_list) {
        initialize_library_enumerator(&enumerator,(OutputLibrary *)context->pointer_list.first);
        if(next_library(&enumerator))list_librarian_members(enumerator.current);
        end_section_enumerator((SectionEnumerator *)&enumerator);
        goto no_inputs;
    }
    if(librarian_extract_member) {
        extract_librarian_member(context);
        free_librarian_inputs(&context->pointer_list);
        goto no_inputs;
    }
    if(ordinary_files.count) {
        read_librarian_objects(context);
        objects=create_library(NULL,NULL,&context->pointer_list);
        objects->flags|=16;
        argument=ordinary_files.first;
        for(index=0;index<ordinary_files.count;++index) {
            if(repro_directory)copy_repro_file(argument->original_name,1);
            if(remove_librarian_member(context,argument->name))link_message(9021,argument->name);
#ifdef __linux__
            /* This native owner models the observed diagnostic prefix only. */
            if(strlen(argument->name)>=sizeof(current_phase)) { __asm { int 3 } }
#endif
            strcpy(current_phase,argument->name);
            current_input_file=open_argument_file(argument->original_name,0x8000,0);
            conversion=NULL;
            format=20;
#ifdef __linux__
            is_import.value=0;
#endif
            is_import.low=0;
            read_coff_header(current_input_file,&header,import_header,&is_import.low);
            if(is_import.low) {
                convert_library_member(context,objects,&header,import_header,current_input_file,
                    0,is_import.value,NULL,NULL,sections,section_flags,section_data,&conversion,0,0);
                format=0;
            }
            close_argument_file(current_input_file,0);
            create_input_module(argument->original_name,argument->name,NULL,&header,objects,
                &converted,0,format,sections,section_flags,section_data,(DWORD)conversion);
            if(!converted)link_warning(argument->name,4042);
            argument=argument->next;
        }
        current_phase[0]=0;
    }
    if(librarian_remove_options) {
        argument=option_arguments.first;
        for(index=0;index<option_arguments.count;++index) {
            if(!_strnicmp(argument->name,"remove:",7) &&
               !remove_librarian_member(context,argument->name+7))
                link_warning(NULL,4014,argument->name+7);
            argument=argument->next;
        }
    }
    if(!output_file_name) {
        _splitpath(((OutputLibrary *)context->pointer_list.first)->name,drive,directory,basename,NULL);
        _makepath(output_name,drive,directory,basename,".lib");
        output_file_name=duplicate_argument(output_name);
    }
    absolute_output=_fullpath(NULL,output_file_name,0);
    if(!absolute_output)out_of_memory();
    destination=output_file_name;
    initialize_library_enumerator(&enumerator,(OutputLibrary *)context->pointer_list.first);
    if(next_library(&enumerator)) {
        do {
            _fullpath(list_path,enumerator.current->name,sizeof(list_path));
            if(!_mbsicmp((unsigned char *)list_path,(unsigned char *)absolute_output)) {
                strncpy(temp_name,destination,2);
                temporary.low=1;temp_name[2]=0;
                strcat(temp_name,"XXXXXX");
                destination=_mktemp(temp_name);
                if(!destination)link_fatal(NULL,1104,"LIBTEMPFILE");
                break;
            }
        } while(next_library(&enumerator));
    }
    end_section_enumerator((SectionEnumerator *)&enumerator);
    free(absolute_output);
    output_file=open_argument_file(destination,0x8302,0x180);
    write_librarian_output(context,temporary.value);
    close_files();
    cleanup_pdb();
    if(temporary.low) {
        if(remove(output_file_name))link_fatal(NULL,1109,output_file_name);
        if(rename(destination,output_file_name))link_fatal(NULL,1110,output_file_name);
    }
    if(repro_directory)close_repro();
    default_phase=saved_phase;
    return 0;
no_inputs:
    if(repro_directory) {
        close_repro();
        return 0;
    }
    return 0;
}

/* Remove matching members from existing libraries before adding an object.
 * The library created for this invocation is marked 0x10 and is skipped. */
extern "C" BYTE __stdcall remove_librarian_member(LinkContext *context,const char *name)
{
    const char *saved_phase=default_phase;
    LibraryEnumerator libraries;
    ModuleEnumerator modules;
    OutputModule **link;
    BYTE removed=0;
    default_phase="FRemoveModule";
    initialize_library_enumerator(&libraries,(OutputLibrary *)context->pointer_list.first);
    while(next_library(&libraries)) {
        OutputLibrary *library=libraries.current;
        if(library->flags&0x10)continue;
        initialize_module_enumerator(&modules,library);
        link=&library->module_head;
        while(next_module(&modules)) {
            OutputModule *module=modules.current;
            if(!_mbsicmp((const unsigned char *)name,(const unsigned char *)module->name)) {
                *link=module->next;
                removed=1;
            } else link=&module->next;
        }
    }
    default_phase=saved_phase;
    return removed;
}
