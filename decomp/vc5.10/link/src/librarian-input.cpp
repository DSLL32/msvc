#include "librarian.h"
#include "input.h"
#include "files.h"
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
