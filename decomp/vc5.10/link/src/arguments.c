#include <windows.h>
#include <stdlib.h>
#include <mbstring.h>
#include <string.h>
#include <io.h>
#include <stdio.h>
#include "link.h"
#include "files.h"
#include "arguments.h"
#include "memory.h"
#include "options.h"
#include "../crt/crt-imports.h"
#include <recon.h>

BYTE input_prepass;
InputArguments input_files;
void __cdecl link_warning(const char *,unsigned,...);

void __stdcall collect_file_arguments(int argc,char **argv,
                                      const char *before,const char *after)
{
    unsigned index;
    InputArgument *argument;
    input_prepass = 1;
    parse_command_line(argc,argv,before,after);
    argument = input_files.first;
    input_prepass = 0;
    for (index=0; index<input_files.count; ++index) {
        argument->switch_info = parse_switch(NULL,argument->name);
        if (!argument->switch_info->value_count) link_warning(NULL,4105,argument->name);
        argument = argument->next;
    }
}
#pragma intrinsic(strlen,strcpy)
SwitchInfo *__stdcall parse_switch(const char *file,const char *text)
{
    const char *colon;
    char *storage,*value,*next,*equals;
    SwitchInfo *info;
    WORD count,index;
    colon=strchr(text,':');
    count=0;
    if (colon) {
        if (!colon[1]) link_fatal(file,1146,text);
        count=1;
        for (next=strchr(colon+1,',');next;next=strchr(next+1,',')) ++count;
    }
    info=(SwitchInfo *)allocate_memory(strlen(text)+9+8*count);
    storage=(char *)&info->values[count];
    strcpy(storage,text);
    info->name=storage;
    value=(char *)text;
    if (colon) {
        value=storage+(colon-text);
        *value++=0;
    }
    if (!*info->name) link_fatal(file,1117,text);
    for (index=0;index<count;++index) {
        next=strchr(value,',');
        if (next) *next++=0;
        else next=value+strlen(value);
        equals=strchr(value,'=');
        if (equals) {
            *equals=0;
            info->values[index].name=value;
            info->values[index].text=equals+1;
        } else {
            info->values[index].name=NULL;
            info->values[index].text=value;
        }
        value=next;
    }
    info->value_count=count;
    /* The adjacent reserved WORD is not initialized by this function. */
    return info;
}

InputArgument *argument_tail;
char *__stdcall duplicate_argument(const char *);
void __stdcall tokenize_arguments(char *);
void __stdcall release_memory(void *);
void __stdcall read_response_file(const char *);
void __stdcall parse_argument(const char *,unsigned char);

void __stdcall parse_command_line(int argc,char **argv,
                                  const char *before,const char *after)
{
    char *value,*copy;
    int index;
    if (before && (value=getenv(before))!=NULL) {
        copy=duplicate_argument(value);
        tokenize_arguments(copy);
        release_memory(copy);
    }
    argument_tail=NULL;
    for (index=1; index<argc; ++index) {
        if (argv[index][0]=='@') read_response_file(argv[index]+1);
        else parse_argument(argv[index],0);
    }
    if (after && (value=getenv(after))!=NULL) {
        copy=duplicate_argument(value);
        tokenize_arguments(copy);
        release_memory(copy);
    }
}
char *__stdcall duplicate_argument(const char *text)
{
    char *copy=_strdup(text);
    if (!copy) out_of_memory();
    return copy;
}
/* Original494888 records the expanded command line, independently of the
 * comment payload and the option/file linked lists. */
