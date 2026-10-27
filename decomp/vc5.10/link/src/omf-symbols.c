#include <windows.h>
#include <string.h>
#include "memory.h"
#include "files.h"
#include "arguments.h"
#include "omf.h"
#include <recon.h>

/* Distinct 211-bucket arrays at original476234 and478a54 hold the
 * actual 24-byte named symbols and 16-byte symbol/displacement records. */
struct OmfNameEntry **omf_symbol_hash;
struct OmfExternalNode **omf_offset_hash;
#pragma intrinsic(memset)
void __cdecl initialize_omf_hashes(void)
{
    if(!omf_symbol_hash)omf_symbol_hash=allocate_memory(211*sizeof(*omf_symbol_hash));
    memset(omf_symbol_hash,0,211*sizeof(*omf_symbol_hash));
    if(!omf_offset_hash)omf_offset_hash=allocate_memory(211*sizeof(*omf_offset_hash));
    memset(omf_offset_hash,0,211*sizeof(*omf_offset_hash));
}

/* A real growable buffer of packed 18-byte COFF symbols. Capacity is bytes. */
IMAGE_SYMBOL *omf_symbols;
DWORD omf_symbol_count,omf_symbol_capacity;
#pragma intrinsic(strlen,memcpy)
DWORD __stdcall append_omf_symbol(const char *name,DWORD value,DWORD section,
                                 DWORD storage_class,DWORD auxiliary_count)
{
    IMAGE_SYMBOL symbol;
    IMAGE_SYMBOL *larger;
    if((LONG)strlen(name)<=8)strncpy((char *)symbol.N.ShortName,name,8);
    else {
        symbol.N.Name.Short=0;
        symbol.N.Name.Long=intern_omf_string(name);
    }
    symbol.Value=value;
    symbol.SectionNumber=(SHORT)section;
    symbol.Type=0;
    symbol.StorageClass=(BYTE)storage_class;
    symbol.NumberOfAuxSymbols=(BYTE)auxiliary_count;
    if(!omf_symbol_count) {
        omf_symbols=zero_allocate(0x20000);
        omf_symbol_capacity=0x20000;
    }
    if((omf_symbol_count+1)*sizeof(symbol)>omf_symbol_capacity) {
        larger=zero_allocate(omf_symbol_capacity+0x20000);
        memcpy(larger,omf_symbols,omf_symbol_capacity);
        release_memory(omf_symbols);
        omf_symbols=larger;
        omf_symbol_capacity+=0x20000;
    }
    omf_symbols[omf_symbol_count]=symbol;
    return omf_symbol_count++;
}

DWORD __stdcall append_omf_auxiliary(const void *record)
{
    IMAGE_SYMBOL *larger;
    if((omf_symbol_count+1)*sizeof(IMAGE_SYMBOL)>omf_symbol_capacity) {
        larger=zero_allocate(omf_symbol_capacity+0x20000);
        if(omf_symbol_capacity)memcpy(larger,omf_symbols,omf_symbol_capacity);
        release_memory(omf_symbols);
        omf_symbols=larger;
        omf_symbol_capacity+=0x20000;
    }
    memcpy(omf_symbols+omf_symbol_count,record,sizeof(IMAGE_SYMBOL));
    return omf_symbol_count++;
}

void __stdcall read_omf_pubdefs(DWORD unused_formal)
{
    WORD segment,type;
    DWORD section,offset;
    char *name;
    read_omf_index(); /* group index is physically read and ignored */
    segment=read_omf_index();
    if(!segment) {
        section=(DWORD)-1;
        read_omf_word(); /* absolute frame */
    } else section=omf_segments[(SHORT)segment].output_section;
    while(omf_record_length>1) {
        name=read_omf_string(1);
        offset=omf_record_is32?read_omf_dword():(read_omf_word()&0xffff);
        type=read_omf_index();
        omf_external_count=(WORD)(omf_external_count+1);
        if(omf_external_count>=4096)omf_error("Too many externals");
        /* The actual callee writes only the low WORD of these four physical
         * DWORD slots. No meaning or equality is claimed for carried uppers. */
        insert_omf_name(name,offset,3,section,omf_external_count,type);
    }
}

