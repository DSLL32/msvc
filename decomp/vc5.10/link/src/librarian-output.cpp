#include <windows.h>
#include <string.h>
#include "librarian.h"
#include "link.h"
#include "memory.h"
#include "files.h"
#include "full-link-functions.h"
#include <recon.h>

extern "C" {
extern const char *output_file_name;
extern BYTE file_access_statistics;
extern const char *default_phase;
DWORD __stdcall archive_big_endian_dword(const void *);
DWORD __stdcall archive_big_endian_count(const DWORD *value)
{
    return archive_big_endian_dword(value);
}
void __stdcall write_archive_symbol_names(int,StringTable *,DWORD);
void __stdcall write_archive_member_offsets(int,StringTable *,DWORD);
void __stdcall set_librarian_output_alignment(int,DWORD);
void __stdcall write_archive_header(int,const char *,DWORD,DWORD,DWORD,DWORD,IMAGE_ARCHIVE_MEMBER_HEADER *);
}

void RECON_THISCALL LibrarianContext::WriteOutputs(DWORD build_argument,
    DWORD symbol_count,DWORD noname_count,DWORD has_inputs)
{
    FileRecord *file;
    output_file=open_argument_file(output_file_name,0x8302,0x180);
    if (file_access_statistics) record_file_access();
    file=file_slots[output_file];
    if (file->flags & 8) mapped_write(file,"!<arch>\n",8);
    else buffered_write(file,"!<arch>\n",8);
    Build(build_argument);
    EmitImportMembers(1,0,0,0,0);
    WriteExportObject(symbol_count,noname_count,has_inputs);
    Finish();
    CompleteLinkerMembers();
}

#pragma intrinsic(strlen)
void RECON_THISCALL ExportLibrarianContext::Build(DWORD argument)
{
    LibrarianContext::Build(argument);
    if (!cached_member_name) {
        cached_member_name=member_name;
        cached_name_length=(BYTE)strlen(member_name);
    }
}

/* The original file-cache operations are inline at every use. */
typedef struct ArchiveOutputPosition { DWORD reserved[3],position; } ArchiveOutputPosition;
#define WRITE_ARCHIVE(bytes,count) do { \
    if (file_access_statistics) record_file_access(); \
    file=file_slots[output_file]; \
    if (file->flags & 8) mapped_write(file,(bytes),(count)); \
    else buffered_write(file,(bytes),(count)); \
} while (0)
#define TELL_ARCHIVE(result) do { \
    if (file_access_statistics) record_file_access(); \
    file=file_slots[output_file]; \
    (result)=(file->flags & 8)?file->file_size:((ArchiveOutputPosition *)file->base)->position; \
} while (0)
#define SEEK_ARCHIVE(offset,origin) do { error=0; seek_file(output_file,(offset),(origin),&error); } while (0)

#pragma intrinsic(strlen,strcpy,strcat)
void RECON_THISCALL LibrarianContext::Build(DWORD symbol_count)
{
    StringTable *symbols;
    FileRecord *file;
    DWORD error,position,big_count,member_count;
    int name_length;
    char *name;
    default_phase="EmitLinkerMembers";
    if (!image) return;
    symbols=image->symbols;
    SEEK_ARCHIVE(60,1);
    big_count=archive_big_endian_count(&symbol_count);
    WRITE_ARCHIVE(&big_count,4);
    SEEK_ARCHIVE(symbol_count*4,1);
    write_archive_symbol_names(output_file,symbols,0);
    TELL_ARCHIVE(position);
    SEEK_ARCHIVE(8,0);
    write_archive_header(output_file,"",0,state2c,0,position-68,0);
    SEEK_ARCHIVE(position,0);
    if (position & 1) WRITE_ARCHIVE("\n",1);
    TELL_ARCHIVE(state18);
    SEEK_ARCHIVE(60,1);
    member_count=reserved04;
    WRITE_ARCHIVE(&member_count,4);
    SEEK_ARCHIVE(member_count*4,1);
    WRITE_ARCHIVE(&symbol_count,4);
    SEEK_ARCHIVE(symbol_count*2,1);
    write_archive_symbol_names(output_file,symbols,1);
    TELL_ARCHIVE(position);
    SEEK_ARCHIVE(state18,0);
    write_archive_header(output_file,"",0,state2c,0,position-state18-60,0);
    SEEK_ARCHIVE(position,0);
    if (position & 1) WRITE_ARCHIVE("\n",1);
    name_length=strlen(owned44)+state40+1;
    name=(char *)allocate_memory(name_length);
    strcpy(name,owned3c);
    strcat(name,owned44);
    member_name=name;
    if (name_length>16) {
        flag20=1;
        write_archive_header(output_file,"/",0,state2c,0,name_length,0);
        WRITE_ARCHIVE(member_name,name_length);
        if (name_length & 1) WRITE_ARCHIVE("\n",1);
        member_name="0";
    }
    if (!cached_member_name) {
        cached_member_name=member_name;
        cached_name_length=(BYTE)strlen(member_name);
    }
}

