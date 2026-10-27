/* Search archive members for the current undefined-symbol list. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mbstring.h>
#include "../crt/crt-imports.h"
#include "input.h"
#include "pass-one.h"
#include "options.h"
#include "files.h"
#include "sections.h"
#include "full-link-functions.h"
#include "memory.h"
#include "comdat.h"
#include "librarian.h"
#include <recon.h>

#pragma intrinsic(strcmp,strlen)

typedef struct UndefinedEnumerator {
    BYTE (__stdcall *next)(struct UndefinedEnumerator *);
    void (__stdcall *end)(struct UndefinedEnumerator *);
    StringNode *next_symbol,*current;
} UndefinedEnumerator;
typedef struct SymbolReferenceEnumerator {
    BYTE (__stdcall *next)(struct SymbolReferenceEnumerator *);
    void (__stdcall *end)(struct SymbolReferenceEnumerator *);
    StringNode *symbol;
    SymbolAuxiliary *block;
    DWORD index;
    OutputModule *current;
} SymbolReferenceEnumerator;
typedef char UndefinedEnumeratorSize[(sizeof(UndefinedEnumerator)==16)?1:-1];
typedef char ReferenceEnumeratorSize[(sizeof(SymbolReferenceEnumerator)==24)?1:-1];
typedef CoffLibrarianContext LibraryConversion;
/* The two producer BYTEs were physically forwarded in DWORD stack slots.
 * No preceding own stores define their three upper bytes. Keep those original
 * unwritten bytes zero, preserving the complete transport slots. */
typedef union ByteTransport { DWORD value; BYTE low; } ByteTransport;

