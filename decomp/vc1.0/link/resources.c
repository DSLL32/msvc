/* Embedded Windows resource converter and its input adapter. */
#include "resources.h"
#include <io.h>
#include <setjmp.h>
#include <time.h>
jmp_buf resource_abort;
unsigned char *resource_input_buffer,*resource_output_buffer;
unsigned char resource_copy_buffer[1024];
char *resource_input_name;
unsigned short resource_machine,resource_relocation;
int resource_debug,resource_verbose,resource_32bit;
#pragma intrinsic(strlen,strcpy,strcat,memcpy)

/* Original [00020fa0,000210d0). The CVTRES driver shares LINK diagnostics. */
void convert_resource_input(Argument *argument,unsigned short machine)
{
    char *arguments[6];
    char prefix[]="tmp";
    int file;
    arguments[0]="cvtres";
    switch (machine) {
    case IMAGE_FILE_MACHINE_I386: arguments[1]="-i386"; break;
    case 0x162:
    case 0x166: arguments[1]="-mips"; break;
    case 0x268: fatal_message(program_name,0x46,argument->text); break;
    default: fatal_message(program_name,0x12,""); break;
    }
    arguments[2]="-o";
    arguments[3]=_tempnam("\\",prefix);
    if (!arguments[3]) fatal_message(program_name,7,"TEMPFILE");
    file=open_file(arguments[3],0x8302,0x180);
    close_file(file,1);
    arguments[4]=argument->text;
    arguments[5]=NULL;
    if (!setjmp(resource_abort)) {
        convert_resources(5,arguments);
        argument->original=duplicate_string(arguments[3]);
    } else fatal_message(argument->text,0x1a,"");
}

/* Original [00042870,00042c10). Command driver retained inside LINK. */
int convert_resources(int argc,char **argv)
{
    unsigned long input_size,first,second;
    int input,output,status,index;
    unsigned char explicit_output=0;
    char *name,*output_name,*copy,*extension;
    if (argc==1) resource_usage(0);
    for (index=1;index<argc;++index) {
        name=argv[index];
        if (*name!='/' && *name!='-') resource_input_name=name;
        else {
            ++name;
            if (!_strcmpi(name,"o")) {
                output_name=argv[++index];
                explicit_output=1;
            } else if (!_strcmpi(name,"d")) resource_debug=1;
            else if (!_strcmpi(name,"v")) resource_verbose=1;
            else if (!_strcmpi(name,"i386")) {
                resource_machine=0x14c;
                resource_relocation=7;
            } else if (!_strcmpi(name,"mips")) {
                resource_machine=0x162;
                resource_relocation=0x22;
            } else if (!_strcmpi(name,"alpha")) {
                resource_machine=0x184;
                resource_relocation=0x10;
            } else resource_usage(1);
        }
    }
    if (!resource_input_name) resource_usage(1);
    if (resource_verbose || resource_debug) {
        printf("Microsoft (R) Windows CVTRES Version %d.%02d\n",3,20);
        printf("Copyright (C) Microsoft Corp. 1991-1992.  All rights reserved.\n\n");
    }
    input=_open(resource_input_name,0x8000);
    if (input==-1) {
        copy=resource_allocate((unsigned short)(strlen(resource_input_name)+5));
        strcpy(copy,resource_input_name);
        resource_input_name=copy;
        strcat(copy,".RES");
        input=_open(resource_input_name,0x8000);
        if (input==-1) fatal_message(program_name,0xa,resource_input_name);
    }
    input_size=resource_seek(input,0,SEEK_END);
    resource_seek(input,0,SEEK_SET);
    if (!explicit_output) {
        output_name=resource_allocate((unsigned short)(strlen(resource_input_name)+5));
        strcpy(output_name,resource_input_name);
        extension=output_name+strlen(output_name)-4;
        if (extension<output_name) extension=output_name;
        while (*extension && *extension!='.') ++extension;
        strcpy(extension,".OBJ");
    }
    output=_open(output_name,0x8301,0x180);
    if (output==-1) fatal_message(program_name,0xb,output_name);
    initialize_resource_buffers();
    resource_read(input,&first,4);
    resource_read(input,&second,4);
    resource_seek(input,0,SEEK_SET);
    resource_32bit=(first==0 && second==0x20);
    status=convert_resource_file(input,output,input_size);
    _close(input);
    _close(output);
    return status==0;
}

