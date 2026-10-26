/* Build the export-object/import-library image using the selected five-slot
 * librarian interface. The source image remains separate from its input owner. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <mbstring.h>
#include "../crt/crt-imports.h"
#include "librarian.h"
#include "options.h"
#include "pass-one.h"
#include "full-link-functions.h"
#include <recon.h>

extern "C" {
extern const char *default_phase,*definition_file,*output_file_name;
extern InputArguments export_arguments;
extern OutputModule *output_module;
extern DWORD undefined_symbols;
extern BYTE ambiguous_exports,reproducible_build;
BYTE underscore_symbols;
/* Encountered scalar and pointer-array owners at 47f05c and 47f0fc. */
DWORD librarian_next_ordinal;
DWORD *librarian_ordinal_symbols;
/* A supported pathname prefix, not a claim about the original full capacity.
 * The sole encountered _makepath writer is guarded before writing natively. */
char librarian_export_file[260];
void __stdcall read_librarian_definition(LinkContext *,const char *);
void __stdcall merge_librarian_exports(StringTable *,StringTable *,void *,DWORD);
DWORD __stdcall count_librarian_symbols(StringTable *,DWORD *,DWORD *,DWORD *);
void *__stdcall zero_allocate(unsigned);
void __cdecl close_files(void);
void __cdecl cleanup_pdb(void);
}

#pragma intrinsic(memcpy,strlen)
int RECON_THISCALL LibrarianContext::Process(void)
{
    InputArgument *argument;
    WORD index;
    DWORD symbol_count,data_count,noname_count,private_count,ordinal_count;
    char drive[4],directory[256],basename[256];
    union { DWORD value; BYTE low; } has_inputs;
    union { DWORD value; BYTE low; } decoration;

    default_phase="DefLibMain";
    image=(LinkContext *)allocate_link_memory(1,sizeof(LinkContext));
    image->Initialize(context->is_vxd);
    image->file_header=context->file_header;
    image->optional_header=context->optional_header;
    /* The four initial option BYTEs are not copied by the original. */
    memcpy((BYTE *)&image->options+4,(BYTE *)&context->options+4,sizeof(LinkOptions)-4);
    memcpy(&image->specified_switches,&context->specified_switches,
        offsetof(LinkContext,sections)-offsetof(LinkContext,specified_switches));
    initialize_output_modules(&output_module);
    machine=image->file_header.Machine;
    file_header=image->file_header;
#ifdef __linux__
    /* Only a BYTE is stored in the original physical DWORD local. Its upper
     * bytes are unwritten; keep the full transport and zero them natively.
     * Pending consumers must establish their consumed width before acceptance. */
    has_inputs.value=0;
#endif
    has_inputs.low=(ordinary_files.count || library_files.count)?1:0;
    image->optional_header.ImageBase=0;
    Prepare(has_inputs.value);
    ordinal_count=reserved04;
    read_librarian_definition(image,definition_file);
    image->options.debug_level=2;
    if(has_inputs.low) {
        argument=ordinary_files.first;
        for(index=0;index<ordinary_files.count;++index) {
            append_argument_record(&link_files,argument->name,argument->original_name);
            argument=argument->next;
        }
        argument=library_files.first;
        for(index=0;index<library_files.count;++index) {
            append_argument_record(&link_files,argument->name,argument->original_name);
            argument=argument->next;
        }
        clear_argument_list(&ordinary_files);
        clear_argument_list(&library_files);
        context->PassOne();
        argument=export_arguments.first;
        for(index=0;index<export_arguments.count;++index) {
            add_librarian_export(argument->name,image,argument->original_name!=NULL,
                argument->original_name);
            argument=argument->next;
        }
        /* DL is replaced after the last ordinary call; the remaining physical
         * register bits are not an established semantic input. Preserve a
         * DWORD formal for the pending consumer rather than guessing its use. */
#ifdef __linux__
        decoration.value=0;
#endif
        decoration.low=underscore_symbols;
        merge_librarian_exports(image->symbols,context->symbols,
            context->pointer_list.first,decoration.value);
        image->symbols->SortSymbols();
        if(undefined_symbols)link_fatal(output_file_name,1120,undefined_symbols);
        if(ambiguous_exports)link_fatal(NULL,1152);
        clear_buffer(&context->symbols->hash.strings);
    }
    if(!output_file_name)link_fatal(NULL,1159);
    _splitpath(output_file_name,drive,directory,basename,NULL);
#ifdef __linux__
    if(strlen(drive)+strlen(directory)+strlen(basename)+strlen(".exp")>=
        sizeof(librarian_export_file)) { __asm { int 3 } }
#endif
    _makepath(librarian_export_file,drive,directory,basename,".exp");
    if(!image->options.library_basename) {
        _splitpath(*definition_file?definition_file:output_file_name,NULL,NULL,basename,NULL);
        image->options.library_basename=duplicate_argument(basename);
    }
    owned44=duplicate_argument(image->options.default_extension);
    owned3c=duplicate_argument(image->options.library_basename);
    state40=strlen(image->options.library_basename);
    if(!_mbsicmp((const unsigned char *)output_file_name,
                 (const unsigned char *)librarian_export_file))
        link_fatal(NULL,1154,output_file_name);
    if(has_inputs.low) {
        check_output_input_collision(output_file_name,link_files.first);
        check_output_input_collision(librarian_export_file,link_files.first);
    } else {
        check_output_input_collision(output_file_name,ordinary_files.first);
        check_output_input_collision(output_file_name,library_files.first);
        check_output_input_collision(librarian_export_file,ordinary_files.first);
        check_output_input_collision(librarian_export_file,library_files.first);
    }
    image->symbols->ReleaseLookupIndex();
    if(!librarian_next_ordinal)librarian_next_ordinal=1;
    _tzset();
    state2c=reproducible_build?(DWORD)-1:(DWORD)time(NULL);
    file_header.TimeDateStamp=state2c;
    symbol_count=count_librarian_symbols(image->symbols,&data_count,&noname_count,&private_count);
    ordinal_span=OrdinalSlots(symbol_count);
    CreateImportSymbols(&ordinal_count);
    ordinal_count+=symbol_count-data_count-private_count;
    librarian_ordinal_symbols=(DWORD *)zero_allocate(reserved04*4+4);
    owned10=zero_allocate(ordinal_span);
    MarkExplicitOrdinals();
    if(image->is_vxd!=1) {
        link_message(9001,output_file_name,librarian_export_file);
        fflush(stdout);
    }
    WriteOutputs(ordinal_count,symbol_count,noname_count,has_inputs.value);
    close_files();
    cleanup_pdb();
    return 0;
}
