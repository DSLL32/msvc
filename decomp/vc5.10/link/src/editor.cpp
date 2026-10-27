/* PE/COFF editor: the ordinary driver, with independent bind/rebase options. */
#include <windows.h>
#include <imagehlp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "link.h"
#include "input.h"
#include "arguments.h"
#include "options.h"
#include "memory.h"
#include "full-link-functions.h"
#include <recon.h>

extern "C" {
LinkContext *editor_context;
IMAGE_SECTION_HEADER *editor_section_headers;
extern int output_file;
extern FILE *map_output;
extern BYTE file_access_statistics,logo_pending,reproducible_build;
extern const char *current_object_name;
extern char *current_module_strings;
void *__stdcall allocate_link_memory(unsigned,unsigned);
void __cdecl close_files(void);
void __cdecl cleanup_pdb(void);
void __cdecl editor_usage(void) { __asm { int 3 } }
void __stdcall convert_editor_omf_argument(InputArgument *argument)
{
    char *temporary=_tempnam(NULL,"lnk");
    if(!temporary)link_fatal(NULL,1104,"TEMPFILE");
    argument->original_name=duplicate_argument(temporary);
    close_argument_file(open_argument_file(argument->original_name,0x8302,0x180),1);
    free(temporary);
    if(!convert_omf_file(argument->name,argument->original_name))
        link_fatal(argument->name,1123);
    link_warning(argument->name,4033);
}
void __cdecl check_editor_input_files(void)
{
    InputArgument *argument=ordinary_files.first;
    WORD index;
    BYTE first;
    unsigned count;
    FileRecord *record;
    for(index=0;index<ordinary_files.count;++index,argument=argument->next) {
        current_input_file=open_argument_file(argument->name,0x8000,0);
        if(file_access_statistics)record_file_access();
        record=file_slots[current_input_file];
        count=record->flags&8?mapped_read(record,&first,1):buffered_read(record,&first,1);
        if(count!=1)link_fatal(argument->name,1136);
        close_argument_file(current_input_file,0);
        if(first==0x80)convert_editor_omf_argument(argument);
    }
}
#pragma intrinsic(strcmp)
void __stdcall prepare_editor_argument(InputArgument *argument)
{
    close_argument_file(open_argument_file(argument->name,0x8002,0),1);
    if(argument->original_name && strcmp(argument->original_name,argument->name)) {
        close_argument_file(open_argument_file(argument->original_name,0x8002,0),1);
        if(!CopyFileA(argument->original_name,argument->name,FALSE))
            link_fatal(argument->name,1148,argument->original_name);
    }
}
BYTE __stdcall is_mppc_editor_file(const char *) { __asm { int 3 } return 0; }
BYTE editor_xoff;
#pragma intrinsic(strlen)
void __stdcall parse_editor_sizes(const char *text,DWORD *reserve,DWORD *commit)
{
    DWORD first,second;
    char suffix[256];
    int kind=0,count;
#ifdef __linux__
    if(strlen(text)>=sizeof(suffix)) { __asm { int 3 } }
#endif
    if(*text==',') {
        count=sscanf(text,",%li%s",&second,suffix);
#ifdef __linux__
        /* The original ignores this conversion count and can consume an
         * unwritten commit local. Only defined numeric input is supported. */
        if(count<1) { __asm { int 3 } }
#endif
        kind=2;
    } else if(*text) {
        count=sscanf(text,"%li,%li%s",&first,&second,suffix);
        if(count==1)kind=1;
        else if(count==2)kind=3;
    }
    switch(kind) {
    case 3:
        *reserve=(first&3)?(first&~3UL)+4:first;
        /* fall through */
    case 2:
        *commit=(second&3)?(second&~3UL)+4:second;
        break;
    case 1:
        *reserve=(first&3)?(first&~3UL)+4:first;
        break;
    default:link_fatal(NULL,1117,text);break;
    }
}
void __cdecl link_system_error(const char *,unsigned,...);
char *__stdcall get_input_section_name(IMAGE_SECTION_HEADER *,const char *);
void __stdcall write_section_header(int,const IMAGE_SECTION_HEADER *,IMAGE_SECTION_HEADER *);
#pragma intrinsic(strlen,strcmp,strcpy,memcpy)
void __stdcall parse_editor_section(const char *text,char **old_result,char **new_result,
    const char *filename,DWORD coff_offset)
{
    const char *start=text,*cursor=text;
    char *old_name,*new_name;
    LONG length;
    DWORD set=0,clear=0,*bits=&set,error,string_offset;
    WORD index;
    BYTE found=0;
    IMAGE_SECTION_HEADER *section;
    while(*cursor && *cursor!='=' && *cursor!=',')++cursor;
    length=cursor-start;
    if(!length)link_system_error(NULL,1137,text);
    old_name=(char *)(length>8?allocate_memory(length+1):zero_allocate(9));
    strncpy(old_name,start,length);old_name[length]=0;
    new_name=NULL;
    if(*cursor=='=') {
        start=++cursor;
        while(*cursor && *cursor!='=' && *cursor!=',')++cursor;
        length=cursor-start;
        if(!length)link_system_error(NULL,1137,text);
        new_name=(char *)(length>8?allocate_memory(length+1):zero_allocate(9));
        strncpy(new_name,start,length);new_name[length]=0;
        if(length>8 && strlen(new_name)>strlen(old_name))link_system_error(NULL,1137,text);
    }
    if(*cursor==',')++cursor;
    else if(*cursor)link_system_error(NULL,1137,text);
    for(;*cursor;++cursor) {
        switch(*cursor) {
        case 'c':*bits|=IMAGE_SCN_CNT_CODE;bits=&set;break;
        case 'i':*bits|=IMAGE_SCN_CNT_INITIALIZED_DATA;bits=&set;break;
        case 'u':*bits|=IMAGE_SCN_CNT_UNINITIALIZED_DATA;bits=&set;break;
        case 'd':*bits|=IMAGE_SCN_MEM_DISCARDABLE;bits=&set;break;
        case 'e':*bits|=IMAGE_SCN_MEM_EXECUTE;bits=&set;break;
        case 'r':*bits|=IMAGE_SCN_MEM_READ;bits=&set;break;
        case 's':*bits|=IMAGE_SCN_MEM_SHARED;bits=&set;break;
        case 'w':*bits|=IMAGE_SCN_MEM_WRITE;bits=&set;break;
        case 'o':*bits|=0x200;bits=&set;break;
        case 'm':*bits|=0x800;bits=&set;break;
        case 'a':
            *bits&=~0x700008;clear|=0x700008;
            if(bits==&clear) {bits=&set;break;}
            switch(*++cursor) {
            case '1':*bits|=0x100000;break;
            case '2':*bits|=0x200000;break;
            case '4':*bits|=0x300000;break;
            case '8':*bits|=0x400000;break;
            case 'p':*bits|=0x500000;break;
            case 't':*bits|=0x600000;break;
            case 's':*bits|=0x700000;break;
            case 'x':*bits|=8;break;
            default:link_system_error(NULL,1137,text);*bits|=0x100000;break;
            }
            bits=&set;break;
        case 'k':*(bits==&set?&clear:&set)|=0x4000000;bits=&set;break;
        case 'p':*(bits==&set?&clear:&set)|=0x8000000;bits=&set;break;
        default:link_system_error(NULL,1137,text);
        case '!':case 'n':
            if(!cursor[1])link_system_error(NULL,1137,text);
            bits=&clear;break;
        }
    }
    for(index=1;index<=editor_context->file_header.NumberOfSections;++index) {
        section=editor_section_headers+index;
        if(strcmp(old_name,get_input_section_name(section,current_module_strings)))continue;
        found=1;
        if(new_name) {
            if(strlen(new_name)>8) {
#ifdef __linux__
                /* Long COFF names require the real loaded string-table owner. */
                if(!current_module_strings) { __asm { int 3 } }
#endif
                sscanf((char *)section->Name+1,"%ld",&string_offset);
                strcpy(current_module_strings+string_offset,new_name);
            } else memcpy(section->Name,new_name,8);
        }
        section->Characteristics&=~clear;
        section->Characteristics|=set;
        error=0;seek_file(output_file,coff_offset+editor_context->file_header.SizeOfOptionalHeader+
            index*sizeof(IMAGE_SECTION_HEADER)-sizeof(IMAGE_FILE_HEADER),FILE_BEGIN,&error);
        write_section_header(output_file,section,NULL);
    }
    if(!found)link_warning(filename,4039,old_name);
    *old_result=old_name;*new_result=new_name;
}

extern const char *default_phase;
extern char short_symbol_name[9];
IMAGE_SYMBOL *__stdcall read_module_symbols(DWORD,DWORD,BYTE);
void __stdcall release_module_symbols(IMAGE_SYMBOL *);
const char *__stdcall get_symbol_name(StringNode *,const void *,WORD *,BYTE *);
void __stdcall rename_editor_symbols(LinkContext *context,const char *old_name,const char *new_name)
{
    IMAGE_SYMBOL *symbols,*symbol;
    DWORD index,error=0,bytes;
    WORD kind;
    const char *name;
    FileRecord *record;
    if((context->file_header.Characteristics&IMAGE_FILE_EXECUTABLE_IMAGE) ||
        !context->file_header.PointerToSymbolTable || !context->file_header.NumberOfSymbols)return;
    default_phase="ReadSymbolTable";
    symbols=read_module_symbols(context->file_header.PointerToSymbolTable,
        context->file_header.NumberOfSymbols,1);
    for(index=0;index<context->file_header.NumberOfSymbols;index+=symbol->NumberOfAuxSymbols+1) {
        symbol=symbols+index;
        name=get_symbol_name((StringNode *)symbol,current_module_strings,&kind,NULL);
        if(kind==1)name=strncpy(short_symbol_name,name,8);
        if(!strcmp(name,old_name)) {
            if(strlen(new_name)<=8)strncpy((char *)symbol,new_name,8);
            else {
#ifdef __linux__
                if(!current_module_strings) { __asm { int 3 } }
#endif
                strcpy(current_module_strings+symbol->N.Name.Long,new_name);
            }
        }
    }
    bytes=context->file_header.NumberOfSymbols*sizeof(IMAGE_SYMBOL);
    seek_file(output_file,context->file_header.PointerToSymbolTable,FILE_BEGIN,&error);
    if(file_access_statistics)record_file_access();
    record=file_slots[output_file];
    if(record->flags&8)mapped_write(record,symbols,bytes);
    else buffered_write(record,symbols,bytes);
    release_module_symbols(symbols);
}
void __stdcall apply_editor_options(const char *filename,int file,DWORD coff_offset)
{
    InputArgument *argument=option_arguments.first;
    SwitchInfo *info;
    WORD index,value_index;
    DWORD major,minor,error;
    LONG stub_limit,position;
    BYTE zero=0;
    FileRecord *record;
    char *old_name,*new_name;
    const char *dot;
    const char *value;
#define EDITOR_OPTION_WRITE(source,length) do { \
    if(file_access_statistics)record_file_access(); \
    record=file_slots[file]; \
    if(record->flags&8)mapped_write(record,(source),(length)); \
    else buffered_write(record,(source),(length)); \
} while(0)
    if(!option_arguments.count)link_warning(NULL,4041);
    for(index=0;index<option_arguments.count;++index,argument=argument->next) {
        info=argument->switch_info;
        value_index=0;
        if(!strcmp(info->name,"?") || !_stricmp(info->name,"bind"))continue;
        if(!_stricmp(info->name,"heap")) {
            parse_editor_sizes(argument->name+5,&editor_context->optional_header.SizeOfHeapReserve,
                &editor_context->optional_header.SizeOfHeapCommit);
        } else if(!_stricmp(info->name,"largeaddressaware")) {
            editor_context->file_header.Characteristics|=0x20 /* LARGE_ADDRESS_AWARE: absent from the shipped VC5 header */;
            for(value_index=0;value_index<info->value_count;++value_index) {
                if(info->values[value_index].name)link_fatal(NULL,1117,argument->name);
                if(!_stricmp(info->values[value_index].text,"no"))
                    editor_context->file_header.Characteristics&=~0x20;
                else link_fatal(NULL,1117,argument->name);
            }
        } else if(!_stricmp(info->name,"nostub")) {
            stub_limit=(LONG)(coff_offset-4);
            if(editor_context->file_header.SizeOfOptionalHeader && stub_limit>=(LONG)sizeof(default_dos_image)) {
                error=0;seek_file(file,0,FILE_BEGIN,&error);
                EDITOR_OPTION_WRITE(&default_dos_image,sizeof(default_dos_image));
                for(position=sizeof(default_dos_image);position<stub_limit;++position)
                    EDITOR_OPTION_WRITE(&zero,1);
                error=0;seek_file(file,28,FILE_BEGIN,&error);
                EDITOR_OPTION_WRITE(&stub_limit,4);
            } else link_warning(filename,4061);
        } else if(!_stricmp(info->name,"osversion")) {
            if(!info->value_count || !*info->values[0].text)goto missing_value;
            if(info->values[0].name)link_fatal(NULL,1117,argument->name);
            value=info->values[0].text;
            minor=0;dot=strchr(value,'.');
            if(dot && (sscanf(dot+1,"%li",&minor)!=1 || minor>0xffff))goto invalid_value;
            if(sscanf(value,"%li",&major)!=1 || major>0xffff)goto invalid_value;
            editor_context->optional_header.MajorOperatingSystemVersion=(WORD)major;
            editor_context->optional_header.MinorOperatingSystemVersion=(WORD)minor;
            value_index=1;goto unused_values;
        } else if(!_stricmp(info->name,"rebase"))continue;
        else if(!_stricmp(info->name,"release")) {
            if(editor_context->file_header.SizeOfOptionalHeader)editor_context->optional_header.CheckSum=1;
        } else if(!_stricmp(info->name,"section")) {
            parse_editor_section(argument->name+8,&old_name,&new_name,filename,coff_offset);
            if(new_name)rename_editor_symbols(editor_context,old_name,new_name);
            release_memory(old_name);release_memory(new_name);
        } else if(!_stricmp(info->name,"stack")) {
            parse_editor_sizes(argument->name+6,&editor_context->optional_header.SizeOfStackReserve,
                &editor_context->optional_header.SizeOfStackCommit);
        } else if(!_stricmp(info->name,"subsystem")) {
            if(!info->value_count || !*info->values[0].text)goto missing_value;
            for(value_index=0;value_index<info->value_count;++value_index) {
                if(info->values[value_index].name)link_fatal(NULL,1117,argument->name);
                value=info->values[value_index].text;
                if(value_index==1) {
                    minor=0;dot=strchr(value,'.');
                    if(dot && (sscanf(dot+1,"%li",&minor)!=1 || minor>0xffff))goto invalid_value;
                    if(sscanf(value,"%li",&major)!=1 || major>0xffff)goto invalid_value;
                    editor_context->optional_header.MajorSubsystemVersion=(WORD)major;
                    editor_context->optional_header.MinorSubsystemVersion=(WORD)minor;
                } else if(value_index)link_fatal(NULL,1117,argument->name);
                else if(!_stricmp(value,"console"))editor_context->optional_header.Subsystem=3;
                else if(!_stricmp(value,"native"))editor_context->optional_header.Subsystem=1;
                else if(!_stricmp(value,"posix"))editor_context->optional_header.Subsystem=7;
                else if(!_stricmp(value,"windows"))editor_context->optional_header.Subsystem=2;
                else if(!_stricmp(value,"windowsce"))editor_context->optional_header.Subsystem=9;
                else link_fatal(NULL,1117,argument->name);
            }
        } else if(!_stricmp(info->name,"swaprun")) {
            if(!info->value_count || !*info->values[0].text)goto missing_value;
            for(value_index=0;value_index<info->value_count;++value_index) {
                if(info->values[value_index].name)link_fatal(NULL,1117,argument->name);
                value=info->values[value_index].text;
                if(!_stricmp(value,"cd"))editor_context->file_header.Characteristics|=0x400;
                else if(!_stricmp(value,"net"))editor_context->file_header.Characteristics|=0x800;
                else if(!_stricmp(value,"!cd"))editor_context->file_header.Characteristics&=~0x400;
                else if(!_stricmp(value,"!net"))editor_context->file_header.Characteristics&=~0x800;
                else link_fatal(NULL,1117,argument->name);
            }
        } else if(!_stricmp(info->name,"version")) {
            if(!info->value_count || !*info->values[0].text)goto missing_value;
            if(info->values[0].name)link_fatal(filename,1117,argument->name);
            value=info->values[0].text;
            minor=0;dot=strchr(value,'.');
            if(dot && (sscanf(dot+1,"%li",&minor)!=1 || minor>0xffff))goto invalid_value;
            if(sscanf(value,"%li",&major)!=1 || major>0xffff)goto invalid_value;
            editor_context->optional_header.MajorImageVersion=(WORD)major;
            editor_context->optional_header.MinorImageVersion=(WORD)minor;
            value_index=1;goto unused_values;
        } else if(!_stricmp(info->name,"ws")) {
            if(!info->value_count || !*info->values[0].text)goto missing_value;
            for(value_index=0;value_index<info->value_count;++value_index) {
                if(info->values[value_index].name)link_fatal(NULL,1117,argument->name);
                value=info->values[value_index].text;
                if(!_stricmp(value,"aggressive"))editor_context->file_header.Characteristics|=0x10;
                else if(!_stricmp(value,"!aggressive"))editor_context->file_header.Characteristics&=~0x10;
                else link_fatal(NULL,1117,argument->name);
            }
        } else if(!_stricmp(info->name,"xoff")) {
            editor_xoff=1;
            goto unused_values;
        } else link_warning(NULL,4044,argument->name);
        continue;
missing_value:
        link_fatal(NULL,1146,argument->name);
invalid_value:
        link_fatal(filename,1147,argument->name);
        value_index=1;
unused_values:
        if(value_index<info->value_count)link_warning(NULL,4056,argument->name);
    }
#undef EDITOR_OPTION_WRITE
}

