#include <windows.h>
#include <string.h>
#include "link.h"
#include "pass-one.h"
#include "full-link-functions.h"
#include "input.h"
#include "options.h"
#include <recon.h>

int current_input_file;
DWORD incremental_status_code;
DWORD archive_member_offset;
BYTE machine_initialized;
extern BYTE file_access_statistics;

/* Only the buffered position field is consumed by this input classifier. */
typedef struct InputBufferedPosition { DWORD reserved[3],position; } InputBufferedPosition;

#pragma intrinsic(memcmp)
BYTE __stdcall is_archive(const char *name,int file)
{
    char signature[8];
    FileRecord *record;
    unsigned bytes;
    DWORD position;
    if (file_access_statistics) record_file_access();
    record=file_slots[file];
    if (record->flags & 8) bytes=mapped_read(record,signature,8);
    else bytes=buffered_read(record,signature,8);
    if (bytes!=8) {
        if (file_access_statistics) record_file_access();
        record=file_slots[file];
        position=(record->flags & 8)?record->file_size:
            ((InputBufferedPosition *)record->base)->position;
        link_fatal(name,1107,position);
    }
    if (!memcmp(signature,"!<arch>\n",8)) {
        archive_member_offset=8;
        return 1;
    }
    return 0;
}

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
BYTE __stdcall is_mac_resource_file(int file)
{
    LONG length=get_file_length(file);
    DWORD error=0,map_offset,data_length,map_length;
    if (length<256) return 0;
    seek_file(file,0,0,&error);
    if (read_big_endian_dword(file)!=256) return 0;
    map_offset=read_big_endian_dword(file);
    data_length=read_big_endian_dword(file);
    map_length=read_big_endian_dword(file);
    if (map_offset!=data_length+256) return 0;
    return (DWORD)length==map_length+256+data_length;
}
DWORD __stdcall read_big_endian_dword(int file)
{
    DWORD value;
    FileRecord *record;
    if (file_access_statistics) record_file_access();
    record=file_slots[file];
    if (record->flags & 8) {
        mapped_read(record,&value,4);
        return byte_swap_dword(value);
    }
    buffered_read(record,&value,4);
    return byte_swap_dword(value);
}
DWORD __stdcall byte_swap_dword(DWORD value)
{
    union { DWORD value; BYTE byte[4]; } result;
    result.byte[0]=(BYTE)(value>>24);
    result.byte[1]=(BYTE)(value>>16);
    result.byte[2]=(BYTE)(value>>8);
    result.byte[3]=(BYTE)value;
    return result.value;
}
OutputModule *__stdcall read_object_file(InputArgument *argument,LinkContext *context,OutputLibrary *library)
{
    IMAGE_FILE_HEADER header;
    OutputModule *module;
    DWORD error=0;
    BYTE created;
    check_input_machine(context,argument);
    if (!machine_initialized) initialize_machine(context);
    seek_file(current_input_file,0,0,&error);
    read_coff_header(current_input_file,&header,NULL,NULL);
    module=create_input_module(argument->original_name,argument->name,NULL,
        &header,library,&created,0,sizeof(header),NULL,NULL,NULL,0);
    module->timestamp=header.TimeDateStamp;
    module->file_size=get_file_length(current_input_file);
    if (!created) {
        link_warning(argument->name,4042);
        return NULL;
    }
    module->input_tag=argument->input_tag;
    if (context->options.effective_reference_opt || context->options.fold_identical_comdats)
        allocate_module_section_headers(module);
    return module;
}
BYTE __stdcall read_coff_header(int file,IMAGE_FILE_HEADER *header,void *alternate_header,BYTE *alternate) { __asm { int 3 } return 0; }
OutputModule *__stdcall create_input_module(const char *original_name,const char *name,const void *identity,IMAGE_FILE_HEADER *header,OutputLibrary *library,BYTE *created,BYTE copy_sections,DWORD section_offset,IMAGE_SECTION_HEADER *sections,BYTE *section_flags,void *optional_data,DWORD auxiliary) { __asm { int 3 } return NULL; }
void __stdcall allocate_module_section_headers(OutputModule *module) { __asm { int 3 } }
void __stdcall load_object_module(LinkContext *context,void *parent,OutputModule *module,WORD flags)
{
    __asm { int 3 }
}
void __stdcall infer_output_file_name(LinkContext *context,InputArgument *argument)
{
    __asm { int 3 }
}
