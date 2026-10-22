/* Construct ordinary COFF objects within the generated import archive. */
#include "import-library.h"
#pragma intrinsic(strlen,strcpy,memset)
extern unsigned long symbol_record_size;

void write_import_descriptor(const ImportMachine *machine,Image *image)
{
    IMAGE_SECTION_HEADER section={0};
    IMAGE_IMPORT_DESCRIPTOR descriptor={0};
    IMAGE_RELOCATION relocation;
    IMAGE_SYMBOL symbol={0};
    unsigned short optional_size=image->file.SizeOfOptionalHeader;
    unsigned long extension_size=strlen(image->options.default_extension)+1;
    unsigned long strings_size=dll_basename_length+strlen(null_thunk_name)+47;
    unsigned long names_size=dll_basename_length+extension_size;
    unsigned long raw_size=20+((names_size+1)&~1UL);
    unsigned long raw_offset=optional_size+100;
    char *strings=malloc(strings_size),*cursor;
    if (!strings) out_of_memory();
    *(unsigned long *)strings=strings_size;
    cursor=strings+4;
    import_member_offsets[1]=tell_file(output_file);
    write_file(output_file,build_archive_header(import_member_name,import_member_long_name,
        symbol_record_size*7+raw_size+strings_size+raw_offset+30),sizeof(IMAGE_ARCHIVE_MEMBER_HEADER));
    image->file.NumberOfSections=2;
    image->file.PointerToSymbolTable=raw_size+raw_offset+30;
    image->file.NumberOfSymbols=7;
    write_coff_header(output_file,&image->file);
    write_optional_header(output_file,&image->optional,optional_size);
    image->file.SizeOfOptionalHeader=0;
    strncpy((char *)section.Name,idata_directory_defaults.name,8);
    section.SizeOfRawData=20;
    section.PointerToRawData=raw_offset;
    section.PointerToRelocations=optional_size+120;
    section.NumberOfRelocations=3;
    section.Characteristics=idata_directory_defaults.flags;
    write_section_header(output_file,&section);
    strncpy((char *)section.Name,".idata$6",8);
    section.VirtualAddress+=section.SizeOfRawData;
    section.PointerToRawData+=section.NumberOfRelocations*10+section.SizeOfRawData;
    section.SizeOfRawData=(names_size+1)&~1UL;
    section.NumberOfRelocations=0;
    section.Characteristics=0x48;
    write_section_header(output_file,&section);
    descriptor.TimeDateStamp=0;
    write_file(output_file,&descriptor,sizeof(descriptor));
    relocation.VirtualAddress=12;
    relocation.SymbolTableIndex=2;
    relocation.Type=machine->descriptor_relocation;
    write_coff_relocations(output_file,&relocation,1);
    relocation.VirtualAddress=0;
    ++relocation.SymbolTableIndex;
    write_coff_relocations(output_file,&relocation,1);
    relocation.VirtualAddress=16;
    ++relocation.SymbolTableIndex;
    write_coff_relocations(output_file,&relocation,1);
    write_file(output_file,image->options.library_image_name,dll_basename_length);
    write_file(output_file,image->options.default_extension,extension_size);
    if (names_size!=section.SizeOfRawData) write_file(output_file,"\n",1);
    symbol.N.Name.Long=cursor-strings;
    strcpy(cursor,image->options.library_image_name);
    strcpy(cursor+dll_basename_length,"_IMPORT_DESCRIPTOR");
    symbol.SectionNumber=1;
    symbol.StorageClass=IMAGE_SYM_CLASS_EXTERNAL;
    write_file(output_file,&symbol,symbol_record_size);
    memset(&symbol,0,sizeof(symbol));
    strncpy((char *)symbol.N.ShortName,idata_directory_defaults.name,8);
    symbol.Value=0x48;
    symbol.SectionNumber=1;
    symbol.StorageClass=IMAGE_SYM_CLASS_SECTION;
    write_file(output_file,&symbol,symbol_record_size);
    memset(&symbol,0,sizeof(symbol));
    strncpy((char *)symbol.N.ShortName,".idata$6",8);
    symbol.SectionNumber=2;
    symbol.StorageClass=IMAGE_SYM_CLASS_STATIC;
    write_file(output_file,&symbol,symbol_record_size);
    memset(&symbol,0,sizeof(symbol));
    strncpy((char *)symbol.N.ShortName,".idata$4",8);
    symbol.Value=0x48;
    symbol.StorageClass=IMAGE_SYM_CLASS_SECTION;
    write_file(output_file,&symbol,symbol_record_size);
    strncpy((char *)symbol.N.ShortName,".idata$5",8);
    write_file(output_file,&symbol,symbol_record_size);
    memset(&symbol,0,sizeof(symbol));
    cursor+=dll_basename_length+19;
    symbol.N.Name.Long=cursor-strings;
    strcpy(cursor,"NULL_IMPORT_DESCRIPTOR");
    symbol.StorageClass=IMAGE_SYM_CLASS_EXTERNAL;
    write_file(output_file,&symbol,symbol_record_size);
    memset(&symbol,0,sizeof(symbol));
    cursor+=23;
    symbol.N.Name.Long=cursor-strings;
    strcpy(cursor,null_thunk_name);
    symbol.StorageClass=IMAGE_SYM_CLASS_EXTERNAL;
    write_file(output_file,&symbol,symbol_record_size);
    write_file(output_file,strings,strings_size);
    free(strings);
    if (tell_file(output_file)&1) write_file(output_file,"\n",1);
}


