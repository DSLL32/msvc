/* COFF members implementing individual exports in an import library. */
#include <windows.h>
#include <string.h>
#include "librarian.h"
#include "files.h"
#include "memory.h"
#include "full-link-functions.h"
#include <recon.h>

extern "C" {
extern const char *default_phase;
extern BYTE file_access_statistics;
extern int selected_tool_kind;
/* One lazily allocated string table: 47f24c,467354 and47f370.bit0. */
static BYTE *import_member_strings;
static DWORD import_member_string_capacity=1024;
static BYTE import_member_initialized;
extern BYTE underscore_symbols;
char *__stdcall duplicate_argument(const char *);
#pragma intrinsic(strlen)
char *__stdcall select_import_name(DWORD strip,DWORD undecorate,const char *name,DWORD *length)
{
    char *selected,*suffix;
    if((BYTE)strip) {
        selected=duplicate_argument(name);
        if(*selected=='?' || *selected=='@' || (underscore_symbols && *selected=='_'))++selected;
        suffix=strchr(selected,'@');
        if(suffix)*suffix=0;
    } else selected=(char *)name;
    *length=strlen(selected)+1;
    if((BYTE)undecorate && *selected!='?' && *selected!='@')--*length;
    return selected;
}
char *__stdcall undecorate_import_symbol(const char *) { __asm { int 3 } return NULL; }
char *__stdcall import_debug_name(const char *name,BYTE *length)
{
    char *selected,*suffix;
    if(*name=='?')selected=undecorate_import_symbol(name);
    else {
        selected=duplicate_argument(name);
        if(*selected=='@' || (underscore_symbols && *selected=='_'))++selected;
        suffix=strchr(selected,'@');
        if(suffix)*suffix=0;
    }
    *length=(BYTE)strlen(selected);
    return selected;
}
#pragma intrinsic(memset,strncpy,memcpy)
void __stdcall initialize_import_section(IMAGE_SECTION_HEADER *section,DWORD *offset,
    const char *name,DWORD bytes,DWORD relocations,DWORD attributes)
{
    DWORD position=*offset;
    memset(section,0,sizeof(*section));
    strncpy((char *)section->Name,name,8);
    section->SizeOfRawData=bytes;
    section->NumberOfRelocations=(WORD)relocations;
    section->Characteristics=attributes;
    if(position&1)position=(position&~1U)+2;
    section->PointerToRawData=position;
    position+=bytes;
    if((WORD)relocations) {
        if(position&1)position=(position&~1U)+2;
        section->PointerToRelocations=position;
        position+=(WORD)relocations*10;
    }
    *offset=position;
}
void __stdcall set_librarian_output_alignment(int fd,DWORD alignment)
{
    struct Position { DWORD reserved[3],position; };
    FileRecord *file;
    DWORD position,remaining;
    if(file_access_statistics)record_file_access();
    file=file_slots[fd];
    position=(file->flags&8)?file->file_size:((Position *)file->base)->position;
    remaining=position&(alignment-1)?
        (position&~(alignment-1))+alignment-position:0;
    while(remaining--) {
        if(file_access_statistics)record_file_access();
        file=file_slots[fd];
        if(file->flags&8)mapped_write(file,"",1);
        else buffered_write(file,"",1);
    }
}
void __stdcall write_archive_header(int,const char *,DWORD,DWORD,DWORD,DWORD,IMAGE_ARCHIVE_MEMBER_HEADER *);
void __stdcall write_file_header(int,IMAGE_FILE_HEADER *,IMAGE_FILE_HEADER *);
void __stdcall write_section_header(int,const IMAGE_SECTION_HEADER *,IMAGE_SECTION_HEADER *);
void __stdcall write_relocations(int,const IMAGE_RELOCATION *,DWORD);
void __stdcall copy_relocations(IMAGE_RELOCATION *destination,
    const IMAGE_RELOCATION *source,DWORD count)
{
    memcpy(destination,source,count*sizeof(IMAGE_RELOCATION));
}
}

