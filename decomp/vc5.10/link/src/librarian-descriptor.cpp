/* Emit the ordinary import-descriptor COFF archive member. */
#include <windows.h>
#include <string.h>
#include "librarian.h"
#include "files.h"
#include "memory.h"
#include "full-link-functions.h"
#include <recon.h>

extern "C" {
extern const char *default_phase;
extern BYTE file_access_statistics;
extern DWORD *librarian_ordinal_symbols;
/* Original47f358 is one zero-initialized IMAGE_IMPORT_DESCRIPTOR owner. */
IMAGE_IMPORT_DESCRIPTOR librarian_import_descriptor;
void __stdcall write_archive_header(int,const char *,DWORD,DWORD,DWORD,DWORD,IMAGE_ARCHIVE_MEMBER_HEADER *);
void __stdcall write_file_header(int,IMAGE_FILE_HEADER *,IMAGE_FILE_HEADER *);
void __stdcall write_optional_header(int,const IMAGE_OPTIONAL_HEADER *,WORD);
void __stdcall write_section_header(int,const IMAGE_SECTION_HEADER *,IMAGE_SECTION_HEADER *);
void __stdcall write_relocations(int,const IMAGE_RELOCATION *,DWORD);
}
typedef struct LibrarianBufferedPosition { DWORD reserved[3],position; } LibrarianBufferedPosition;
#define WRITE_MEMBER(data,bytes) do { \
    if(file_access_statistics) record_file_access(); \
    file=file_slots[output_file]; \
    if(file->flags & 8)mapped_write(file,(data),(bytes)); \
    else buffered_write(file,(data),(bytes)); \
} while(0)
#define TELL_MEMBER(result) do { \
    if(file_access_statistics) record_file_access(); \
    file=file_slots[output_file]; \
    (result)=(file->flags & 8)?file->file_size:((LibrarianBufferedPosition *)file->base)->position; \
} while(0)

