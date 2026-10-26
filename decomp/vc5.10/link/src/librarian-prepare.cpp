#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include "librarian.h"
#include <recon.h>

extern "C" {
extern BYTE mac_target;
extern DWORD mac_current_import_version;
extern LONG mac_old_code_version;
void __stdcall read_librarian_directive(const char *);
}
#pragma intrinsic(strcpy,strcat)
void RECON_THISCALL ExportLibrarianContext::Prepare(DWORD has_inputs)
{
    char directive[128],number[12];
    if(image->file_header.Machine==0x601) {
        reserved04=0;
        image->file_header.SizeOfOptionalHeader=0;
    } else reserved04=3;
    InitializeMachine(has_inputs);
    if(mac_target && (mac_current_import_version || mac_old_code_version!=-1)) {
        strcpy(directive,"/IMPORT:CurrentVer=");
        strcat(directive,_itoa(mac_current_import_version,number,10));
        strcat(directive,",OldAPIVer=");
        /* This method really reads the same 4670b0 owner named by its earlier
         * old-code-version producer, even though its directive says OldAPI. */
        strcat(directive,_itoa(mac_old_code_version==-1?0:mac_old_code_version,number,10));
        read_librarian_directive(directive);
    }
}

#pragma intrinsic(strcpy,strcat)
void RECON_THISCALL CoffLibrarianContext::Prepare(DWORD has_inputs)
{
    char directive[128],number[12];
    reserved04=3;
    InitializeMachine(has_inputs);
    if(mac_target && (mac_current_import_version || mac_old_code_version!=-1)) {
        strcpy(directive,"/IMPORT:CurrentVer=");
        strcat(directive,_itoa(mac_current_import_version,number,10));
        strcat(directive,",OldAPIVer=");
        strcat(directive,_itoa(mac_old_code_version==-1?0:mac_old_code_version,number,10));
        read_librarian_directive(directive);
    }
}
