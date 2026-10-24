#include <windows.h>
#include "input.h"
#include <recon.h>

/* CheckMachine is an original thiscall: ECX is the context and RET8 removes
   its two stack slots. VC5 C cannot express a member-call ABI. This TU adds
   no wrapper vertex, fields, constructor, destructor or virtual dispatch. */
void __stdcall check_input_machine(LinkContext *context,InputArgument *argument)
{
    WORD machine=detect_input_machine(context,argument);
    if (machine) {
        if (!context->file_header.Machine) {
            if (machine==0x162) machine=0x166;
            context->file_header.Machine=machine;
        } else context->CheckMachine(argument->name,machine);
    }
}
WORD __stdcall detect_input_machine(LinkContext *context,InputArgument *argument)
{
    __asm { int 3 }
    return 0;
}
void RECON_THISCALL LinkContext::CheckMachine(const char *name,WORD machine)
{
    __asm { int 3 }
}
