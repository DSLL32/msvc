#include <string.h>
#include "librarian.h"
#include "input.h"
#include "files.h"
#include "memory.h"
#include "arguments.h"
#include "full-link-functions.h"
#include <recon.h>

extern "C" void __stdcall read_librarian_objects(LinkContext *context)
{
    InputArgument *argument=ordinary_files.first;
    WORD index;
    for(index=0;index<ordinary_files.count;++index) {
        current_input_file=open_argument_file(argument->name,0x8000,0);
        check_input_machine(context,argument);
        close_argument_file(current_input_file,0);
        argument=argument->next;
    }
}

#pragma intrinsic(strlen,strcpy,strcat,memset)
RECON_THISCALL LibrarianContext::LibrarianContext(int file,const char *base,
    const char *extension,DWORD formats,WORD target_machine,DWORD timestamp)
{
    char *name;
    DWORD length;
    reserved04=0;context=NULL;image=NULL;owned10=NULL;thunk=NULL;state18=0;
    ordinal_span=0;ordinal_name_indices=NULL;
    state2c=timestamp;debug_formats=formats;machine=target_machine;
    owned3c=duplicate_argument(base);
    state40=strlen(base);
    owned44=duplicate_argument(extension);
    length=strlen(owned44)+state40+1;
    name=(char *)allocate_memory(length);
    member_name=name;
    strcpy(name,owned3c);
    strcat(name,owned44);
    flag20=length>16;
    null_thunk_name=(char *)allocate_memory(state40+18);
    null_thunk_name[0]=0x7f;
    strcpy(null_thunk_name+1,owned3c);
    strcat(null_thunk_name,"_NULL_THUNK_DATA");
    memset(&file_header,0,sizeof(file_header));
    file_header.Machine=target_machine;
    file_header.TimeDateStamp=timestamp;
    output_file=file;
    InitializeMachine(1);
    cached_member_name=member_name;
    cached_name_length=(BYTE)strlen(member_name);
}