#pragma pack(push,2)
struct ImportSectionAux {
    DWORD length;
    WORD relocations,lines;
    DWORD checksum;
    SHORT number;
    BYTE selection,reserved;
    SHORT high_number;
};
union ImportSymbolRecord { IMAGE_SYMBOL symbol; ImportSectionAux section; };
#pragma pack(pop)
typedef char ImportRecordLayout[(sizeof(ImportSymbolRecord)==18)?1:-1];
struct ImportBufferedPosition { DWORD reserved[3],position; };
#define WRITE_THUNK(data,bytes) do { \
    if(file_access_statistics)record_file_access(); \
    file=file_slots[output_file]; \
    if(file->flags&8)mapped_write(file,(data),(bytes)); \
    else buffered_write(file,(data),(bytes)); \
} while(0)
#define TELL_THUNK(result) do { \
    if(file_access_statistics)record_file_access(); \
    file=file_slots[output_file]; \
    (result)=(file->flags&8)?file->file_size:((ImportBufferedPosition *)file->base)->position; \
} while(0)
#define THUNK_SECTION(slot,text,size,relocs,attributes) do { \
    indices[slot]=next_section++; \
    initialize_import_section(&sections[slot],&position,(text),(size),(relocs),(attributes)); \
} while(0)
#define THUNK_SECTION_SYMBOL(text,slot,nreloc,association,select) do { \
    strncpy((char *)out->symbol.N.ShortName,(text),8); \
    out->symbol.SectionNumber=indices[slot]; \
    out->symbol.StorageClass=IMAGE_SYM_CLASS_STATIC; \
    out->symbol.NumberOfAuxSymbols=1; \
    ++out; \
    out->section.length=sections[slot].SizeOfRawData; \
    out->section.relocations=(nreloc); \
    out->section.number=(association); \
    out->section.selection=(select); \
    ++out; \
} while(0)

