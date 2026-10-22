/* Ordinary COFF archive creation: indexes, long names and original members. */
#include "import-library.h"
#pragma intrinsic(strlen,strcpy)

int write_library(unsigned char raw_members,Image *image)
{
    LibraryEnumerator libraries;
    ModuleEnumerator modules;
    Module *module;
    SymbolTable *table=image->symbols;
    IMAGE_ARCHIVE_MEMBER_HEADER header;
    unsigned short long_name_bytes=0,member_count=0,member,index,machine;
    unsigned short *long_offsets=NULL;
    unsigned char new_externals=0,symbols_changed=0,second_index=0;
    unsigned long count,members,end,second_offset,aliases=0,exported=0;
    unsigned long long_size,position,size,remaining,amount;
    char *long_names,*cursor,*input,*name;
    char number[16],buffer[512];
    abort_phase="BuildSymbolTable";
    archive_data_offset=0;
    initialize_library_enumerator(&libraries,image->libraries.first);
    while (next_library(&libraries)) {
        initialize_module_enumerator(&modules,libraries.current);
        while (next_module(&modules)) {
            module=modules.current;
            if (!raw_members) {
                input=module->library->name;
                if (!input) input=module->name.display_name;
                input_file=open_file(input,0x8000,0);
            }
            if (strlen(module->path)>15) long_name_bytes+=(unsigned short)(strlen(module->path)+1);
            if (!raw_members) {
                archive_data_offset=module->library->name ? module->name.member_offset : 0;
                if (archive_data_offset) archive_data_offset+=sizeof(header);
                seek_file(input_file,archive_data_offset,SEEK_SET);
                read_file(input_file,&machine,2);
                seek_file(input_file,archive_data_offset,SEEK_SET);
                process_object(image,&new_externals,&symbols_changed,module,0,
                               (unsigned short)(member_count+1));
                close_file(input_file,0);
            }
            ++member_count;
        }
    }
    abort_file[0]=0;
    members=member_count;
    write_file(output_file,"!<arch>\n",8);
    abort_phase="BuildLinkerMember";
    if (count_externals(table)) {
        exported=count_export_symbols(table,&aliases);
        exported-=aliases;
        seek_file(output_file,sizeof(header),SEEK_CUR);
        count=encode_big_endian_dword(&exported);
        write_file(output_file,&count,4);
        seek_file(output_file,exported*4,SEEK_CUR);
        member=1;
        initialize_library_enumerator(&libraries,image->libraries.first);
        while (next_library(&libraries)) {
            initialize_module_enumerator(&modules,libraries.current);
            while (next_module(&modules)) write_import_member_names(table,member++);
        }
        end=tell_file(output_file);
        seek_file(output_file,8,SEEK_SET);
        write_file(output_file,build_archive_header("",0,end-68),sizeof(header));
        seek_file(output_file,end,SEEK_SET);
        if (end&1) write_file(output_file,"\n",1);
    }
    if (count_externals(table)) {
        second_index=1;
        second_offset=tell_file(output_file);
        seek_file(output_file,sizeof(header),SEEK_CUR);
        write_file(output_file,&members,4);
        seek_file(output_file,members*4,SEEK_CUR);
        write_file(output_file,&exported,4);
        seek_file(output_file,exported*2,SEEK_CUR);
        write_import_member_names(table,0);
        end=tell_file(output_file);
        seek_file(output_file,second_offset,SEEK_SET);
        write_file(output_file,build_archive_header("",0,end-second_offset-60),sizeof(header));
        seek_file(output_file,end,SEEK_SET);
        if (end&1) write_file(output_file,"\n",1);
    }
    abort_phase="EmitLongNames";
    if (long_name_bytes) {
        long_offsets=malloc(members*sizeof(*long_offsets));
        if (!long_offsets) out_of_memory();
        long_size=long_name_bytes;
        long_names=malloc(long_size);
        if (!long_names) out_of_memory();
        cursor=long_names;
        index=0;
        initialize_library_enumerator(&libraries,image->libraries.first);
        while (next_library(&libraries)) {
            initialize_module_enumerator(&modules,libraries.current);
            while (next_module(&modules)) {
                name=modules.current->path;
                if (strlen(name)<16) long_offsets[index]=0;
                else {
                    long_offsets[index]=(unsigned short)(cursor-long_names+1);
                    strcpy(cursor,name);
                    cursor+=strlen(name)+1;
                }
                ++index;
            }
        }
        write_file(output_file,build_archive_header("/",0,long_size),sizeof(header));
        write_file(output_file,long_names,long_size);
        free(long_names);
        if (long_name_bytes&1) write_file(output_file,"\n",1);
    }
    import_member_offsets=calloc(members+4,4);
    if (!import_member_offsets) out_of_memory();
    abort_phase="EmitMember";
    index=0;
    initialize_library_enumerator(&libraries,image->libraries.first);
    while (next_library(&libraries)) {
        initialize_module_enumerator(&modules,libraries.current);
        while (next_module(&modules)) {
            module=modules.current;
            input=module->library->name;
            if (!input) input=module->name.display_name;
            input_file=open_file(input,0x8000,0);
            if (!long_offsets || !long_offsets[index]) name=module->path;
            else {
                _itoa(long_offsets[index]-1,number,10);
                name=number;
            }
            archive_data_offset=module->library->name ? module->name.member_offset : 0;
            seek_file(input_file,archive_data_offset,SEEK_SET);
            position=tell_file(output_file);
            import_member_offsets[index+1]=position;
            if (!module->library->name || !module->name.member_offset)
                size=file_size(input_file);
            else {
                read_file(input_file,&header,sizeof(header));
                sscanf((char *)header.Size,"%ld",&size);
                archive_data_offset+=sizeof(header);
            }
            write_file(output_file,build_archive_header(name,
                (unsigned char)(long_offsets && long_offsets[index]),size),sizeof(header));
            remaining=size;
            do {
                amount=(long)remaining>511 ? 512 : remaining;
                read_file(input_file,buffer,amount);
                write_file(output_file,buffer,amount);
                remaining-=amount;
            } while (remaining);
            if (size&1) write_file(output_file,"\n",1);
            close_file(input_file,0);
            ++index;
        }
    }
    if (long_offsets) free(long_offsets);
    abort_phase="EmitOffsets";
    if (count_externals(table)) {
        seek_file(output_file,72,SEEK_SET);
        for (member=1;member<=(unsigned short)members;++member)
            write_import_member_indexes(table,member);
        if (second_index) {
            seek_file(output_file,second_offset+64,SEEK_SET);
            for (member=1;member<=(unsigned short)members;++member)
                write_file(output_file,import_member_offsets+member,4);
            seek_file(output_file,4,SEEK_CUR);
            write_import_member_indexes(table,0);
        }
    }
    free(import_member_offsets);
    return 0;
}
