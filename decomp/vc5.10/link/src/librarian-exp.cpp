/* The export object uses offsets into a growable .edata buffer until emission. */
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "librarian.h"
#include "files.h"
#include "options.h"
#include "full-link-functions.h"
#include "mac-format.h"
#include <recon.h>

extern "C" {
extern const char *default_phase;
extern BYTE file_access_statistics;
extern char librarian_export_file[260];
extern DWORD librarian_next_ordinal;
extern ByteBuffer definition_directives;
DWORD __stdcall append_zero_bytes(ByteBuffer *,DWORD);
void __stdcall sort_export_names(BYTE *,DWORD *,DWORD,DWORD);
void __stdcall write_mac_shared_library(int,const char *,BYTE *,DWORD,DWORD,DWORD);
void __stdcall write_file_header(int,IMAGE_FILE_HEADER *,IMAGE_FILE_HEADER *);
void __stdcall write_section_header(int,const IMAGE_SECTION_HEADER *,IMAGE_SECTION_HEADER *);
void __stdcall write_relocations(int,const IMAGE_RELOCATION *,DWORD);
void __stdcall write_symbol_strings(int,StringTable *);
}
#define WRITE_EXP(data,bytes) do { \
    if(file_access_statistics)record_file_access(); \
    file=file_slots[exp_file]; \
    if(file->flags & 8)mapped_write(file,(data),(bytes)); \
    else buffered_write(file,(data),(bytes)); \
} while(0)
#define WRITE_EXP_SYMBOL(name,value) do { \
    memset(&symbol,0,sizeof(symbol)); \
    strncpy((char *)symbol.N.ShortName,(name),8); \
    symbol.Value=(value);symbol.SectionNumber=1;symbol.StorageClass=IMAGE_SYM_CLASS_STATIC; \
    WRITE_EXP(&symbol,18); \
} while(0)

#define WRITE_MAC(data,bytes) do { \
    if(file_access_statistics)record_file_access(); \
    file=file_slots[fd]; \
    if(file->flags & 8)mapped_write(file,(data),(bytes)); \
    else buffered_write(file,(data),(bytes)); \
} while(0)
#pragma intrinsic(strlen,strcpy,memset)
extern "C" void __stdcall write_mac_shared_library(int fd,const char *library,
    BYTE *exports,DWORD names_offset,DWORD named_count,DWORD section_file_offset)
{
    MacSharedLibraryHeader header;
    DWORD *names=(DWORD *)(exports+names_offset);
    LONG index=0,first;
    int character;
    const signed char *name;
    char record[200];
    FileRecord *file;
    memset(&header,0,sizeof(header));
    if(named_count) {
        for(character='0';character<='z';++character) {
            name=(const signed char *)(exports+names[index]);
            while(character<='z' && *name!=character) {
                header.buckets[character-'0'].first=-1;
                header.buckets[character-'0'].count=0;
                ++character;
            }
            header.buckets[character-'0'].first=index;
            if(*name==character) {
                while(index<(LONG)(named_count-1)) {
                    ++index;
                    if((signed char)exports[names[index]]!=character)break;
                }
            }
            first=header.buckets[character-'0'].first;
            /* Preserve the actual index difference, including the final run. */
            header.buckets[character-'0'].count=index-first;
        }
    } else {
        for(character=0;character<76;++character) {
            header.buckets[character].first=-1;
            header.buckets[character].count=0;
        }
    }
    header.named_count=named_count;
    header.format=2;
    header.section_file_offset=section_file_offset;
#ifdef __linux__
    /* The emitted header has exactly 52 name bytes. The original unchecked
     * strcpy is retained on Win32; longer native names are unqualified. */
    if(strlen(library)>=sizeof(header.library)) { __asm { int 3 } }
#endif
    strcpy(header.library,library);
    WRITE_MAC(&header,sizeof(header));
    for(index=0;(DWORD)index<named_count;++index) {
        memset(record,0,sizeof(record));
        strncpy(record,(const char *)exports+names[index],sizeof(record));
        WRITE_MAC(record,sizeof(record));
    }
}
#undef WRITE_MAC

