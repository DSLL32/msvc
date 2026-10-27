/* Per-file dispatch preserves the original inlined cache reads and C++ owner. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <new.h>
#include "link.h"
#include "input.h"
#include "files.h"
#include "full-link-functions.h"
#include "dumper.h"
#include "librarian.h"
#include "../crt/crt-imports.h"
#include <recon.h>

extern "C" {
extern BYTE file_access_statistics;
extern const char *default_phase;
extern DWORD archive_member_offset,archive_member_size;
extern FILE *map_output;
void __stdcall free_librarian_inputs(LinkPointerList *);
DWORD dump_file_size,dump_file_type;
IMAGE_ARCHIVE_MEMBER_HEADER *__cdecl read_archive_member_header(void);
void __stdcall dump_archive_header(OutputLibrary *,IMAGE_ARCHIVE_MEMBER_HEADER,DWORD);
void __stdcall dump_archive(const char *);
void __stdcall dump_separate_debug_file(const char *);
void __stdcall dump_pe_file(const char *);
void __stdcall dump_ne_file(const char *);
void __stdcall dump_le_file(const char *);
void __stdcall dump_unknown_file(const char *);
void __stdcall dump_pef_file(void *);
void __stdcall dump_short_import(int,const CoffImportHeader *);
BYTE __stdcall is_supported_coff_file(const char *,const IMAGE_FILE_HEADER *);
}

RECON_FORCEINLINE RECON_THISCALL CoffDumper::CoffDumper(LinkContext *source,const char *file)
    : context(source),name(_strdup(file)) {}
RECON_FORCEINLINE RECON_THISCALL CoffDumper::~CoffDumper() { free(name); }

#define READ_DUMP(data,bytes) do { \
    if(file_access_statistics)record_file_access(); \
    record=file_slots[current_input_file]; \
    if(record->flags & 8)mapped_read(record,(data),(bytes)); \
    else buffered_read(record,(data),(bytes)); \
} while(0)

extern "C" void __stdcall dump_file(const char *file)
{
    WORD signature;
    DWORD error,pe_signature,offset;
    BYTE is_import;
    union {
        IMAGE_DOS_HEADER dos;
        struct { DWORD tag1,tag2;BYTE unread[32]; } pef;
        CoffImportHeader import_header;
    } header;
    FileRecord *record;
    CoffDumper *dumper;
    void *mapped;
    fprintf(dumper_context->options.dump_map ? stderr : map_output,"\nDump of file %s\n",file);
    archive_member_size=0;
    archive_member_offset=0;
    current_input_file=open_argument_file(file,0x8000,0);
    dump_file_size=(DWORD)get_file_length(current_input_file);
    if(is_archive(file,current_input_file))dump_archive(file);
    else {
        error=0;seek_file(current_input_file,0,0,&error);
        READ_DUMP(&signature,sizeof(signature));
        error=0;seek_file(current_input_file,0,0,&error);
        if(signature==0x4944)dump_separate_debug_file(file);
        else if(signature==0x5a4d && dump_file_size>=sizeof(IMAGE_DOS_HEADER)) {
            READ_DUMP(&header.dos,sizeof(header.dos));
            offset=(DWORD)header.dos.e_lfanew;
            if(offset && offset+4<=dump_file_size) {
                dumper_context->pe_header_offset=offset;
                error=0;seek_file(current_input_file,offset,0,&error);
                READ_DUMP(&pe_signature,sizeof(pe_signature));
                if(pe_signature==0x4550) {dump_file_type=2;dump_pe_file(file);}
                else if((WORD)pe_signature==0x454e) {dump_file_type=5;dump_ne_file(file);}
                else if((WORD)pe_signature==0x454c) {dump_file_type=6;dump_le_file(file);}
                else {dump_file_type=0;dump_unknown_file(file);}
            } else {dump_file_type=0;dump_unknown_file(file);}
        } else if(signature==0x6f4a && dump_file_size>=sizeof(header.pef)) {
            READ_DUMP(&header.pef,sizeof(header.pef));
            if((WORD)(header.pef.tag1>>16)==0x2179 && header.pef.tag2==0x66666570) {
                mapped=map_file_range(current_input_file,0,dump_file_size);
                if(!mapped)link_fatal(NULL,1104,file);
                dump_file_type=8;
                dump_pef_file(mapped);
            } else goto coff;
        } else {
coff:
            dump_file_type=0;
            read_coff_header(current_input_file,&dumper_context->file_header,
                &header.import_header,&is_import);
            if(is_import)dump_short_import(current_input_file,&header.import_header);
            else if(is_supported_coff_file(file,&dumper_context->file_header)) {
                dumper=new CoffDumper(dumper_context,file);
                dumper->Dump(0,sizeof(IMAGE_FILE_HEADER));
                delete dumper;
            }
        }
    }
    close_argument_file(current_input_file,1);
    free_librarian_inputs(&dumper_context->pointer_list);
}
#undef READ_DUMP

/* Each real archive member shares the per-file dumper type.  Import conversion
 * temporarily switches the file/offset owners, then restores the archive. */