/* Original [00042cd0,00042d10). */
long resource_seek(int file,long position,int origin)
{
    long result=_lseek(file,position,origin);
    if (result==-1) fatal_message(program_name,9,0);
    return result;
}

/* Original [000449a0,00044a00). Scratch for converting Win16 resources. */
void initialize_resource_buffers(void)
{
    resource_input_buffer=GlobalAlloc(GMEM_FIXED,0x10000);
    resource_output_buffer=GlobalAlloc(GMEM_FIXED,0x10000);
    if (!resource_input_buffer) fatal_message(program_name,4,0);
    if (!resource_output_buffer) fatal_message(program_name,5,0);
}

/* Original [00042c90,00042cd0). */
unsigned short resource_read(int file,void *buffer,unsigned short count)
{
    if ((unsigned short)_read(file,buffer,count)!=count)
        fatal_message(resource_input_name,6,0);
    return 0;
}

/* Shared conversion state and the COFF records emitted by the embedded driver. */
unsigned char *resource_directory;
ResourceName *resource_strings;
ResourceEntry *resource_file_entries;
ResourceType *resource_id_types,*resource_named_types;
unsigned short resource_string_bytes,resource_name_count,resource_name_directories;
unsigned short resource_type_count,resource_id_type_count,resource_named_type_count;
unsigned short resource_count,resource_language_count;
unsigned long resource_padding;
const char *resource_type_names[18]={NULL,"CURSOR","BITMAP","ICON","MENU","DIALOG",
    "STRING","FONTDIR","FONT","ACCELERATOR","RCDATA","MESSAGETABLE","GROUP_CURSOR",
    NULL,"GROUP_ICON",NULL,"VERSION","DLGINCLUDE"};
IMAGE_FILE_HEADER resource_coff_header={0,2,0,0,1,0,0x100};
IMAGE_SYMBOL resource_section_symbol={{".rsrc$02"},0,2,0,3,0};
IMAGE_RELOCATION resource_coff_relocation={0,0,6};
IMAGE_SECTION_HEADER resource_directory_section={".rsrc$01",0,0,0,0,0,0,0,0,0x40};
IMAGE_SECTION_HEADER resource_data_section={".rsrc$02",0,0,0,0,0,0,0,0,0x40};
/* These are native x86 layouts, including the variable-size name prefix. */
typedef char resource_name_layout[(sizeof(ResourceName)==20)?1:-1];
typedef char resource_record_layout[(sizeof(ResourceRecord)==24)?1:-1];
typedef char resource_entry_layout[(sizeof(ResourceEntry)==32)?1:-1];
typedef char resource_type_layout[(sizeof(ResourceType)==24)?1:-1];

void resource_error_prefix(void);
int resource_name_to_unicode(unsigned short *,const char *,unsigned long);

/* Original [000432d0,00043d50). Build a COFF object with two resource sections.
 * Directory and string offsets are WORD-sized in this version of CVTRES. */
