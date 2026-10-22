/* Library-manager driver and archive member replacement/extraction. */
#include "library.h"
#include <io.h>
#pragma intrinsic(strlen,strcat)
extern char *output_name,default_output_name[],*def_filename;
extern Module *output_module;
char *library_listing_name,*extract_member;
Library *first_input_library;
unsigned char remove_members_requested;

int library_main(int argc,char **argv)
{
    Image *image;
    Library *library=NULL,*added,*current;
    LibraryEnumerator libraries;
    ModuleEnumerator modules;
    IMAGE_ARCHIVE_MEMBER_HEADER member;
    IMAGE_FILE_HEADER header;
    Argument *argument;
    char *name,*full_output,*full_input,*destination;
    char temporary[52],filename[512],extension[256],default_name[512],buffer[512];
    unsigned short index,machine;
    unsigned long offset,size,remaining,amount;
    unsigned char raw_members=0,replace_output=0;
    FILE *listing;
    int result;
    if (argc<2) {
        setvbuf(stdout,NULL,0,1);
        if (banner_pending) print_banner();
        printf("usage: LIB [options] [files]\n\n",program_name);
        printf("   options:\n\n");
        printf("      /DEF:[filename]\n");
        printf("      /EXPORT:symbol\n");
        printf("      /EXTRACT:membername\n");
        printf("      /LIST[:filename]\n");
        printf("      /OUT:filename\n");
        printf("      /REMOVE:membername\n");
        printf("      /SUBSYSTEM:{WINDOWS|CONSOLE}[,#[.#]]\n");
        fflush(stdout);
        return 3;
    }
    create_image(&image);
    collect_arguments(argc,argv,NULL);
    image->optional.Subsystem=0;
    process_library_options(image);
    if (banner_pending) print_banner();
    if (image->options.library_definition) {
        argv[1]=def_filename;
        argv[2]=NULL;
        return build_import_library(2,argv,image);
    }
    image->options.dump.display.library_mode=3;
    initialize_modules((void (*)(char *,int,char *))fatal_message,&output_module);
    if (!library_files.count && !other_inputs.count) return 0;
    argument=other_inputs.first;
    for (index=0;index<other_inputs.count;++index,argument=argument->next) {
        library=create_library(argument->text,0,1,&image->libraries);
        if (!index) first_input_library=library;
        input_file=open_file(library->name,0x8000,0);
        archive_data_offset=8;
        archive_member_size=0;
        read_archive_index(library,0,image);
        offset=archive_data_offset=8;
        archive_member_size=0;
        seek_file(input_file,8,SEEK_SET);
        size=file_size(input_file);
        while (offset<size) {
            read_file(input_file,&member,sizeof(member));
            if (strncmp((char *)member.Name,"/               ",16) &&
                strncmp((char *)member.Name,"//              ",16)) {
                name=get_archive_member_name(library,&member);
                if (!name) fatal_message(argument->text,0x1e,"");
                name=duplicate_string(name);
                read_coff_header(input_file,&header);
                create_module(NULL,name,offset,header.Machine,header.PointerToSymbolTable,
                    header.NumberOfSymbols,header.SizeOfOptionalHeader,header.Characteristics,
                    header.NumberOfSections,library,NULL);
            }
            sscanf((char *)member.Size,"%ld",&archive_member_size);
            offset+=archive_member_size;
            offset+=sizeof(member)+(offset&1);
            seek_file(input_file,offset,SEEK_SET);
        }
        close_file(input_file,0);
    }
    if (image->options.library_list) {
        listing=stdout;
        if (library_listing_name) {
            listing=fopen(library_listing_name,"w");
            if (!listing) fatal_message(program_name,7,library_listing_name);
        }
        initialize_module_enumerator(&modules,library);
        while (next_module(&modules)) fprintf(listing,"%s\n",modules.current->path);
        return 0;
    }
    if (extract_member) {
        initialize_library_enumerator(&libraries,image->libraries.first);
        next_library(&libraries);
        end_library_enumerator(&libraries);
        current=libraries.current;
        input_file=open_file(current->name,0x8000,0);
        offset=archive_data_offset=8;
        archive_member_size=0;
        seek_file(input_file,8,SEEK_SET);
        size=file_size(input_file);
        while (offset<size) {
            read_file(input_file,&member,sizeof(member));
            sscanf((char *)member.Size,"%ld",&archive_member_size);
            name=get_archive_member_name(current,&member);
            if (!name) fatal_message(current->name,0x1e,"");
            if (!_strcmpi(extract_member,name)) {
                sscanf((char *)member.Size,"%ld",&archive_member_size);
                if (output_name==default_output_name) {
                    _splitpath(name,NULL,NULL,filename,extension);
                    strcat(filename,extension);
                    output_name=duplicate_string(filename);
                }
                output_file=open_file(output_name,0x8302,0x180);
                remaining=archive_member_size;
                do {
                    amount=(long)remaining<512?remaining:512;
                    read_file(input_file,buffer,amount);
                    write_file(output_file,buffer,amount);
                    remaining-=amount;
                } while (remaining);
                release_library_list(&image->libraries);
                close_file(output_file,1);
                close_file(input_file,0);
                exit(0);
            }
            offset+=archive_member_size;
            offset+=sizeof(member)+(offset&1);
            seek_file(input_file,offset,SEEK_SET);
        }
        warning_message(program_name,0x55,extract_member,"");
        release_library_list(&image->libraries);
        close_file(input_file,0);
        return 0;
    }
    if (library_files.count) {
        convert_omf_inputs(0,&image->options);
        if (!image->file.Machine) {
            input_file=open_file(library_files.first->original,0x8000,0);
            read_file(input_file,&machine,sizeof(machine));
            close_file(input_file,0);
            if (machine!=0x14c && machine!=0x14d && machine!=0x162 &&
                machine!=0x166 && machine!=0x268) raw_members=1;
        }
        if (!raw_members) validate_library_inputs(image);
    }
    added=create_library(NULL,0,1,&image->libraries);
    argument=library_files.first;
    for (index=0;index<library_files.count;++index,argument=argument->next) {
        if (find_module(library,argument->text,0)) {
            remove_library_member(image,argument->text);
            printf("Replacing %s\n",argument->text);
            fflush(stdout);
        }
        input_file=open_file(argument->original,0x8000,0);
        read_coff_header(input_file,&header);
        create_module(argument->original,argument->text,0,header.Machine,
            header.PointerToSymbolTable,header.NumberOfSymbols,header.SizeOfOptionalHeader,
            header.Characteristics,header.NumberOfSections,added,NULL);
        close_file(input_file,0);
    }
    if (remove_members_requested) {
        argument=switches.first;
        for (index=0;index<switches.count;++index,argument=argument->next) {
            if (!_strnicmp(argument->text,"remove",6) &&
                !remove_library_member(image,argument->text+7))
                warning_message(program_name,0x55,argument->text+7,"");
        }
    }
    if (output_name==default_output_name) {
        if (!first_argument) warning_message(program_name,0x69);
        else {
            _splitpath(first_argument->text,NULL,NULL,default_name,NULL);
            strcat(default_name,".lib");
            output_name=duplicate_string(default_name);
        }
    }
    full_output=_fullpath(NULL,output_name,0);
    if (!full_output) out_of_memory();
    destination=output_name;
    initialize_library_enumerator(&libraries,image->libraries.first);
    while (next_library(&libraries)) {
        current=libraries.current;
        full_input=_fullpath(NULL,current->name,0);
        if (!full_input) out_of_memory();
        if (current->name && !_strcmpi(full_input,full_output)) {
            replace_output=1;
            strncpy(temporary,program_name,2);
            temporary[2]=0;
            strcat(temporary,"XXXXXX");
            destination=_mktemp(temporary);
            if (!destination) fatal_message(program_name,7);
            end_library_enumerator(&libraries);
            free(full_input);
            break;
        }
        free(full_input);
    }
    free(full_output);
    output_file=open_file(destination,0x8302,0x180);
    result=write_library(raw_members,image);
    close_file(output_file,1);
    close_all_files();
    remove_temporary_inputs();
    if (replace_output) {
        if (remove(output_name)) fatal_message(program_name,0xc,output_name);
        if (rename(destination,output_name)) fatal_message(program_name,0xd,destination,output_name);
    }
    return result;
}