#pragma intrinsic(strlen,memset,memcpy)
void RECON_THISCALL LibrarianContext::WriteExportObject(DWORD symbol_count,
    DWORD noname_count,DWORD has_inputs)
{
    ByteBuffer exports;
    StringTable *symbols;
    FileRecord *file;
    IMAGE_SECTION_HEADER section;
    IMAGE_EXPORT_DIRECTORY directory;
    IMAGE_RELOCATION relocation;
    IMAGE_SYMBOL symbol;
    DWORD *relocation_values;
    DWORD named_count,section_count,symbols_to_write,relocation_count;
    DWORD functions_offset,names_offset,ordinals_offset,module_name_offset;
    DWORD aligned_size,directives_size,mac_size,debug_size,offset,headers,index;
    DWORD codeview_signature=1,zero=0;
    BYTE special,overflow=0,filename_length=0;
    WORD *ordinals;
    int exp_file;
    default_phase="EmitDllExportDirectory";
    if(!image)return;
    exp_file=open_argument_file(librarian_export_file,0x8302,0x180);
    symbols=image->symbols;
    named_count=symbol_count-noname_count;
    special=(machine==0x601 || image->is_vxd==1)?1:0;
    if(special) {
        section_count=0;symbols_to_write=0;relocation_count=0;
    } else {
        section_count=1;
        symbols_to_write=named_count+6;
        relocation_count=named_count+symbol_count+4;
        if(relocation_count!=(WORD)relocation_count) {
            ++relocation_count;overflow=1;
        }
    }
    if(image->is_vxd!=1)symbols_to_write+=symbol_count;
    exports.data=NULL;exports.used=0;exports.capacity=0;
    append_zero_bytes(&exports,40);
    functions_offset=append_zero_bytes(&exports,ordinal_span*4);
    names_offset=append_zero_bytes(&exports,named_count*4);
    ordinals_offset=append_zero_bytes(&exports,named_count*2);
    module_name_offset=append_bytes(&exports,owned3c,strlen(owned3c));
    append_bytes(&exports,owned44,strlen(owned44)+1);
    relocation_values=special?NULL:(DWORD *)allocate_memory(named_count*4);
    FillExportTables(&exports,relocation_values,names_offset,ordinals_offset);
    /* The original pushes the full physical DWORD, but this complete consumer
     * reads only its low BYTE, once at41b3d8. Its original upper bytes remain
     * unwritten; this does not establish an upper-bit semantic input. */
    if((BYTE)has_inputs)
        sort_export_names(exports.data,special?(DWORD *)(exports.data+names_offset):relocation_values,
                               ordinals_offset,named_count);
    ordinal_name_indices=(WORD *)allocate_memory(ordinal_span*2);
    ordinals=(WORD *)(exports.data+ordinals_offset);
    for(index=0;index<named_count;++index)ordinal_name_indices[ordinals[index]]=(WORD)index;
    aligned_size=(exports.used+1)&~1;
    if(debug_formats & 2) {
        ++section_count;
        filename_length=(BYTE)strlen(librarian_export_file);
        debug_size=filename_length+47;
    } else debug_size=0;
    if(definition_directives.data) {
        ++section_count;
        append_bytes(&definition_directives,"",1);
        directives_size=(definition_directives.used+1)&~1;
    } else directives_size=0;
    if(machine==0x601) {
        ++section_count;
        mac_size=named_count*200+676;
    } else mac_size=0;
    InternExportNames();
    file_header.NumberOfSections=(WORD)section_count;
    file_header.SizeOfOptionalHeader=0;
    file_header.NumberOfSymbols=symbols_to_write;
    headers=(WORD)section_count*40+20;
    file_header.PointerToSymbolTable=headers+(special?0:aligned_size)+mac_size+
        directives_size+debug_size+relocation_count*10;
    if(file_header.Machine==0x601)file_header.Characteristics|=0x4000;
    write_file_header(exp_file,&file_header,NULL);
    offset=headers;
    if(!special) {
        memset(&section,0,sizeof(section));
        strncpy((char *)section.Name,".edata",8);
        section.SizeOfRawData=exports.used;
        section.PointerToRawData=offset;
        section.PointerToRelocations=offset+aligned_size;
        section.NumberOfRelocations=overflow?0xffff:(WORD)relocation_count;
        section.Characteristics=0x40000040;
        if(overflow)section.Characteristics|=0x01000000;
        write_section_header(exp_file,&section,NULL);
        offset+=aligned_size+relocation_count*10;
    }
    if(debug_formats & 2) {
        memset(&section,0,sizeof(section));
        strncpy((char *)section.Name,".debug$S",8);
        section.SizeOfRawData=debug_size;
        section.PointerToRawData=offset;
        section.Characteristics=0x42100040;
        write_section_header(exp_file,&section,NULL);
        offset+=debug_size;
    }
    if(definition_directives.data) {
        memset(&section,0,sizeof(section));
        strncpy((char *)section.Name,".drectve",8);
        section.SizeOfRawData=definition_directives.used;
        section.PointerToRawData=offset;
        section.Characteristics=0xa00;
        write_section_header(exp_file,&section,NULL);
        offset+=directives_size;
    }
    if(machine==0x601) {
        memset(&section,0,sizeof(section));
        strncpy((char *)section.Name,".ppcshl",8);
        section.SizeOfRawData=mac_size;
        section.PointerToRawData=offset;
        /* The Mac section contains serialized names, with no relocations. */
        section.Characteristics=0x02000800;
        write_section_header(exp_file,&section,NULL);
    }
    if(!special) {
        memset(&directory,0,sizeof(directory));
        directory.TimeDateStamp=file_header.TimeDateStamp;
        directory.Base=librarian_next_ordinal;
        directory.NumberOfFunctions=ordinal_span;
        directory.NumberOfNames=named_count;
        memcpy(exports.data,&directory,40);
        WRITE_EXP(exports.data,exports.used);
        if(aligned_size!=exports.used)WRITE_EXP(&zero,aligned_size-exports.used);
        if(overflow) {
            relocation.VirtualAddress=relocation_count;
            relocation.SymbolTableIndex=0;
            relocation.Type=(WORD)thunk->flags;
            write_relocations(exp_file,&relocation,1);
        }
        relocation.VirtualAddress=12;relocation.SymbolTableIndex=2;
        relocation.Type=thunk->reference_types[0];
        write_relocations(exp_file,&relocation,1);
        relocation.VirtualAddress=28;relocation.SymbolTableIndex=3;
        write_relocations(exp_file,&relocation,1);
        relocation.VirtualAddress=32;relocation.SymbolTableIndex=4;
        write_relocations(exp_file,&relocation,1);
        relocation.VirtualAddress=36;relocation.SymbolTableIndex=5;
        write_relocations(exp_file,&relocation,1);
        EmitExportRelocations(exp_file,named_count,functions_offset,names_offset);
    }
    if(debug_formats & 2) {
        WRITE_EXP(&codeview_signature,4);
        librarian_codeview_object.size=(WORD)(filename_length+7);
        WRITE_EXP(&librarian_codeview_object,8);
        WRITE_EXP(&filename_length,1);
        WRITE_EXP(librarian_export_file,filename_length);
        WRITE_EXP(&librarian_codeview_compile,34);
    }
    if(definition_directives.data) {
        WRITE_EXP(definition_directives.data,definition_directives.used);
        if(directives_size!=definition_directives.used)
            WRITE_EXP(&zero,directives_size-definition_directives.used);
        clear_buffer(&definition_directives);
    }
    if(machine==0x601) {
        default_phase="MppcWriteShlSection";
        write_mac_shared_library(exp_file,owned3c,exports.data,names_offset,named_count,offset);
        default_phase="EmitDllExportDirectory";
    }
    if(!special) {
        memset(&symbol,0,sizeof(symbol));
        strncpy((char *)symbol.N.ShortName,"@comp.id",8);
        symbol.Value=0x21c87;symbol.SectionNumber=-1;symbol.StorageClass=IMAGE_SYM_CLASS_STATIC;
        WRITE_EXP(&symbol,18);
        WRITE_EXP_SYMBOL(".edata",0);
        WRITE_EXP_SYMBOL("szName",module_name_offset);
        WRITE_EXP_SYMBOL("rgpv",functions_offset);
        WRITE_EXP_SYMBOL("rgszName",names_offset);
        WRITE_EXP_SYMBOL("rgwOrd",ordinals_offset);
        ordinals=(WORD *)(exports.data+ordinals_offset);
        memset(&symbol,0,sizeof(symbol));
        symbol.SectionNumber=1;symbol.StorageClass=IMAGE_SYM_CLASS_STATIC;
        for(index=0;index<named_count;++index) {
            sprintf((char *)symbol.N.ShortName,"$N%05u",librarian_next_ordinal+ordinals[index]);
            symbol.Value=relocation_values[index];
            WRITE_EXP(&symbol,18);
        }
    }
    release_memory(relocation_values);
    if(image->is_vxd!=1)EmitExportSymbols(exp_file);
    write_symbol_strings(exp_file,symbols);
    clear_buffer(&exports);
    close_argument_file(exp_file,1);
}