#pragma intrinsic(strlen,strcpy,strcat,memset)
void RECON_THISCALL LibrarianContext::EmitImportDescriptor(const IMAGE_OPTIONAL_HEADER *optional_header)
{
    FileRecord *file;
    DWORD string_bytes,extension_bytes,headers,data_bytes,debug_bytes=0,position;
    BYTE *strings;
    char *next_string;
    WORD section_count=2;
    IMAGE_SECTION_HEADER section;
    IMAGE_RELOCATION relocation;
    IMAGE_SYMBOL symbol;
    DWORD codeview_signature=1;
    default_phase="EmitImportDescriptor";
    extension_bytes=strlen(owned44)+1;
    string_bytes=strlen(null_thunk_name)+state40+51;
    strings=(BYTE *)allocate_memory(string_bytes);
    *(DWORD *)strings=string_bytes;
    next_string=(char *)strings+4;
    if(debug_formats & 2) {
        section_count=3;
        debug_bytes=cached_name_length+47;
    }
    headers=(WORD)file_header.SizeOfOptionalHeader+section_count*40+20;
    data_bytes=extension_bytes+state40;
    if(data_bytes & 1)++data_bytes;
    data_bytes+=debug_bytes+20;
    TELL_MEMBER(position);
    librarian_ordinal_symbols[1]=position;
    write_archive_header(output_file,member_name,flag20,state2c,0,
        string_bytes+data_bytes+headers+174,NULL);
    file_header.NumberOfSections=section_count;
    file_header.PointerToSymbolTable=data_bytes+headers+30;
    file_header.NumberOfSymbols=8;
    write_file_header(output_file,&file_header,NULL);
    if(image || optional_header)
        write_optional_header(output_file,image?&image->optional_header:optional_header,
                              file_header.SizeOfOptionalHeader);
    file_header.SizeOfOptionalHeader=0;
    if(debug_formats & 2) {
        memset(&section,0,sizeof(section));
        strncpy((char *)section.Name,".debug$S",8);
        section.SizeOfRawData=debug_bytes;
        section.PointerToRawData=headers;
        section.Characteristics=0x42100040;
        write_section_header(output_file,&section,NULL);
    }
    memset(&section,0,sizeof(section));
    strncpy((char *)section.Name,".idata$2",8);
    section.SizeOfRawData=20;
    section.PointerToRawData=headers+debug_bytes;
    section.PointerToRelocations=section.PointerToRawData+20;
    section.NumberOfRelocations=3;
    section.Characteristics=0xc0300040;
    write_section_header(output_file,&section,NULL);
    strncpy((char *)section.Name,".idata$6",8);
    section.PointerToRawData+=section.SizeOfRawData+section.NumberOfRelocations*10;
    section.SizeOfRawData=extension_bytes+state40;
    if(section.SizeOfRawData & 1)++section.SizeOfRawData;
    section.NumberOfRelocations=0;
    section.Characteristics=0xc0200040;
    write_section_header(output_file,&section,NULL);
    if(debug_formats & 2) {
        WRITE_MEMBER(&codeview_signature,4);
        WRITE_MEMBER(&librarian_codeview_object,8);
        WRITE_MEMBER(&cached_name_length,1);
        WRITE_MEMBER(cached_member_name,cached_name_length);
        WRITE_MEMBER(&librarian_codeview_compile,34);
    }
    WRITE_MEMBER(&librarian_import_descriptor,20);
    relocation.VirtualAddress=12;
    relocation.SymbolTableIndex=3;
    relocation.Type=thunk->reference_types[1];
    write_relocations(output_file,&relocation,1);
    ++relocation.SymbolTableIndex;
    relocation.VirtualAddress=0;
    write_relocations(output_file,&relocation,1);
    ++relocation.SymbolTableIndex;
    relocation.VirtualAddress=16;
    write_relocations(output_file,&relocation,1);
    WRITE_MEMBER(owned3c,state40);
    WRITE_MEMBER(owned44,extension_bytes);
    if(extension_bytes+state40!=section.SizeOfRawData)WRITE_MEMBER("",1);
    memset(&symbol,0,sizeof(symbol));
    strncpy((char *)symbol.N.ShortName,"@comp.id",8);
    symbol.Value=0x21c87;
    symbol.SectionNumber=-1;
    symbol.StorageClass=IMAGE_SYM_CLASS_STATIC;
    WRITE_MEMBER(&symbol,18);
    memset(&symbol,0,sizeof(symbol));
    symbol.N.Name.Long=next_string-(char *)strings;
    strcpy(next_string,"__IMPORT_DESCRIPTOR_");
    strcat(next_string,owned3c);
    next_string+=state40+21;
    symbol.SectionNumber=section_count-1;
    symbol.StorageClass=IMAGE_SYM_CLASS_EXTERNAL;
    WRITE_MEMBER(&symbol,18);
    memset(&symbol,0,sizeof(symbol));
    strncpy((char *)symbol.N.ShortName,".idata$2",8);
    symbol.Value=0xc0000040;
    symbol.SectionNumber=section_count-1;
    symbol.StorageClass=IMAGE_SYM_CLASS_SECTION;
    WRITE_MEMBER(&symbol,18);
    memset(&symbol,0,sizeof(symbol));
    strncpy((char *)symbol.N.ShortName,".idata$6",8);
    symbol.SectionNumber=section_count;
    symbol.StorageClass=IMAGE_SYM_CLASS_STATIC;
    WRITE_MEMBER(&symbol,18);
    memset(&symbol,0,sizeof(symbol));
    strncpy((char *)symbol.N.ShortName,".idata$4",8);
    symbol.Value=0xc0000040;
    symbol.StorageClass=IMAGE_SYM_CLASS_SECTION;
    WRITE_MEMBER(&symbol,18);
    strncpy((char *)symbol.N.ShortName,".idata$5",8);
    WRITE_MEMBER(&symbol,18);
    memset(&symbol,0,sizeof(symbol));
    symbol.N.Name.Long=next_string-(char *)strings;
    strcpy(next_string,"__NULL_IMPORT_DESCRIPTOR");
    next_string+=25;
    symbol.StorageClass=IMAGE_SYM_CLASS_EXTERNAL;
    WRITE_MEMBER(&symbol,18);
    memset(&symbol,0,sizeof(symbol));
    symbol.N.Name.Long=next_string-(char *)strings;
    strcpy(next_string,null_thunk_name);
    symbol.StorageClass=IMAGE_SYM_CLASS_EXTERNAL;
    WRITE_MEMBER(&symbol,18);
    WRITE_MEMBER(strings,string_bytes);
    release_memory(strings);
    TELL_MEMBER(position);
    if(position & 1)WRITE_MEMBER("\n",1);
}