extern "C" void __stdcall dump_archive_members(OutputLibrary *library,const char *file)
{
    IMAGE_ARCHIVE_MEMBER_HEADER *member;
    CoffImportHeader import_header;
    FileRecord *record;
    DWORD header_offset,position,header_bytes,error,saved_offset,saved_size;
    int saved_file;
    BYTE is_import,converted;
    CoffLibrarianContext *conversion;
    CoffDumper *dumper;
    do {
        member=read_archive_member_header();
        if(file_access_statistics)record_file_access();
        record=file_slots[current_input_file];
        position=(record->flags&8)?record->file_size:((FileBufferHeader *)record->base)->cursor;
        header_offset=position-60;
        dump_file_type=0;
        read_coff_header(current_input_file,&dumper_context->file_header,&import_header,&is_import);
        header_bytes=sizeof(IMAGE_FILE_HEADER);
        converted=0;
        if(is_import && dumper_context->options.dump_convert) {
            conversion=get_import_conversion(NULL,library,current_input_file,
                &import_header,NULL,NULL,1);
            saved_size=archive_member_size;
            saved_offset=archive_member_offset;
            header_bytes=0;
            error=0;
            archive_member_offset=seek_file(conversion->output_file,0,0,&error);
            archive_member_size=0;
            conversion->ConvertShortMember(current_input_file,&import_header,1,
                (DWORD)member,&dumper_context->file_header,NULL,NULL,NULL);
            saved_file=current_input_file;
            converted=1;
            current_input_file=conversion->output_file;
        }
        if(dumper_context->options.dump_archive_members)
            dump_archive_header(library,*member,header_offset);
        if(is_import && !dumper_context->options.dump_convert)
            dump_short_import(current_input_file,&import_header);
        else if(is_supported_coff_file(file,&dumper_context->file_header)) {
            dumper=new CoffDumper(dumper_context,file);
            dumper->Dump(1,header_bytes);
            delete dumper;
        }
        if(converted) {
            current_input_file=saved_file;
            archive_member_offset=saved_offset;
            archive_member_size=saved_size;
        }
    } while(archive_member_size+archive_member_offset+1<dump_file_size);
}

extern "C" {
IMAGE_SYMBOL *__stdcall read_module_symbols(DWORD,DWORD,BYTE);
void __stdcall release_module_symbols(void *);
void __stdcall release_module_strings(void *);
BOOL __stdcall load_dumper_debug(const char *,LONG *,char *,char *,DWORD *,DWORD *,DumperDatabase **);
void __stdcall build_dumper_symbol_index(DumperSectionState *,DWORD,DWORD);
void __stdcall release_dumper_symbol_index(DumperSectionState *);
void __stdcall read_dumper_strings(const char *);
}
#pragma intrinsic(memset,strcpy,strlen)
extern "C" void __stdcall dump_pe_file(const char *name)
{
    DumperSectionState state;
    FileRecord *record;
    DWORD header_position;
    LONG status;
    BOOL loaded;
    if(dumper_context->options.dump_headers)fputs("\nPE signature found\n",map_output);
    if(file_access_statistics)record_file_access();
    record=file_slots[current_input_file];
    header_position=(record->flags&8)?record->file_size:((FileBufferHeader *)record->base)->cursor;
    read_coff_header(current_input_file,&dumper_context->file_header,NULL,NULL);
    prepare_dumper_file(0);
    if(dumper_context->file_header.Machine==0x601)dump_file_type=7;
    if(dumper_context->file_header.PointerToSymbolTable && dumper_context->file_header.NumberOfSymbols) {
        default_phase="ReadStringTable";
        read_dumper_strings(name);
        default_phase="ReadSymbolTable";
        dump_symbols=read_module_symbols(archive_member_offset+dumper_context->file_header.PointerToSymbolTable,
            dumper_context->file_header.NumberOfSymbols,0);
    } else dump_symbols=NULL;
    memset(&state,0,sizeof(state));
    if(dumper_context->options.dump_disassembly || dumper_context->options.dump_fixups ||
        dumper_context->options.dump_fpo || dumper_context->options.dump_omap_from ||
        dumper_context->options.dump_omap_to || dumper_context->options.dump_pdata) {
        loaded=load_dumper_debug(name,&status,0,state.debug_filename,
            &state.codeview_offset,&state.codeview_bytes,&state.database);
        if(!state.debug_filename[0]) {
            state.external_name=0;
#ifdef __linux__
            /* Original filename copying is unchecked; qualify only a live
             * NUL-terminated name that fits this encountered owner. */
            if(strlen(name)>=sizeof(state.debug_filename)) { __asm { int 3 } }
#endif
            strcpy(state.debug_filename,name);
        } else state.external_name=1;
        if(loaded) {
#ifdef __linux__
            /* No external PDB/DBI receiver behavior is qualified here. */
            __asm { int 3 }
#endif
            if(!state.database->OpenDebugInfo(0,"r",&state.debug_info))state.debug_info=NULL;
            else if(!state.debug_info->OpenPublics(&state.public_symbols))state.public_symbols=NULL;
        }
        build_dumper_symbol_index(&state,0,dumper_context->optional_header.ImageBase);
    }
    dump_sections(&state,header_position,sizeof(IMAGE_FILE_HEADER),0);
    if(dumper_context->options.dump_map)dump_coff_map();
    if(dumper_context->options.dump_symbols)dump_coff_symbols();
    release_dumper_symbol_index(&state);
    if(state.database) {
#ifdef __linux__
        __asm { int 3 }
#endif
        if(state.debug_info) {
            if(state.public_symbols)state.public_symbols->Close();
            state.debug_info->Close();
        }
        state.database->Close();
    }
    if(dump_symbols) {release_module_symbols(dump_symbols);dump_symbols=NULL;}
    if(dump_strings_owned) {release_module_strings(dump_string_table);dump_strings_owned=0;}
}
