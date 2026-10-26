#include <windows.h>
#include <string.h>
#include <stdio.h>
#include "link.h"
#include "files.h"
#include "symbols.h"
#include "link-driver.h"
#include "full-link-functions.h"
#include <recon.h>

extern "C" {
extern ByteBuffer symbol_comparison_strings;
StringNode **__stdcall build_symbol_order(StringTable *,int (__cdecl *)(const void *,const void *));
int __cdecl compare_symbol_members(const void *,const void *);
}
StringNode **RECON_THISCALL StringTable::GetSymbols(void)
{
    if (!hash.lookup_buffers[2]) {
        symbol_comparison_strings=hash.strings;
        hash.lookup_buffers[2]=build_symbol_order(this,compare_symbol_members);
    }
    return (StringNode **)hash.lookup_buffers[2];
}

extern "C" {
extern const char *default_phase;
extern BYTE file_access_statistics;
extern char short_symbol_name[9];
}

#pragma intrinsic(strlen)
extern "C" void __stdcall write_archive_symbol_names(int output,StringTable *symbols,DWORD sorted)
{
    const char *saved_phase=default_phase;
    StringNode **order;
    StringNode *symbol;
    const char *name;
    WORD kind;
    unsigned count,index,length;
    FileRecord *file;
    default_phase="EmitStrings";
    order=(BYTE)sorted?symbols->GetSortedSymbols():symbols->GetSymbols();
    count=count_hash_symbols(symbols->hash.state);
    for (index=0;index<count;++index) {
        symbol=order[index];
        if ((symbol->flags & 0x102000) || !(symbol->flags & 0x4001)) continue;
        name=get_symbol_name(symbol,symbols->hash.string_storage,&kind,NULL);
        if (kind==1) name=strncpy(short_symbol_name,name,8);
        length=strlen(name)+1;
        if (file_access_statistics) record_file_access();
        file=file_slots[output];
        if (file->flags & 8) mapped_write(file,name,length);
        else buffered_write(file,name,length);
    }
    default_phase=saved_phase;
}

#pragma intrinsic(strcmp)
extern "C" int __cdecl compare_symbol_members(const void *left,const void *right)
{
    StringNode *first=*(StringNode * const *)left;
    StringNode *second=*(StringNode * const *)right;
    const char *left_name,*right_name;
    char left_short[9],right_short[9];
    if (first->input_flags<second->input_flags) return -1;
    if (first->input_flags>second->input_flags) return 1;
    if (!first->symbol.N.Name.Short)
        left_name=(const char *)symbol_comparison_strings.data+first->symbol.N.Name.Long;
    else {
        left_name=strncpy(left_short,(const char *)&first->symbol,8);
        left_short[8]=0;
    }
    if (!second->symbol.N.Name.Short)
        right_name=(const char *)symbol_comparison_strings.data+second->symbol.N.Name.Long;
    else {
        right_name=strncpy(right_short,(const char *)&second->symbol,8);
        right_short[8]=0;
    }
    return strcmp(left_name,right_name);
}

#pragma intrinsic(memcpy,memset)
extern "C" void __stdcall write_archive_header(int output,const char *name,
    DWORD reference,DWORD timestamp,DWORD mode,DWORD size,IMAGE_ARCHIVE_MEMBER_HEADER *destination)
{
    IMAGE_ARCHIVE_MEMBER_HEADER local_header,*header;
    const char *saved_phase=default_phase;
    unsigned length;
    FileRecord *file;
    default_phase="WriteMemberHeader";
    header=destination?destination:&local_header;
    length=strlen(name);
    if ((BYTE)reference) {
        header->Name[0]='/';
        memcpy(header->Name+1,name,length);
    } else {
        memcpy(header->Name,name,length);
        header->Name[length]='/';
    }
    ++length;
    memset(header->Name+length,' ',16-length);
    sprintf((char *)header->Date,"%-12ld",(LONG)timestamp);
    memset(header->UserID,' ',6);
    memset(header->GroupID,' ',6);
    sprintf((char *)header->Mode,"%-8ho",(WORD)mode);
    sprintf((char *)header->Size,"%-10ld",(LONG)size);
    header->EndHeader[0]='`';
    header->EndHeader[1]='\n';
    if (!destination) {
        if (file_access_statistics) record_file_access();
        file=file_slots[output];
        if (file->flags & 8) mapped_write(file,header,60);
        else buffered_write(file,header,60);
    }
    default_phase=saved_phase;
}

extern "C" {
extern DWORD *librarian_ordinal_symbols;
DWORD __stdcall archive_big_endian_count(const DWORD *);
}
extern "C" void __stdcall write_archive_member_offsets(int output,StringTable *symbols,DWORD sorted)
{
    const char *saved_phase=default_phase;
    StringNode **order,*symbol;
    DWORD count,index,offset;
    FileRecord *file;
    default_phase="EmitOffsets";
    order=(BYTE)sorted?symbols->GetSortedSymbols():symbols->GetSymbols();
    count=count_hash_symbols(symbols->hash.state);
    for(index=0;index<count;++index) {
        symbol=order[index];
        if((symbol->flags&0x102000) || !(symbol->flags&0x4001))continue;
        if((BYTE)sorted) {
            if(file_access_statistics)record_file_access();
            file=file_slots[output];
            if(file->flags&8)mapped_write(file,&symbol->input_flags,2);
            else buffered_write(file,&symbol->input_flags,2);
        } else {
            offset=archive_big_endian_count(&librarian_ordinal_symbols[symbol->input_flags]);
            if(file_access_statistics)record_file_access();
            file=file_slots[output];
            if(file->flags&8)mapped_write(file,&offset,4);
            else buffered_write(file,&offset,4);
        }
    }
    default_phase=saved_phase;
}

extern "C" int __cdecl compare_symbol_values(const void *,const void *);
StringNode **RECON_THISCALL StringTable::GetSymbolsByValue(void)
{
    if(!hash.lookup_buffers[1])
        hash.lookup_buffers[1]=build_symbol_order(this,compare_symbol_values);
    return (StringNode **)hash.lookup_buffers[1];
}
extern "C" int __cdecl compare_symbol_values(const void *left,const void *right)
{
    const StringNode *a=*(StringNode *const *)left;
    const StringNode *b=*(StringNode *const *)right;
    return (int)(a->final_value-b->final_value);
}