ByteBuffer parsed_command_line;
char *__stdcall next_directive_token(char *,BYTE *);
#pragma intrinsic(strlen)
void __stdcall tokenize_arguments(char *text)
{
    BYTE quoted;
    char *token=next_directive_token(text,&quoted);
    if(!token)return;
    do {
        if(!input_prepass) {
            if(quoted)append_bytes(&parsed_command_line,"\"",1);
            append_bytes(&parsed_command_line,token,strlen(token));
            if(quoted)append_bytes(&parsed_command_line,"\"",1);
            append_bytes(&parsed_command_line," ",1);
        }
        parse_argument(token,1);
        token=next_directive_token(NULL,&quoted);
    } while(token);
    if(!input_prepass)append_bytes(&parsed_command_line,"\n",1);
}
void __stdcall release_memory(void *memory)
{
    free(memory);
}
void __stdcall read_response_file(const char *name)
{
    FILE *file=fopen(name,"rt");
    unsigned capacity=0x4000,length;
    long position=0;
    char *buffer;
    if(!file)link_fatal(NULL,1104,name);
    buffer=(char *)allocate_memory(capacity);
    while(fgets(buffer,capacity,file)) {
        length=strlen(buffer);
        if(length>=capacity-1 && buffer[length-1]!='\n') {
            if(capacity>=0x20000)link_fatal(name,1170,capacity-1);
            if(fseek(file,position,SEEK_SET))link_fatal(name,1170,capacity-1);
            capacity+=capacity;
            buffer=(char *)reallocate_memory(buffer,capacity);
        } else {
            if(buffer[length-1]=='\n')buffer[length-1]=0;
            tokenize_arguments(buffer);
            position=ftell(file);
        }
    }
    release_memory(buffer);
    fflush(stdout);
    fclose(file);
}
InputArguments library_files, ordinary_files, link_files, option_arguments;
int argument_file;
int __stdcall is_libpath(const char *);
void __stdcall expand_wildcards(const char *);
char *__stdcall canonicalize_argument(const char *,const char *);
int __stdcall open_argument_file(const char *,int,int);
unsigned char __stdcall is_archive(const char *,int);
int __stdcall close_argument_file(int,int);
InputArgument *__stdcall append_argument(InputArguments *,const char *);

void __stdcall parse_argument(const char *text,unsigned char source)
{
    InputArguments *list;
    if (*text=='/' || *text=='-') {
        ++text;
        if (input_prepass!=(unsigned char)is_libpath(text)) return;
        if (input_prepass) list=&input_files;
        else {
            list=&option_arguments;
            if (*text=='?' && text[1]) ++text;
        }
        if (source) text=duplicate_argument(text);
        append_argument(list,text);
        return;
    }
    if (input_prepass) return;
    if (_mbspbrk((const unsigned char *)text,(const unsigned char *)"?*")) {
        expand_wildcards(text);
        return;
    }
    text=canonicalize_argument(text,selected_tool_kind==1?".obj":NULL);
    if (selected_tool_kind==1 || selected_tool_kind==3) list=&link_files;
    else {
        argument_file=open_argument_file(text,0x8000,0);
        list=is_archive(text,argument_file)?&library_files:&ordinary_files;
        close_argument_file(argument_file,0);
    }
    append_argument(list,text);
}

