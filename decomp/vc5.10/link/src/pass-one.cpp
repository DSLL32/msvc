/* This original method and its input reader use genuine ECX thiscall. */
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <mbstring.h>
#include "link.h"
#include "../crt/crt-imports.h"
#include "options.h"
#include "full-link-functions.h"
#include "pass-one.h"
#include "memory.h"
#include "input.h"
#include "sections.h"
#include <recon.h>

extern "C" {
extern BYTE verbose_link,mac_target,incremental_reuse;
extern BYTE module_section_pools_ready,incremental_enabled;
extern MemoryArena *output_library_arena;
extern const char *output_file_name,*entry_point_name,*definition_file;
extern char *repro_directory;
extern char current_phase[41];
extern StringNode *entry_symbol;
extern DWORD weak_external_count;
extern WORD warning_level;
extern int output_file;
extern InputArguments mac_resources;
OutputLibrary *current_input_library;
BYTE output_file_open,arch_data_ignored;
int current_image_file=-1;
}

#pragma intrinsic(strcmp,strcpy,strlen)
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
    if (weak_external_count) resolve_weak_externals(this,0x40);
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
    if (weak_external_count) {
        resolve_weak_externals(this,0x4080);
        if (weak_external_count) resolve_weak_externals(this,0x40);
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


extern "C" OutputLibrary *__stdcall create_library(const char *name,const char *path,LinkPointerList *libraries)
{
    OutputLibrary *library=(OutputLibrary *)find_library((const unsigned char *)name,libraries->first,0);
    if (!library) {
        if (incremental_enabled) library=(OutputLibrary *)allocate_link_memory(1,sizeof(OutputLibrary));
        else library=(OutputLibrary *)reserve_memory(&output_library_arena,sizeof(OutputLibrary));
        library->name=name?duplicate_link_string(name):NULL;
        library->path=path;
        library->module_head=NULL;
        library->next=NULL;
        library->reserved40=0;
        library->conversion_capacity=5;
        library->conversion_count=0;
        library->import_conversions=(CoffLibrarianContext **)allocate_memory(5*sizeof(*library->import_conversions));
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
#ifdef __linux__
    if(strlen(argument->name)>=sizeof(current_phase))
        recon_unsupported("current_phase input exceeds observed prefix");
#endif
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
extern "C" OutputLibrary *__stdcall load_next_default_library(LinkPointerList *list);
extern "C" void __stdcall ensure_library_loaded(LinkContext *context,OutputLibrary *library)
{
    if (library->name && !library->index_data[3]) {
        int saved_file=current_input_file;
        current_input_file=open_argument_file(library->name,0x8000,0);
        read_library_index(library,context);
        close_argument_file(current_input_file,0);
        current_input_file=saved_file;
    }
}
extern "C" void __stdcall search_libraries(LinkContext *context)
{
    OutputLibrary *last_changed=NULL,*library;
    LibraryEnumerator libraries;
    BYTE keep_searching=1,initial_libraries,changed;
    if (verbose_link) {
        fputc('\n',stdout);
        link_message(9002);
    }
    do {
        initialize_library_enumerator(&libraries,(OutputLibrary *)context->pointer_list.first);
        initial_libraries=1;
        for (;;) {
            if (initial_libraries && next_library(&libraries)) library=libraries.current;
            else initial_libraries=0;
            if (!initial_libraries) {
                library=load_next_default_library(&context->pointer_list);
                if (!library) break;
            }
            if (library==last_changed) goto done;
            if (library->flags & 4) continue;
            changed=0;
            ensure_library_loaded(context,library);
            library->SearchSymbols(context,&changed,&keep_searching);
            if (incremental_enabled && incremental_status_code) goto done;
            if (!keep_searching) {
                if (initial_libraries)
                    end_section_enumerator((SectionEnumerator *)&libraries);
                goto done;
            }
            if (changed) last_changed=library;
        }
        if (!last_changed) break;
    } while (keep_searching);
done:
    if (verbose_link) {
        fputc('\n',stdout);
        link_message(9003);
    }
}
extern "C" StringNode *__stdcall resolve_entry_symbol(LinkContext *context)
{
    char *name=(char *)context->entry_name;
    WORD machine=context->file_header.Machine;
    if ((machine==IMAGE_FILE_MACHINE_I386 || machine==0x1a2 ||
         machine==0x1a6 || machine==0x601) && context->is_vxd!=1 && name[0]!='?') {
        name=(char *)allocate_memory(strlen(context->entry_name)+2);
        name[0]='_';
        strcpy(name+1,context->entry_name);
    }
    entry_symbol=context->symbols->Lookup((const unsigned char *)name,NULL);
    /* Lookup may update the context through its symbol-table callbacks. */
    if (name!=context->entry_name) release_memory(name);
    return entry_symbol;
}
extern "C" BYTE __stdcall resolve_initial_entry(LinkContext *context)
{
    if (!context->entry_name)
        context->entry_name=duplicate_link_string(entry_point_name);
    entry_symbol=resolve_entry_symbol(context);
    return !(entry_symbol->flags & 1);
}
extern "C" void __stdcall find_console_entry_symbols(LinkContext *,StringNode **,StringNode **);
extern "C" void __stdcall find_gui_entry_symbols(LinkContext *,StringNode **,StringNode **);
extern "C" BYTE __stdcall infer_subsystem(LinkContext *context)
{
    StringNode *main_symbol,*wide_main_symbol,*win_main_symbol,*wide_win_main_symbol;
    BYTE console,gui;
    find_console_entry_symbols(context,&main_symbol,&wide_main_symbol);
    find_gui_entry_symbols(context,&win_main_symbol,&wide_win_main_symbol);
    console=main_symbol!=NULL || wide_main_symbol!=NULL;
    gui=win_main_symbol!=NULL || wide_win_main_symbol!=NULL;
    if (console) {
        if (gui) link_warning(NULL,4031);
        context->optional_header.Subsystem=IMAGE_SUBSYSTEM_WINDOWS_CUI;
    } else {
        if (!gui && !(context->file_header.Characteristics & IMAGE_FILE_DLL)) return 0;
        context->optional_header.Subsystem=IMAGE_SUBSYSTEM_WINDOWS_GUI;
    }
    if (entry_symbol) return 0;
    return infer_entry_point(context);
}
extern "C" void __stdcall find_console_entry_symbols(LinkContext *context,
    StringNode **main_symbol,StringNode **wide_main_symbol)
{
    StringTable *table=context->symbols;
    /* The original default passes its defined context pointer as both names.
     * Every supported machine below replaces that consumed legacy fallback. */
    const char *main_name=(const char *)context,*wide_name=(const char *)context;
    switch (context->file_header.Machine) {
    case IMAGE_FILE_MACHINE_I386:
    case 0x1a2: case 0x1a6: case 0x601:
        main_name="_main";
        wide_name="_wmain";
        break;
    case IMAGE_FILE_MACHINE_R4000:
    case 0x168: case IMAGE_FILE_MACHINE_ALPHA:
    case 0x1c0: case IMAGE_FILE_MACHINE_POWERPC:
        main_name="main";
        wide_name="wmain";
        break;
    }
    *main_symbol=table->FindSymbol(main_name);
    *wide_main_symbol=table->FindSymbol(wide_name);
}
extern "C" void __stdcall find_gui_entry_symbols(LinkContext *context,
    StringNode **win_main_symbol,StringNode **wide_win_main_symbol)
{
    StringTable *table=context->symbols;
    /* Preserve the same consumed context-pointer fallback as the console probe. */
    const char *name=(const char *)context,*wide_name=(const char *)context;
    switch (context->file_header.Machine) {
    case IMAGE_FILE_MACHINE_I386:
        name="_WinMain@16";
        wide_name="_wWinMain@16";
        break;
    case 0x1a2: case 0x1a6: case 0x601:
        name="_WinMain";
        wide_name="_wWinMain";
        break;
    case IMAGE_FILE_MACHINE_R4000:
    case 0x168: case IMAGE_FILE_MACHINE_ALPHA:
    case 0x1c0: case IMAGE_FILE_MACHINE_POWERPC:
        name="WinMain";
        wide_name="wWinMain";
        break;
    }
    *win_main_symbol=table->FindSymbol(name);
    *wide_win_main_symbol=table->FindSymbol(wide_name);
}
extern "C" BYTE __stdcall infer_entry_point(LinkContext *context)
{
    const char *startup=NULL;
    StringNode *main_symbol,*wide_main_symbol;
    BYTE ambiguous=0;
    if (context->options.rom || context->is_vxd==1) return 0;
    if (context->file_header.Characteristics & IMAGE_FILE_DLL) {
        if (!context->options.no_entry && !incremental_reuse) {
            switch (context->optional_header.Subsystem) {
            case IMAGE_SUBSYSTEM_WINDOWS_GUI:
            case IMAGE_SUBSYSTEM_WINDOWS_CUI:
                startup=context->file_header.Machine==IMAGE_FILE_MACHINE_I386 ?
                    "_DllMainCRTStartup@12" : "_DllMainCRTStartup";
                break;
            case 9:
                startup="_DllMainCRTStartup";
                break;
            }
        }
    } else {
        switch (context->optional_header.Subsystem) {
        case IMAGE_SUBSYSTEM_NATIVE:
            startup="NtProcessStartup";
            break;
        case IMAGE_SUBSYSTEM_WINDOWS_GUI:
        case 9:
            find_gui_entry_symbols(context,&main_symbol,&wide_main_symbol);
            startup="WinMainCRTStartup";
            if (wide_main_symbol) {
                if (main_symbol) ambiguous=1;
                else startup="wWinMainCRTStartup";
            }
            break;
        case IMAGE_SUBSYSTEM_WINDOWS_CUI:
            find_console_entry_symbols(context,&main_symbol,&wide_main_symbol);
            startup="mainCRTStartup";
            if (wide_main_symbol) {
                if (main_symbol) ambiguous=1;
                else startup="wmainCRTStartup";
            }
            break;
        case IMAGE_SUBSYSTEM_POSIX_CUI:
            startup="__PosixProcessStartup";
            break;
        }
    }
    if (!startup) return 0;
    if (ambiguous) link_warning(NULL,4067,startup);
    entry_point_name=duplicate_argument(startup);
    return resolve_initial_entry(context);
}
extern "C" void __stdcall finalize_mac_entry(StringNode **entry,LinkContext *context) { __asm { int 3 } }
extern "C" void __stdcall set_subsystem_version(LinkContext *context)
{
    WORD major,minor;
    switch(context->optional_header.Subsystem) {
    case IMAGE_SUBSYSTEM_NATIVE:
    case IMAGE_SUBSYSTEM_WINDOWS_GUI:
    case IMAGE_SUBSYSTEM_WINDOWS_CUI:
        major=4; minor=0; break;
    case IMAGE_SUBSYSTEM_POSIX_CUI:
        major=19; minor=90; break;
    case 9:
        major=2; minor=0; break;
    default:
        major=minor=0; break;
    }
    context->optional_header.MajorSubsystemVersion=major;
    context->optional_header.MinorSubsystemVersion=minor;
}
extern "C" void __stdcall warn_unused_libraries(OutputLibrary *library) { __asm { int 3 } }
extern "C" OutputLibrary *__stdcall find_loaded_default_library(const char *,LinkPointerList *);
extern "C" void __stdcall warn_missing_default_libraries(LinkPointerList *libraries)
{
    DefaultLibrary *item;
    OutputLibrary *loaded;
    char module_name[MAX_PATH*2];
    if(libraries->suppress_default_libraries)return;
    for(item=libraries->default_libraries;item;item=item->next) {
        if(item->flags & 4)continue;
        loaded=find_loaded_default_library(item->name,libraries);
        if(loaded && !(loaded->flags & 8))continue;
        if((item->flags & 8) && (item->flags & 0x40)) {
            if(item->owner) {
                format_module_name((OutputModule *)item->owner,module_name);
                link_warning(module_name,4098,item->name);
            } else link_warning(NULL,4098,item->name);
        }
    }
}
extern "C" void __stdcall warn_unused_delay_libraries(void)
{
    InputArgument *argument=delay_libraries.first;
    unsigned index=0;
    while(argument && index<delay_libraries.count) {
        if(!((WORD)argument->flags & 0x200))
            link_warning(NULL,4199,argument->name,argument->name);
        argument=argument->next;
        ++index;
    }
}

extern "C" void __stdcall read_library_index(OutputLibrary *library,LinkContext *context)
{
    DWORD error=0;
    seek_file(current_input_file,8,0,&error);
    library->ReadIndex(context);
}

extern "C" char * __stdcall derive_old_image_name(LinkContext *context)
{
    InputArgument *argument;
    unsigned index;
    int file;
    BYTE archive;
    char drive[4],directory[256],name[256],path[260];
    if(!output_file_name) {
        argument=link_files.first;
        for(index=0;index<link_files.count;++index) {
            file=open_argument_file(argument->name,0x8000,0);
            archive=is_archive(argument->name,file);
            close_argument_file(file,0);
            if(!archive) {
                infer_output_file_name(context,argument);
                break;
            }
            argument=argument->next;
        }
    }
    if(!output_file_name)return NULL;
    _splitpath(output_file_name,drive,directory,name,NULL);
    _makepath(path,drive,directory,name,".ilk");
    return duplicate_argument(path);
}

extern "C" {
extern BYTE duplicate_symbols;
extern char short_symbol_name[9];
void __stdcall merge_librarian_exports(StringTable *,StringTable *,void *,DWORD);
void __stdcall initialize_hash_enumerator(HashState *);
void __stdcall end_hash_enumerator(HashState *);
StringNode *__stdcall record_incremental_entry(const char *,Contribution *,LinkContext *,DWORD)
{ __asm { int 3 } return NULL; }
}
void RECON_THISCALL LinkContext::ResolveEntryPoint(void)
{
    StringTable *matches;
    StringNode *candidate;
    const char *name;
    union { DWORD value; WORD kind; BYTE decorated; } mode;
    /* get_symbol_name writes WORD0, then this caller replaces BYTE0. The
     * forwarded DWORD's upper half is unwritten in the original. Its actual
     * merge consumer reads only BYTE0; native zero does not qualify wider use. */
#ifdef __linux__
    mode.value=0;
#endif
    create_string_table(&matches,1024,128);
    name=get_symbol_name(entry_symbol,symbols->hash.string_storage,&mode.kind,NULL);
    if(mode.kind==1)name=strncpy(short_symbol_name,name,8);
    switch(file_header.Machine) {
    case IMAGE_FILE_MACHINE_I386: case 0x1a2: case 0x1a6: case 0x601:
        if(is_vxd!=1 && *name=='_')++name;
        mode.decorated=1;
        break;
    default:
        mode.decorated=0;
        break;
    }
    candidate=matches->Lookup((const unsigned char *)name,NULL);
    set_symbol_defined(candidate,1,matches);
    merge_librarian_exports(matches,symbols,NULL,mode.value);
    if(duplicate_symbols)link_fatal(NULL,1152);
    initialize_hash_enumerator(matches->hash.state);
    while((candidate=matches->NextSymbol())!=NULL) {
        if((candidate->flag_byte&1) && (candidate->flag_byte&0x10)) {
            entry_symbol->flags|=0x100;
            name=get_symbol_name(candidate,matches->hash.string_storage,&mode.kind,NULL);
            if(mode.kind==1)name=strncpy(short_symbol_name,name,8);
            entry_symbol=symbols->FindSymbol(name);
        }
    }
    end_hash_enumerator(matches->hash.state);
    if(incremental_enabled && entry_symbol) {
        symbols->ReleaseLookupIndex();
        record_incremental_entry(name,entry_symbol->contribution,this,0);
    }
    clear_buffer(&matches->hash.strings);
}