void __stdcall insert_omf_name(char *name,DWORD value,DWORD kind,
    DWORD section,DWORD index,DWORD type)
{
    OmfNameEntry *entry=zero_allocate(sizeof(*entry));
    DWORD bucket;
    entry->value=value;
    entry->kind=(WORD)kind;
    entry->section=(SHORT)section;
    entry->type_index=(WORD)type;
    entry->name=name;
    entry->external_index=(WORD)index;
    bucket=hash_omf_name(name);
    if(omf_symbol_hash[bucket])entry->next=omf_symbol_hash[bucket];
    omf_symbol_hash[bucket]=entry;
}

DWORD __stdcall hash_omf_name(const char *name)
{
    DWORD hash=0;
    /* MOVSX of each source char and unsigned DIV after DWORD wraparound. */
    while(*name)hash=hash*3+(signed char)*name++;
    return hash%211;
}

void __cdecl finish_omf_external_symbols(void)
{
    DWORD bucket,sequence=0;
    OmfExternalNode *entry;
    IMAGE_SYMBOL *symbol;
    char name[12];
    for(bucket=0;bucket<211;++bucket) {
        for(entry=omf_offset_hash[bucket];entry;entry=entry->next) {
            symbol=&omf_symbols[entry->symbol];
            if(symbol->SectionNumber>0) {
                sprintf(name,"$$R%04X",++sequence);
                entry->output_symbol=append_omf_symbol(name,
                    symbol->Value+entry->displacement,
                    (DWORD)symbol->SectionNumber,IMAGE_SYM_CLASS_STATIC,0);
            } else entry->output_symbol=entry->symbol;
        }
    }
}

void __stdcall remember_omf_target_offset(DWORD symbol,DWORD offset)
{
    DWORD bucket=(symbol+offset)%211;
    OmfExternalNode *entry;
    for(entry=omf_offset_hash[bucket];entry;entry=entry->next)
        if(entry->symbol==symbol && entry->displacement==offset)return;
    entry=allocate_memory(sizeof(*entry));
    entry->symbol=symbol;
    entry->displacement=offset;
    /* output_symbol is written by finish_omf_external_symbols before lookup. */
    entry->next=omf_offset_hash[bucket];
    omf_offset_hash[bucket]=entry;
}

DWORD __stdcall lookup_omf_target_offset(DWORD symbol,DWORD offset)
{
    OmfExternalNode *entry=omf_offset_hash[(symbol+offset)%211];
    for(;;) {
#ifdef __linux__
        /* The original dereferences missing keys. Only the pass-one inserted
         * and subsequently resolved keys have a supported lookup result. */
        if(!entry) { __asm { int 3 } }
#endif
        if(entry->symbol==symbol && entry->displacement==offset)
            return entry->output_symbol;
        entry=entry->next;
    }
}

DWORD omf_function_auxiliary_index; /*479a78*/
void __cdecl resolve_omf_segments(void)
{
    DWORD bucket,storage_class,auxiliary_count=0,fields[2];
    WORD i;
    BYTE auxiliary[18];
    OmfNameEntry *entry,*next;
    OmfLine *lines;
    omf_function_auxiliary_index=0;
    for(bucket=0;bucket<211;++bucket) {
        for(entry=omf_symbol_hash[bucket];entry;entry=entry->next) {
            if(omf_public_map[entry->external_index] || entry->kind==5 || entry->kind==6)
                continue;
            if(entry->kind==1)auxiliary_count=0;
            storage_class=(entry->kind==2 || entry->kind==4)?IMAGE_SYM_CLASS_STATIC:IMAGE_SYM_CLASS_EXTERNAL;
            if(omf_line_count && !omf_function_auxiliary_index &&
               entry->section==(SHORT)omf_default_line_segment)auxiliary_count=1;
            omf_public_map[entry->external_index]=append_omf_symbol(entry->name,
                entry->value,(DWORD)entry->section,storage_class,auxiliary_count);
            if((WORD)auxiliary_count) {
                omf_function_auxiliary_index=omf_symbol_count;
                lines=omf_line_buffer;
                append_omf_function_auxiliary(omf_line_count,omf_default_line_segment,
                    lines[0].offset,lines[0].line,
                    lines[omf_line_count-1].offset-lines[0].offset,
                    lines[omf_line_count-1].line);
                if((LONG)omf_line_count>1) {
                    for(i=1;(LONG)(DWORD)i<(LONG)omf_line_count;++i)
                        lines[i].line=(WORD)(lines[i].line-lines[0].line);
                }
                auxiliary_count=0;
                lines[0].line=0;
                lines[0].offset=omf_function_auxiliary_index-1;
            }
        }
    }
    memset(auxiliary,0,sizeof(auxiliary));
    for(bucket=0;bucket<211;++bucket) {
        for(entry=omf_symbol_hash[bucket];entry;entry=next) {
            if(!omf_public_map[entry->external_index]) {
                omf_public_map[entry->external_index]=append_omf_symbol(entry->name,
                    entry->value,(DWORD)entry->section,IMAGE_SYM_CLASS_WEAK_EXTERNAL,1);
                fields[0]=omf_public_map[entry->weak_default_index];
                fields[1]=(DWORD)(entry->kind==5?1:2);
                memcpy(auxiliary,fields,sizeof(fields));
                append_omf_auxiliary(auxiliary);
            }
            next=entry->next;
            release_memory(entry);
        }
    }
}

