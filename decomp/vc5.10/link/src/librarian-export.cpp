/* Define the real export symbol and its synthetic module/contribution owner. */
#include <windows.h>
#include <string.h>
#include <stdlib.h>
#include <mbstring.h>
#include "librarian.h"
#include "input.h"
#include "pass-one.h"
#include "full-link-functions.h"
#include <recon.h>
extern "C" {
/* The period SDK omits this identified private six-argument CRT export. */
char *__cdecl __unDName(char *,const char *,int,
    void *(__cdecl *)(size_t),void (__cdecl *)(void *),unsigned short);
extern char *program_name;
extern const char *definition_file;
extern BYTE duplicate_symbols,mac_target,librarian_undecorated_exports;
OutputLibrary *definition_library;
OutputModule *definition_module;
Contribution *definition_contribution;
SectionList definition_sections={NULL,&definition_sections.first};
void __stdcall record_librarian_ordinal(DWORD);
void __stdcall release_memory(void *);
char *__stdcall duplicate_argument(const char *);
void __stdcall check_export_name(StringNode *,const char *,const char *);
BYTE __stdcall same_librarian_export(StringNode *,const char *,const char *,DWORD,
    DWORD,DWORD,DWORD,DWORD,DWORD);
const char *__stdcall display_symbol_name(const char *,BYTE);
void __cdecl link_unresolved_error(const char *,unsigned,...);
}
#pragma intrinsic(strlen,strcpy)
extern "C" void __stdcall define_librarian_export(const char *name,const char *internal,
    DWORD noname,DWORD type,DWORD ordinal,const char *file,DWORD from_command_line,
    LinkContext *image,DWORD decorate,DWORD private_export)
{
    StringTable *symbols=librarian_context->image->symbols;
    StringNode *symbol;
    char *storage,*copy;
    const char *display;
    DWORD index;
    if(!file)file=program_name;
    if(!definition_library) {
        LinkPointerList libraries;
        IMAGE_FILE_HEADER header;
        libraries.first=NULL;
        libraries.last=&libraries.first;
        libraries.suppress_default_libraries=0;
        libraries.default_libraries=NULL;
        definition_library=create_library(NULL,NULL,&libraries);
        definition_library->flags|=0x14;
        /* Actual 20-byte caller-local COFF header, all fields defined. */
        header.Machine=0;
        header.NumberOfSections=1;
        header.TimeDateStamp=0;
        header.PointerToSymbolTable=0;
        header.NumberOfSymbols=0;
        header.SizeOfOptionalHeader=0;
        header.Characteristics=0;
        definition_module=create_input_module(file,file,NULL,&header,definition_library,
            NULL,0,20,NULL,NULL,NULL,0);
        definition_contribution=create_contribution("",0,0,0,definition_module,
            &definition_sections,image);
    }
    storage=(char *)allocate_memory(strlen(name)+2);
    copy=storage;
    if((BYTE)decorate && *name!='?' && *name!='@')*copy++='_';
    strcpy(copy,name);
    symbol=symbols->Lookup((const unsigned char *)storage,NULL);
    if(symbol->flags&1) {
        if(symbol->flags&0x400) {
            if(same_librarian_export(symbol,name,internal,noname,type,ordinal,
                from_command_line,decorate,private_export))goto alias;
            link_warning(file,4197,name);
            return;
        }
        display=display_symbol_name(storage,1);
        link_unresolved_error(file,2005,display,file);
        duplicate_symbols=1;
        if(display!=storage)release_memory((void *)display);
    }
    symbol->flags|=0x400;
    set_symbol_defined(symbol,1,symbols);
    if(type==1)symbol->flags|=8;
    else if(type==2)symbol->flags|=0x2000;
    symbol->final_value=0;
    if((BYTE)private_export)symbol->input_flags=0;
    else {
        index=librarian_context->reserved04+1;
        if(index>65535)link_fatal(NULL,1189,65535);
        ++librarian_context->reserved04;
        symbol->input_flags=(WORD)librarian_context->reserved04;
    }
    symbol->symbol.Value=ordinal;
    symbol->contribution=definition_contribution;
    if(ordinal)record_librarian_ordinal(ordinal);
    if((BYTE)from_command_line && !strchr(storage,'@'))symbol->flags|=0x10;
    if((BYTE)noname)symbol->flags|=0x8000;
    if((BYTE)private_export)symbol->flags|=0x100000;
    check_export_name(symbol,storage,file);
alias:
    if(!internal)return;
    if(strchr(internal,'.')) {
        symbol->flags|=0x200;
        symbol->diagnostic_name=duplicate_argument(internal);
        return;
    }
    copy=(char *)allocate_memory(strlen(internal)+2);
    storage=copy;
    if((BYTE)decorate && *internal!='?' && *internal!='@')*copy++='_';
    strcpy(copy,internal);
    symbol->diagnostic_name=storage;
}