#pragma intrinsic(strlen,strcpy,strcat,strncpy,memset,memcpy)
void RECON_THISCALL LibrarianContext::EmitImportSymbol(const char *name,DWORD undecorate,
    DWORD flags,DWORD by_ordinal,DWORD ordinal,DWORD archive_header,
    IMAGE_ARCHIVE_MEMBER_HEADER *archive_output,IMAGE_FILE_HEADER *header_output,
    IMAGE_SECTION_HEADER *section_output,BYTE *kind_output,LibrarianImportInfo *info)
{
    IMAGE_SECTION_HEADER sections[6];
    WORD indices[6]={0,0,0,0,0,0},symbol_indices[4];
    BYTE kinds[6]={255,255,255,255,255,255};
    ImportSymbolRecord records[28],*out;
    FileRecord *file;
    char *import_name,*debug_name=NULL,*strings;
    DWORD name_length,import_length,string_bytes,position,padding=0;
    DWORD code_attributes,first_symbol,name_type,import_type,data_bytes;
    WORD section_count=3,symbol_count=10,next_section=1;
    BYTE debug=(BYTE)((debug_formats>>1)&1),debug_length=0;
    BYTE code=(flags&0x2008)==0,data=(flags&8)!=0;
    DWORD header_bytes=header_output?0:20;
    if(!(import_member_initialized&1)) {
        import_member_initialized|=1;
        import_member_strings=(BYTE *)allocate_memory(import_member_string_capacity);
    }
    default_phase="EmitThunk";
    name_length=strlen(name);
    import_name=select_import_name((flags>>4)&1,undecorate,name,&import_length);
    string_bytes=4;
    if(data && name_length>8)string_bytes=name_length+5;
    string_bytes+=name_length+7+state40+21;
    if(debug) { ++section_count;++symbol_count; }
    if(code) {
        ++section_count;symbol_count+=2;
        if(debug) { ++section_count;symbol_count+=2; }
    } else if(flags&0x2000)--symbol_count;
    if((BYTE)by_ordinal) { --section_count;symbol_count-=2; }
    if(string_bytes>import_member_string_capacity) {
        import_member_string_capacity=string_bytes;
        import_member_strings=(BYTE *)reallocate_memory(import_member_strings,string_bytes);
    }
    memset(import_member_strings,0,string_bytes);
    *(DWORD *)import_member_strings=string_bytes;
    if(!section_output)header_bytes+=section_count*40;
    position=header_bytes;
    if(debug) {
        kinds[0]=0;
        THUNK_SECTION(0,".debug$S",cached_name_length+47,0,0x42100040);
    }
    if(code) {
        kinds[1]=2;
        code_attributes=machine==0x14c?0x60201020:
                        machine==0x184?0x60501020:0x60301020;
        THUNK_SECTION(1,".text",thunk->code_size,thunk->relocation_count,code_attributes);
        if(debug) {
            debug_name=import_debug_name(name,&debug_length);
            kinds[2]=3;
            THUNK_SECTION(2,".debug$S",debug_length+30,2,0x42101040);
        }
    }
    kinds[3]=(BYTE)((BYTE)by_ordinal?5:7);
    THUNK_SECTION(3,".idata$5",4,(BYTE)by_ordinal?0:1,0xc0301040);
    kinds[4]=(BYTE)((BYTE)by_ordinal?4:6);
    THUNK_SECTION(4,".idata$4",4,(BYTE)by_ordinal?0:1,0xc0301040);
    if(!(BYTE)by_ordinal) {
        kinds[5]=8;
        padding=(import_length&1)?1:0;
        THUNK_SECTION(5,".idata$6",import_length+padding+2,0,0xc0201040);
    }
    if(info)position=0;
    else if(position&1)position=(position&~1U)+2;
    if((BYTE)archive_header)
        write_archive_header(output_file,member_name,flag20,state2c,0,
            position+string_bytes+symbol_count*18,archive_output);
    file_header.NumberOfSections=section_count;
    file_header.PointerToSymbolTable=position;
    file_header.NumberOfSymbols=symbol_count;
    file_header.SizeOfOptionalHeader=0;
    write_file_header(output_file,&file_header,header_output);
    for(unsigned index=0;index<6;++index) {
        if(!indices[index])continue;
        if(kind_output)*kind_output=kinds[index];
        write_section_header(output_file,&sections[index],section_output);
        if(section_output) { ++section_output;++kind_output; }
    }
    first_symbol=debug?2:1;
    /* The original writes these WORDs conditionally, and the next helper also
     * copies the adjacent unwritten fourth WORD. Preserve Win32 definedness;
     * native represents unwritten transport bytes as zero, not equal inputs. */
#ifdef __linux__
    memset(symbol_indices,0,sizeof(symbol_indices));
#endif
    if(code) {
        first_symbol+=2;
        symbol_indices[0]=(WORD)first_symbol++;
        if(debug)first_symbol+=2;
    }
    first_symbol+=2;
    symbol_indices[1]=(WORD)first_symbol++;
    if(data)symbol_indices[0]=(WORD)first_symbol++;
    first_symbol+=2;
    if(!(BYTE)by_ordinal)symbol_indices[2]=(WORD)first_symbol;
    if(info)name_type=info->name_type;
    else if((BYTE)by_ordinal)name_type=0;
    else name_type=(BYTE)undecorate?2:1;
    import_type=data?2:(flags>>13)&1;
    EmitImportData(info!=NULL,info,symbol_indices,ordinal,import_type,name_type,
        import_name,import_length,debug_name,debug_length,padding);
    memset(records,0,sizeof(records));
    out=records;
    strings=(char *)import_member_strings+4;
    set_librarian_output_alignment(output_file,2);
    strncpy((char *)out->symbol.N.ShortName,selected_tool_kind==2?"@comp.id":"Rich",8);
    out->symbol.Value=0x21c87;
    out->symbol.SectionNumber=-1;
    out->symbol.StorageClass=IMAGE_SYM_CLASS_STATIC;
    ++out;
    if(debug) {
        strncpy((char *)out->symbol.N.ShortName,".debug$S",8);
        out->symbol.SectionNumber=indices[0];
        out->symbol.StorageClass=IMAGE_SYM_CLASS_STATIC;
        ++out;
    }
    if(code) {
        THUNK_SECTION_SYMBOL(".text",1,1,0,1);
        if(name_length>8)out->symbol.N.Name.Long=strings-(char *)import_member_strings+6;
        else strncpy((char *)out->symbol.N.ShortName,name,8);
        out->symbol.SectionNumber=indices[1];out->symbol.Type=0x20;
        out->symbol.StorageClass=IMAGE_SYM_CLASS_EXTERNAL;
        ++out;
        if(debug)THUNK_SECTION_SYMBOL(".debug$S",2,2,indices[1],5);
    }
    THUNK_SECTION_SYMBOL(".idata$5",3,(BYTE)by_ordinal?0:1,0,1);
    out->symbol.N.Name.Long=strings-(char *)import_member_strings;
    strcpy(strings,"__imp_");strcat(strings,name);
    strings+=name_length+7;
    out->symbol.SectionNumber=indices[3];out->symbol.StorageClass=IMAGE_SYM_CLASS_EXTERNAL;
    ++out;
    if(data) {
        if(name_length>8) {
            out->symbol.N.Name.Long=strings-(char *)import_member_strings;
            strcpy(strings,name);strings+=name_length+1;
        } else strncpy((char *)out->symbol.N.ShortName,name,8);
        out->symbol.SectionNumber=indices[3];
        out->symbol.StorageClass=IMAGE_SYM_CLASS_EXTERNAL;
        ++out;
    }
    THUNK_SECTION_SYMBOL(".idata$4",4,(BYTE)by_ordinal?0:1,indices[3],5);
    if(!(BYTE)by_ordinal)THUNK_SECTION_SYMBOL(".idata$6",5,0,indices[3],5);
    out->symbol.N.Name.Long=strings-(char *)import_member_strings;
    strcpy(strings,"__IMPORT_DESCRIPTOR_");strcat(strings,owned3c);
    out->symbol.StorageClass=IMAGE_SYM_CLASS_EXTERNAL;
    ++out;
    if(info)TELL_THUNK(info->file_offset);
    data_bytes=(BYTE *)out-(BYTE *)records;
    WRITE_THUNK(records,data_bytes);
    WRITE_THUNK(import_member_strings,string_bytes);
    TELL_THUNK(position);
    if(position&1)WRITE_THUNK("\n",1);
}
void RECON_THISCALL LibrarianContext::EmitImportDebug(LibrarianImportInfo *,void *destination,void *)
{
    DWORD signature=1;
    BYTE *cursor;
    FileRecord *file;
    if(!(debug_formats&2))return;
    librarian_codeview_object.size=(WORD)(cached_name_length+7);
    if(destination) {
        cursor=(BYTE *)destination;
        memcpy(cursor,&signature,4);cursor+=4;
        memcpy(cursor,&librarian_codeview_object,8);cursor+=8;
        *cursor++=cached_name_length;
        memcpy(cursor,cached_member_name,cached_name_length);cursor+=cached_name_length;
        memcpy(cursor,&librarian_codeview_compile,34);
    } else {
        set_librarian_output_alignment(output_file,2);
        WRITE_THUNK(&signature,4);
        WRITE_THUNK(&librarian_codeview_object,8);
        WRITE_THUNK(&cached_name_length,1);
        WRITE_THUNK(cached_member_name,cached_name_length);
        WRITE_THUNK(&librarian_codeview_compile,34);
    }
}

