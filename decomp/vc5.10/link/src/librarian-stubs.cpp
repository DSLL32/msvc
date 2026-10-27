#include <new.h>
#include "librarian.h"
#include "pass-one.h"
#include "memory.h"
#include <recon.h>

/* Destructor-like base cleanup at original40ef40 remains a real pending
 * entry. Its current name does not assert an emitted C++ lifetime role. */
void RECON_THISCALL LibrarianContext::Release(void) { __asm { int 3 } }

void RECON_THISCALL LibrarianContext::Prepare(DWORD) { __asm { int 3 } }

DWORD RECON_THISCALL ExportLibrarianContext::Emit(CoffImportHeader *,const char *) { __asm { int 3 } return 0; }

void RECON_THISCALL ExportLibrarianContext::Finish(void)
{
    if(debug_formats&2)librarian_codeview_object.size=cached_name_length+7;
    LibrarianContext::Finish();
}

void RECON_THISCALL LibrarianContext::EmitImportDebugRelocations(LibrarianImportInfo *,void *,void *) {}









extern "C" {
void __cdecl librarian_usage(void) { __asm { int 3 } }
void __stdcall list_librarian_members(OutputLibrary *) { __asm { int 3 } }
void __stdcall free_librarian_inputs(LinkPointerList *list)
{
    LibraryEnumerator libraries;
    OutputLibrary *library;
    CoffLibrarianContext *conversion;
    LONG i;
    unsigned j;
    initialize_library_enumerator(&libraries,(OutputLibrary *)list->first);
    while(next_library(&libraries)) {
        library=libraries.current;
        for(j=0;j<5;++j)release_memory(library->index_data[j]);
        for(i=0;i<library->conversion_count;++i) {
            conversion=library->import_conversions[i];
            if(conversion) {
                conversion->Release();
                operator delete(conversion);
            }
        }
        release_memory(library->import_conversions);
        library->import_conversions=NULL;
        library->conversion_capacity=0;
        library->conversion_count=0;
    }
    list->first=NULL;
    list->last=&list->first;
    list->suppress_default_libraries=0;
    list->default_libraries=NULL;
}
}









extern "C" {
}

DWORD RECON_THISCALL CoffLibrarianContext::ConvertDelayedMember(DWORD,DWORD,CoffImportHeader *,int,
    IMAGE_FILE_HEADER *,const char *,const char *,IMAGE_SECTION_HEADER *,BYTE *,LibrarianImportInfo *)
{ __asm { int 3 } return 0; }