extern "C" void __stdcall check_export_name(StringNode *symbol,const char *name,const char *file)
{
    static const char *const registration_names[]={
        "DllCanUnloadNow","DllGetClassObject","DllInstall","DllRegisterServer",
        "DllRegisterServerEx","DllUnregisterServer",NULL
    };
    const char *const *entry;
    char *decoded;
    if(!_mbsnbcmp((const unsigned char *)name,(const unsigned char *)"??_",3)) {
        if(name[3]!='E' && name[3]!='G')return;
        decoded=__unDName(NULL,name,0,malloc,free,0x800);
        if(decoded)link_warning(file,4102,decoded);
        /* The original returns without freeing this optional decoded text. */
        return;
    }
    if(symbol->flags&0x100000)return;
    for(entry=registration_names;*entry;++entry)
        if(!_mbscmp((const unsigned char *)name,(const unsigned char *)*entry))
            link_warning(file,4104,name);
}

extern "C" char *__stdcall split_export_field(char *,const char *,const char *,BYTE *);
extern "C" void __stdcall add_librarian_export(const char *name,void *image,BYTE from_switch,const char *file)
{
    char *part,*digits,*after;
    const char *internal=NULL;
    BYTE separator=0,noname=0,private_export=0;
    DWORD ordinal=0,type=0;
    part=split_export_field((char *)name,"=,",file,&separator);
    if(separator=='=') {
        internal=part;
        part=split_export_field(part,",",file,&separator);
    }
    if(separator=='@') {
        digits=++part;
        after=split_export_field(part,",",file,&separator);
        if(sscanf(digits,"%lu",&ordinal)!=1 || !ordinal || ordinal>65535)
            link_fatal(file,1119,digits);
        part=after;
        if(!_strnicmp(part,"NONAME",6)) {
            noname=1;
            part+=6;
            if(*part==',') {
                if(!*++part)link_warning(file,4051);
            } else if(*part)link_fatal(file,1161);
        }
    }
    if(!_strnicmp(part,"DATA",4)) {
        type=mac_target?0:2;
        part+=4;
        if(*part==',') {
            if(!*++part)link_warning(file,4051);
        } else if(*part)link_fatal(file,1161);
    }
    if(!_strnicmp(part,"PRIVATE",7)) {
        private_export=1;
        part+=7;
        if(*part==',') {
            if(*++part)link_fatal(file,1161);
            link_warning(file,4051);
        } else if(*part)link_fatal(file,1161);
    }
    if(*part)link_fatal(file,1161);
    define_librarian_export(name,internal,noname,type,ordinal,file,from_switch,
        (LinkContext *)image,librarian_undecorated_exports,private_export);
}

extern "C" char *__stdcall split_export_field(char *text,const char *delimiters,
    const char *file,BYTE *separator)
{
    char *end=text+strcspn(text,delimiters);
    if(*end) {
        if(!end[1])link_warning(file,4051);
        *separator=*end;
        *end++=0;
    }
    if(*separator==',')*separator=*end;
    return end;
}