void __stdcall read_dumper_optional_header(int,IMAGE_OPTIONAL_HEADER *,DWORD);
void __stdcall write_optional_header(int,IMAGE_OPTIONAL_HEADER *,DWORD);
BYTE __stdcall is_supported_coff_file(const char *,const IMAGE_FILE_HEADER *);
char *__stdcall read_module_string_table(const char *,DWORD,DWORD *);
void __stdcall release_module_strings(void *);
#pragma intrinsic(strcmp)
int __stdcall run_editor(int argc,char **argv)
{
    InputArgument *argument,**previous;
    SwitchInfo *info;
    FileRecord *record;
    FILE *base_file;
    WORD index,value_index,signature;
    DWORD error,coff_offset,section_offset,section_bytes,string_bytes,pe_signature;
    DWORD new_base,old_base,old_size,new_size,next_base,timestamp;
    const char *dll_path,*base_filename;
    BYTE going_down,is_import;
    IMAGE_DOS_HEADER dos;
    CoffImportHeader import_header;
    char basename[_MAX_FNAME];
#define EDITOR_READ(destination,length) do { \
    if(file_access_statistics)record_file_access(); \
    record=file_slots[output_file]; \
    if(record->flags&8)mapped_read(record,(destination),(length)); \
    else buffered_read(record,(destination),(length)); \
} while(0)
#define EDITOR_WRITE(source,length) do { \
    if(file_access_statistics)record_file_access(); \
    record=file_slots[output_file]; \
    if(record->flags&8)mapped_write(record,(source),(length)); \
    else buffered_write(record,(source),(length)); \
} while(0)
    if(argc<2)editor_usage();
    editor_context=(LinkContext *)allocate_link_memory(1,sizeof(LinkContext));
    editor_context->Initialize(0);
    parse_command_line(argc,argv,NULL,NULL);
    previous=&option_arguments.first;
    argument=option_arguments.first;
    for(index=0;index<option_arguments.count;++index) {
        if(!_stricmp(argument->name,"nologo")) {
            *previous=argument->next;
            logo_pending=0;
            release_memory(argument);
            --option_arguments.count;
            break;
        }
        previous=&argument->next;
        argument=*previous;
    }
    if(logo_pending)print_logo();
    argument=library_files.first;
    for(index=0;index<library_files.count;++index,argument=argument->next)
        link_warning(NULL,4047,argument->original_name);
    check_editor_input_files();
    argument=option_arguments.first;
    for(index=0;index<option_arguments.count;++index,argument=argument->next) {
        info=argument->switch_info=parse_switch(NULL,argument->name);
        if(!strcmp(info->name,"?"))editor_usage();
        if(!_stricmp(info->name,"bind")) {
            dll_path=NULL;
            for(value_index=0;value_index<info->value_count;++value_index) {
                if(info->values[value_index].name && !_stricmp(info->values[value_index].name,"path"))
                    dll_path=info->values[value_index].text;
                else link_fatal(NULL,1117,argument->name);
            }
            InputArgument *file=ordinary_files.first;
            for(WORD i=0;i<ordinary_files.count;++i,file=file->next) {
                prepare_editor_argument(file);
                BindImageEx(1,file->name,(char *)dll_path,NULL,NULL);
            }
        } else if(!_stricmp(info->name,"rebase")) {
            new_base=0x400000;
            base_filename=NULL;
            base_file=NULL;
            going_down=0;
            _tzset();
            timestamp=reproducible_build?(DWORD)-1:(DWORD)time(NULL);
            for(value_index=0;value_index<info->value_count;++value_index) {
                if(info->values[value_index].name) {
                    if(_stricmp(info->values[value_index].name,"base"))link_fatal(NULL,1117,argument->name);
                    else if(!switch_number(info,value_index,&new_base))link_fatal(NULL,1147,argument->name);
                } else if(!_stricmp(info->values[value_index].text,"basefile"))base_filename="coffbase.txt";
                else if(!_stricmp(info->values[value_index].text,"down"))going_down=1;
                else link_fatal(NULL,1117,argument->name);
            }
            if(base_filename) {
                base_file=fopen(base_filename,"wt");
                if(!base_file)link_fatal(NULL,1168,base_filename);
            }
            InputArgument *file=ordinary_files.first;
            for(WORD i=0;i<ordinary_files.count;++i,file=file->next) {
                if(is_mppc_editor_file(file->name))link_fatal(NULL,1549,file->name);
                prepare_editor_argument(file);
                next_base=new_base;
                if(!ReBaseImage(file->name,NULL,TRUE,TRUE,going_down,0,
                    &old_size,&old_base,&new_size,&next_base,timestamp)) {
                    if(GetLastError()==193)link_fatal(NULL,1174,file->name);
                    link_fatal(NULL,1175,file->name);
                }
                if(base_file) {
                    _splitpath(file->name,NULL,NULL,basename,NULL);
                    fprintf(base_file,"%s\t%8.8lx\n",basename,going_down?next_base:new_base);
                }
                new_base=next_base;
            }
            if(base_file) {fflush(base_file);fclose(base_file);}
        }
    }
    argument=ordinary_files.first;
    for(index=0;index<ordinary_files.count;++index,argument=argument->next) {
        prepare_editor_argument(argument);
        current_object_name=argument->name;
        current_input_file=output_file=open_argument_file(argument->name,0x8002,0);
        EDITOR_READ(&signature,2);
        error=0;seek_file(output_file,-2,FILE_CURRENT,&error);
        if(signature==IMAGE_DOS_SIGNATURE) {
            EDITOR_READ(&dos,sizeof(dos));
            error=0;seek_file(output_file,dos.e_lfanew,FILE_BEGIN,&error);
            EDITOR_READ(&pe_signature,4);
            if(editor_context->options.editor_require_pe) {
                if(pe_signature!=IMAGE_NT_SIGNATURE) {
                    fprintf(map_output,"\nPE signature not found\n");
                    close_argument_file(output_file,1);
                    continue;
                }
            }
        }
        if(file_access_statistics)record_file_access();
        record=file_slots[output_file];
        coff_offset=record->flags&8?record->file_size:((FileBufferHeader *)record->base)->cursor;
        read_coff_header(output_file,&editor_context->file_header,&import_header,&is_import);
        if(is_import)continue;
        if(!is_supported_coff_file(argument->original_name,&editor_context->file_header))continue;
        if(editor_context->file_header.SizeOfOptionalHeader)
            read_dumper_optional_header(output_file,&editor_context->optional_header,
                editor_context->file_header.SizeOfOptionalHeader);
        printf("\n");
        section_offset=coff_offset+sizeof(IMAGE_FILE_HEADER)+editor_context->file_header.SizeOfOptionalHeader;
        section_bytes=editor_context->file_header.NumberOfSections*sizeof(IMAGE_SECTION_HEADER);
        editor_section_headers=(IMAGE_SECTION_HEADER *)allocate_memory(section_bytes+sizeof(IMAGE_SECTION_HEADER));
        error=0;seek_file(output_file,section_offset,FILE_BEGIN,&error);
        EDITOR_READ(editor_section_headers+1,section_bytes);
        if(signature!=IMAGE_DOS_SIGNATURE)
            current_module_strings=read_module_string_table(argument->name,
                editor_context->file_header.PointerToSymbolTable+editor_context->file_header.NumberOfSymbols*18,&string_bytes);
        apply_editor_options(argument->name,output_file,coff_offset);
        if(signature!=IMAGE_DOS_SIGNATURE) {
            release_module_strings(current_module_strings);
            current_module_strings=NULL;
            _tzset();
            time((time_t *)&editor_context->file_header.TimeDateStamp);
        }
        error=0;seek_file(output_file,coff_offset,FILE_BEGIN,&error);
        EDITOR_WRITE(&editor_context->file_header,sizeof(IMAGE_FILE_HEADER));
        if(editor_context->file_header.SizeOfOptionalHeader)
            write_optional_header(output_file,&editor_context->optional_header,
                editor_context->file_header.SizeOfOptionalHeader);
        release_memory(editor_section_headers);
        if(signature==IMAGE_DOS_SIGNATURE)editor_context->UpdateChecksum();
        close_argument_file(output_file,1);
    }
    close_files();
    cleanup_pdb();
    return 0;
#undef EDITOR_WRITE
#undef EDITOR_READ
}
}

/* Both editor and linker use the same output-file and header-writer owners. */
void RECON_THISCALL LinkContext::UpdateChecksum(void)
{
    DWORD length=(DWORD)get_file_length(output_file),header_checksum,checksum;
    void *view=map_file_range(output_file,0,length);
    IMAGE_NT_HEADERS *headers;
    if(!view) {
        close_argument_file(output_file,1);
        if(MapFileAndCheckSumA((char *)current_object_name,&header_checksum,&checksum)!=CHECKSUM_SUCCESS)
            link_warning(NULL,4069);
        output_file=open_argument_file(current_object_name,0x8002,0x180);
        /* A failed fallback leaves checksum unwritten in the original. The
         * cold native API denies before this unqualified state is consumed. */
        optional_header.CheckSum=checksum;
        (this->*image_write_headers)(output_file);
    } else {
        headers=CheckSumMappedFile(view,length,&header_checksum,&checksum);
        if(!headers)link_warning(NULL,4069);
        else headers->OptionalHeader.CheckSum=checksum;
    }
}