int convert_resource_file(int input,int output,unsigned long size)
{
    ResourceName *type_name,*name,*string;
    ResourceRecord *record;
    ResourceEntry *entry,*last=NULL,*member;
    ResourceType *type;
    IMAGE_RESOURCE_DIRECTORY_ENTRY *root_entry,*name_entry,*language_entry;
    IMAGE_RESOURCE_DATA_ENTRY *data_entry;
    unsigned char *type_directory,*language_directory,*strings;
    unsigned short flags,root_size,name_start,language_start,data_start;
    unsigned short string_start,directory_size,padding,remaining;
    unsigned long data_size,header_size,position,stamp;
    char display[1024];
    int type_kind,name_kind;
    stamp=debug_display?debug_display:time(NULL);
    if (size) do {
        if (!resource_32bit) {
            type_name=read_resource_name(input);
            name=read_resource_name(input);
            resource_read(input,&flags,2);
            resource_read(input,&data_size,4);
            record=resource_allocate(sizeof(*record));
            record->header_size=0;
            record->size=data_size;
        } else {
            resource_read(input,&data_size,4);
            resource_read(input,&header_size,4);
            if (!data_size) {
                position=resource_seek(input,header_size-8,SEEK_CUR);
                continue;
            }
            record=resource_allocate(sizeof(*record));
            record->header_size=header_size;
            record->size=data_size;
            type_name=read_resource_name(input);
            name=read_resource_name(input);
            position=resource_seek(input,0,SEEK_CUR);
            while (position&3) position=resource_seek(input,1,SEEK_CUR);
            resource_read(input,&record->data_version,16);
        }
        if (resource_debug) {
            if (type_name->kind==1) {
                resource_name_to_ansi(display,type_name->text,type_name->length+1);
                printf("Adding resource - Type:%s, ",display);
            } else if (type_name->value.id<=17)
                printf("Adding resource - Type:%s, ",resource_type_names[type_name->value.id]);
            else printf("Adding resource - Type:%ld, ",(unsigned long)type_name->value.id);
            if (name->kind==1) {
                resource_name_to_ansi(display,name->text,name->length+1);
                printf("Name:%s, ",display);
            } else printf("Name:0x%4.4x, ",name->value.id);
            printf("Language:%x, Flags:%x, Size:%ld\n",record->language,record->flags,record->size);
        }
        entry=insert_resource(type_name,name,record);
        entry->position=resource_seek(input,0,SEEK_CUR);
        entry->sequence=resource_count++;
        if (!last) resource_file_entries=entry;
        else last->file_next=entry;
        last=entry;
        position=resource_seek(input,record->size,SEEK_CUR);
        if (resource_32bit)
            while (position&3) position=resource_seek(input,1,SEEK_CUR);
    } while (position<size);

    root_size=(unsigned short)(16+resource_type_count*8);
    name_start=(unsigned short)(root_size+(unsigned short)(resource_type_count*2+resource_name_count)*8);
    language_start=(unsigned short)(name_start+(unsigned short)(resource_name_directories*2+resource_language_count)*8);
    data_start=language_start;
    string_start=(unsigned short)(data_start+(unsigned short)(resource_count*16));
    directory_size=(unsigned short)(((unsigned short)(string_start+resource_string_bytes)+3)/4*4);
    resource_directory=resource_allocate((unsigned short)(directory_size+4));
    while ((unsigned long)resource_directory&3) ++resource_directory;
    set_resource_directory((IMAGE_RESOURCE_DIRECTORY *)resource_directory,0,0,stamp,
                           resource_named_type_count,resource_id_type_count);
    root_entry=(IMAGE_RESOURCE_DIRECTORY_ENTRY *)(resource_directory+16);
    type_directory=resource_directory+root_size;
    language_directory=resource_directory+name_start;
    data_entry=(IMAGE_RESOURCE_DATA_ENTRY *)(resource_directory+data_start);
    strings=resource_directory+string_start;
    for (type_kind=0;type_kind<2;++type_kind) {
        type=type_kind?resource_id_types:resource_named_types;
        for (;type;type=type->next) {
            root_entry->Name=type_kind?type->name->value.id:
                (type->name->string_offset+string_start)|IMAGE_RESOURCE_NAME_IS_STRING;
            root_entry->OffsetToData=(type_directory-resource_directory)|IMAGE_RESOURCE_DATA_IS_DIRECTORY;
            ++root_entry;
            set_resource_directory((IMAGE_RESOURCE_DIRECTORY *)type_directory,0,0,stamp,
                                   type->named_count,type->id_count);
            name_entry=(IMAGE_RESOURCE_DIRECTORY_ENTRY *)(type_directory+16);
            for (name_kind=0;name_kind<2;++name_kind) {
                member=name_kind?type->id_entries:type->named_entries;
                while (member) {
                    set_resource_directory((IMAGE_RESOURCE_DIRECTORY *)language_directory,
                                           0,0,stamp,0,member->language_count);
                    name_entry->Name=name_kind?member->name->value.id:
                        (member->name->string_offset+string_start)|IMAGE_RESOURCE_NAME_IS_STRING;
                    name_entry->OffsetToData=(language_directory-resource_directory)|IMAGE_RESOURCE_DATA_IS_DIRECTORY;
                    language_entry=(IMAGE_RESOURCE_DIRECTORY_ENTRY *)(language_directory+16);
                    remaining=member->language_count;
                    while (remaining--) {
                        language_entry->Name=member->record->language;
                        language_entry->OffsetToData=(unsigned char *)data_entry-resource_directory;
                        member->directory_offset=language_entry->OffsetToData;
                        ++language_entry;
                        set_resource_data(data_entry,0,0,0,0);
                        ++data_entry;
                        member=member->next;
                    }
                    language_directory=(unsigned char *)language_entry;
                    ++name_entry;
                }
            }
            type_directory=(unsigned char *)name_entry;
        }
    }
    for (string=resource_strings;string;string=string->value.next) {
        *(unsigned short *)strings=string->length;
        memcpy(strings,&string->length,string->encoded_size);
        strings+=string->encoded_size;
        while ((unsigned long)strings&1) ++strings;
    }
    resource_coff_header.TimeDateStamp=stamp;
    resource_coff_header.Machine=resource_machine;
    resource_directory_section.PointerToRawData=100;
    resource_directory_section.SizeOfRawData=directory_size;
    resource_directory_section.PointerToRelocations=100+directory_size;
    resource_directory_section.NumberOfRelocations=resource_count;
    resource_data_section.VirtualAddress=directory_size;
    resource_data_section.PointerToRawData=resource_directory_section.PointerToRelocations+resource_count*10;
    if (resource_data_section.PointerToRawData&2) resource_data_section.PointerToRawData+=2;
    resource_seek(output,resource_data_section.PointerToRawData,SEEK_SET);
    for (entry=resource_file_entries;entry;entry=entry->file_next) {
        position=resource_seek(input,entry->position,SEEK_SET);
        if (resource_32bit)
            while (position&3) position=resource_seek(input,1,SEEK_CUR);
        entry->position=resource_seek(output,0,SEEK_CUR)-resource_data_section.PointerToRawData;
        if (resource_32bit || entry->type->kind==1)
            copy_resource_data(input,output,entry->record->size);
        else convert_legacy_resource(input,output,entry->record,entry->type->value.id,entry->name->value.id);
        padding=(unsigned short)(-entry->record->size)&3;
        if (padding) resource_write(output,&resource_padding,padding);
        data_entry=(IMAGE_RESOURCE_DATA_ENTRY *)(resource_directory+entry->directory_offset);
        data_entry->Size=entry->record->size;
        data_entry->OffsetToData=entry->position;
    }
    resource_coff_header.PointerToSymbolTable=resource_seek(output,0,SEEK_CUR);
    resource_data_section.SizeOfRawData=resource_coff_header.PointerToSymbolTable-resource_data_section.PointerToRawData;
    resource_write(output,&resource_section_symbol,IMAGE_SIZEOF_SYMBOL);
    resource_write(output,&resource_padding,4);
    position=resource_seek(output,0,SEEK_CUR);
    _chsize(output,position);
    resource_seek(output,0,SEEK_SET);
    resource_write(output,&resource_coff_header,sizeof(resource_coff_header));
    resource_write(output,&resource_directory_section,sizeof(resource_directory_section));
    resource_write(output,&resource_data_section,sizeof(resource_data_section));
    resource_write(output,resource_directory,directory_size);
    resource_seek(output,resource_directory_section.PointerToRelocations,SEEK_SET);
    resource_coff_relocation.Type=resource_relocation;
    for (entry=resource_file_entries;entry;entry=entry->file_next) {
        resource_coff_relocation.VirtualAddress=entry->directory_offset;
        resource_write(output,&resource_coff_relocation,sizeof(resource_coff_relocation));
    }
    return 1;
}