void write_null_import_descriptor(Image *image)
{
    IMAGE_SECTION_HEADER section={0};
    IMAGE_IMPORT_DESCRIPTOR descriptor={0};
    IMAGE_SYMBOL symbol={0};
    char *strings=malloc(27);
    if (!strings) out_of_memory();
    *(unsigned long *)strings=27;
    import_member_offsets[2]=tell_file(output_file);
    write_file(output_file,build_archive_header(import_member_name,import_member_long_name,
               symbol_record_size+107),sizeof(IMAGE_ARCHIVE_MEMBER_HEADER));
    image->file.NumberOfSections=1;
    image->file.PointerToSymbolTable=80;
    image->file.NumberOfSymbols=1;
    write_coff_header(output_file,&image->file);
    strncpy((char *)section.Name,idata_directory_defaults.name,8);
    ++section.Name[strlen((char *)section.Name)-1];
    section.SizeOfRawData=20;
    section.PointerToRawData=60;
    section.Characteristics=idata_directory_defaults.flags;
    write_section_header(output_file,&section);
    write_file(output_file,&descriptor,sizeof(descriptor));
    symbol.N.Name.Long=4;
    strcpy(strings+4,"NULL_IMPORT_DESCRIPTOR");
    symbol.SectionNumber=1;
    symbol.StorageClass=IMAGE_SYM_CLASS_EXTERNAL;
    write_file(output_file,&symbol,symbol_record_size);
    write_file(output_file,strings,27);
    free(strings);
    if (tell_file(output_file)&1) write_file(output_file,"\n",1);
}


void write_null_thunk_member(Image *image)
{
    IMAGE_SECTION_HEADER section={0};
    IMAGE_SYMBOL symbol={0};
    unsigned long *zero=calloc(4,1);
    unsigned long strings_size;
    char *strings;
    if (!zero) out_of_memory();
    strings_size=strlen(null_thunk_name)+5;
    strings=malloc(strings_size);
    if (!strings) out_of_memory();
    *(unsigned long *)strings=strings_size;
    import_member_offsets[3]=tell_file(output_file);
    write_file(output_file,build_archive_header(import_member_name,import_member_long_name,
               symbol_record_size+strings_size+108),sizeof(IMAGE_ARCHIVE_MEMBER_HEADER));
    image->file.NumberOfSections=2;
    image->file.PointerToSymbolTable=108;
    image->file.NumberOfSymbols=1;
    write_coff_header(output_file,&image->file);
    strncpy((char *)section.Name,".idata$5",8);
    section.SizeOfRawData=4;
    section.PointerToRawData=100;
    section.Characteristics=0x48;
    write_section_header(output_file,&section);
    strncpy((char *)section.Name,".idata$4",8);
    section.PointerToRawData+=4;
    write_section_header(output_file,&section);
    write_file(output_file,zero,4);
    write_file(output_file,zero,4);
    symbol.N.Name.Long=4;
    strcpy(strings+4,null_thunk_name);
    symbol.SectionNumber=1;
    symbol.StorageClass=IMAGE_SYM_CLASS_EXTERNAL;
    write_file(output_file,&symbol,symbol_record_size);
    write_file(output_file,strings,strings_size);
    free(strings);
    if (tell_file(output_file)&1) write_file(output_file,"\n",1);
    free(zero);
}


void write_import_members(Image *image,const ImportMachine *machine)
{
    External **ordered=get_externals_by_name(image->symbols),*external;
    unsigned long count=count_externals(image->symbols),index;
    for (index=0;index<count;++index) {
        external=ordered[index];
        if ((external->flags&EXTERNAL_DEFINED) && !(external->flags&0x800))
            write_single_import_member(image,external,machine);
    }
}