void RECON_THISCALL LibrarianContext::EmitNullImportDescriptor(void)
{
    FileRecord *file;
    BYTE *strings;
    DWORD headers,debug_bytes=0,position;
    WORD section_count=1;
    IMAGE_SECTION_HEADER section;
    IMAGE_SYMBOL symbol;
    DWORD codeview_signature=1;
    default_phase="EmitNullImportDescriptor";
    strings=(BYTE *)allocate_memory(29);
    *(DWORD *)strings=29;
    if(debug_formats & 2) {
        section_count=2;
        debug_bytes=cached_name_length+47;
    }
    headers=section_count*40+20;
    TELL_MEMBER(position);
    librarian_ordinal_symbols[2]=position;
    write_archive_header(output_file,member_name,flag20,state2c,0,
        headers+debug_bytes+20+65,NULL);
    file_header.NumberOfSections=section_count;
    file_header.PointerToSymbolTable=headers+debug_bytes+20;
    file_header.NumberOfSymbols=2;
    write_file_header(output_file,&file_header,NULL);
    if(debug_formats & 2) {
        memset(&section,0,sizeof(section));
        strncpy((char *)section.Name,".debug$S",8);
        section.SizeOfRawData=debug_bytes;
        section.PointerToRawData=headers;
        section.Characteristics=0x42100040;
        write_section_header(output_file,&section,NULL);
    }
    memset(&section,0,sizeof(section));
    strncpy((char *)section.Name,".idata$3",8);
    section.SizeOfRawData=20;
    section.PointerToRawData=headers+debug_bytes;
    section.Characteristics=0xc0300040;
    write_section_header(output_file,&section,NULL);
    if(debug_formats & 2) {
        WRITE_MEMBER(&codeview_signature,4);
        WRITE_MEMBER(&librarian_codeview_object,8);
        WRITE_MEMBER(&cached_name_length,1);
        WRITE_MEMBER(cached_member_name,cached_name_length);
        WRITE_MEMBER(&librarian_codeview_compile,34);
    }
    WRITE_MEMBER(&librarian_import_descriptor,20);
    memset(&symbol,0,sizeof(symbol));
    strncpy((char *)symbol.N.ShortName,"@comp.id",8);
    symbol.Value=0x21c87;
    symbol.SectionNumber=-1;
    symbol.StorageClass=IMAGE_SYM_CLASS_STATIC;
    WRITE_MEMBER(&symbol,18);
    memset(&symbol,0,sizeof(symbol));
    symbol.N.Name.Long=4;
    strcpy((char *)strings+4,"__NULL_IMPORT_DESCRIPTOR");
    symbol.SectionNumber=section_count;
    symbol.StorageClass=IMAGE_SYM_CLASS_EXTERNAL;
    WRITE_MEMBER(&symbol,18);
    WRITE_MEMBER(strings,29);
    release_memory(strings);
    TELL_MEMBER(position);
    if(position & 1)WRITE_MEMBER("\n",1);
}

void RECON_THISCALL LibrarianContext::EmitNullThunk(void)
{
    FileRecord *file;
    DWORD *zero;
    BYTE *strings;
    DWORD headers,debug_bytes=0,position,string_bytes;
    WORD section_count=2;
    IMAGE_SECTION_HEADER section;
    IMAGE_SYMBOL symbol;
    DWORD codeview_signature=1;
    default_phase="EmitNullThunk";
    zero=(DWORD *)zero_allocate(4);
    string_bytes=strlen(null_thunk_name)+5;
    strings=(BYTE *)allocate_memory(string_bytes);
    *(DWORD *)strings=string_bytes;
    if(debug_formats & 2) {
        section_count=3;
        debug_bytes=cached_name_length+47;
    }
    headers=section_count*40+20;
    TELL_MEMBER(position);
    librarian_ordinal_symbols[3]=position;
    write_archive_header(output_file,member_name,flag20,state2c,0,
        headers+debug_bytes+8+36+string_bytes,NULL);
    file_header.NumberOfSections=section_count;
    file_header.PointerToSymbolTable=headers+debug_bytes+8;
    file_header.NumberOfSymbols=2;
    write_file_header(output_file,&file_header,NULL);
    if(debug_formats & 2) {
        memset(&section,0,sizeof(section));
        strncpy((char *)section.Name,".debug$S",8);
        section.SizeOfRawData=debug_bytes;
        section.PointerToRawData=headers;
        section.Characteristics=0x42100040;
        write_section_header(output_file,&section,NULL);
    }
    memset(&section,0,sizeof(section));
    strncpy((char *)section.Name,".idata$5",8);
    section.SizeOfRawData=4;
    section.PointerToRawData=headers+debug_bytes;
    section.Characteristics=0xc0300040;
    write_section_header(output_file,&section,NULL);
    strncpy((char *)section.Name,".idata$4",8);
    section.PointerToRawData+=4;
    write_section_header(output_file,&section,NULL);
    if(debug_formats & 2) {
        WRITE_MEMBER(&codeview_signature,4);
        WRITE_MEMBER(&librarian_codeview_object,8);
        WRITE_MEMBER(&cached_name_length,1);
        WRITE_MEMBER(cached_member_name,cached_name_length);
        WRITE_MEMBER(&librarian_codeview_compile,34);
    }
    WRITE_MEMBER(zero,4);
    WRITE_MEMBER(zero,4);
    memset(&symbol,0,sizeof(symbol));
    strncpy((char *)symbol.N.ShortName,"@comp.id",8);
    symbol.Value=0x21c87;
    symbol.SectionNumber=-1;
    symbol.StorageClass=IMAGE_SYM_CLASS_STATIC;
    WRITE_MEMBER(&symbol,18);
    memset(&symbol,0,sizeof(symbol));
    symbol.N.Name.Long=4;
    strcpy((char *)strings+4,null_thunk_name);
    symbol.SectionNumber=section_count-1;
    symbol.StorageClass=IMAGE_SYM_CLASS_EXTERNAL;
    WRITE_MEMBER(&symbol,18);
    WRITE_MEMBER(strings,string_bytes);
    release_memory(strings);
    TELL_MEMBER(position);
    if(position & 1)WRITE_MEMBER("\n",1);
    release_memory(zero);
}
