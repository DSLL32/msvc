/* This original method and its input reader use genuine ECX thiscall. */
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <mbstring.h>
#include "link.h"
#include "crt-imports.h"
#include "options.h"
#include "full-link-functions.h"
#include "pass-one.h"
#include "memory.h"
#include "input.h"
#include <recon.h>

extern "C" {
extern BYTE verbose_link,mac_target,incremental_reuse;
extern BYTE module_section_pools_ready,incremental_enabled;
extern MemoryArena *output_library_arena;
extern const char *output_file_name,*entry_point_name,*definition_file;
extern char *repro_directory;
extern char current_phase[32];
extern StringNode *entry_symbol;
extern DWORD undefined_symbols;
extern WORD warning_level;
extern int output_file;
extern InputArguments mac_resources;
OutputLibrary *current_input_library;
BYTE output_file_open,arch_data_ignored;
int current_image_file=-1;
}

#pragma intrinsic(strcmp,strcpy)
void RECON_THISCALL LinkContext::PassOne(void)
{
    InputArgument *argument;
    LibraryEnumerator libraries;
    OutputLibrary *library;
    unsigned index;
    if (verbose_link) {
        fputc('\n',stdout);
        link_message(9007);
    }
    if (!module_section_pools_ready) initialize_contribution_pools();
    if (selected_tool_kind==1 && options.output_name_set && !output_file_open) {
        check_output_input_collision(output_file_name,link_files.first);
        output_file=open_argument_file(output_file_name,0x8302,0x180);
        output_file_open=1;
        current_image_file=output_file;
    }
    command_line_library=create_library(NULL,NULL,&pointer_list);
    current_input_library=command_line_library;
    command_line_library->flags|=0x14;
    argument=link_files.first;
    for (index=0;index<link_files.count;++index) {
        if (repro_directory) copy_repro_file(argument->original_name,1);
        LoadInputArgument(argument,command_line_library);
        argument=argument->next;
    }
    current_phase[0]=0;
    check_input_modules(this,command_line_library);
    if (undefined_symbols) resolve_weak_externals(this,0x40);
    if (mac_target && (selected_tool_kind==1 || selected_tool_kind==2)) {
        if (specified_switches & 0x4000) symbols->Lookup(mac_initializer_name,NULL);
        if (specified_switches & 0x10000) symbols->Lookup(mac_terminator_name,NULL);
        if (!entry_point_name) search_libraries(this);
    }
    if (selected_tool_kind==1) {
        if (!entry_symbol && entry_point_name) resolve_initial_entry(this);
        if (!optional_header.Subsystem) infer_subsystem(this);
        else if (!entry_symbol) infer_entry_point(this);
    }
    search_libraries(this);
    if (selected_tool_kind==1) {
        if (process_definition_file(this,definition_file)) search_libraries(this);
        if (!optional_header.Subsystem) {
            if (infer_subsystem(this)) search_libraries(this);
        } else if (!entry_symbol) {
            if (infer_entry_point(this)) search_libraries(this);
        }
        if (mac_target) {
            finalize_mac_entry(&entry_symbol,this);
            search_libraries(this);
        }
        if (!entry_symbol && !mac_target &&
            !(file_header.Characteristics & IMAGE_FILE_DLL) && is_vxd!=1)
            link_fatal(NULL,1561);
        if (!optional_header.Subsystem) link_fatal(NULL,1221);
        if (optional_header.Subsystem && !optional_header.MajorSubsystemVersion &&
            !optional_header.MinorSubsystemVersion) set_subsystem_version(this);
        if (!output_file_open) {
            if (!output_file_name) link_fatal(NULL,1159);
            check_output_input_collision(output_file_name,link_files.first);
            output_file=open_argument_file(output_file_name,0x8302,0x180);
            output_file_open=1;
        }
        current_image_file=output_file;
    }
    if (warning_level>1) warn_unused_libraries((OutputLibrary *)pointer_list.first);
    if (arch_data_ignored) link_warning(NULL,4106,".arch","INCREMENTAL");
    if (selected_tool_kind==1) warn_missing_default_libraries(&pointer_list);
    if (undefined_symbols) {
        resolve_weak_externals(this,0x4080);
        if (undefined_symbols) resolve_weak_externals(this,0x40);
    }
    if (selected_tool_kind==1) {
        initialize_library_enumerator(&libraries,(OutputLibrary *)pointer_list.first);
        while (next_library(&libraries)) {
            library=libraries.current;
            if (library!=command_line_library) {
                release_memory(library->index_data[0]);
                release_memory(library->index_data[1]);
                release_memory(library->index_data[2]);
                release_memory(library->index_data[3]);
                release_memory(library->index_data[4]);
                library->index_data[0]=NULL;
                library->index_data[1]=NULL;
                library->index_data[2]=NULL;
                library->index_data[3]=NULL;
                library->index_data[4]=NULL;
            }
        }
    }
    if (verbose_link) {
        fputc('\n',stdout);
        link_message(9008);
        fputc('\n',stdout);
    }
    warn_unused_delay_libraries();
    if (mac_target) {
        argument=mac_resources.first;
        for (index=0;index<mac_resources.count;++index) {
            if (!strcmp(argument->original_name,"resntBinaryResource"))
                add_mac_data((const unsigned char *)argument->name,0,(DWORD)-1);
            argument=argument->next;
        }
        clear_argument_list(&mac_resources);
    }
}

