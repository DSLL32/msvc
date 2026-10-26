#ifndef LINK_LIBRARIAN_CONSTRUCT_H
#define LINK_LIBRARIAN_CONSTRUCT_H
#include <stdlib.h>
#include "librarian.h"
#include "arguments.h"
#include <recon.h>

/* These constructors and base cleanup are wholly inline in the target driver.
 * Padding is not initialized: only the observed scalar/header stores belong. */
RECON_FORCEINLINE RECON_THISCALL LibrarianContext::LibrarianContext(LinkContext *source,DWORD formats)
{
    reserved04=0;image=NULL;context=source;
    owned10=NULL;thunk=NULL;state18=0;member_name=NULL;
    flag20=0;ordinal_span=0;ordinal_name_indices=NULL;state2c=0;null_thunk_name=NULL;
    machine=0;owned3c=NULL;state40=0;owned44=NULL;
    debug_formats=formats;
    file_header.Machine=0;file_header.NumberOfSections=0;
    file_header.TimeDateStamp=0;file_header.PointerToSymbolTable=0;
    file_header.NumberOfSymbols=0;file_header.SizeOfOptionalHeader=0;
    file_header.Characteristics=0;
    output_file=0;cached_member_name=NULL;cached_name_length=0;
}
RECON_FORCEINLINE RECON_THISCALL ExportLibrarianContext::ExportLibrarianContext(LinkContext *source)
    : LibrarianContext(source,source->options.debug_formats) {}
RECON_FORCEINLINE RECON_THISCALL CoffLibrarianContext::CoffLibrarianContext(LinkContext *source)
    : LibrarianContext(source,2) {}
RECON_FORCEINLINE RECON_THISCALL CoffLibrarianContext::CoffLibrarianContext(int file,
    const char *base,const char *extension,DWORD formats,WORD target_machine,DWORD timestamp)
    : LibrarianContext(file,base,extension,formats,target_machine,timestamp) {}
RECON_FORCEINLINE RECON_THISCALL LibrarianContext::~LibrarianContext()
{
    if(owned10)release_memory(owned10);
    if(ordinal_name_indices)release_memory(ordinal_name_indices);
    if(owned3c)free(owned3c);
    if(owned44)free(owned44);
}
#endif