void RECON_THISCALL LibrarianContext::EmitImportCode(LibrarianImportInfo *info,void *destination,void *)
{
    FileRecord *file;
    if(info->type)return;
    if(destination)memcpy(destination,thunk->code,thunk->code_size);
    else {
        set_librarian_output_alignment(output_file,2);
        WRITE_THUNK(thunk->code,thunk->code_size);
    }
}

void RECON_THISCALL LibrarianContext::EmitImportCodeRelocations(LibrarianImportInfo *info,
    void *destination,void *)
{
    IMAGE_RELOCATION relocation,*out=(IMAGE_RELOCATION *)destination;
    const ImportThunkRelocation *source;
    WORD index;
    if(info->type)return;
    if(!out)set_librarian_output_alignment(output_file,2);
    for(index=0;index<thunk->relocation_count;++index) {
        source=&thunk->relocations[index];
        relocation.VirtualAddress=source->offset;
        relocation.SymbolTableIndex=info->symbol_indices[source->symbol];
        relocation.Type=source->type;
        /* The paired-relocation encodings carry the descriptor's raw index. */
        if((machine==IMAGE_FILE_MACHINE_R4000 && relocation.Type==0x25) ||
           (machine==IMAGE_FILE_MACHINE_ALPHA && relocation.Type==0x0d))
            relocation.SymbolTableIndex=source->symbol;
        if(out)copy_relocations(out++,&relocation,1);
        else write_relocations(output_file,&relocation,1);
    }
}