/* Original [00042c10,00042c40). Resource tree nodes start cleared. */
void *resource_allocate(unsigned short size)
{
    void *allocation=calloc(1,size);
    if (!allocation) fatal_message(program_name,0xc,0);
    return allocation;
}

/* Original [00042e10,00042f90). Resource names are interned for directory use. */
ResourceName *read_resource_name(int file)
{
    char bytes[256],*cursor=bytes;
    unsigned short unicode[256],length;
    ResourceName *name,**link;
    resource_read(file,cursor,(unsigned short)(resource_32bit?2:1));
    if ((unsigned char)*cursor==0xff) {
        name=resource_allocate(sizeof(*name));
        name->kind=2;
        resource_read(file,&name->value.id,2);
        return name;
    }
    while (*cursor) {
        ++cursor;
        resource_read(file,cursor,(unsigned short)(resource_32bit?2:1));
    }
    cursor[1]=0;
    length=(unsigned short)(cursor-bytes);
    if (!resource_name_to_unicode(unicode,bytes,length))
        warning_message(resource_input_name,2,0,0);
    unicode[length]=0;
    link=&resource_strings;
    while (*link) {
        if (!wcscmp((*link)->text,unicode)) return *link;
        link=&(*link)->value.next;
    }
    name=resource_allocate((unsigned short)(length*2+20));
    name->kind=1;
    name->string_offset=resource_string_bytes;
    name->length=length;
    name->encoded_size=(unsigned short)(length*2+2);
    memcpy(name->text,unicode,length*2);
    resource_string_bytes+=name->encoded_size;
    name->value.next=NULL;
    *link=name;
    return name;
}