char *omf_strings; /*47621c*/
DWORD omf_string_length,omf_string_capacity; /*47eeb4/4675b4*/
DWORD __stdcall intern_omf_string(const char *name)
{
    DWORD offset,length;
    char *larger;
    if(!omf_string_length) {
        omf_strings=zero_allocate(0x20000);
        omf_string_length=4;
        omf_string_capacity=0x20000;
    }
    offset=omf_string_length;
    length=strlen(name)+1;
    if(omf_string_length+length>omf_string_capacity) {
        larger=zero_allocate(omf_string_capacity+length+0x20000);
        memcpy(larger,omf_strings,omf_string_capacity);
        release_memory(omf_strings);
        omf_strings=larger;
        omf_string_capacity+=length+0x20000;
    }
    /* Despite the provisional helper name, the whole original appends each
     * supplied string, including NUL; it does not deduplicate names. */
    memcpy(omf_strings+offset,name,length);
    omf_string_length+=length;
    return offset;
}

#pragma intrinsic(strcmp)
void __stdcall read_omf_extdefs(DWORD indexed_names,DWORD storage_class)
{
    WORD name_index=0,type;
    const char *name;
    OmfNameEntry *previous;
    DWORD kind,section;
    while(omf_record_length>1) {
        if((BYTE)indexed_names) {
            name_index=read_omf_index();
            name=omf_names[(SHORT)name_index];
        } else name=read_omf_string((WORD)storage_class==3?0:1);
        type=read_omf_index();
        if((BYTE)indexed_names && omf_name_kinds[(SHORT)name_index])continue;
        omf_external_count=(WORD)(omf_external_count+1);
        if(omf_external_count>=4096)omf_error("Too many externals");
        omf_external_symbol_count=(WORD)(omf_external_symbol_count+1);
        omf_external_map[(SHORT)omf_external_symbol_count]=omf_external_count;
        if(!strcmp(name,"_acrtused") || !strcmp(name,"__acrtused"))continue;
        kind=(DWORD)((WORD)storage_class==3?2:1);
        previous=find_omf_name(name);
        section=previous?(DWORD)previous->section:0;
        /* The actual insertion writer consumes low WORD type/index/section;
         * full volatile return uppers from read_index are not semantic data. */
        insert_omf_name((char *)name,0,kind,section,omf_external_count,type);
    }
}

OmfNameEntry *__stdcall find_omf_name(const char *name)
{
    OmfNameEntry *entry=omf_symbol_hash[hash_omf_name(name)];
    while(entry && strcmp(entry->name,name))entry=entry->next;
    return entry;
}

