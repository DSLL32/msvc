#include <windows.h>
#include "link.h"
#include "pass-one.h"
#include "full-link-functions.h"
#include "input.h"
#include <recon.h>

int current_input_file;
DWORD incremental_status_code;

OutputModule *__stdcall read_input_argument(InputArgument *argument,OutputLibrary *library,LinkContext *context)
{
    DWORD position;
    if (is_archive(argument->name,current_input_file)) {
        library=create_library(argument->name,NULL,&context->pointer_list);
        if (selected_tool_kind!=1) {
            position=0;
            seek_file(current_input_file,0,0,&position);
            read_library_index(library,context);
        }
        library->flags&=~4;
        library->input_tag=argument->input_tag;
        return NULL;
    }
    if (is_mac_resource_file(current_input_file)) {
        add_mac_data((const unsigned char *)argument->name,0,(DWORD)current_input_file);
        return NULL;
    }
    return read_object_file(argument,context,library);
}
void __stdcall read_library_index(OutputLibrary *library,LinkContext *context) { __asm { int 3 } }
BYTE __stdcall is_mac_resource_file(int file) { __asm { int 3 } return 0; }
OutputModule *__stdcall read_object_file(InputArgument *argument,LinkContext *context,OutputLibrary *library) { __asm { int 3 } return NULL; }
void __stdcall load_object_module(LinkContext *context,void *parent,OutputModule *module,WORD flags)
{
    __asm { int 3 }
}
void __stdcall infer_output_file_name(LinkContext *context,InputArgument *argument)
{
    __asm { int 3 }
}
