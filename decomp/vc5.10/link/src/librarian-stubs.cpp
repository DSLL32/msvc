#include "librarian.h"
#include <recon.h>

DWORD RECON_THISCALL LibrarianContext::AddSymbol(DWORD,DWORD,DWORD,DWORD,DWORD,DWORD,DWORD,DWORD)
{ __asm { int 3 } return 0; }
void RECON_THISCALL LibrarianContext::Prepare(DWORD) { __asm { int 3 } }

DWORD RECON_THISCALL ExportLibrarianContext::Emit(CoffImportHeader *,const char *) { __asm { int 3 } return 0; }

void RECON_THISCALL ExportLibrarianContext::Finish(void)
{
    if(debug_formats&2)librarian_codeview_object.size=cached_name_length+7;
    LibrarianContext::Finish();
}

void RECON_THISCALL LibrarianContext::EmitImportDebugRelocations(LibrarianImportInfo *,void *,void *) {}









void RECON_THISCALL LibrarianContext::EmitMacImportMember(StringNode *)
{ __asm { int 3 } }
extern "C" {
void __cdecl librarian_usage(void) { __asm { int 3 } }
void __stdcall list_librarian_members(OutputLibrary *) { __asm { int 3 } }
void __stdcall extract_librarian_member(LinkContext *) { __asm { int 3 } }
void __stdcall free_librarian_inputs(LinkPointerList *) { __asm { int 3 } }
void __stdcall write_converted_archive_member(LinkContext *,OutputModule *,int,DWORD,DWORD,const char *,DWORD,DWORD *) { __asm { int 3 } }
}









extern "C" {
}