#pragma pack(push,1)
/* Actual25-byte S_THUNK32 record at467338. The name follows as a Pascal string. */
struct LibrarianCodeViewThunk {
    WORD size,kind;
    DWORD parent,end,next,offset;
    WORD segment,length;
    BYTE ordinal;
};
#pragma pack(pop)
typedef char CodeViewThunkLayout[(sizeof(LibrarianCodeViewThunk)==25 &&
    offsetof(LibrarianCodeViewThunk,length)==22)?1:-1];
static LibrarianCodeViewThunk librarian_codeview_thunk={0,0x206,0,0,0,0,0,0,0};

void RECON_THISCALL LibrarianContext::EmitImportFunctionDebug(LibrarianImportInfo *info,
    void *destination,void *)
{
    struct CodeViewEnd { WORD size,kind; } end={2,6};
    FileRecord *file;
    BYTE *cursor,length;
    const char *name;
    if(info->type || !(debug_formats&2))return;
    name=info->debug_name;
    length=info->debug_length;
    librarian_codeview_thunk.size=length+24;
    librarian_codeview_thunk.length=(WORD)thunk->code_size;
    if(destination) {
        cursor=(BYTE *)destination;
        memcpy(cursor,&librarian_codeview_thunk,25);cursor+=25;
        *cursor++=length;
        memcpy(cursor,name,length);cursor+=length;
        memcpy(cursor,&end,4);
    } else {
        set_librarian_output_alignment(output_file,2);
        WRITE_THUNK(&librarian_codeview_thunk,25);
        WRITE_THUNK(&length,1);
        WRITE_THUNK(name,length);
        WRITE_THUNK(&end,4);
    }
}

void RECON_THISCALL LibrarianContext::EmitImportFunctionRelocations(LibrarianImportInfo *info,
    void *destination,void *)
{
    IMAGE_RELOCATION relocation,*out=(IMAGE_RELOCATION *)destination;
    if(info->type || !(debug_formats&2))return;
    if(!out)set_librarian_output_alignment(output_file,2);
    relocation.VirtualAddress=16;
    relocation.SymbolTableIndex=info->symbol_indices[0];
    relocation.Type=thunk->section_relative_type;
    if(out)copy_relocations(out++,&relocation,1);
    else write_relocations(output_file,&relocation,1);
    relocation.VirtualAddress=20;
    relocation.Type=thunk->section_index_type;
    if(out)copy_relocations(out,&relocation,1);
    else write_relocations(output_file,&relocation,1);
}

void RECON_THISCALL LibrarianContext::EmitImportOrdinalThunk(LibrarianImportInfo *info,
    void *destination,void *)
{
    DWORD value;
    FileRecord *file;
    if(info->name_type)return;
    value=0x80000000U|info->ordinal;
    if(destination)*(DWORD *)destination=value;
    else {
        set_librarian_output_alignment(output_file,2);
        WRITE_THUNK(&value,4);
    }
}

