/* Archive linker members, symbol-name indexes, and optional long-name tables. */
#include "archive.h"
#include "machine.h"
#include "pass-one.h"
#include <recon.h>
extern unsigned read_coff_header(int,IMAGE_FILE_HEADER *);

void read_archive_index(Library *library,unsigned short flags,Image *image)
{
    PIMAGE_ARCHIVE_MEMBER_HEADER header;
    IMAGE_ARCHIVE_MEMBER_HEADER next_header;
    IMAGE_FILE_HEADER file_header;
    IMAGE_OPTIONAL_HEADER optional;
    unsigned long count,names_size,next_offset,index;
    char *name;
    header=read_archive_header();
    if (strncmp((char *)header->Name,"/               ",16)) {
        if (tool_kind!=2) warning_message(library->name,0x4a,"","");
        archive_data_offset=8;
        archive_member_size=0;
        return;
    }
    if (flags&1) dump_archive_header(NULL,*header,8);
    read_file(input_file,&count,4);
    library->symbol_count=count=read_big_endian_dword(&count);
    library->member_offsets=malloc(count*4+4);
    if (!library->member_offsets) out_of_memory();
    read_file(input_file,library->member_offsets,count*4);
    names_size=archive_member_size-tell_file(input_file)+68;
    library->symbol_name_buffer=malloc(names_size);
    if (!library->symbol_name_buffer) out_of_memory();
    read_file(input_file,library->symbol_name_buffer,names_size);
    if (flags&1) {
        printf("\n% 8lX public symbols\n\n",library->symbol_count);
        name=library->symbol_name_buffer;
        for (index=0;index<library->symbol_count;++index) {
            printf("% 8lX ",read_big_endian_dword(&library->member_offsets[index]));
            name+=printf("%s\n",name);
        }
    }
    next_offset=(archive_data_offset+archive_member_size+1)&~1UL;
    seek_file(input_file,next_offset,SEEK_SET);
    read_file(input_file,&next_header,sizeof(next_header));
    if (!strncmp((char *)next_header.Name,"/               ",16)) {
        header=read_archive_header();
        if (flags&2) dump_archive_header(NULL,*header,next_offset);
        library->search_flags |= 1;
        read_file(input_file,&count,4);
        free(library->member_offsets);
        library->member_offsets=malloc(count*4+4);
        if (!library->member_offsets) out_of_memory();
        read_file(input_file,library->member_offsets+1,count*4);
        read_file(input_file,&count,4);
        library->symbol_indices=malloc(count*2);
        if (!library->symbol_indices) out_of_memory();
        read_file(input_file,library->symbol_indices,count*2);
        read_file(input_file,library->symbol_name_buffer,names_size);
        if (flags&2) {
            printf("\n% 8lX offsets\n\n",count);
            for (index=1;index<=count;++index)
                printf("    %lX % 8lX\n",index,library->member_offsets[index]);
            printf("\n% 8lX public symbols\n\n",library->symbol_count);
            name=library->symbol_name_buffer;
            for (index=0;index<library->symbol_count;++index) {
                printf("% 8hX ",library->symbol_indices[index]);
                name+=printf("%s\n",name);
            }
        }
    }
    library->symbol_names=malloc(library->symbol_count*sizeof(*library->symbol_names));
    if (!library->symbol_names) out_of_memory();
    name=library->symbol_name_buffer;
    for (index=0;index<library->symbol_count;++index) {
        library->symbol_names[index]=name;
        while (*name++) {}
    }
    next_offset=(archive_data_offset+archive_member_size+1)&~1UL;
    seek_file(input_file,next_offset,SEEK_SET);
    read_file(input_file,&next_header,sizeof(next_header));
    if (!strncmp((char *)next_header.Name,"//              ",16)) {
        header=read_archive_header();
        if (flags&2) dump_archive_header(NULL,*header,next_offset);
        name=malloc(archive_member_size);
        if (!name) out_of_memory();
        read_file(input_file,name,archive_member_size);
        library->long_names=name;
    } else library->long_names=NULL;
    if (image->file.Machine) {
        next_offset=(archive_data_offset+archive_member_size+1)&~1UL;
        seek_file(input_file,next_offset,SEEK_SET);
        read_file(input_file,&next_header,sizeof(next_header));
        read_coff_header(input_file,&file_header);
        if (file_header.SizeOfOptionalHeader==224) {
            read_optional_header(input_file,&optional,file_header.SizeOfOptionalHeader);
            if (file_header.Machine) {
                validate_machine(library->name,file_header.Machine,image);
                if (optional.Subsystem && optional.Subsystem!=image->optional.Subsystem)
                    fatal_message(library->name,0x1c,"");
                if (optional.MajorLinkerVersion && optional.MajorLinkerVersion!=2)
                    warning_message(library->name,0x61,"","");
            }
        }
    }
}

/* Header storage is shared by successive archive-member reads. */
IMAGE_ARCHIVE_MEMBER_HEADER archive_header;
PIMAGE_ARCHIVE_MEMBER_HEADER read_archive_header(void)
{
    unsigned long offset=(archive_data_offset+archive_member_size+1)&~1UL;
    seek_file(input_file,offset,SEEK_SET);
    read_file(input_file,&archive_header,sizeof(archive_header));
    archive_data_offset=offset+sizeof(archive_header);
    sscanf((char *)archive_header.Size,"%ld",(long *)&archive_member_size);
    return &archive_header;
}

unsigned long read_big_endian_dword(const void *bytes)
{
    const unsigned char *value=bytes;
    return ((unsigned long)value[0]<<24)|((unsigned long)value[1]<<16)|
           ((unsigned long)value[2]<<8)|value[3];
}

char archive_member_name[17];
char *get_archive_member_name(Library *library,PIMAGE_ARCHIVE_MEMBER_HEADER header)
{
    char *end;
    strncpy(archive_member_name,(char *)header->Name,16);
    if (archive_member_name[0]=='/' && archive_member_name[1]!=' ' &&
        archive_member_name[1]!='/') {
        end=strchr(archive_member_name,' ');
        if (!end) return NULL;
        *end=0;
        return library->long_names+atoi(archive_member_name+1);
    }
    end=strchr(archive_member_name,archive_member_name[0]=='/'?' ':'/');
    if (!end) return NULL;
    *end=0;
    return archive_member_name;
}

unsigned long encode_big_endian_dword(const void *bytes)
{
    return read_big_endian_dword(bytes);
}