extern "C" void __stdcall merge_export_symbol(StringNode *,StringTable *,StringTable *,void *,DWORD);
#pragma intrinsic(memcpy)
extern "C" void __stdcall merge_librarian_exports(StringTable *exports,StringTable *objects,
    void *libraries,DWORD decoration)
{
    StringNode **sorted=exports->GetSortedSymbols();
    DWORD count=count_hash_symbols(exports->hash.state),index;
    StringNode **snapshot=(StringNode **)allocate_memory(count*4);
    memcpy(snapshot,sorted,count*4);
    exports->ReleaseLookupIndex();
    for(index=0;index<count;++index) {
        StringNode *symbol=snapshot[index];
        if((symbol->flag_byte&1) && !(symbol->flag_byte&0x10))
            merge_export_symbol(symbol,exports,objects,libraries,decoration);
    }
    release_memory(snapshot);
}

extern "C" {
ByteBuffer symbol_comparison_strings;
StringNode **__stdcall build_symbol_order(StringTable *,int (__cdecl *)(const void *,const void *));
int __cdecl compare_symbol_names(const void *,const void *);
}
StringNode **RECON_THISCALL StringTable::GetSortedSymbols(void)
{
    if(!hash.lookup_buffers[0]) {
        symbol_comparison_strings=hash.strings;
        hash.lookup_buffers[0]=build_symbol_order(this,compare_symbol_names);
    }
    return (StringNode **)hash.lookup_buffers[0];
}

extern "C" {
void __stdcall set_hash_flags(HashState *,WORD);
void *__stdcall zero_allocate(unsigned);
void __stdcall initialize_hash_enumerator(HashState *);
void __stdcall end_hash_enumerator(HashState *);
}
extern "C" StringNode **__stdcall build_symbol_order(StringTable *table,
    int (__cdecl *compare)(const void *,const void *))
{
    DWORD count,index;
    StringNode **symbols;
    set_hash_flags(table->hash.state,2);
    count=count_hash_symbols(table->hash.state);
    symbols=(StringNode **)zero_allocate(count*4);
    initialize_hash_enumerator(table->hash.state);
    for(index=0;index<count;++index)symbols[index]=table->NextSymbol();
    end_hash_enumerator(table->hash.state);
    qsort(symbols,count,4,compare);
    return symbols;
}

extern "C" HashEntry *__stdcall next_hash_entry(HashState *);
StringNode *RECON_THISCALL StringTable::NextSymbol(void)
{
    HashEntry *entry=next_hash_entry(hash.state);
    return entry?(StringNode *)entry->value:NULL;
}

#pragma intrinsic(strcmp)
extern "C" int __cdecl compare_symbol_names(const void *left,const void *right)
{
    StringNode *first=*(StringNode *const *)left;
    StringNode *second=*(StringNode *const *)right;
    char left_short[9],right_short[9];
    const char *left_name,*right_name;
    if(!first->symbol.N.Name.Short)
        left_name=(const char *)symbol_comparison_strings.data+first->symbol.N.Name.Long;
    else {
        left_name=strncpy(left_short,(const char *)&first->symbol,8);
        left_short[8]=0;
    }
    if(!second->symbol.N.Name.Short)
        right_name=(const char *)symbol_comparison_strings.data+second->symbol.N.Name.Long;
    else {
        right_name=strncpy(right_short,(const char *)&second->symbol,8);
        right_short[8]=0;
    }
    return strcmp(left_name,right_name);
}