void RECON_THISCALL LibrarianContext::EmitImportMembers(DWORD include_null_descriptor,
    DWORD *null_descriptor_offset,DWORD *descriptor_offset,DWORD *null_thunk_offset,
    const IMAGE_OPTIONAL_HEADER *optional_header)
{
    FileRecord *file;
    if (debug_formats & 2) librarian_codeview_object.size=(WORD)(cached_name_length+7);
    if (machine==0x601) return;
    if (descriptor_offset) TELL_ARCHIVE(*descriptor_offset);
    EmitImportDescriptor(optional_header);
    if ((BYTE)include_null_descriptor) {
        if (null_descriptor_offset) TELL_ARCHIVE(*null_descriptor_offset);
        EmitNullImportDescriptor();
    }
    if (null_thunk_offset) TELL_ARCHIVE(*null_thunk_offset);
    EmitNullThunk();
}

extern "C" {
extern char short_symbol_name[9];
extern BYTE librarian_undecorated_exports;
extern DWORD librarian_next_ordinal,*librarian_ordinal_symbols;
}
#pragma intrinsic(strncpy)
void RECON_THISCALL LibrarianContext::Finish(void)
{
    StringTable *symbols=image->symbols;
    StringNode **order;
    StringNode *symbol;
    const char *name;
    DWORD count,index,position,explicit_ordinal;
    WORD kind,ordinal;
    FileRecord *file;
    if(debug_formats&2)librarian_codeview_object.size=cached_name_length+7;
    order=symbols->GetSortedSymbols();
    count=count_hash_symbols(symbols->hash.state);
    for(index=0;index<count;++index) {
        symbol=order[index];
        if(!(symbol->flags&1) || (symbol->flags&0x100800))continue;
        if(machine==0x601) {
            EmitMacImportMember(symbol);
            continue;
        }
        name=get_symbol_name(symbol,symbols->hash.strings.data,&kind,NULL);
        if(kind==1)name=strncpy(short_symbol_name,name,8);
        explicit_ordinal=(symbol->symbol.Value>>31)&1;
        if(explicit_ordinal)ordinal=(WORD)symbol->symbol.Value;
        else ordinal=ordinal_name_indices[symbol->symbol.Value-librarian_next_ordinal];
        TELL_ARCHIVE(position);
        librarian_ordinal_symbols[symbol->input_flags]=position;
        /* Both byte-valued arguments occupy full DWORD transport slots. The
         * original's unused upper bits are not an equality assertion. */
        EmitImportSymbol(name,librarian_undecorated_exports,symbol->flags,
            explicit_ordinal,ordinal,1,0,0,0,0,0);
    }
}

void RECON_THISCALL LibrarianContext::CompleteLinkerMembers(void)
{
    StringTable *symbols=image->symbols;
    FileRecord *file;
    DWORD error,index;
    default_phase="CompleteLinkerMembers";
    SEEK_ARCHIVE(0x48,FILE_BEGIN);
    write_archive_member_offsets(output_file,symbols,0);
    SEEK_ARCHIVE(state18+64,FILE_BEGIN);
    for(index=0;(WORD)index<reserved04;++index) {
        WRITE_ARCHIVE(&librarian_ordinal_symbols[(WORD)index+1],4);
    }
    SEEK_ARCHIVE(4,FILE_CURRENT);
    write_archive_member_offsets(output_file,symbols,1);
}

void RECON_THISCALL CoffLibrarianContext::Finish(void)
{
    StringTable *symbols=image->symbols;
    StringNode **order=symbols->GetSortedSymbols();
    DWORD count=count_hash_symbols(symbols->hash.state),index;
    for(index=0;index<count;++index) {
        StringNode *symbol=order[index];
        if((symbol->flags&1) && !(symbol->flags&0x100800)) {
            EmitShortImport(symbol);
            if(!index)file_header.SizeOfOptionalHeader=0;
        }
    }
}