extern "C" {
extern BYTE verbose_link,verbose_libraries,incremental_reuse,incremental_enabled;
extern BYTE undefined_list_changed;
extern DWORD archive_member_size;
DWORD __stdcall archive_big_endian_dword(const void *);
extern char short_symbol_name[9];
int __cdecl compare_archive_symbol_names(const void *key,const void *element)
{
    return strcmp(*(const char *const *)key,*(const char *const *)element);
}
IMAGE_ARCHIVE_MEMBER_HEADER *__cdecl read_archive_member_header(void);
/* This result is overwritten by each call; its shared address is observable. */
static char archive_name[17];
const char *__stdcall archive_member_name(const IMAGE_ARCHIVE_MEMBER_HEADER *header,const char *long_names)
{
    char *end;
    BYTE special;
    strncpy(archive_name,(const char *)header->Name,16);
    archive_name[16]=0;
    if (archive_name[0]=='/') {
        special=archive_name[1]==' ' || archive_name[1]=='/';
        end=strchr(archive_name,' ');
        if (!end) return NULL;
        *end=0;
        if (!special)
            return long_names+atoi(archive_name+1);
    } else {
        end=strrchr(archive_name,'/');
        if (!end) return NULL;
        *end=0;
    }
    return archive_name;
}
void __stdcall initialize_undefined_enumerator(UndefinedEnumerator *e,StringTable *table);
BYTE __stdcall next_undefined(UndefinedEnumerator *e)
{
    e->current=e->next_symbol;
    if (!e->current) return 0;
    e->next_symbol=e->current->undefined_next;
    return 1;
}
BYTE __stdcall next_symbol_reference(SymbolReferenceEnumerator *e)
{
    if(!e->symbol)return 0;
    if(!(e->symbol->flags&0x1000)) {
        e->current=(OutputModule *)e->symbol->reference_owner;
        e->symbol=NULL;
        return (BYTE)(e->current!=NULL);
    }
    while(e->block) {
        while(e->index<4 && !e->block->owners[e->index])++e->index;
        if(e->index!=4) {
            e->current=(OutputModule *)e->block->owners[e->index++];
            return 1;
        }
        e->block=e->block->next;
        e->index=0;
    }
    return 0;
}
void __stdcall initialize_symbol_reference_enumerator(SymbolReferenceEnumerator *e,StringNode *symbol)
{
    e->next=next_symbol_reference;
    e->end=(void (__stdcall *)(SymbolReferenceEnumerator *))end_section_enumerator;
    e->symbol=symbol;
    if(symbol->flags&0x1000) {
        e->index=0;
        e->block=symbol->auxiliary;
    }
    /* The single-owner representation does not initialize block/index;
     * current is written by the genuine next function before consumption. */
}
char *__cdecl __unDName(char *,const char *,int,
    void *(__cdecl *)(size_t),void (__cdecl *)(void *),unsigned short);
#pragma intrinsic(strlen,strcpy,strcat)
char *__stdcall display_symbol_name(const char *name,DWORD flags)
{
    const char *symbol=name;
    char *decoded,*display;
    BYTE imported,glue=0,native_entry,function_header=0,call_table=0;
    unsigned extra=0,bytes;
    imported=!strncmp(name,"__imp_",6);
    if(imported)symbol=name+6;
    else {
        glue=!strncmp(name,"__glue_",6);
        if(glue)symbol=name+7;
    }
    native_entry=!strncmp(symbol,"__nep",5);
    if(native_entry)symbol+=5;
    else {
        function_header=!strncmp(symbol,"__fh",4);
        if(function_header)symbol+=4;
        else {
            call_table=!strncmp(symbol,"__pcd_tbl",9);
            if(call_table)symbol+=9;
        }
    }
    if(*symbol!='?')return (char *)name;
    decoded=__unDName(NULL,symbol,0,malloc,free,0x800);
    if(!decoded)return (char *)name;
    if(imported)extra=22;
    else if(glue)extra=17;
    if(native_entry)extra+=15;
    else if(function_header)extra+=18;
    else if(call_table)extra+=19;
    /* The physical flags slot is DWORD; only AL selects this display form. */
    bytes=strlen(name)+strlen(decoded)+extra+((BYTE)flags?6:4);
    display=(char *)allocate_memory(bytes);
    if((BYTE)flags)strcpy(display,"\"");
    else { strcpy(display,name); strcat(display," ("); }
    if(imported)strcat(display,"__declspec(dllimport) ");
    else if(glue)strcat(display,"[Cross TOC Glue] ");
    if(native_entry)strcat(display,"[Native Entry] ");
    else if(function_header)strcat(display,"[Function Header] ");
    else if(call_table)strcat(display,"[PCode Call Table] ");
    strcat(display,decoded);
    if((BYTE)flags) { strcat(display,"\" ("); strcat(display,name); }
    strcat(display,")");
    /* The original retains the CRT undecoration allocation here. */
    return display;
}
/* 4808b0 is the separate /NODELAYLOAD exclusion list. */
InputArguments delay_load_exclusions;
BYTE __stdcall is_delay_load_excluded(const char *name)
{ __asm { int 3 } return 0; }
BYTE __stdcall select_delay_load_library(const char *name)
{
    InputArgument *argument=delay_libraries.first;
    unsigned index;
    for (index=0;argument && index<delay_libraries.count;++index) {
        if (argument->name && !_stricmp(argument->name,name) &&
            !is_delay_load_excluded(name)) {
            argument->flags |= 0x200;
            return 1;
        }
        argument=argument->next;
    }
    return 0;
}
extern IMAGE_SECTION_HEADER *module_section_headers;
extern unsigned module_section_capacity;
extern BYTE file_access_statistics;
void __stdcall resize_module_section_buffers(unsigned count);
IMAGE_SECTION_HEADER *__stdcall find_section_by_name(IMAGE_SECTION_HEADER *sections,
    DWORD count,const char *name,BYTE *duplicate)
{
    IMAGE_SECTION_HEADER *found=NULL,*end=sections+count;
    for (;sections<end;++sections) {
        if (!strncmp((const char *)sections->Name,name,strlen(name))) found=sections;
        else if (duplicate && !strncmp((const char *)sections->Name,".text",5))
            *duplicate=1;
    }
    return found;
}
IMAGE_SECTION_HEADER *__stdcall find_member_section(int file,IMAGE_FILE_HEADER *header,
    const char *name,BYTE *duplicate)
{
    unsigned count=header->NumberOfSections,bytes;
    DWORD error=0;
    IMAGE_SECTION_HEADER *sections;
    FileRecord *record;
    if (!count) return NULL;
    if (module_section_capacity<count) resize_module_section_buffers(count);
    seek_file(file,header->SizeOfOptionalHeader,1,&error);
    sections=module_section_headers;
    bytes=count*sizeof(IMAGE_SECTION_HEADER);
    if (file_access_statistics) record_file_access();
    record=file_slots[file];
    if (record->flags & 8) mapped_read(record,sections,bytes);
    else buffered_read(record,sections,bytes);
    return find_section_by_name(module_section_headers,count,name,duplicate);
}
BYTE __stdcall member_contains_symbol(int file,IMAGE_FILE_HEADER *header,DWORD convert,
    const char *member,const char *symbol)
{
    BYTE duplicate=0;
    if (!find_member_section(file,header,".idata$5",&duplicate)) return 0;
    /* The physical third argument is a DWORD; this body consumes only AL. */
    if (!duplicate && (BYTE)convert)
        link_fatal(NULL,1194,member,display_symbol_name(symbol,1),member);
    return 1;
}
void __stdcall convert_library_member(LinkContext *context,OutputLibrary *library,
    IMAGE_FILE_HEADER *header,void *import_header,int file,DWORD convert,DWORD is_import,
    const char *symbol,const char *member,IMAGE_SECTION_HEADER *sections,BYTE *section_flags,
    void *section_data,LibraryConversion **result,DWORD option1,DWORD option2)
{
    LibraryConversion *conversion=get_import_conversion(context,library,file,
        (CoffImportHeader *)import_header,header,member,is_import);
    LibrarianImportInfo *info=(LibrarianImportInfo *)section_data;
    CoffImportHeader *import=(CoffImportHeader *)import_header;
    *result=conversion;
    info->machine=import->machine;
    info->type=(BYTE)import->kind;
    info->name_type=import->name_type;
    info->ordinal=import->ordinal_hint;
    if((BYTE)convert) {
        /* The load into AL preserves the carried member-pointer upper bits;
         * the pending consumer has not qualified their semantic use. */
        DWORD library_flags=((DWORD)member&0xffffff00)|library->active;
        conversion->ConvertDelayedMember(library_flags,is_import,import,file,
            header,member,symbol,sections,section_flags,info);
        library->active=0;
    } else conversion->ConvertShortMember(file,import,option1,option2,header,
        sections,section_flags,info);
}
void __stdcall record_incremental_module(OutputModule *module)
{ __asm { int 3 } }
}
extern "C" void __stdcall initialize_undefined_enumerator(UndefinedEnumerator *e,StringTable *table)
{
    e->next=next_undefined;
    e->end=(void (__stdcall *)(UndefinedEnumerator *))end_section_enumerator;
    e->next_symbol=table->undefined_first;
}
BYTE RECON_THISCALL OutputLibrary::FindSymbolMember(LinkContext *context,StringNode *symbol,
    const char *name,DWORD *offset)
{
    const char **found;
    DWORD index;
    if (flags & 1) {
        found=(const char **)bsearch(&name,index_data[3],archive_symbol_count,4,
            compare_archive_symbol_names);
        if (!found) return 0;
        index=found-(const char **)index_data[3];
    } else {
        for (index=0;index<archive_symbol_count;++index)
            if (!strcmp(((const char **)index_data[3])[index],name)) break;
        if (index==archive_symbol_count) return 0;
    }
    if (flags & 1)
        *offset=((DWORD *)index_data[0])[((WORD *)index_data[1])[index]];
    else *offset=archive_big_endian_dword((DWORD *)index_data[0]+index);
    return 1;
}
void RECON_THISCALL OutputLibrary::RememberSymbol(StringNode *symbol)
{ __asm { int 3 } }
const char *RECON_THISCALL OutputLibrary::MemberName(const IMAGE_ARCHIVE_MEMBER_HEADER *header)
{
    return archive_member_name(header,(const char *)index_data[4]);
}