void __stdcall read_omf_comdefs(DWORD storage_class)
{
    char *name;
    BYTE kind;
    DWORD size;
    while(omf_record_length>1) {
        name=read_omf_string(1);
        read_omf_index(); /* physical type index is ignored here */
        kind=(BYTE)read_omf_byte();
        size=read_omf_number();
        if(kind==0x61)size*=read_omf_number();
        omf_external_count=(WORD)(omf_external_count+1);
        if(omf_external_count>=4096)omf_error("Too many externals");
        omf_external_symbol_count=(WORD)(omf_external_symbol_count+1);
        omf_external_map[(SHORT)omf_external_symbol_count]=omf_external_count;
        omf_public_map[omf_external_count]=
            append_omf_symbol(name,size,0,storage_class,0);
    }
}

void __stdcall append_omf_function_auxiliary(DWORD count,DWORD section,
    DWORD unused_start,DWORD first_line,DWORD length,DWORD last_line)
{
    IMAGE_AUX_SYMBOL auxiliary;
    IMAGE_SYMBOL marker;
    /* The original marks the immediately preceding symbol as a function.
     * All six physical slots remain; the start-offset slot is not consumed. */
    omf_symbols[omf_symbol_count-1].Type=0x20;
    memset(&auxiliary,0,sizeof(auxiliary));
    auxiliary.Sym.TagIndex=omf_symbol_count+1;
    auxiliary.Sym.Misc.TotalSize=length+1;
    append_omf_auxiliary(&auxiliary);
    memset(&marker,0,sizeof(marker));
    memcpy(marker.N.ShortName,".bf",4);
    marker.SectionNumber=(SHORT)section;
    marker.StorageClass=IMAGE_SYM_CLASS_FUNCTION;
    marker.NumberOfAuxSymbols=1;
    append_omf_auxiliary(&marker);
    memset(&auxiliary,0,sizeof(auxiliary));
    auxiliary.Sym.Misc.LnSz.Linenumber=(WORD)first_line;
    append_omf_auxiliary(&auxiliary);
    memcpy(marker.N.ShortName,".lf",4);
    marker.Value=count;
    marker.NumberOfAuxSymbols=0;
    append_omf_auxiliary(&marker);
    memcpy(marker.N.ShortName,".ef",4);
    marker.Value=0;
    marker.NumberOfAuxSymbols=1;
    append_omf_auxiliary(&marker);
    memset(&auxiliary,0,sizeof(auxiliary));
    auxiliary.Sym.Misc.LnSz.Linenumber=(WORD)last_line;
    append_omf_auxiliary(&auxiliary);
}

void __stdcall append_omf_line(DWORD section,DWORD offset,DWORD line)
{
    SHORT index=(SHORT)(section-1);
    ++omf_output_line_count;
    ++omf_output_sections[index].NumberOfLinenumbers;
    append_omf_line_node(&omf_line_lists[index],offset,line);
}

void __stdcall append_omf_line_node(OmfLineList *list,DWORD offset,DWORD line)
{
    OmfLineNode *node=allocate_memory(sizeof(*node));
    node->next=NULL;
    node->offset=offset;
    node->line=(WORD)line;
    /* The original does not initialize the unused word at +6. Its line
     * serializer writes only the defined four-byte offset/two-byte line. */
    if(!list->head)list->head=node;
    else list->tail->next=node;
    ++list->count;
    list->tail=node;
}

void __stdcall read_omf_lpubdefs(DWORD unused_formal)
{
    WORD segment,type;
    DWORD section,offset;
    char *name;
    read_omf_index(); /* group index is physically read and ignored */
    segment=read_omf_index();
    if(!segment) {
        section=(DWORD)-1;
        read_omf_word(); /* absolute frame */
    } else section=omf_segments[(SHORT)segment].output_section;
    while(omf_record_length>1) {
        name=read_omf_string(0);
        offset=omf_record_is32?read_omf_dword():(read_omf_word()&0xffff);
        type=read_omf_index();
        update_omf_name(name,offset,4,section,type);
    }
}

void __stdcall update_omf_name(char *name,DWORD value,DWORD kind,
    DWORD section,DWORD type)
{
    OmfNameEntry *entry=find_omf_name(name);
    if(entry) {
        entry->value=value;
        entry->kind=(WORD)kind;
        entry->section=(SHORT)section;
        entry->type_index=(WORD)type;
    } else insert_omf_name(name,value,kind,section,omf_external_count,type);
}