extern "C" void __cdecl link_message(unsigned message,...) { __asm { int 3 } }
extern "C" OutputLibrary *__stdcall create_library(const char *name,const char *path,LinkPointerList *libraries)
{
    OutputLibrary *library=(OutputLibrary *)find_library((const unsigned char *)name,libraries->first,0);
    if (!library) {
        if (incremental_enabled) library=(OutputLibrary *)allocate_link_memory(1,sizeof(OutputLibrary));
        else library=(OutputLibrary *)reserve_memory(&output_library_arena,sizeof(OutputLibrary));
        library->name=name?duplicate_link_string(name):NULL;
        library->path=path;
        library->reserved34[1]=0;
        library->next=NULL;
        library->reserved40=0;
        library->module_capacity=5;
        library->module_count=0;
        library->modules=(OutputModule **)allocate_memory(5*sizeof(*library->modules));
        library->active=1;
        *libraries->last=library;
        libraries->last=(void **)&library->next;
    }
    return library;
}
extern "C" void __stdcall check_output_input_collision(const char *output,InputArgument *inputs)
{
    char output_path[260],input_path[260];
    if (!_fullpath(output_path,output,259)) link_fatal(NULL,1104,output);
    while (inputs) {
        if (!_fullpath(input_path,inputs->name,259))
            link_fatal(NULL,1104,inputs->name);
        if (!_mbsicmp((const unsigned char *)output_path,(const unsigned char *)input_path)) {
            output_file_name=NULL;
            link_fatal(NULL,1149,output_path);
        }
        inputs=inputs->next;
    }
}
void RECON_THISCALL LinkContext::LoadInputArgument(InputArgument *argument,OutputLibrary *library)
{
    OutputModule *module;
    strcpy(current_phase,argument->name);
    current_input_file=open_argument_file(argument->original_name,0x8000,0);
    module=read_input_argument(argument,library,this);
    if (module) {
        load_object_module(this,NULL,module,0);
        if (incremental_reuse && incremental_status_code) return;
        if (!output_file_name) infer_output_file_name(this,argument);
    }
    close_argument_file(current_input_file,0);
}
extern "C" void __stdcall check_input_modules(LinkContext *context,OutputLibrary *library) { __asm { int 3 } }
extern "C" void __stdcall resolve_weak_externals(LinkContext *context,DWORD flags) { __asm { int 3 } }
extern "C" void __stdcall search_libraries(LinkContext *context) { __asm { int 3 } }
extern "C" BYTE __stdcall resolve_initial_entry(LinkContext *context) { __asm { int 3 } return 0; }
extern "C" BYTE __stdcall infer_subsystem(LinkContext *context) { __asm { int 3 } return 0; }
extern "C" BYTE __stdcall infer_entry_point(LinkContext *context) { __asm { int 3 } return 0; }
extern "C" BYTE __stdcall process_definition_file(LinkContext *context,const char *name) { __asm { int 3 } return 0; }
extern "C" void __stdcall finalize_mac_entry(StringNode **entry,LinkContext *context) { __asm { int 3 } }
extern "C" void __stdcall set_subsystem_version(LinkContext *context) { __asm { int 3 } }
extern "C" void __stdcall warn_unused_libraries(OutputLibrary *library) { __asm { int 3 } }
extern "C" void __stdcall warn_missing_default_libraries(LinkPointerList *libraries) { __asm { int 3 } }
extern "C" void __stdcall warn_unused_delay_libraries(void) { __asm { int 3 } }