void RECON_THISCALL OutputLibrary::SearchSymbols(LinkContext *context,
    BYTE *changed,BYTE *keep_searching)
{
    UndefinedEnumerator symbols;
    SymbolReferenceEnumerator references;
    StringNode *symbol,*previous;
    const char *symbol_name,*member_name,*identity;
    char *display_name;
    char reference_name[260],loaded_name[260];
    IMAGE_ARCHIVE_MEMBER_HEADER *member_header;
    IMAGE_FILE_HEADER header;
    BYTE import_header[20],section_flags[32],section_data[60];
    IMAGE_SECTION_HEADER sections[30];
    OutputModule *module;
    LibraryConversion *conversion;
    DWORD saved_file,member_start,file_length,section_offset;
    WORD name_kind;
    BYTE converted,loaded;
    ByteTransport convert={0},is_import={0};
    if (flags & 4) return;
    if (name && verbose_link) {
        fputs("    ",stdout);
        link_message(9009,name);
    }
    *keep_searching=0;
    initialize_undefined_enumerator(&symbols,context->symbols);
    while (next_undefined(&symbols)) {
        symbol=symbols.current;
        saved_file=0;
        converted=0;
        if ((symbol->flags & 0x4140) || symbol->symbol.SectionNumber ||
            symbol->symbol.Value) continue;
        symbol_name=get_symbol_name(symbol,context->symbols->hash.string_storage,&name_kind,NULL);
        if (name_kind==1) symbol_name=strncpy(short_symbol_name,symbol_name,8);
        if (!FindSymbolMember(context,symbol,symbol_name,&archive_member_offset)) {
            *keep_searching=1;
            continue;
        }
        if (incremental_reuse) RememberSymbol(symbol);
        if (verbose_libraries) {
            display_name=display_symbol_name(symbol_name,1);
            fputs("      ",stdout);
            link_message(9010,display_name);
            if (display_name!=symbol_name) free(display_name);
            initialize_symbol_reference_enumerator(&references,symbol);
            while (next_symbol_reference(&references)) {
                fputs("        ",stdout);
                link_message(9011,format_module_name(references.current,reference_name));
            }
        }
        previous=symbol==context->symbols->undefined_first ? NULL :
            (StringNode *)((BYTE *)symbol->undefined_previous-0x20);
        flags |= 2;
        current_input_file=open_argument_file(name,0x8000,0);
        archive_member_size=0;
        file_length=get_file_length(current_input_file);
        if (file_length<archive_member_offset) link_fatal(name,1127,0);
        member_header=read_archive_member_header();
        member_start=archive_member_offset;
        if (file_length<archive_member_offset+archive_member_size) link_fatal(name,1127,0);
        identity=MemberName(member_header);
        if (!identity) link_fatal(name,1127,0);
        member_name=MemberName(member_header);
        convert.low=select_delay_load_library(member_name);
        read_coff_header(current_input_file,&header,import_header,&is_import.low);
        conversion=NULL;
        section_offset=20;
        if (!is_import.low)
            convert.low &= member_contains_symbol(current_input_file,&header,convert.value,
                member_name,symbol_name);
        if (is_import.low || convert.low) {
            if (is_import.low && convert.low && (import_header[18] & 3))
                link_fatal(NULL,1194,member_name,display_symbol_name(symbol_name,1),member_name);
            convert_library_member(context,this,&header,import_header,current_input_file,
                convert.value,is_import.value,symbol_name,member_name,sections,section_flags,
                section_data,&conversion,0,0);
            converted=1;
            section_offset=0;
            saved_file=current_input_file;
            current_input_file=conversion->output_file;
        }
        module=create_input_module(NULL,identity,(const void *)member_start,&header,this,NULL,
            is_import.low || convert.low,section_offset,sections,section_flags,section_data,
            (DWORD)conversion);
        if (is_import.low && *(WORD *)(import_header+4)==0) module->compiler_id=0x10000;
        if (incremental_enabled) record_incremental_module(module);
        if (verbose_libraries) {
            fputs("        ",stdout);
            link_message(9012,format_module_name(module,loaded_name));
        }
        if (context->options.effective_reference_opt || context->options.fold_identical_comdats)
            allocate_module_section_headers(module);
        loaded=0;
        load_object_module(context,&loaded,module,0);
        if (converted) current_input_file=saved_file;
        close_argument_file(current_input_file,0);
        if (incremental_enabled && incremental_status_code) return;
        if (loaded) *changed=1;
        if (previous && !(previous->flags & 1) && !undefined_list_changed)
            symbols.next_symbol=previous->undefined_next;
        else {
            symbols.next_symbol=context->symbols->undefined_first;
            undefined_list_changed=0;
        }
    }
}

