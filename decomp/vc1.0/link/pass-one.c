/* First link pass: explicit objects, library extraction, and subsystem inference. */
#include "link-pass.h"
#include "archive.h"
#include "machine.h"
#include "comdat.h"
unsigned long archive_data_offset,archive_member_size;
Library *explicit_input_library;
unsigned char output_open, infer_entry_point;
int output_file_handle=-1;
int output_file;
unsigned char machine_initialized;
extern unsigned char verbose_link, mac_image;
extern unsigned short warning_level;
extern char *output_name,default_output_name[],*def_filename,*repro_directory;
extern unsigned long weak_symbol_count;

void pass_one(Image *image)
{
    Argument *argument;
    unsigned count;
    LibraryEnumerator libraries;
    if (tool_kind==1 && image->options.output_specified && !output_open) {
        output_file=open_file(output_name,0x8302,0x180);
        output_open=1;
        output_file_handle=output_file;
    }
    if (verbose_link) printf("\nStart Pass1\n");
    archive_data_offset=0;
    explicit_input_library=create_library(NULL,0,1,&image->libraries);
    explicit_input_library->search_flags |= 4;
    argument=input_files.first;
    for (count=0;count<input_files.count;++count) {
        if (repro_directory) reproduce_input(argument->original,1);
        process_input_argument(argument,image);
        argument=argument->next;
    }
    check_explicit_modules(image,explicit_input_library);
    if (weak_symbol_count) resolve_weak_externals(image->symbols,0x40);
    infer_entry_point=tool_kind==1 && !image->optional.Subsystem && !mac_image &&
        !(image->file.Characteristics&IMAGE_FILE_DLL) && !image->options.rom;
    if (infer_entry_point) infer_subsystem(image);
    search_libraries(image);
    if (tool_kind==1) {
        if (create_import_library(image,def_filename,executable_path)) search_libraries(image);
        if (tool_kind==1 && !output_open) {
            output_file=open_file(output_name,0x8302,0x180);
            output_open=1;
        }
    }
    output_file_handle=output_file;
    if (warning_level>1) warn_unused_libraries(image->libraries.first);
    if (weak_symbol_count) {
        resolve_weak_externals(image->symbols,0x4080);
        if (weak_symbol_count) resolve_weak_externals(image->symbols,0x40);
    }
    apply_section_overrides(image);
    apply_section_merges(image);
    if (tool_kind==1) {
        initialize_library_enumerator(&libraries,image->libraries.first);
        while (next_library(&libraries)) {
            free_pointer((void **)&libraries.current->member_offsets);
            free_pointer((void **)&libraries.current->symbol_name_buffer);
            free_pointer((void **)&libraries.current->symbol_indices);
            free_pointer((void **)&libraries.current->symbol_names);
            free_pointer((void **)&libraries.current->long_names);
        }
    }
    if (verbose_link) printf("\nEnd Pass1\n\n");
}

void process_input_argument(Argument *argument,Image *image)
{
    unsigned long previous=archive_data_offset;
    Module *module;
    archive_data_offset=0;
    input_file=open_file(argument->original,0x8000,0);
    recognize_input_file(argument,explicit_input_library,image,&module);
    if (module) {
        if (!machine_initialized) initialize_machine(image);
        process_object(image,NULL,NULL,module,1,0);
    }
    close_file(input_file,0);
    archive_data_offset=previous;
}

void recognize_input_file(Argument *argument,Library *library,Image *image,Module **output)
{
    char signature[8];
    unsigned short machine=0;
    unsigned char object=0;
    unsigned long magic;
    Library *archive;
    *output=NULL;
    if ((unsigned short)read_file(input_file,signature,8)<8)
        fatal_message(argument->original,0xa,tell_file(input_file));
    else if (strncmp(signature,"!<arch>\n",8)) object=1;
    if (!object) {
        seek_file(input_file,0,SEEK_SET);
        archive=create_library(argument->text,0,1,&image->libraries);
        begin_library(archive,image);
        archive->search_flags &= ~4UL;
        archive_data_offset=0;
        return;
    }
    if ((unsigned char)signature[0]==0x80) convert_omf(argument,&machine,&image->options);
    else {
        magic=*(unsigned long *)signature;
        if ((unsigned char)signature[0]==0xff || !magic)
            convert_object(argument,&machine,image);
        else {
            machine=*(unsigned short *)signature;
            if (machine!=0 && machine!=0x14c && machine!=0x14d && machine!=0x162 &&
                machine!=0x166 && machine!=0x268) convert_object(argument,&machine,image);
        }
    }
    *output=read_object_module(argument,machine,image,library);
}

