/* Build the COFF .exp object containing the PE export directory. */
#include "import-library.h"
#pragma intrinsic(strlen,strcpy,memcpy)
extern unsigned long symbol_record_size;
char *export_description,*export_directives;
unsigned long export_name_bytes;
/* The NT 3.1 header declares the three RVA fields as pointer-to-pointer.
 * A COFF file contains 32-bit relative addresses, not host pointers. */
typedef struct ExportDirectory {
    unsigned long Characteristics,TimeDateStamp;
    unsigned short MajorVersion,MinorVersion;
    unsigned long Name,Base,NumberOfFunctions,NumberOfNames;
    unsigned long AddressOfFunctions,AddressOfNames,AddressOfNameOrdinals;
} ExportDirectory;

void write_export_object(Image *image,unsigned long slots,unsigned long exports,
                         const ImportMachine *machine,unsigned char linked)
{
    ExportDirectory directory={0};
    IMAGE_SECTION_HEADER section={0};
    IMAGE_RELOCATION relocation;
    IMAGE_SYMBOL symbol={0};
    SymbolTable *table=image->symbols;
    unsigned short sections=1,relocations=(unsigned short)(exports*2+4);
    unsigned long allocation,description_size=0,directives_size=0,headers_size;
    unsigned long data_size,string_size,symbols=(unsigned short)(exports+2),name_cursor;
    unsigned char *base;
    unsigned long *names;
    unsigned short *name_ordinals;
    char *cursor;
    allocation=strlen(image->options.default_extension)+1+exports*7+dll_basename_length+
        slots*4+sizeof(directory)+table->strings.length+export_name_bytes+export_forwarder_bytes;
    if (allocation&0xfff) allocation=(allocation&~0xfffUL)+0x1000;
    base=calloc(allocation,1);
    if (!base) out_of_memory();
    if (export_description) {
        sections=2;
        description_size=strlen(export_description);
    }
    if (export_directives) {
        ++sections;
        append_export_directive("");
        directives_size=strlen(export_directives)+1;
    }
    directory.TimeDateStamp=image->file.TimeDateStamp;
    directory.Base=first_export_ordinal;
    directory.NumberOfFunctions=slots;
    directory.NumberOfNames=exports;
    directory.AddressOfFunctions=sizeof(directory);
    names=(unsigned long *)(base+sizeof(directory)+slots*4);
    directory.AddressOfNames=(unsigned char *)names-base;
    name_ordinals=(unsigned short *)(names+exports);
    directory.AddressOfNameOrdinals=(unsigned char *)name_ordinals-base;
    cursor=(char *)(name_ordinals+exports);
    directory.Name=(unsigned char *)cursor-base;
    strcpy(cursor,image->options.library_image_name);
    cursor+=dll_basename_length;
    strcpy(cursor,image->options.default_extension);
    cursor+=strlen(image->options.default_extension)+1;
    fill_export_tables(table,export_ordinals,base,&names,&name_ordinals,&cursor);
    if (linked) sort_export_name_table(base,directory.AddressOfNames,
                                      directory.AddressOfNameOrdinals,directory.NumberOfNames);
    intern_export_targets(table);
    string_size=table->strings.length;
    headers_size=sections*sizeof(IMAGE_SECTION_HEADER)+sizeof(IMAGE_FILE_HEADER);
    data_size=(unsigned char *)cursor-base;
    import_member_offsets[4]=tell_file(output_file);
    write_file(output_file,build_archive_header(import_member_name,import_member_long_name,
        data_size+headers_size+description_size+directives_size+symbols*symbol_record_size+
        relocations*sizeof(IMAGE_RELOCATION)+string_size),sizeof(IMAGE_ARCHIVE_MEMBER_HEADER));
    image->file.NumberOfSections=sections;
    image->file.PointerToSymbolTable=data_size+headers_size+description_size+directives_size+
                                    relocations*sizeof(IMAGE_RELOCATION);
    image->file.NumberOfSymbols=symbols;
    write_coff_header(output_file,&image->file);
    strncpy((char *)section.Name,".edata",8);
    section.SizeOfRawData=data_size;
    section.PointerToRawData=headers_size;
    section.PointerToRelocations=headers_size+data_size;
    section.NumberOfRelocations=relocations;
    section.Characteristics=0x40000040;
    write_section_header(output_file,&section);
    if (description_size) {
        strncpy((char *)section.Name,readonly_section_defaults.name,8);
        section.VirtualAddress+=section.SizeOfRawData;
        section.PointerToRawData+=section.SizeOfRawData+section.NumberOfRelocations*10;
        section.SizeOfRawData=description_size;
        section.PointerToRelocations=0;
        section.NumberOfRelocations=0;
        section.Characteristics=readonly_section_defaults.flags;
        write_section_header(output_file,&section);
    }
    if (directives_size) {
        strncpy((char *)section.Name,".drectve",8);
        section.VirtualAddress+=section.SizeOfRawData;
        section.PointerToRawData+=section.SizeOfRawData+section.NumberOfRelocations*10;
        section.SizeOfRawData=directives_size;
        section.PointerToRelocations=0;
        section.NumberOfRelocations=0;
        section.Characteristics=0x40000a40;
        write_section_header(output_file,&section);
    }
    memcpy(base,&directory,sizeof(directory));
    write_file(output_file,base,data_size);
    relocation.VirtualAddress=12;
    relocation.SymbolTableIndex=0;
    relocation.Type=machine->export_relocation;
    write_coff_relocations(output_file,&relocation,1);
    relocation.VirtualAddress=28;
    write_coff_relocations(output_file,&relocation,1);
    relocation.VirtualAddress=32;
    write_coff_relocations(output_file,&relocation,1);
    relocation.VirtualAddress=36;
    write_coff_relocations(output_file,&relocation,1);
    name_cursor=directory.AddressOfNames;
    write_export_relocations(table,directory.AddressOfFunctions,&name_cursor,machine->export_relocation);
    if (description_size) write_file(output_file,export_description,description_size);
    if (directives_size) write_file(output_file,export_directives,directives_size);
    strncpy((char *)symbol.N.ShortName,".edata",8);
    symbol.SectionNumber=1;
    symbol.StorageClass=IMAGE_SYM_CLASS_STATIC;
    write_file(output_file,&symbol,symbol_record_size);
    symbol.N.Name.Short=0;
    symbol.N.Name.Long=exports_table_symbol->symbol.N.Name.Long;
    symbol.StorageClass=IMAGE_SYM_CLASS_EXTERNAL;
    write_file(output_file,&symbol,symbol_record_size);
    write_export_symbols(table);
    emit_string_table(table);
    free(base);
    if (tell_file(output_file)&1) write_file(output_file,"\n",1);
}


