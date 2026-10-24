#include <windows.h>
#include "input.h"
#include "options.h"
#include "full-link-functions.h"
#include <recon.h>

extern "C" BYTE file_access_statistics;

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
    DWORD signature,error=0;
    WORD machine;
    FileRecord *record;
    seek_file(current_input_file,0,0,&error);
    if (file_access_statistics) record_file_access();
    record=file_slots[current_input_file];
    if (record->flags & 8) mapped_read(record,&signature,4);
    else buffered_read(record,&signature,4);
    if ((BYTE)signature==0x80) {
        convert_omf_input(argument,&machine);
        return machine;
    }
    if (!signature) {
        convert_input_object(context,argument,&machine);
        return machine;
    }
    machine=(WORD)signature;
    switch (machine) {
    case 0: case 0x14c: case 0x162: case 0x166: case 0x168:
    case 0x184: case 0x1a2: case 0x1a6: case 0x1c0:
    case 0x1f0: case 0x200: case 0x601:
        break;
    default:
        link_fatal(argument->name,1136);
    }
    return machine;
}
void __stdcall convert_omf_input(InputArgument *argument,WORD *machine)
{
    __asm { int 3 }
}
void __stdcall convert_input_object(LinkContext *context,InputArgument *argument,WORD *machine)
{
    __asm { int 3 }
}
void RECON_THISCALL LinkContext::CheckMachine(const char *name,WORD machine)
{
    __asm { int 3 }
}
