/* Construct ordinary COFF objects within the generated import archive. */
#include "import-library.h"
#pragma intrinsic(strlen,strcpy,memset)
extern unsigned long symbol_record_size;
extern unsigned long mapped_close_seek_position;
/* Instruction templates serialized into generated COFF members. LINK never
 * executes these target instructions. Relocations fill their import slots. */
const I386ImportJump import_jump_i386={0x25ff,0};
const unsigned long import_jump_i860[3]={0x68000000,0x301f0000,0};
const unsigned long import_jump_mips[5]={0x3c080000,0x8d080000,0,0x01000008,0};
const ImportThunkRelocation import_relocations_i386[1]={{2,3,6}};
const ImportThunkRelocation import_relocations_i860[2]={{0,4,0x28},{8,3,6}};
const ImportThunkRelocation import_relocations_mips[3]={{0,3,4},{0,0,0x25},{4,3,5}};

#pragma pack(1)
typedef struct ImportThunkDebug {
    unsigned long signature;
    unsigned short record_length,record_type;
    unsigned long parent,end,next,offset;
    unsigned short segment,length;
    unsigned char ordinal;
} ImportThunkDebug;
typedef struct ImportDebugEnd {
    unsigned short record_length,record_type;
} ImportDebugEnd;
typedef struct ImportSectionAuxiliary {
    unsigned long length;
    unsigned short relocations,lines;
    unsigned long checksum;
    short section;
    unsigned char selection,reserved;
    unsigned short old_mapped_close_position;
} ImportSectionAuxiliary;
#pragma pack()
/* CodeView S_THUNK32 followed by a separately written counted name. */
ImportThunkDebug import_thunk_debug={1,0,0x206,0,0,0,0,0,0,0};
const ImportDebugEnd import_debug_end={2,6};

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
    if (names_size!=section.SizeOfRawData) write_file(output_file,"",1);
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

