/* Archive linker members and long-name bookkeeping for generated libraries. */
#include "import-library.h"
#pragma intrinsic(strlen)
extern char *def_filename;
unsigned char new_archive_index,import_member_long_name;
unsigned long second_index_offset;
char *import_member_name;

void write_import_archive_indexes(unsigned long members,SymbolTable *table)
{
    unsigned long swapped,end,count,length;
    unsigned short member=0;
    const char *name;
    seek_file(output_file,sizeof(IMAGE_ARCHIVE_MEMBER_HEADER),SEEK_CUR);
    swapped=encode_big_endian_dword(&members);
    write_file(output_file,&swapped,sizeof(swapped));
    seek_file(output_file,members*4,SEEK_CUR);
    while (member<import_member_base) write_import_member_names(table,++member);
    end=tell_file(output_file);
    seek_file(output_file,8,SEEK_SET);
    write_file(output_file,build_archive_header("",0,end-68),sizeof(IMAGE_ARCHIVE_MEMBER_HEADER));
    seek_file(output_file,end,SEEK_SET);
    if (end&1) write_file(output_file,"\n",1);
    new_archive_index=1;
    second_index_offset=tell_file(output_file);
    seek_file(output_file,sizeof(IMAGE_ARCHIVE_MEMBER_HEADER),SEEK_CUR);
    count=import_member_base;
    write_file(output_file,&count,sizeof(count));
    seek_file(output_file,count*4,SEEK_CUR);
    write_file(output_file,&members,sizeof(members));
    seek_file(output_file,members*2,SEEK_CUR);
    write_import_member_names(table,0);
    end=tell_file(output_file);
    seek_file(output_file,second_index_offset,SEEK_SET);
    write_file(output_file,build_archive_header("",0,end-second_index_offset-60),
               sizeof(IMAGE_ARCHIVE_MEMBER_HEADER));
    seek_file(output_file,end,SEEK_SET);
    if (end&1) write_file(output_file,"\n",1);
    import_member_name=def_filename;
    if (!*def_filename) import_member_name=export_image->options.library_image_name;
    name=import_member_name;
    length=strlen(name)+1;
    if ((long)(length-1)>15) {
        import_member_long_name=1;
        import_member_name="0";
        write_file(output_file,build_archive_header("/",0,length),sizeof(IMAGE_ARCHIVE_MEMBER_HEADER));
        write_file(output_file,name,length);
        if (length&1) write_file(output_file,"\n",1);
    }
}


void write_import_member_names(SymbolTable *table,unsigned short member)
{
    External **ordered=get_externals_by_name(table),*external;
    unsigned long count=count_externals(table),index;
    char *name;
    for (index=0;index<count;++index) {
        external=ordered[index];
        if (!(external->flags&0x4001) || (external->flags&0x2000)) continue;
        if (member && external->archive_member!=member) continue;
        if (external->symbol.N.Name.Short)
            name=strncpy(short_symbol_name,(char *)&external->symbol,8);
        else name=table->strings.data+external->symbol.N.Name.Long;
        write_file(output_file,name,strlen(name)+1);
    }
}


#include <time.h>
#pragma intrinsic(memcpy,memset)
IMAGE_ARCHIVE_MEMBER_HEADER generated_archive_header;

PIMAGE_ARCHIVE_MEMBER_HEADER build_archive_header(const char *name,unsigned char long_name,
                                                 unsigned long size)
{
    IMAGE_ARCHIVE_MEMBER_HEADER *header=&generated_archive_header;
    unsigned long length=strlen(name);
    char number[32];
    time_t stamp;
    if (long_name) {
        header->Name[0]='/';
        memcpy(header->Name+1,name,length);
    } else {
        memcpy(header->Name,name,length);
        header->Name[length]='/';
    }
    ++length;
    memset(header->Name+length,' ',sizeof(header->Name)-length);
    memcpy(header->Date,"0         0 ",sizeof(header->Date));
    memcpy(header->UserID,"    0 ",sizeof(header->UserID));
    memcpy(header->GroupID,"    0 ",sizeof(header->GroupID));
    memcpy(header->Mode,"      0 ",sizeof(header->Mode));
    memcpy(header->Size,"        ",8);
    header->Size[8]=header->Size[9]=0;
    header->EndHeader[0]=header->EndHeader[1]=0;
    _ultoa((unsigned long)time(&stamp),number,10);
    memcpy(header->Date,number,strlen(number));
    _ultoa(size,number,10);
    memcpy(header->Size,number,strlen(number));
    header->EndHeader[0]='`';
    header->EndHeader[1]='\n';
    return header;
}
