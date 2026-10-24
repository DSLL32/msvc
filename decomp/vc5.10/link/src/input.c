#include <windows.h>
#include "input.h"
#include <recon.h>

int current_input_file;
DWORD incremental_status_code;

OutputModule *__stdcall read_input_argument(InputArgument *argument,OutputLibrary *library,LinkContext *context)
{
    __asm { int 3 }
    return NULL;
}
void __stdcall load_object_module(LinkContext *context,void *parent,OutputModule *module,WORD flags)
{
    __asm { int 3 }
}
void __stdcall infer_output_file_name(LinkContext *context,InputArgument *argument)
{
    __asm { int 3 }
}