/* Merge one export against object and archive symbol names. */
extern "C" {
BYTE ambiguous_exports;
void __stdcall find_export_candidates(StringTable *,const char *,WORD *,char **,const char **,DWORD);
extern char short_symbol_name[9];
}
#pragma intrinsic(strlen,strcmp,strcat)
extern "C" void __stdcall merge_export_symbol(StringNode *symbol,StringTable *exports,
    StringTable *objects,void *libraries,DWORD decoration)
{
    static char export_short_name[9];
    char *names[60],*selected,*replacement,*suffix;
    const char *sources[60],*name,*external,*display,*candidate;
    union { DWORD value; WORD low; } match_count;
    DWORD &count=match_count.value;
    DWORD index,length,bytes;
    WORD i,chosen=0,ordinary=0,last_ordinary=0,kind;
    BYTE all_questions=1,prefix=0;
    LibraryEnumerator enumerator;
    OutputLibrary *library;
    StringNode *resolved;
    count=0; /* Defined full DWORD; the candidate finder updates its low WORD. */
    name=symbol->diagnostic_name;
    if(name && (symbol->flags&0x200)) {
        name=strchr(name,'.')+1;
        if(*name=='#')name=NULL;
    }
    if(!name) {
        if(!symbol->symbol.N.Name.Short)
            name=(const char *)exports->hash.strings.data+symbol->symbol.N.Name.Long;
        else name=strncpy(export_short_name,(const char *)&symbol->symbol,8);
    }
    if(strchr(name,'@'))return;
    find_export_candidates(objects,name,&match_count.low,names,sources,decoration);
    if(libraries) {
        for(i=0;i<(WORD)count;++i)
            if(*names[i]!='?') { all_questions=0;break; }
        if(!(WORD)count || all_questions) {
            length=strlen(name);
            initialize_library_enumerator(&enumerator,(OutputLibrary *)libraries);
            while(next_library(&enumerator)) {
                library=enumerator.current;
                for(index=0;index<library->archive_symbol_count;++index) {
                    candidate=((const char **)library->index_data[3])[index];
                    prefix=(*candidate=='?' || *candidate=='@' ||
                        ((BYTE)decoration && *candidate=='_'))?1:0;
                    if(strncmp(name,candidate+prefix,length))continue;
                    if(candidate[prefix+length] && candidate[prefix+length]!='@')continue;
                    if((WORD)count && !strcmp(names[0],candidate))continue;
                    if((WORD)count<60) {
                        names[(WORD)count]=duplicate_argument(candidate);
                        sources[(WORD)count]=library->name;
                        ++count;
                    }
                }
            }
        }
    }
    if(!(WORD)count) {
        set_symbol_defined(symbol,0,exports);
        if(symbol->contribution)record_symbol_reference(symbol,symbol->contribution->module);
        return;
    }
    if((WORD)count==1)selected=names[0];
    else {
        for(i=0;i<(WORD)count;++i) {
            selected=names[i];
            if(!strcmp(selected,name))break;
            if(*selected!='?') { ++ordinary;last_ordinary=i; }
        }
        if(i==(WORD)count) {
            if(ordinary!=1)goto ambiguous;
            i=last_ordinary;
            selected=names[i];
        }
        chosen=i;
        for(i=0;i<(WORD)count;++i)if(i!=chosen)release_memory(names[i]);
    }
    if(symbol->diagnostic_name) {
        replacement=selected;
        if(!(symbol->flags&0x200)) {
            release_memory((void *)symbol->diagnostic_name);
            symbol->diagnostic_name=selected;
        }
        external=get_symbol_name(symbol,exports->hash.string_storage,&kind,NULL);
        if(kind==1)external=strncpy(short_symbol_name,external,8);
        bytes=strlen(external);
        prefix=(*selected=='?' || *selected=='@' ||
            ((BYTE)decoration && *selected=='_'))?1:0;
        if(prefix)++bytes;
        suffix=strchr(selected,'@');
        if(suffix)bytes+=strlen(suffix);
        selected=(char *)zero_allocate(bytes+1);
        if(prefix)*selected=*replacement;
        strcat(selected,external);
        if(suffix)strcat(selected,suffix);
        if(replacement!=symbol->diagnostic_name)free(replacement);
    }
    resolved=exports->Lookup((const unsigned char *)selected,NULL);
    if(resolved!=symbol) {
        resolved->contribution=symbol->contribution;
        resolved->diagnostic_name=symbol->diagnostic_name;
        set_symbol_defined(resolved,1,exports);
        resolved->flags=symbol->flags|0x10;
        resolved->final_value=symbol->final_value;
        resolved->input_flags=symbol->input_flags;
        resolved->symbol.Value=symbol->symbol.Value;
        set_symbol_defined(symbol,0,exports);
        if(symbol->contribution)record_symbol_reference(symbol,symbol->contribution->module);
        symbol->flags|=0x100;
    }
    release_memory(selected);
    return;
ambiguous:
    ambiguous_exports=1;
    link_warning(definition_file,4022,name);
    for(i=0;i<(WORD)count;++i) {
        display=display_symbol_name(names[i],1);
        link_warning(definition_file,4002,display,sources[i]);
        if(display!=names[i])release_memory((void *)display);
    }
}