extern "C" {
extern BYTE librarian_undecorated_exports,underscore_symbols;
extern char short_symbol_name[9];
}
#pragma intrinsic(strlen)
void RECON_THISCALL LibrarianContext::FillExportTables(ByteBuffer *buffer,
    DWORD *relocation_values,DWORD names_offset,DWORD ordinals_offset)
{
    StringTable *symbols;
    StringNode **order,*symbol;
    LinkContext *output;
    const char *name;
    char *copy,*suffix;
    WORD kind;
    DWORD count,index,ordinal_index;
    BYTE *assigned;
    default_phase="BuildRawDataFromExternTable";
    if(!image)return;
    symbols=image->symbols;
    order=symbols->GetSortedSymbols();
    count=count_hash_symbols(symbols->hash.state);
    for(index=0;index<count;++index) {
        symbol=order[index];
        if(!(symbol->flags & 1) || (symbol->flags & 0x800))continue;
        if(symbol->symbol.Value) {
            ordinal_index=symbol->symbol.Value-librarian_next_ordinal;
            symbol->symbol.Value|=0x80000000;
        } else {
            assigned=(BYTE *)owned10;
            ordinal_index=0;
            while(assigned[ordinal_index])++ordinal_index;
            assigned[ordinal_index]=1;
            symbol->symbol.Value=ordinal_index+librarian_next_ordinal;
        }
        if(!(symbol->flags & 0x8000)) {
            output=librarian_context->image;
            name=get_symbol_name(symbol,symbols->hash.string_storage,&kind,NULL);
            if(kind==1)name=strncpy(short_symbol_name,name,8);
            if(output->file_header.Machine!=0x601) {
                if(librarian_undecorated_exports && *name!='?' && *name!='@')++name;
                if(symbol->flags & 0x10) {
                    if(*name=='?' || *name=='@' || (underscore_symbols && *name=='_'))++name;
                    copy=duplicate_argument(name);
                    suffix=strchr(copy,'@');
                    if(suffix)*suffix=0;
                    name=copy;
                }
            }
            if(relocation_values)*relocation_values++=buffer->used;
            else {
                *(DWORD *)(buffer->data+names_offset)=buffer->used;
                names_offset+=4;
            }
            if(output->file_header.Machine==0x601 && symbol->diagnostic_name) {
                append_bytes(buffer,symbol->diagnostic_name,strlen(symbol->diagnostic_name));
                append_bytes(buffer,"!",1);
            }
            append_bytes(buffer,name,strlen(name)+1);
            if(output->file_header.Machine!=0x601 && (symbol->flags & 0x10))
                release_memory((void *)name);
            *(WORD *)(buffer->data+ordinals_offset)=(WORD)ordinal_index;
            ordinals_offset+=2;
        }
        if(symbol->flags & 0x200)
            symbol->final_value=append_bytes(buffer,symbol->diagnostic_name,
                                            strlen(symbol->diagnostic_name)+1);
    }
}