Module *read_object_module(Argument *argument,unsigned short machine,Image *image,Library *library)
{
    IMAGE_FILE_HEADER header;
    Module *module;
    unsigned char inserted;
    char filename[_MAX_FNAME],*end;
    const char *extension;
    select_machine(argument,machine,image);
    seek_file(input_file,0,SEEK_SET);
    read_coff_header(input_file,&header);
    module=create_module(argument->original,argument->text,0,header.Machine,
        header.PointerToSymbolTable,header.NumberOfSymbols,header.SizeOfOptionalHeader,
        header.Characteristics,header.NumberOfSections,library,&inserted);
    if (!inserted) {
        warning_message(argument->text,0x71,"","");
        return NULL;
    }
    if (image->options.eliminate_unreferenced) module->comdat=create_comdat_module(module);
    seek_file(input_file,0,SEEK_SET);
    if (output_name==default_output_name) {
        _splitpath(argument->text,NULL,NULL,filename,NULL);
        extension=(image->file.Characteristics&IMAGE_FILE_DLL)?".dll":".exe";
        end=filename;
        while (*end) ++end;
        while ((*end++=*extension++)!=0) {}
        output_name=duplicate_string(filename);
    }
    return module;
}

void check_explicit_modules(Image *image,Library *library)
{
    ModuleEnumerator modules;
    initialize_module_enumerator(&modules,library);
    if (!next_module(&modules)) {
        warning_message(program_name,0x48,"","");
        initialize_machine(image);
    }
    end_module_enumerator(&modules);
}

/* Probe the two application entry points before choosing a CRT startup. */
External *main_symbol,*winmain_symbol;
extern char *entry_point;

void infer_subsystem(Image *image)
{
    SymbolTable *table=image->symbols;
    External *unused=NULL;
    unsigned char created=0,imports=0;
    if (!main_symbol) {
        main_symbol=lookup_external(table,1,"_main",&created,&imports);
        if (!(main_symbol->flags&EXTERNAL_DEFINED)) record_undefined_reference(main_symbol,output_module);
    }
    if (!winmain_symbol) {
        winmain_symbol=lookup_external(table,0,"_WinMain@16",&created,&imports);
        if (!(winmain_symbol->flags&EXTERNAL_DEFINED)) record_undefined_reference(winmain_symbol,output_module);
    }
    if ((main_symbol->flags&EXTERNAL_DEFINED) || !external_referenced_by(main_symbol,output_module)) {
        image->optional.Subsystem=IMAGE_SUBSYSTEM_WINDOWS_CUI;
        unused=winmain_symbol;
    }
    if ((winmain_symbol->flags&EXTERNAL_DEFINED) || !external_referenced_by(winmain_symbol,output_module)) {
        if (!(main_symbol->flags&EXTERNAL_DEFINED) && external_referenced_by(main_symbol,output_module)) {
            image->optional.Subsystem=IMAGE_SUBSYSTEM_WINDOWS_GUI;
            unused=main_symbol;
        } else {
            warning_message(program_name,0x66,"","");
            image->optional.Subsystem=IMAGE_SUBSYSTEM_WINDOWS_CUI;
            unused=NULL;
        }
    }
    if (image->optional.Subsystem) {
        if (!image->optional.MajorSubsystemVersion && !image->optional.MinorSubsystemVersion)
            set_subsystem_version(&image->optional);
        if (!entry_point) select_entry_point(image,1);
        if (unused) unused->flags |= 0x100;
        infer_entry_point=0;
    }
}

void search_libraries(Image *image)
{
    Library *last_changed=NULL,*library,*default_library;
    LibraryEnumerator libraries;
    unsigned char unresolved=1,enumerating,changed;
    unsigned long previous_offset;
    int previous_file;
    if (verbose_link) printf("\nSearching Libraries\n");
    while (unresolved) {
        enumerating=1;
        initialize_library_enumerator(&libraries,image->libraries.first);
        for (;;) {
            if (enumerating && next_library(&libraries)) library=libraries.current;
            else {
                enumerating=0;
                default_library=tool_kind==1?load_next_default_library(&image->libraries):NULL;
                if (default_library) library=default_library;
                else {
                    if (old_library_format || !last_changed) goto finished;
                    break;
                }
            }
            if (!old_library_format && library==last_changed) goto finished;
            if (library->search_flags&4) continue;
            changed=0;
            if (library->name && !library->symbol_names) {
                previous_offset=archive_data_offset;
                previous_file=input_file;
                input_file=open_file(library->name,0x8000,0);
                begin_library(library,image);
                close_file(input_file,0);
                input_file=previous_file;
                archive_data_offset=previous_offset;
            }
            if (library->name && verbose_link) printf("\tSearching %s:\n",library->name);
            search_library_members(image,library,&changed,&unresolved);
            if (infer_entry_point) infer_subsystem(image);
            if (!unresolved) {
                if (enumerating) end_library_enumerator(&libraries);
                break;
            }
            if (changed) {
                if (old_library_format) {
                    if (enumerating) end_library_enumerator(&libraries);
                    initialize_library_enumerator(&libraries,image->libraries.first);
                    enumerating=1;
                } else last_changed=library;
            }
        }
    }
finished:
    if (verbose_link) printf("\nDone Searching Libraries\n");
}

void begin_library(Library *library,Image *image)
{
    seek_file(input_file,8,SEEK_SET);
    archive_data_offset=8;
    archive_member_size=0;
    read_archive_index(library,0,image);
}