extern "C" {
extern OutputModule *output_module;
extern DWORD fixup_count;
extern ComdatRoot *comdat_roots;
void __stdcall define_input_symbol(StringNode *,Contribution *,DWORD,SHORT,WORD,DWORD,OutputModule *,StringTable *);
}
void RECON_THISCALL LinkContext::ResolveLocalImports(Contribution **contribution,IncludeSymbol **aliases)
{
    UndefinedEnumerator enumerator;
    StringNode *symbol,*defined;
    const char *name;
    char *display;
    WORD name_kind;
    DWORD offset;
    IncludeSymbol **last=aliases;
    *contribution=NULL;
    initialize_undefined_enumerator(&enumerator,symbols);
    while(next_undefined(&enumerator)) {
        symbol=enumerator.current;
        if(symbol->flags & 0x100)continue;
        name=get_symbol_name(symbol,symbols->hash.string_storage,&name_kind,NULL);
        if(name_kind==1)name=strncpy(short_symbol_name,name,8);
        if(strncmp(name,"__imp_",6))continue;
        name+=6;
        defined=symbols->FindSymbol(name);
        if(!defined || !(defined->flags & 1))continue;
        display=display_symbol_name(name,1);
        link_warning(NULL,4049,display);
        if(display!=name)free(display);
        if(!*contribution) {
            *contribution=create_contribution(".rdata",0,0x40300040,0x40300040,
                output_module,&sections,this);
            if(options.effective_reference_opt || options.fold_identical_comdats)
                attach_comdat(*contribution,NULL,1);
        }
        offset=(*contribution)->raw_size;
        (*contribution)->raw_size=offset+4;
        define_input_symbol(symbol,*contribution,offset,0,0,0,output_module,symbols);
        *last=(IncludeSymbol *)allocate_memory(8);
        (*last)->symbol=symbol;
        last=&(*last)->next;
        *last=(IncludeSymbol *)allocate_memory(8);
        (*last)->symbol=defined;
        last=&(*last)->next;
        ++fixup_count;
        if(!options.fixed_image && !options.rom)
            ++optional_header.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size;
        if(options.effective_reference_opt || options.fold_identical_comdats)
            add_comdat_root(NULL,defined,NULL,&comdat_roots);
    }
    *last=NULL;
}

