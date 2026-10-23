#include <windows.h>
#include <stdlib.h>
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
void __stdcall parse_argument(const char *,int);

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
void __stdcall parse_argument(const char *text,int source) { __asm { int 3 } }