#pragma intrinsic(strcmp)
extern "C" void __stdcall sort_export_names(BYTE *data,DWORD *names,DWORD ordinals_offset,DWORD count)
{
    DWORD first,next,value;
    WORD ordinal,*ordinals=(WORD *)(data+ordinals_offset);
    default_phase="ReSortExportNamePtrs";
    if(count<=1)return;
    for(first=0;first<count-1;++first) {
        for(next=first+1;next<count;++next) {
            if(strcmp((const char *)data+names[next],(const char *)data+names[first])<0) {
                value=names[first];names[first]=names[next];names[next]=value;
                ordinal=ordinals[first];ordinals[first]=ordinals[next];ordinals[next]=ordinal;
            }
        }
    }
}

extern "C" DWORD __stdcall intern_symbol_string(StringTable *,const char *);
#pragma intrinsic(strlen)
void RECON_THISCALL LibrarianContext::InternExportNames(void)
{
    StringTable *symbols;
    StringNode **order,*symbol;
    DWORD index,count;
    default_phase="AddInternalNamesToStringTable";
    if(!image)return;
    symbols=image->symbols;
    order=symbols->GetSortedSymbols();
    count=count_hash_symbols(symbols->hash.state);
    for(index=0;index<count;++index) {
        symbol=order[index];
        if((symbol->flags & 1) && !(symbol->flags & 4) && symbol->diagnostic_name &&
           !(symbol->flags & 0x200) && strlen(symbol->diagnostic_name)>8)
            intern_symbol_string(symbols,symbol->diagnostic_name);
    }
}