void RECON_THISCALL LibrarianContext::EmitImportNamedThunk(LibrarianImportInfo *info,
    void *destination,void *)
{
    DWORD value=0;
    FileRecord *file;
    if(!info->name_type)return;
    if(destination)*(DWORD *)destination=value;
    else {
        set_librarian_output_alignment(output_file,2);
        WRITE_THUNK(&value,4);
    }
}

void RECON_THISCALL LibrarianContext::EmitImportNameRelocation(LibrarianImportInfo *info,
    void *destination,void *)
{
    IMAGE_RELOCATION relocation;
    if(!info->name_type)return;
    if(!destination)set_librarian_output_alignment(output_file,2);
    relocation.VirtualAddress=0;
    relocation.SymbolTableIndex=info->symbol_indices[2];
    relocation.Type=thunk->reference_types[2];
    if(destination)copy_relocations((IMAGE_RELOCATION *)destination,&relocation,1);
    else write_relocations(output_file,&relocation,1);
}

void RECON_THISCALL LibrarianContext::EmitImportHintName(LibrarianImportInfo *info,
    void *destination,void *)
{
    const char *name;
    BYTE *cursor;
    DWORD zero=0;
    FileRecord *file;
    if(!info->name_type)return;
    name=info->name;
    if(info->name_type==2 && *name!='?' && *name!='@')++name;
    if(destination) {
        cursor=(BYTE *)destination;
        *(WORD *)cursor=info->ordinal;cursor+=2;
        memcpy(cursor,name,info->name_length);cursor+=info->name_length;
        if(info->padding)memcpy(cursor,&zero,info->padding);
    } else {
        set_librarian_output_alignment(output_file,2);
        WRITE_THUNK(&info->ordinal,2);
        WRITE_THUNK(name,info->name_length);
        /* This actual producer aligns to two bytes, giving at most one zero. */
        if(info->padding)WRITE_THUNK(&zero,info->padding);
    }
}

#undef THUNK_SECTION_SYMBOL
#undef THUNK_SECTION
#undef TELL_THUNK
#undef WRITE_THUNK

void RECON_THISCALL LibrarianContext::EmitImportData(DWORD retain,LibrarianImportInfo *destination,
    const WORD *indices,DWORD ordinal,DWORD type,DWORD name_type,const char *name,
    DWORD name_length,const char *debug_name,DWORD debug_length,DWORD padding)
{
    LibrarianImportInfo info;
    /* The original leaves unused transport/padding bytes unwritten. This case
     * does not emit them. Native zeros represent those bytes without equality. */
#ifdef __linux__
    memset(&info,0,sizeof(info));
#endif
    info.name=name?duplicate_argument(name):NULL;
    info.name_length=name_length;
    info.padding=padding;
    info.debug_name=debug_name;
    info.debug_length=(BYTE)debug_length;
    info.machine=machine;
    info.type=(BYTE)type;
    info.name_type=(WORD)name_type;
    info.ordinal=(WORD)ordinal;
    memcpy(info.symbol_indices,indices,8);
    if((BYTE)retain) {
        memcpy(destination,&info,sizeof(info));
        return;
    }
    EmitImportDebug(&info,NULL,NULL);
    EmitImportDebugRelocations(&info,NULL,NULL);
    EmitImportCode(&info,NULL,NULL);
    EmitImportCodeRelocations(&info,NULL,NULL);
    EmitImportFunctionDebug(&info,NULL,NULL);
    EmitImportFunctionRelocations(&info,NULL,NULL);
    EmitImportOrdinalThunk(&info,NULL,NULL);
    EmitImportOrdinalThunk(&info,NULL,NULL);
    EmitImportNamedThunk(&info,NULL,NULL);
    EmitImportNameRelocation(&info,NULL,NULL);
    EmitImportNamedThunk(&info,NULL,NULL);
    EmitImportNameRelocation(&info,NULL,NULL);
    EmitImportHintName(&info,NULL,NULL);
}
