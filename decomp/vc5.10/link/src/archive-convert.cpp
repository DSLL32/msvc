/* Serialize converted short-import members as ordinary COFF objects. */
#include <windows.h>
#include <string.h>
#include "librarian.h"
#include "definition-buffer-construct.h"
#include "definition-buffer-set.h"
#include "definition-buffer-cleanup.h"
#include "files.h"
#include "input.h"
#include "full-link-functions.h"
#include <recon.h>

extern "C" {
extern const char *default_phase;
extern BYTE file_access_statistics;
extern char *current_module_strings;
IMAGE_SYMBOL *__stdcall read_module_symbols(DWORD,DWORD,BYTE);
char *__stdcall read_module_string_table(const char *,DWORD,DWORD *);
void __stdcall release_module_strings(char *);
void __stdcall release_module_symbols(IMAGE_SYMBOL *);
void __stdcall set_librarian_output_alignment(int,DWORD);
void __stdcall write_archive_header(int,const char *,DWORD,DWORD,DWORD,DWORD,IMAGE_ARCHIVE_MEMBER_HEADER *);
void __stdcall write_file_header(int,IMAGE_FILE_HEADER *,IMAGE_FILE_HEADER *);
}
struct ConvertedBufferedPosition { DWORD reserved[3],position; };
#define CONVERT_TELL(result) do { \
    if(file_access_statistics)record_file_access(); \
    file=file_slots[output]; \
    (result)=(file->flags&8)?file->file_size:((ConvertedBufferedPosition *)file->base)->position; \
} while(0)
#define CONVERT_WRITE(data,bytes) do { \
    if(file_access_statistics)record_file_access(); \
    file=file_slots[output]; \
    if(file->flags&8)mapped_write(file,(data),(bytes)); \
    else buffered_write(file,(data),(bytes)); \
} while(0)
#define CONVERT_OFFSET(m) (((m)->library->flags&16)?0: \
    (((m)->flags&0x40)?((LibrarianImportInfo *)(m)->optional_data)->file_offset:(DWORD)(m)->identity))

extern "C" void __stdcall write_converted_archive_member(LinkContext *context,
    OutputModule *module,int output,DWORD timestamp,DWORD mode,const char *name,
    DWORD long_name,DWORD *member_size)
{
    const char *saved_phase=default_phase;
    DWORD start,finish,symbol_position,bytes,maximum=0,index,error=0,string_bytes;
    int saved_input;
    IMAGE_SECTION_HEADER *section;
    IMAGE_FILE_HEADER header;
    IMAGE_SYMBOL *symbols;
    FileRecord *file;
    BYTE kind;
    default_phase="WriteConvertedMember";
    CONVERT_TELL(start);
    bytes=20+module->section_count*40;
    seek_file(output,start+80,FILE_BEGIN,&error);
    for(index=0;index<module->section_count;++index) {
        section=&module->section_headers[index];
        section->PointerToRawData+=bytes;
        section->PointerToRelocations+=bytes;
        section->PointerToLinenumbers+=bytes;
        CONVERT_WRITE(section,40);
        if(section->SizeOfRawData>=maximum)maximum=section->SizeOfRawData;
        if(section->NumberOfRelocations*10U>=maximum)
            maximum=section->NumberOfRelocations*10U;
    }
    DefinitionBuffer buffer;
    if(buffer.Reserve(maximum)) {
#ifdef __linux__
        [[clang::noinline]] buffer.Set(buffer.data,buffer.end+maximum);
#else
#pragma inline_depth(0)
        buffer.Set(buffer.data,buffer.end+maximum);
#pragma inline_depth(8)
#endif
    }
    for(index=0;index<module->section_count;++index) {
        bytes=module->section_headers[index].SizeOfRawData;
        kind=module->section_flags[index];
        if(kind>=9) { __asm { int 3 } }
        (((LibrarianContext *)module->auxiliary)->*converted_data_emitters[kind])(
            (LibrarianImportInfo *)module->optional_data,buffer.data,(void *)bytes);
        set_librarian_output_alignment(output,2);
        CONVERT_WRITE(buffer.data,bytes);
        if(module->section_headers[index].NumberOfRelocations) {
            bytes=module->section_headers[index].NumberOfRelocations*10U;
            kind=module->section_flags[index];
            if(kind>=9) { __asm { int 3 } }
            (((LibrarianContext *)module->auxiliary)->*converted_relocation_emitters[kind])(
                (LibrarianImportInfo *)module->optional_data,buffer.data,(void *)bytes);
            set_librarian_output_alignment(output,2);
            CONVERT_WRITE(buffer.data,bytes);
        }
    }
    saved_input=current_input_file;
    current_input_file=((LibrarianContext *)module->auxiliary)->output_file;
    symbols=read_module_symbols(CONVERT_OFFSET(module)+module->symbol_offset,module->symbol_count,0);
    current_module_strings=read_module_string_table(
        (module->library->flags&16)?(const char *)module->identity:module->library->name,
        CONVERT_OFFSET(module)+module->symbol_offset+module->symbol_count*18,&string_bytes);
    current_input_file=saved_input;
    set_librarian_output_alignment(output,2);
    CONVERT_TELL(symbol_position);
    CONVERT_WRITE(symbols,module->symbol_count*18);
    CONVERT_WRITE(current_module_strings,string_bytes);
    release_module_strings(current_module_strings);
    release_module_symbols(symbols);
    CONVERT_TELL(finish);
    *member_size=finish-start-60;
    error=0;seek_file(output,start,FILE_BEGIN,&error);
    write_archive_header(output,name,long_name,timestamp,mode,*member_size,NULL);
    header.Machine=((LibrarianImportInfo *)module->optional_data)->machine;
    header.NumberOfSections=(WORD)module->section_count;
    header.TimeDateStamp=module->timestamp;
    header.PointerToSymbolTable=symbol_position-start-60;
    header.NumberOfSymbols=module->symbol_count;
    header.SizeOfOptionalHeader=0;
    header.Characteristics=module->characteristics;
    write_file_header(output,&header,NULL);
    error=0;seek_file(output,finish,FILE_BEGIN,&error);
    default_phase=saved_phase;
}