void RECON_THISCALL LinkContext::ResolveUndefined(void)
{
    WORD machine=file_header.Machine;
    const char *name=(machine==0x14c || machine==0x1a2 || machine==0x1a6) ?
        "___ImageBase" : "__ImageBase";
    StringNode *symbol=symbols->FindSymbol(name);
    if(symbol && !(symbol->flags & 1))
        define_input_symbol(symbol,NULL,0,-2,0,0,output_module,symbols);
}

extern "C" {
extern BYTE mac_target,verbose_libraries,file_access_statistics;
extern DWORD undefined_symbols;
/* Set by the as-yet unreconstructed Mac library-binding paths. */
BYTE mac_library_binding_seen;
void __cdecl link_unresolved_error(const char *file,unsigned message,...);
}
extern "C" WORD __stdcall get_hash_flags(HashState *state)
{ return state->flags; }
extern "C" void __stdcall set_hash_flags(HashState *state,WORD flags)
{ state->flags=flags; }
void RECON_THISCALL StringTable::ReleaseLookupIndex(void)
{
    WORD flags=get_hash_flags(hash.state);
    if(flags & 2) {
        set_hash_flags(hash.state,(WORD)(flags & ~2));
        if(hash.lookup_buffers[1]) {
            release_memory(hash.lookup_buffers[1]);
            hash.lookup_buffers[1]=NULL;
        }
        if(hash.lookup_buffers[2]) {
            release_memory(hash.lookup_buffers[2]);
            hash.lookup_buffers[2]=NULL;
        }
        if(hash.lookup_buffers[0]) {
            release_memory(hash.lookup_buffers[0]);
            hash.lookup_buffers[0]=NULL;
        }
    }
}

