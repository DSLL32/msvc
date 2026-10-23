#include <windows.h>
#include <stdlib.h>
#include <mbstring.h>
#include <string.h>
#include "link.h"
#include "arguments.h"
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
        argument->module = load_input_module(NULL,argument->name);
        if (!argument->module->format) link_warning(NULL,4105,argument->name);
        argument = argument->next;
    }
}
InputModule *__stdcall load_input_module(void *context,const char *name)
{
    __asm { int 3 }
    return NULL;
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
    __asm { int 3 }
    return NULL;
}