/* Original [00042f90,000432d0). Sorted type/name lists group each language set. */
ResourceEntry *insert_resource(ResourceName *type_name,ResourceName *name,ResourceRecord *record)
{
    ResourceType **type_link,*type;
    ResourceEntry **link=NULL,*existing=NULL,*entry,*candidate;
    unsigned char ordinal_type=(type_name->kind==2),ordinal_name=(name->kind==2);
    char display[1024];
    type_link=ordinal_type?&resource_id_types:&resource_named_types;
    while ((type=*type_link)!=NULL) {
        if (type->name->value.id==type_name->value.id) {
            link=ordinal_name?&type->id_entries:&type->named_entries;
            break;
        }
        if (ordinal_type) {
            if (type->name->value.id>type_name->value.id) break;
        } else if (wcscmp(type_name->text,type->name->text)<0) break;
        type_link=&type->next;
    }
    if (!link) {
        ++resource_type_count;
        if (ordinal_type) ++resource_id_type_count;
        else ++resource_named_type_count;
        type=resource_allocate(sizeof(*type));
        type->next=*type_link;
        *type_link=type;
        type->name=type_name;
        link=ordinal_name?&type->id_entries:&type->named_entries;
    }
    while ((candidate=*link)!=NULL) {
        if (ordinal_name) {
            if (candidate->name->value.id==name->value.id) { existing=candidate; break; }
            if (candidate->name->value.id>name->value.id) break;
        } else {
            if (!wcscmp(name->text,candidate->name->text)) { existing=candidate; break; }
            if (wcscmp(name->text,candidate->name->text)<0) break;
        }
        link=&candidate->next;
    }
    if (existing) {
        while ((candidate=*link)!=NULL) {
            if (candidate->record->language==record->language) {
                resource_error_prefix();
                if (type_name->kind==1) {
                    resource_name_to_ansi(display,type_name->text,type_name->length+1);
                    printf("Duplicate resource - Type:%s, ",display);
                } else if (type_name->value.id<=17)
                    printf("Duplicate resource - Type:%s, ",resource_type_names[type_name->value.id]);
                else printf("Duplicate resource - Type:%ld, ",(unsigned long)type_name->value.id);
                if (name->kind==1) {
                    resource_name_to_ansi(display,name->text,name->length+1);
                    printf("Name:%s, Language:0x%.4x\n",display,record->language);
                } else printf("Name:%ld, Language:0x%.4x\n",(unsigned long)name->value.id,record->language);
                longjmp(resource_abort,-1);
            }
            if (candidate->record->language>record->language) break;
            if (candidate!=existing && candidate->language_count) break;
            link=&candidate->next;
        }
    }
    entry=resource_allocate(sizeof(*entry));
    entry->next=*link;
    *link=entry;
    entry->name=name;
    entry->type=type_name;
    entry->record=record;
    if (existing) {
        ++resource_language_count;
        if (entry->next==existing) {
            entry->language_count=existing->language_count+1;
            existing->language_count=0;
        } else {
            ++existing->language_count;
            entry->language_count=0;
        }
    } else {
        ++resource_name_count;
        ++resource_language_count;
        ++resource_name_directories;
        entry->language_count=1;
        if (ordinal_name) ++type->id_count;
        else ++type->named_count;
    }
    return entry;
}