void RECON_THISCALL StringTable::SortSymbols(void)
{
    UndefinedEnumerator enumerator;
    SymbolReferenceEnumerator references;
    StringNode *symbol;
    const char *name;
    char *display;
    char module_name[520];
    WORD name_kind;
    BYTE referenced;
    if(mac_target && mac_library_binding_seen && !verbose_libraries)
        link_warning(NULL,4546);
    initialize_undefined_enumerator(&enumerator,this);
    while(next_undefined(&enumerator)) {
        symbol=enumerator.current;
        if((symbol->flags & 0x100) || (symbol->flags & 1))continue;
        name=symbol->diagnostic_name;
        if(name) {
            if(symbol->flags & 0x200) {
                name=strchr(name,'.')+1;
                if(*name=='#') {
                    name=get_symbol_name(symbol,hash.string_storage,&name_kind,NULL);
                    if(name_kind==1)name=strncpy(short_symbol_name,name,8);
                }
            }
        } else {
            name=get_symbol_name(symbol,hash.string_storage,&name_kind,NULL);
            if(name_kind==1)name=strncpy(short_symbol_name,name,8);
        }
        ++undefined_symbols;
        display=display_symbol_name(name,1);
        referenced=0;
        initialize_symbol_reference_enumerator(&references,symbol);
        while(next_symbol_reference(&references)) {
            referenced=1;
            if(!strncmp(name,"___@@_PchSym_@",14) || !strncmp(name,"__@@_PchSym_@",13)) {
                link_unresolved_error(format_module_name(references.current,module_name),2011);
                end_section_enumerator((SectionEnumerator *)&references);
                break;
            }
            link_unresolved_error(format_module_name(references.current,module_name),2001,display);
        }
        if(!referenced)link_unresolved_error(NULL,2001,display);
        if(display!=name)free(display);
        if(file_access_statistics)record_file_access();
    }
    end_section_enumerator((SectionEnumerator *)&enumerator);
    ReleaseLookupIndex();
}

extern "C" StringNode *__stdcall find_contribution_symbol(LinkContext *context,Contribution *contribution)
{
    StringTable *symbols=context->symbols;
    StringNode **order=symbols->GetSymbolsByValue(),*symbol=NULL;
    DWORD count=count_hash_symbols(symbols->hash.state),index;
    BYTE found=0;
    for(index=0;index<count && !found;++index) {
        symbol=order[index];
        found=symbol->contribution==contribution;
    }
    symbols->ReleaseLookupIndex();
    return found?symbol:NULL;
}
