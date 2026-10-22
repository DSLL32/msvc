/* Import-library and .exp construction orchestrator. */
#include "import-library.h"
#include <time.h>
#pragma intrinsic(strlen,strcpy,strcat,memcpy)
extern char *output_name,default_output_name[],*def_filename;
extern Module *output_module;
extern unsigned long symbol_record_size;
extern void __tzset(void);
extern unsigned long undefined_symbols;
extern unsigned char mac_image;
SymbolTable *export_symbol_table;
Image *export_image;
External *exports_table_symbol;
char *null_thunk_name,*export_object_name="";
unsigned long dll_basename_length;
ExternalLink *import_aliases;
unsigned long *import_member_offsets;
unsigned char *export_ordinals;
unsigned char import_only_export,export_error;
unsigned short import_member_base=4;
unsigned long first_export_ordinal=1,last_export_ordinal=1;
ImportMachine import_i386={IMAGE_FILE_MACHINE_I386,0};
ImportMachine import_i860={0x14d,1};
ImportMachine import_mips={0x162,0};

int build_import_library(int argc,char **argv,Image *image)
{
    Image *linked=NULL;
    ImportMachine *machine;
    UndefinedEnumerator undefined;
    External *external;
    Argument *argument;
    unsigned short index,first_machine;
    unsigned char created,imports,has_inputs;
    unsigned long members,export_count,ordinal_slots,import_symbols,buffer_size;
    unsigned long export_offset,end,object_size;
    char object_name[264],*dot,*name,*cursor,*names;
    void *object;
    int file;
    export_symbol_table=image->symbols;
    initialize_modules((void (*)(char *,int,char *))fatal_message,&output_module);
    export_image=image;
    if (!*export_object_name) {
        object_name[0]=0;
        dot=strrchr(output_name,'.');
        if (!dot || dot[1]=='.' || dot[1]=='\\') strcpy(object_name,output_name);
        else strncat(object_name,output_name,dot-output_name);
        strcat(object_name,".exp");
    } else strcpy(object_name,export_object_name);
    reject_output_input_collision(output_name,library_files.first);
    reject_output_input_collision(output_name,other_inputs.first);
    if (library_files.count) {
        convert_omf_inputs(0,&image->options);
        if (!image->file.Machine) {
            input_file=open_file(library_files.first->original,0x8000,0);
            read_file(input_file,&first_machine,sizeof(first_machine));
            close_file(input_file,0);
        }
        validate_library_inputs(image);
    }
    image->optional.ImageBase=0;
    switch (image->file.Machine) {
    case IMAGE_FILE_MACHINE_I386:
        machine=&import_i386;
        if (!library_files.count && !other_inputs.count) import_only_export=1;
        break;
    case 0x14d: machine=&import_i860; break;
    case 0x162: case 0x166: machine=&import_mips; break;
    default: fatal_message(program_name,0x12,""); break;
    }
    if (machine->helper_thunk) ++import_member_base;
    members=import_member_base;
    has_inputs=library_files.count || other_inputs.count;
    if (has_inputs) {
        create_image(&linked);
        linked->file=image->file;
        linked->optional=image->optional;
        linked->options=image->options;
        initialize_undefined_enumerator(&undefined,image->symbols);
        while (next_undefined(&undefined)) {
            external=undefined.current;
            if (external->symbol.N.Name.Short)
                name=strncpy(short_symbol_name,(char *)&external->symbol,8);
            else name=image->symbols->strings.data+external->symbol.N.Name.Long;
            lookup_external(linked->symbols,(unsigned short)(strlen(name)<=8),name,&created,&imports);
        }
    }
    if (*argv[1]) read_definition_file(image,argv[1]);
    image->options.optimize=2;
    if (has_inputs) {
        argument=library_files.first;
        for (index=0;index<library_files.count;++index,argument=argument->next)
            copy_argument_node(&input_files,argument->text,argument->original);
        clear_argument_list(&library_files);
        if (!search_paths.first) collect_search_path("LIB",&search_paths);
        argument=other_inputs.first;
        for (index=0;index<other_inputs.count;++index,argument=argument->next) {
            argument->original=find_library_file(argument->text,".lib",&search_paths);
            copy_argument_node(&input_files,argument->text,argument->original);
        }
        clear_argument_list(&other_inputs);
        pass_one(linked);
        argument=exports.first;
        for (index=0;index<exports.count;++index,argument=argument->next)
            parse_export(argument->text,image,(unsigned char)(argument->original!=NULL),argument->original);
        merge_export_definitions(image->symbols,linked->symbols,1,linked->libraries.first);
        emit_externals(image->symbols,0,0,1,&image->options);
        if (undefined_symbols) {
            fatal_message(output_name,0x17);
            return 0x17;
        }
        if (export_error) return 0x5d;
        release_text(&linked->symbols->strings);
    }
    invalidate_symbol_sorts(image->symbols);
    if (!image->options.library_image_name) {
        image->options.library_image_name=duplicate_string(*argv[1]?argv[1]:output_name);
        name=image->options.library_image_name;
        dot=strrchr(name,'.');
        if (dot) *dot=0;
        cursor=name+strlen(name);
        while (cursor>name && cursor[-1]!='\\') --cursor;
        if (cursor>name) strcpy(name,cursor);
    }
    dll_basename_length=strlen(image->options.library_image_name);
    __tzset();
    time((time_t *)&image->file.TimeDateStamp);
    symbol_record_size=18;
    export_count=count_export_symbols(image->symbols,&import_symbols);
    ordinal_slots=last_export_ordinal-first_export_ordinal+1;
    if (ordinal_slots<export_count) ordinal_slots=export_count;
    buffer_size=dll_basename_length*3+0x45;
    if (machine->helper_thunk) buffer_size+=dll_basename_length+0xf;
    names=malloc(buffer_size);
    if (!names) out_of_memory();
    import_aliases=create_import_aliases(image->symbols);
    members+=export_count;
    strcpy(names,image->options.library_image_name);
    strcat(names,"_IMPORT_DESCRIPTOR");
    external=lookup_external(image->symbols,0,names,&created,&imports);
    set_external_defined(external,1,image->symbols);
    external->flags |= 0x800;
    external->archive_member=1;
    cursor=names+dll_basename_length+0x13;
    strcpy(cursor,"NULL_IMPORT_DESCRIPTOR");
    external=lookup_external(image->symbols,0,cursor,&created,&imports);
    set_external_defined(external,1,image->symbols);
    external->flags |= 0x800;
    external->archive_member=2;
    null_thunk_name=cursor+0x17;
    *null_thunk_name=0x7f;
    cursor+=0x18;
    strcpy(cursor,image->options.library_image_name);
    strcat(cursor,"_NULL_THUNK_DATA");
    external=lookup_external(image->symbols,0,null_thunk_name,&created,&imports);
    set_external_defined(external,1,image->symbols);
    external->flags |= 0x800;
    external->archive_member=3;
    cursor+=dll_basename_length+0x11;
    strcpy(cursor,image->options.library_image_name);
    strcat(cursor,"_EXPORTS");
    exports_table_symbol=lookup_external(image->symbols,0,cursor,&created,&imports);
    set_external_defined(exports_table_symbol,1,image->symbols);
    exports_table_symbol->flags |= 0x800;
    exports_table_symbol->value.final_value=0;
    exports_table_symbol->archive_member=4;
    members=members-import_symbols+export_count;
    if (machine->helper_thunk) {
        strcpy(cursor,image->options.library_image_name);
        strcat(cursor,"_THUNK_ROUTINE");
        external=lookup_external(image->symbols,0,cursor,&created,&imports);
        set_external_defined(external,1,image->symbols);
        external->flags |= 0x800;
        external->archive_member=5;
    }
    import_member_offsets=calloc(members+1,sizeof(*import_member_offsets));
    if (!import_member_offsets) out_of_memory();
    export_ordinals=calloc(ordinal_slots,1);
    if (!export_ordinals) out_of_memory();
    mark_explicit_export_ordinals(image->symbols,export_ordinals);
    printf("   Creating library %s and object %s ...\n",output_name,object_name);
    fflush(stdout);
    output_file=open_file(output_name,0x8302,0x180);
    write_file(output_file,"!<arch>\n",8);
    write_import_archive_indexes(members,image->symbols);
    write_import_descriptor(machine,image);
    write_null_import_descriptor(image);
    write_null_thunk_member(image);
    if (machine->helper_thunk) write_helper_thunk_member(machine,image);
    export_offset=tell_file(output_file);
    write_export_object(image,ordinal_slots,export_count,machine,has_inputs);
    end=tell_file(output_file);
    object_size=end-(export_offset+sizeof(IMAGE_ARCHIVE_MEMBER_HEADER));
    object=calloc(object_size,1);
    if (!object) out_of_memory();
    seek_file(output_file,export_offset+sizeof(IMAGE_ARCHIVE_MEMBER_HEADER),SEEK_SET);
    read_file(output_file,object,object_size);
    seek_file(output_file,end,SEEK_SET);
    file=open_file(object_name,0x8302,0x180);
    write_file(file,object,object_size);
    close_file(file,1);
    free(object);
    write_import_members(image,machine);
    finish_import_archive_indexes(image->symbols);
    close_file(output_file,1);
    close_all_files();
    remove_temporary_inputs();
    free(import_member_offsets);
    return 0;
}