void reject_output_input_collision(const char *output,Argument *inputs)
{
    char full_output[260],full_input[260];
    if (!_fullpath(full_output,output,259)) fatal_message(program_name,7,output);
    while (inputs) {
        if (!_fullpath(full_input,inputs->text,259)) fatal_message(program_name,7,inputs->text);
        if (!_strcmpi(full_output,full_input)) {
            output_name=NULL;
            fatal_message(program_name,0x34,full_output);
        }
        inputs=inputs->next;
    }
}


void convert_omf_inputs(unsigned char warn,ImageOptions *options)
{
    Argument *argument=library_files.first;
    unsigned short index;
    unsigned char first;
    for (index=0;index<library_files.count;++index,argument=argument->next) {
        input_file=open_file(argument->text,0x8000,0);
        if (read_file(input_file,&first,1)!=1) fatal_message(argument->text,0x27,"");
        close_file(input_file,0);
        if (first==0x80) {
            if (warn) warning_message(argument->text,0x68,"","");
            convert_omf_object(argument,(unsigned char)!warn,options);
        }
    }
}


void validate_library_inputs(Image *image)
{
    Argument *argument=library_files.first;
    unsigned short index=0,machine;
    if (library_files.count) do {
        input_file=open_file(argument->original,0x8000,0);
        read_file(input_file,&machine,sizeof(machine));
        close_file(input_file,0);
        select_machine(argument,machine,image);
        argument=argument->next;
        ++index;
    } while (index>library_files.count);
}


/* Original [0002fe00,0002fee0). Unlink the first named member in each library. */
unsigned char remove_library_member(Image *image,const char *name)
{
    LibraryEnumerator libraries;
    ModuleEnumerator modules;
    Module **link,*module;
    const unsigned char *left,*right;
    unsigned char removed=0;
    initialize_library_enumerator(&libraries,image->libraries.first);
    while (next_library(&libraries)) {
        initialize_module_enumerator(&modules,libraries.current);
        link=&libraries.current->modules;
        while (next_module(&modules)) {
            module=modules.current;
            left=(const unsigned char *)name;
            right=(const unsigned char *)module->path;
            while (*left && *left==*right) { ++left; ++right; }
            if (*left==*right) {
                removed=1;
                *link=module->next;
                break;
            }
            link=&module->next;
        }
    }
    return removed;
}