/* The output counter is genuinely a WORD. The caller initializes its containing
 * DWORD first; these increments preserve its defined upper bytes. */
#pragma intrinsic(strlen,strcmp)
extern "C" void __stdcall find_export_candidates(StringTable *table,const char *name,
    WORD *matches,char **names,const char **sources,DWORD decoration)
{
    StringNode **symbols=table->GetSortedSymbols();
    DWORD count=count_hash_symbols(table->hash.state),index,length=strlen(name);
    StringNode *symbol;
    const char *candidate;
    BYTE prefix;
    WORD kind;
    for(index=0;index<count;++index) {
        symbol=symbols[index];
        if(!(symbol->flag_byte&1) || (symbol->flag_byte&0x10))continue;
        candidate=get_symbol_name(symbol,table->hash.string_storage,&kind,NULL);
        if(kind==1)candidate=strncpy(short_symbol_name,candidate,8);
        prefix=(*candidate=='?' || *candidate=='@' ||
            ((BYTE)decoration && *candidate=='_'))?1:0;
        if(strncmp(name,candidate+prefix,length))continue;
        if(candidate[prefix+length] && candidate[prefix+length]!='@')continue;
        if(*matches && !strcmp(names[0],candidate))continue;
        /* As in the original, this helper itself has no array-capacity formal. */
        names[*matches]=duplicate_argument(candidate);
        sources[*matches]=symbol->contribution?symbol->contribution->module->name:"(common)";
        ++*matches;
    }
}

extern "C" { extern const char *default_phase; }
extern "C" DWORD __stdcall count_librarian_symbols(StringTable *table,DWORD *data_count,
    DWORD *noname_count,DWORD *private_count)
{
    const char *saved_phase=default_phase;
    DWORD total=0,data=0,noname=0,private_exports=0,flags;
    StringNode *symbol;
    default_phase="CountExternTable";
    initialize_hash_enumerator(table->hash.state);
    while((symbol=table->NextSymbol())!=NULL) {
        flags=symbol->flags;
        if(flags&0x4001) {
            ++total;
            if(flags&0x2000)++data;
            if(flags&0x8000)++noname;
            if(flags&0x100000)++private_exports;
        }
    }
    end_hash_enumerator(table->hash.state);
    *data_count=data;
    if(noname_count)*noname_count=noname;
    if(private_count)*private_count=private_exports;
    default_phase=saved_phase;
    return total;
}

extern "C" { extern DWORD librarian_next_ordinal,librarian_last_ordinal; }
DWORD RECON_THISCALL LibrarianContext::OrdinalSlots(DWORD symbols)
{
    DWORD span=librarian_last_ordinal-librarian_next_ordinal+1;
    if(span<=symbols) {
        span=symbols;
        if(symbols+librarian_next_ordinal>65536)
            librarian_next_ordinal=65536-symbols;
    }
    if(span>=65536)link_fatal(NULL,1182);
    return span;
}

