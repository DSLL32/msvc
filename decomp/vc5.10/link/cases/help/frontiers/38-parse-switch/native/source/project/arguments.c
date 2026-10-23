#include <windows.h>
#include <stdlib.h>
#include <mbstring.h>
#include <string.h>
#include "link.h"
#include "files.h"
#include "arguments.h"
#include "memory.h"
#include "options.h"
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
char *__stdcall duplicate_argument(const char *text) { __asm { int 3 } return NULL; }
void __stdcall tokenize_arguments(char *text) { __asm { int 3 } }
void __stdcall release_memory(void *memory) { __asm { int 3 } }
void __stdcall read_response_file(const char *name) { __asm { int 3 } }
InputArguments library_files, ordinary_files, link_files, option_arguments;
int argument_file;
int __stdcall is_libpath(const char *);
void __stdcall expand_wildcards(const char *);
char *__stdcall canonicalize_argument(const char *,const char *);
int __stdcall open_argument_file(const char *,int,int);
unsigned char __stdcall is_archive(const char *,int);
void __stdcall close_argument_file(int,int);
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
void __stdcall expand_wildcards(const char *text) { __asm { int 3 } }
char *__stdcall canonicalize_argument(const char *text,const char *extension) { __asm { int 3 } return NULL; }
int __stdcall open_argument_file(const char *name,int mode,int flags) { __asm { int 3 } return 0; }
unsigned char __stdcall is_archive(const char *name,int file) { __asm { int 3 } return 0; }
void __stdcall close_argument_file(int file,int flags) { __asm { int 3 } }
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