void fill_export_tables(SymbolTable *table,unsigned char *ordinals,unsigned char *base,
                        unsigned long **names,unsigned short **name_ordinals,char **strings)
{
    External **ordered=get_externals_by_name(table),*external;
    unsigned long count=count_externals(table),index,ordinal;
    char *name,*suffix;
    for (index=0;index<count;++index) {
        external=ordered[index];
        if (!(external->flags&EXTERNAL_DEFINED) || (external->flags&0x800)) continue;
        if (external->symbol.N.Name.Short)
            name=strncpy(short_symbol_name,(char *)&external->symbol,8);
        else name=table->strings.data+external->symbol.N.Name.Long;
        if (import_only_export && *name!='?' && *name!='@') ++name;
        if (external->flags&0x10) {
            if (*name=='?' || *name=='_') ++name;
            name=_strdup(name);
            suffix=strchr(name,'@');
            if (suffix) *suffix=0;
        }
        **names=(unsigned char *)*strings-base;
        ++*names;
        strcpy(*strings,name);
        *strings+=strlen(name)+1;
        if (external->flags&0x10) free(name);
        if (external->flags&0x200) {
            external->value.final_value=(unsigned char *)*strings-base;
            strcpy(*strings,external->export_target);
            *strings+=strlen(external->export_target)+1;
        }
        if (external->symbol.Value) ordinal=external->symbol.Value-first_export_ordinal;
        else {
            ordinal=0;
            while (ordinals[ordinal]) ++ordinal;
            ordinals[ordinal]=1;
        }
        **name_ordinals=(unsigned short)ordinal;
        ++*name_ordinals;
        external->symbol.Value=ordinal;
    }
}