/* The NT-era ANSI table differs from identity only in 80h..9fh. The reverse
 * name conversion uses this same mapping; FFFF denotes an unsupported byte. */
const unsigned short resource_ansi_special[32]={
    0xffff,0xffff,0x201a,0x0192,0x201e,0x2026,0x2020,0x2021,
    0x02c6,0x2030,0x0160,0x2039,0x0152,0xffff,0xffff,0xffff,
    0xffff,0x2018,0x2019,0x201c,0x201d,0x2022,0x2013,0x2014,
    0x02dc,0x2122,0x0161,0x203a,0x0153,0xffff,0xffff,0x0178
};

/* Original [00045130,00045180). Convert the requested character count. */
int resource_name_to_unicode(unsigned short *output,const char *input,unsigned long count)
{
    int valid=1;
    unsigned short character;
    while (count--) {
        character=(unsigned char)*input++;
        if (character>=0x80 && character<0xa0)
            character=resource_ansi_special[character-0x80];
        *output++=character;
        if (character==0xffff) valid=0;
    }
    return valid;
}

/* Original [00043d50,00043d90). */
void set_resource_directory(IMAGE_RESOURCE_DIRECTORY *directory,unsigned long characteristics,
                            unsigned long version,unsigned long stamp,
                            unsigned short named,unsigned short ids)
{
    directory->Characteristics=characteristics;
    directory->TimeDateStamp=stamp;
    directory->MajorVersion=(unsigned short)(version>>16);
    directory->MinorVersion=(unsigned short)version;
    directory->NumberOfNamedEntries=named;
    directory->NumberOfIdEntries=ids;
}

/* Original [00043d90,00043db0). */
void set_resource_data(IMAGE_RESOURCE_DATA_ENTRY *entry,unsigned long offset,
                       unsigned long size,unsigned long code_page,unsigned long reserved)
{
    entry->OffsetToData=offset;
    entry->Size=size;
    entry->CodePage=code_page;
    entry->Reserved=reserved;
}

/* Original [00042d80,00042e00). */
int copy_resource_data(int input,int output,unsigned long size)
{
    unsigned long copied=0;
    unsigned short count;
    while (size) {
        count=(unsigned short)(size>1024?1024:size);
        size-=count;
        if (resource_read(input,resource_copy_buffer,count)) {
            warning_message(program_name,1,0,0);
            return copied;
        }
        copied+=count;
        resource_write(output,resource_copy_buffer,count);
    }
    return copied;
}

/* Original [00042d30,00042d80). */
unsigned long resource_write(int file,const void *buffer,unsigned long count)
{
    if (count>0x10001) fatal_message(program_name,7,0);
    if (_write(file,buffer,count)!=count) fatal_message(program_name,8,0);
    return 0;
}