/* VC5 expands the literal prefix length inline, preserving the CRT boundary. */
#pragma intrinsic(strlen)
int __stdcall is_libpath(const char *text)
{
    const char *prefix="libpath";
    return _mbsnbicmp((const unsigned char *)text,(const unsigned char *)prefix,
                      strlen(prefix))==0;
}
/* The Win32 enumeration child is a separate real application entry. */
static WIN32_FIND_DATAA *wildcard_data; /* Original494814; pointer owner only. */
static HANDLE wildcard_handle;         /* Original4948b0. */
char *__stdcall next_wildcard_file(const char *pattern)
{
    if(pattern) {
        /* Original request is580 bytes; only the320-byte ANSI result is used. */
        if(!wildcard_data)wildcard_data=(WIN32_FIND_DATAA *)allocate_memory(0x244);
        if(wildcard_handle) {
            FindClose(wildcard_handle);
            wildcard_handle=NULL;
        }
        wildcard_handle=FindFirstFileA(pattern,wildcard_data);
        if(wildcard_handle==INVALID_HANDLE_VALUE) {
            wildcard_handle=NULL;
            return NULL;
        }
    } else {
next:
        if(!FindNextFileA(wildcard_handle,wildcard_data)) {
            FindClose(wildcard_handle);
            wildcard_handle=NULL;
            return NULL;
        }
    }
    if(wildcard_data->dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)goto next;
    return wildcard_data->cFileName;
}
void __stdcall expand_wildcards(const char *text)
{
    char drive[4],directory[256],path[260];
    char *name;
    _splitpath(text,drive,directory,NULL,NULL);
    name=next_wildcard_file(text);
    while(name) {
        _makepath(path,drive,directory,name,NULL);
        parse_argument(path,1);
        name=next_wildcard_file(NULL);
    }
}
unsigned char __stdcall search_library_paths(const char *,char *);
char *__stdcall canonicalize_argument(const char *text,const char *extension)
{
    char drive[_MAX_DRIVE],directory[_MAX_DIR];
    char filename[_MAX_FNAME],suffix[_MAX_EXT];
    char path[_MAX_PATH],found[_MAX_PATH];
    _splitpath(text,drive,directory,filename,suffix);
    if (extension && !suffix[0]) {
        _makepath(path,drive,directory,filename,extension);
        text=path;
    }
    if (_access(text,0) && !drive[0] && !directory[0]) {
        if (search_library_paths(text,found)) return duplicate_argument(found);
        _searchenv(text,"LIB",found);
        if (found[0]) return duplicate_argument(found);
    }
    return duplicate_argument(text);
}
#pragma intrinsic(strlen,strcpy)
unsigned char __stdcall search_library_paths(const char *name,char *found)
{
    InputArgument *argument=input_files.first;
    unsigned index,length;
    const char *directory;
    for (index=0;index<input_files.count;++index,argument=argument->next) {
        if (!argument->switch_info->value_count) continue;
        directory=argument->switch_info->values[0].text;
        length=strlen(directory);
        if (length+strlen(name)>=_MAX_PATH) continue;
        strcpy(found,directory);
        if (length && *_mbsdec((unsigned char *)found,
                               (unsigned char *)found+length)!='\\')
            found[length++]='\\';
        strcpy(found+length,name);
        if (!_access(found,0)) return 1;
    }
    return 0;
}
InputArgument *__stdcall append_argument_record(InputArguments *,const char *,const char *);
InputArgument *__stdcall append_argument(InputArguments *list,const char *text)
{
    return append_argument_record(list,text,text);
}
InputArgument *__stdcall append_argument_record(InputArguments *list,const char *text,
                                                 const char *original_name)
{
    InputArgument *argument=(InputArgument *)zero_allocate(sizeof(InputArgument));
    argument->name=(char *)text;
    argument->original_name=(char *)original_name;
    if (!list->first) list->first=argument;
    else list->last->next=argument;
    list->last=argument;
    ++list->count;
    if (!argument_tail && list!=&option_arguments) argument_tail=argument;
    return argument;
}

unsigned char __stdcall has_switch(const char *name)
{
    InputArgument *argument=option_arguments.first;
    unsigned short index;
    for (index=0; index<option_arguments.count; ++index) {
        if (!_stricmp(name,argument->name)) return 1;
        argument=argument->next;
    }
    return 0;
}

void __stdcall clear_argument_list(InputArguments *arguments)
{
    if(arguments->count) {
        InputArgument *argument=arguments->first;
        while(argument) {
            InputArgument *next=argument->next;
            release_memory(argument);
            argument=next;
        }
        arguments->count=0;
        arguments->last=arguments->first=NULL;
    }
}

#pragma intrinsic(strcmp)
void __stdcall remove_converted_arguments(InputArguments *arguments)
{
    InputArgument *argument=arguments->first;
    unsigned index;
    for(index=0;(WORD)index<arguments->count;++index) {
        if(strcmp(argument->name,argument->original_name) && remove(argument->original_name)==-1)
            link_fatal(NULL,1109,argument->original_name);
        argument=argument->next;
    }
}
/* The provisional cleanup_pdb name predates this body recovery. Its real role
 * is removal of converted files in the existing link/ordinary argument lists. */
void __cdecl cleanup_pdb(void)
{
    remove_converted_arguments(&link_files);
    remove_converted_arguments(&ordinary_files);
}