void RECON_THISCALL LibrarianContext::EmitExportRelocations(int output,DWORD named_count,
    DWORD functions_offset,DWORD names_offset)
{
    StringTable *symbols;
    StringNode **order,*symbol;
    IMAGE_RELOCATION relocation;
    WORD type;
    DWORD index,count,symbol_index=named_count+6,name_symbol_index=6;
    if(!image)return;
    type=thunk->reference_types[0];
    symbols=image->symbols;
    default_phase="EmitExportDataFixups";
    order=symbols->GetSortedSymbols();
    count=count_hash_symbols(symbols->hash.state);
    for(index=0;index<count;++index) {
        symbol=order[index];
        if(!(symbol->flags & 1) || (symbol->flags & 0x800))continue;
        relocation.VirtualAddress=functions_offset+((WORD)symbol->symbol.Value-librarian_next_ordinal)*4;
        relocation.SymbolTableIndex=symbol_index++;
        relocation.Type=type;
        write_relocations(output,&relocation,1);
        if(!(symbol->flags & 0x8000)) {
            relocation.VirtualAddress=names_offset;
            relocation.SymbolTableIndex=name_symbol_index++;
            relocation.Type=type;
            write_relocations(output,&relocation,1);
            names_offset+=4;
        }
    }
}

extern "C" {
extern DWORD emitted_symbol_count;
void __stdcall write_coff_symbol(int,IMAGE_SYMBOL *);
}
#pragma intrinsic(strlen)
void RECON_THISCALL LibrarianContext::EmitExportSymbols(int output)
{
    StringTable *symbols;
    StringNode **order,*symbol;
    Contribution *contribution;
    IMAGE_SYMBOL raw;
    DWORD index,count;
    default_phase="EmitDefLibExternals";
    if(!image)return;
    symbols=image->symbols;
    order=symbols->GetSortedSymbols();
    count=count_hash_symbols(symbols->hash.state);
    for(index=0;index<count;++index) {
        symbol=order[index];
        if(!(symbol->flags & 1) || (symbol->flags & 0x800) || (symbol->flags & 4))continue;
        raw=symbol->symbol;
        if(symbol->diagnostic_name && !(symbol->flags & 0x200)) {
            if(strlen(symbol->diagnostic_name)>8) {
                raw.N.Name.Short=0;
                raw.N.Name.Long=intern_symbol_string(symbols,symbol->diagnostic_name);
            } else strncpy((char *)raw.N.ShortName,symbol->diagnostic_name,8);
        }
        raw.Value=symbol->final_value;
        if(symbol->flags & 0x200) {
            raw.SectionNumber=1;
            raw.StorageClass=IMAGE_SYM_CLASS_STATIC;
        } else if(symbol->contribution) {
            contribution=symbol->contribution;
            while(contribution && (contribution->characteristics & 0x4000))
                contribution=contribution->associated_contribution;
            if(contribution)raw.SectionNumber=contribution->group->section->index;
        }
        raw.Type=0;
        raw.NumberOfAuxSymbols=0;
        write_coff_symbol(output,&raw);
        symbol->flags|=4;
        ++emitted_symbol_count;
    }
}