#pragma intrinsic(strcpy,strcat)
void RECON_THISCALL LibrarianContext::CreateImportSymbols(DWORD *count)
{
    StringTable *symbols;
    StringNode *symbol;
    char *name;
    DWORD added;
    if(machine==0x601)return;
    symbols=image->symbols;
    added=CreateImportReferences();
    *count+=added;
    name=(char *)allocate_memory(state40+21);
    strcpy(name,"__IMPORT_DESCRIPTOR_");
    strcat(name,image->options.library_basename);
    symbol=symbols->Lookup((const unsigned char *)name,NULL);
    set_symbol_defined(symbol,1,image->symbols);
    symbol->flags|=0x800;
    symbol->input_flags=1;
    release_memory(name);
    symbol=symbols->Lookup((const unsigned char *)"__NULL_IMPORT_DESCRIPTOR",NULL);
    set_symbol_defined(symbol,1,symbols);
    symbol->input_flags=2;
    symbol->flags|=0x800;
    name=(char *)allocate_memory(state40+18);
    *name=0x7f;
    strcpy(name+1,(const char *)owned3c);
    strcat(name,"_NULL_THUNK_DATA");
    null_thunk_name=name;
    symbol=symbols->Lookup((const unsigned char *)name,NULL);
    set_symbol_defined(symbol,1,image->symbols);
    symbol->input_flags=3;
    symbol->flags|=0x800;
}

struct ImportReferenceNode {
    StringNode *symbol;
    ImportReferenceNode *next;
};
typedef char ImportReferenceNodeSize[(sizeof(ImportReferenceNode)==8)?1:-1];
#pragma intrinsic(strlen,strcpy,strcat)
DWORD RECON_THISCALL LibrarianContext::CreateImportReferences(void)
{
    StringTable *symbols=image->symbols;
    StringNode *symbol,*created;
    ImportReferenceNode *list=NULL,*node,*next;
    const char *name;
    char *import_name;
    WORD kind;
    DWORD count=0;
    default_phase="CsymCreateThunkSymbols";
    if(!symbols)return 0;
    initialize_hash_enumerator(symbols->hash.state);
    while((symbol=symbols->NextSymbol())!=NULL) {
        if((symbol->flag_byte&1) && !(symbol->flags&0x100000)) {
            node=(ImportReferenceNode *)allocate_memory(sizeof(ImportReferenceNode));
            node->next=list;
            node->symbol=symbol;
            list=node;
        }
    }
    end_hash_enumerator(symbols->hash.state);
    while(list) {
        symbol=list->symbol;
        next=list->next;
        release_memory(list);
        name=get_symbol_name(symbol,symbols->hash.string_storage,&kind,NULL);
        if(kind==1)name=strncpy(short_symbol_name,name,8);
        import_name=(char *)allocate_memory(strlen(name)+7);
        strcpy(import_name,"__imp_");
        strcat(import_name,name);
        created=symbols->Lookup((const unsigned char *)import_name,NULL);
        set_symbol_defined(created,1,symbols);
        created->flags|=0x800;
        created->final_value=0;
        created->input_flags=symbol->input_flags;
        created->contribution=symbol->contribution;
        ++count;
        /* This allocated input spelling is likewise not freed by the original. */
        list=next;
    }
    return count;
}

void RECON_THISCALL LibrarianContext::MarkExplicitOrdinals(void)
{
    StringTable *symbols;
    StringNode **ordered,*symbol;
    DWORD count,index,ordinal,offset;
    default_phase="IdentifyAssignedOrdinals";
    if(!image)return;
    symbols=image->symbols;
    ordered=symbols->GetSortedSymbols();
    count=count_hash_symbols(symbols->hash.state);
    for(index=0;index<count;++index) {
        symbol=ordered[index];
        if(!(symbol->flag_byte&1))continue;
        ordinal=symbol->symbol.Value;
        if(!ordinal)continue;
        offset=ordinal-librarian_next_ordinal;
        if(((BYTE *)owned10)[offset])link_fatal(definition_file,1121,ordinal);
        ((BYTE *)owned10)[offset]=1;
    }
}