#pragma intrinsic(strcmp)

void sort_export_name_table(unsigned char *base,unsigned long name_offset,
                            unsigned long ordinal_offset,unsigned long count)
{
    unsigned long *names=(unsigned long *)(base+name_offset),i,j,name;
    unsigned short *ordinals=(unsigned short *)(base+ordinal_offset),ordinal;
    for (i=0;i+1<count;++i) {
        for (j=i+1;j<count;++j) {
            if (strcmp((char *)base+names[j],(char *)base+names[i])<0) {
                name=names[i]; names[i]=names[j]; names[j]=name;
                ordinal=ordinals[i]; ordinals[i]=ordinals[j]; ordinals[j]=ordinal;
            }
        }
    }
}


void write_export_relocations(SymbolTable *table,unsigned long addresses,
                              unsigned long *names,unsigned short type)
{
    External **ordered=get_externals_by_name(table),*external;
    unsigned long count=count_externals(table),index;
    unsigned short symbol=2;
    IMAGE_RELOCATION relocation;
    for (index=0;index<count;++index) {
        external=ordered[index];
        if (!(external->flags&EXTERNAL_DEFINED) || (external->flags&0x800)) continue;
        relocation.VirtualAddress=addresses+external->symbol.Value*4;
        relocation.SymbolTableIndex=symbol++;
        relocation.Type=type;
        write_coff_relocations(output_file,&relocation,1);
        relocation.VirtualAddress=*names;
        *names+=4;
        relocation.SymbolTableIndex=0;
        relocation.Type=type;
        write_coff_relocations(output_file,&relocation,1);
    }
}


unsigned long forwarder_symbol_serial;

void write_export_symbols(SymbolTable *table)
{
    External **ordered=get_externals_by_name(table),*external;
    unsigned long count=count_externals(table),index;
    IMAGE_SYMBOL symbol;
    Contribution forwarder_contribution;
    Group forwarder_group;
    Section forwarder_section;
    for (index=0;index<count;++index) {
        external=ordered[index];
        if (!(external->flags&EXTERNAL_DEFINED) || (external->flags&(0x800|EXTERNAL_EMITTED))) continue;
        symbol=external->symbol;
        if (!symbol.N.Name.Short) symbol.N.Name.Short=0;
        if (external->flags&0x200) {
            sprintf((char *)symbol.N.ShortName,"_F@%03X",forwarder_symbol_serial++);
            forwarder_section.index=1;
            forwarder_group.section=&forwarder_section;
            forwarder_group.first=forwarder_group.last=&forwarder_contribution;
            forwarder_contribution.group=&forwarder_group;
            external->owner.contribution=&forwarder_contribution;
        } else if (external->export_target) {
            if (strlen(external->export_target)>8) {
                symbol.N.Name.Short=0;
                symbol.N.Name.Long=intern_symbol_name(table,external->export_target);
            } else strncpy((char *)symbol.N.ShortName,external->export_target,8);
        }
        symbol.Value=external->value.final_value;
        if (external->owner.contribution) {
            if (external->flags&0x200) symbol.SectionNumber=1;
            else symbol.SectionNumber=external->owner.contribution->group->section->index;
        }
        write_coff_symbol(output_file,&symbol);
        external->flags |= EXTERNAL_EMITTED;
        ++symbol_count;
    }
}