void write_single_import_member(Image *image,External *external,const ImportMachine *machine)
{
    IMAGE_SECTION_HEADER section={0};
    IMAGE_RELOCATION relocation;
    IMAGE_SYMBOL symbol={0};
    ImportSectionAuxiliary auxiliary={0};
    const ImportThunkRelocation *recipe;
    const char *original,*name,*emitted;
    char *suffix,*strings,*cursor;
    unsigned long original_length,strings_size,extra_name,debug_size,hint_size,padding;
    unsigned long headers_size,raw_size,iat_rva,lookup_rva,debug_rva,zero=0;
    unsigned short sections=3,relocations=2,symbols,index;
    short symbol_bias;
    unsigned char debug_name_length;
    int function=!(external->flags&0x2008);
    int debug=function && (image->options.debug_types&2);
    if (external->symbol.N.Name.Short)
        original=strncpy(short_symbol_name,(char *)&external->symbol,8);
    else original=image->symbols->strings.data+external->symbol.N.Name.Long;
    name=original;
    if (external->flags&0x10) {
        name=_strdup(original);
        if (*name=='?' || *name=='_') ++name;
        suffix=strchr(name,'@');
        if (suffix) *suffix=0;
    }
    original_length=strlen(original);
    extra_name=original_length>8 ? original_length+1 : 0;
    hint_size=strlen(name)+1;
    emitted=name;
    if (import_only_export && *name!='?' && *name!='@') {
        --hint_size;
        ++emitted;
    }
    debug_size=debug ? hint_size+33 : 0;
    strings_size=extra_name+dll_basename_length+original_length+30;
    if (function) {
        sections=debug ? 5 : 4;
        symbol_bias=2;
        relocations=machine->jump_relocation_count+(debug ? 4 : 2);
    } else symbol_bias=(external->flags&0x2000) ? -1 : 0;
    symbols=symbol_bias+4;
    if (function && machine->helper_thunk) {
        ++symbols;
        strings_size+=dll_basename_length+15;
    }
    strings=malloc(strings_size);
    if (!strings) out_of_memory();
    memset(strings,0,strings_size);
    *(unsigned long *)strings=strings_size;
    headers_size=sections*sizeof(section)+sizeof(image->file);
    padding=hint_size&1;
    raw_size=hint_size+padding+10;
    if (function) raw_size+=machine->jump_size+debug_size;
    import_member_offsets[external->archive_member]=tell_file(output_file);
    write_file(output_file,build_archive_header(import_member_name,import_member_long_name,
        symbols*symbol_record_size+relocations*10+raw_size+headers_size+strings_size),
        sizeof(IMAGE_ARCHIVE_MEMBER_HEADER));
    image->file.NumberOfSections=sections;
    image->file.PointerToSymbolTable=raw_size+relocations*10+headers_size;
    image->file.NumberOfSymbols=symbols;
    write_coff_header(output_file,&image->file);
    section.PointerToRawData=headers_size;
    section.PointerToRelocations=headers_size;
    if (function) {
        strncpy((char *)section.Name,".text",8);
        section.SizeOfRawData=machine->jump_size;
        section.PointerToRelocations+=section.SizeOfRawData;
        section.NumberOfRelocations=machine->jump_relocation_count;
        section.Characteristics=0x1028;
        write_section_header(output_file,&section);
    }
    strncpy((char *)section.Name,".idata$6",8);
    section.VirtualAddress+=section.SizeOfRawData;
    section.PointerToRawData+=section.SizeOfRawData+section.NumberOfRelocations*10;
    section.SizeOfRawData=hint_size+padding+2;
    section.PointerToRelocations=section.PointerToRawData+section.SizeOfRawData;
    section.NumberOfRelocations=0;
    section.Characteristics=0x48;
    write_section_header(output_file,&section);
    strncpy((char *)section.Name,".idata$5",8);
    section.VirtualAddress+=section.SizeOfRawData;
    section.PointerToRawData+=section.SizeOfRawData;
    section.SizeOfRawData=4;
    section.PointerToRelocations=section.PointerToRawData+4;
    section.NumberOfRelocations=1;
    iat_rva=section.VirtualAddress;
    write_section_header(output_file,&section);
    strncpy((char *)section.Name,".idata$4",8);
    section.VirtualAddress+=4;
    section.PointerToRawData+=14;
    section.PointerToRelocations=section.PointerToRawData+4;
    lookup_rva=section.VirtualAddress;
    write_section_header(output_file,&section);
    if (debug) {
        strncpy((char *)section.Name,".debug$S",8);
        section.VirtualAddress+=4;
        section.PointerToRawData+=14;
        section.SizeOfRawData=debug_size;
        section.PointerToRelocations=section.PointerToRawData+debug_size;
        section.NumberOfRelocations=2;
        section.Characteristics=0x42000048;
        debug_rva=section.VirtualAddress;
        write_section_header(output_file,&section);
    }
    if (function) {
        write_file(output_file,machine->jump,machine->jump_size);
        for (index=0;index<machine->jump_relocation_count;++index) {
            recipe=machine->jump_relocations+index;
            relocation.VirtualAddress=recipe->offset;
            relocation.SymbolTableIndex=recipe->symbol+symbol_bias;
            relocation.Type=recipe->type;
            write_coff_relocations(output_file,&relocation,1);
        }
    }
    write_file(output_file,&external->symbol.Value,2);
    write_file(output_file,emitted,hint_size);
    if (padding) write_file(output_file,"",padding);
    write_file(output_file,&zero,4);
    relocation.VirtualAddress=iat_rva;
    relocation.SymbolTableIndex=symbol_bias+1;
    relocation.Type=machine->lookup_relocation;
    write_coff_relocations(output_file,&relocation,1);
    write_file(output_file,&zero,4);
    relocation.VirtualAddress=lookup_rva;
    write_coff_relocations(output_file,&relocation,1);
    if (debug) {
        debug_name_length=(unsigned char)(hint_size-1);
        import_thunk_debug.record_length=debug_name_length+24;
        write_file(output_file,&import_thunk_debug,sizeof(import_thunk_debug));
        write_file(output_file,&debug_name_length,1);
        write_file(output_file,emitted,debug_name_length);
        write_file(output_file,&import_debug_end,sizeof(import_debug_end));
        relocation.VirtualAddress=debug_rva+20;
        relocation.SymbolTableIndex=0;
        relocation.Type=machine->debug_offset_relocation;
        write_coff_relocations(output_file,&relocation,1);
        relocation.VirtualAddress=debug_rva+24;
        relocation.Type=machine->debug_section_relocation;
        write_coff_relocations(output_file,&relocation,1);
    }
    cursor=strings+4;
    if (function) {
        strncpy((char *)symbol.N.ShortName,".text",8);
        symbol.SectionNumber=1;
        symbol.StorageClass=IMAGE_SYM_CLASS_STATIC;
        symbol.NumberOfAuxSymbols=1;
        write_file(output_file,&symbol,symbol_record_size);
        auxiliary.length=machine->jump_size;
        auxiliary.relocations=1;
        auxiliary.section=1;
        auxiliary.selection=1;
        /* The target initializes the first 15 bytes only. Its final WORD is
         * leftover mapped-close seek state; see SOURCE-NOTES.txt. */
        auxiliary.old_mapped_close_position=(unsigned short)mapped_close_seek_position;
        write_file(output_file,&auxiliary,symbol_record_size);
    }
    if (!(external->flags&0x2000)) {
        memset(&symbol,0,sizeof(symbol));
        if (original_length<=8) strncpy((char *)symbol.N.ShortName,original,8);
        else {
            symbol.N.Name.Long=cursor-strings;
            strcpy(cursor,original);
            cursor+=original_length+1;
        }
        symbol.StorageClass=IMAGE_SYM_CLASS_EXTERNAL;
        symbol.SectionNumber=function ? 1 : 2;
        write_file(output_file,&symbol,symbol_record_size);
    }
    memset(&symbol,0,sizeof(symbol));
    strncpy((char *)symbol.N.ShortName,".idata$6",8);
    symbol.StorageClass=IMAGE_SYM_CLASS_STATIC;
    symbol.SectionNumber=function ? 2 : 1;
    write_file(output_file,&symbol,symbol_record_size);
    memset(&symbol,0,sizeof(symbol));
    symbol.N.Name.Long=cursor-strings;
    strcpy(cursor,image->options.library_image_name);
    strcpy(cursor+dll_basename_length,"_IMPORT_DESCRIPTOR");
    cursor+=dll_basename_length+19;
    symbol.StorageClass=IMAGE_SYM_CLASS_EXTERNAL;
    write_file(output_file,&symbol,symbol_record_size);
    memset(&symbol,0,sizeof(symbol));
    symbol.N.Name.Long=cursor-strings;
    strcpy(cursor,"__imp_");
    strcpy(cursor+6,original);
    cursor+=original_length+7;
    symbol.StorageClass=IMAGE_SYM_CLASS_EXTERNAL;
    symbol.SectionNumber=function ? 3 : 2;
    write_file(output_file,&symbol,symbol_record_size);
    if (function && machine->helper_thunk) {
        memset(&symbol,0,sizeof(symbol));
        symbol.N.Name.Long=cursor-strings;
        strcpy(cursor,image->options.library_image_name);
        strcpy(cursor+dll_basename_length,"_THUNK_ROUTINE");
        symbol.StorageClass=IMAGE_SYM_CLASS_EXTERNAL;
        write_file(output_file,&symbol,symbol_record_size);
    }
    write_file(output_file,strings,strings_size);
    free(strings);
    if (tell_file(output_file)&1) write_file(output_file,"\n",1);
}