/* Original4672f0: one encountered short-import template, field-defined. */
static const CoffImportHeader short_import_template={0,0xffff,0,0,0,0,0,0,0,0};

void RECON_THISCALL CoffLibrarianContext::EmitShortImport(StringNode *symbol)
{
    CoffImportHeader header=short_import_template;
    const char *name;
    WORD kind;
    DWORD position;
    FileRecord *file;
    default_phase="EmitMember";
    name=get_symbol_name(symbol,image->symbols->hash.strings.data,&kind,NULL);
    if(kind==1)name=strncpy(short_symbol_name,name,8);
    header.machine=machine;
    header.timestamp=state2c;
    header.kind=(symbol->flags&8)?2:(symbol->flags>>13)&1;
    if(symbol->symbol.Value&0x80000000) {
        header.name_type=0;
        header.ordinal_hint=(WORD)symbol->symbol.Value;
    } else {
        if(librarian_undecorated_exports && *name=='_')header.name_type=2;
        else if(symbol->flags&0x10) {
            if(*name=='?' || *name=='@' || strchr(name,'@'))header.name_type=3;
            else header.name_type=(*name=='_')?2:1;
        } else header.name_type=1;
        header.ordinal_hint=ordinal_name_indices[symbol->symbol.Value-librarian_next_ordinal];
    }
    if(file_access_statistics)record_file_access();
    file=file_slots[output_file];
    position=(file->flags&8)?file->file_size:((ArchiveOutputPosition *)file->base)->position;
    librarian_ordinal_symbols[symbol->input_flags]=position;
    /* The sole recovered caller ignores the provisional DWORD return. */
    Emit(&header,name);
}

DWORD RECON_THISCALL CoffLibrarianContext::Emit(CoffImportHeader *header,const char *name)
{
    FileRecord *file;
    DWORD position,length;
    header->data_size=strlen(name)+strlen(owned44)+strlen(owned3c)+2;
    write_archive_header(output_file,member_name,flag20,state2c,0,
        header->data_size+sizeof(*header),NULL);
    set_librarian_output_alignment(output_file,2);
    WRITE_ARCHIVE(header,sizeof(*header));
    set_librarian_output_alignment(output_file,2);
    length=strlen(name)+1;
    WRITE_ARCHIVE(name,length);
    WRITE_ARCHIVE(owned3c,state40);
    length=strlen(owned44)+1;
    WRITE_ARCHIVE(owned44,length);
    TELL_ARCHIVE(position);
    /* The current caller ignores EAX. Preserve its actual two return paths:
     * the even cursor, or the final write result after an odd cursor. */
    if(position&1) {
        if(file_access_statistics)record_file_access();
        file=file_slots[output_file];
        if(file->flags&8)return mapped_write(file,"\n",1);
        return buffered_write(file,"\n",1);
    }
    return position;
}

void RECON_THISCALL LibrarianContext::AddSymbol(CoffImportHeader *import,const char *name,
    DWORD create_object,IMAGE_ARCHIVE_MEMBER_HEADER *archive_header,
    IMAGE_FILE_HEADER *header,IMAGE_SECTION_HEADER *sections,BYTE *section_flags,
    LibrarianImportInfo *info)
{
    DWORD flags=0;
    /* These are defined pointer-derived physical DWORD transports. The target
     * replaces only their low BYTE/WORD; the emitter consumes those low parts.
     * Keep the carried address bits instead of treating them as unwritten. */
    DWORD no_prefix=(DWORD)this&0xffffff00;
    DWORD by_ordinal=(DWORD)import&0xffffff00;
    DWORD ordinal=((DWORD)import&0xffff0000)|import->ordinal_hint;
    switch(import->kind) {
    case 1:flags=0x2000;break;
    case 2:flags=8;break;
    }
    if(info)info->name_type=import->name_type;
    switch(import->name_type) {
    case 0:by_ordinal|=1;break;
    case 2:no_prefix|=1;break;
    case 3:flags|=0x10;break;
    }
    if(debug_formats&2)librarian_codeview_object.size=(WORD)(cached_name_length+7);
    EmitImportSymbol(name,no_prefix,flags,by_ordinal,ordinal,create_object,
        archive_header,header,sections,section_flags,info);
}
