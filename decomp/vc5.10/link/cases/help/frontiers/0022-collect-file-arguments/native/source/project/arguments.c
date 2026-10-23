#include <windows.h>
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
