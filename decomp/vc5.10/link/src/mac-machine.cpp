/* Mac PowerPC machine setup. The generated PPC glue is output data, never
 * executed by this native linker. */
#include <windows.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include "link.h"
#include "input.h"
#include "options.h"
#include "memory.h"
#include "pass-one.h"
#include "sections.h"
#include "full-link-functions.h"
#include "mac-format.h"
#include "mac-options.h"
#include <recon.h>

extern "C" {
MacLoaderHeader mac_loader_header;
/* Source aliases preserve the single original56B owner. The finalizer swaps
 * the whole object in place; later field views retain that actual byte state. */
#define mac_relocation_instruction_offset mac_loader_header.relocation_instruction_offset
#define mac_loader_strings_offset mac_loader_header.strings_offset
#define mac_export_hash_offset mac_loader_header.export_hash_offset
#define mac_export_hash_power_value mac_loader_header.export_hash_power
#define mac_export_count mac_loader_header.export_count
extern BYTE pdb_enabled,incremental_enabled,incremental_reuse,reproducible_build;
extern BYTE mapped_debug,mac_target,mac_file_padding,reference_optimization_specified;
extern DWORD fixup_count;
extern InputArguments mac_import_arguments;
/* Actual49458c counts four-byte TOC entries;494510 counts Mac relocation
 * records. Both are distinct from generic fixup_count47eef4. */
DWORD mac_toc_entry_count,mac_relocation_count;
extern Contribution *mac_toc,*mac_function_info;
extern StringNode *mac_toc_symbol,*mac_function_symbol;
void __stdcall define_input_symbol(StringNode *,Contribution *,DWORD,SHORT,WORD,DWORD,OutputModule *,StringTable *);
DWORD __cdecl get_mac_toc_bias(void)
{
    DWORD bytes=mac_toc_entry_count*4;
    return bytes<=0x8000?0:bytes-0x8000;
}
Contribution *mac_glue_contribution; /* Original494574; distinct from other Mac contributions. */
/* Setup writes BYTE; the export consumer reads DWORD then masks its low BYTE. */
DWORD mac_export_state;
DWORD mac_incremental_tag; /* Actual49456c, time-derived low byte in a DWORD. */

/* Six genuine generated glue instructions at4673a0. Each constant describes
 * the output instruction rather than an executable implementation here. */
DWORD mac_import_glue[]={
    0x81820000UL, /* lwz r12,0(r2), displacement patched by relocation code */
    0x800c0000UL, /* lwz r0,0(r12) */
    0x90410014UL, /* stw r2,20(r1) */
    0x7c0903a6UL, /* mtctr r0 */
    0x804c0004UL, /* lwz r2,4(r12) */
    0x4e800420UL  /* bctr */
};
/* AFP_AfpInfo output record: file bytes retain Macintosh type/creator order. */
typedef struct MacAfpInfo {
    char signature[4];
    DWORD version,reserved,backup_time;
    char file_type[4],creator[4];
    BYTE finder_flags[2],finder_reserved[22];
    BYTE prodos_type,prodos_reserved[5];
    WORD reserved_word;
    BYTE reserved_tail[4];
} MacAfpInfo;
typedef char MacAfpInfoLayout[(sizeof(MacAfpInfo)==60 &&
    offsetof(MacAfpInfo,file_type)==16 && offsetof(MacAfpInfo,finder_flags)==24 &&
    offsetof(MacAfpInfo,prodos_type)==48 && offsetof(MacAfpInfo,reserved_word)==54)?1:-1];
#pragma intrinsic(memcpy)
extern MacResource *mac_resource_list;
/* Original467394 points at the next link to fill; initial467390 is the head. */
MacResource **mac_resource_tail=&mac_resource_list;
void __stdcall append_mac_resource(const void *data,DWORD size,DWORD kind)
{
    *mac_resource_tail=(MacResource *)allocate_incremental(sizeof(MacResource));
    (*mac_resource_tail)->next=NULL;
    (*mac_resource_tail)->filename=NULL;
    (*mac_resource_tail)->data=allocate_memory(size);
    memcpy((*mac_resource_tail)->data,data,size);
    (*mac_resource_tail)->size=size;
    (*mac_resource_tail)->kind=kind;
    /* contribution and reserved18 are not initialized by this producer. */
    mac_resource_tail=&(*mac_resource_tail)->next;
}
#pragma intrinsic(memcpy,memset,strlen,strcpy,strcat)
void __stdcall initialize_mac_resource_data(LinkContext *context,DWORD bundle,
    const char *type,const char *creator)
{
    MacAfpInfo header;
    char name[9];
    memset(&header,0,sizeof(header));
    memcpy(header.signature,"AFP",4);
    header.version=0x10000;
    header.backup_time=0x80000000UL;
    header.prodos_type=4;
    if((BYTE)bundle)header.finder_flags[0]|=0x20;
    memset(name,0,sizeof(name));
    if(type) {
        strncpy(name,type,4);
        strcat(name,"    ");
    } else strcpy(name,mac_target && (context->file_header.Characteristics & IMAGE_FILE_DLL)?
        "shlb":"APPL");
    memcpy(header.file_type,name,4);
    memset(name,0,sizeof(name));
    if(creator) {
        strncpy(name,creator,4);
        strcat(name,"    ");
    } else strcpy(name,mac_target && (context->file_header.Characteristics & IMAGE_FILE_DLL)?
        "CFMG":"????");
    memcpy(header.creator,name,4);
    append_mac_resource(&header,sizeof(header),2);
}
extern OutputModule *output_module;
void __stdcall initialize_mac_imports(LinkContext *context)
{
    Contribution *contribution=create_contribution(".text",0,0x60100020,0x60100020,
        output_module,&context->sections,context);
    InputArgument *argument;
    unsigned index;
    if(context->options.effective_reference_opt || context->options.fold_identical_comdats)
        attach_comdat(contribution,NULL,1);
    argument=mac_import_arguments.first;
    for(index=0;(WORD)index<mac_import_arguments.count;++index) {
        /* Existing dispatcher view consumes this same Contribution prefix. */
        if(!add_mac_import((const unsigned char *)argument->name,
            (const unsigned char *)argument->original_name,
            contribution,context))
            link_warning(argument->original_name,4544,argument->name);
        argument=argument->next;
    }
}
BYTE __stdcall classify_mac_ppc_relocation(Contribution *,WORD,DWORD,BYTE)
{ __asm { int 3 } return 0; }
void __stdcall process_mac_ppc_relocations(Contribution *,IMAGE_RELOCATION *,DWORD,
    BYTE *,IMAGE_SYMBOL *,LinkContext *,DWORD *) { __asm { int 3 } }

void __stdcall initialize_mac_ppc_backend(LinkContext *context,BYTE *incremental_supported)
{
    const char *subsystem=NULL;
    mac_target=1;
    if(!pdb_enabled && context->options.debug_level)link_fatal(NULL,1192);
    if(!context->options.mppc32_types)mapped_debug=0;
    context->symbols->ReleaseLookupIndex();
    if(context->specified_switches & 8) {
        link_warning(NULL,4062,"GPSIZE","MPPC");
        context->options.gp_size=0;
    }
    if(context->options.rom) {
        link_warning(NULL,4062,"ROM","MPPC");
        context->options.rom=0;
    }
    *incremental_supported=1;
    process_relocations=process_mac_ppc_relocations;
    classify_relocation=classify_mac_ppc_relocation;
    if(incremental_enabled)mac_file_padding=0;
    if(!(context->specified_switches & 1))context->optional_header.SectionAlignment=0x1000;
    switch(context->optional_header.Subsystem) {
    case 1:subsystem="SUBSYSTEM:NATIVE";break;
    case 2:subsystem="SUBSYSTEM:CONSOLE";break;
    case 3:subsystem="SUBSYSTEM:WINDOWS";break;
    case 7:subsystem="SUBSYSTEM:POSIX";break;
    case 9:subsystem="SUBSYSTEM:WINDOWSCE";break;
    }
    if(context->optional_header.Subsystem)link_warning(NULL,4062,subsystem,"MPPC");
    context->optional_header.Subsystem=3;
    context->optional_header.FileAlignment=32;
    if(!reference_optimization_specified)context->options.effective_reference_opt=0;
    if(context->specified_switches & 0x400000)link_warning(NULL,4062,"OPT:ICF","MPPC");
    context->options.fold_identical_comdats=0;
    if(incremental_enabled && !reproducible_build)
        mac_incremental_tag=(DWORD)time(NULL)&255;
    if(!incremental_reuse) {
        mac_toc_entry_count+=17;
        mac_relocation_count+=3;
        fixup_count+=3;
        *(BYTE *)&mac_export_state=1;
        context->mac_glue_bytes_used=0;
        /* The physical second slot is a DWORD; its real consumer reads BYTE. */
        initialize_mac_resource_data(context,context->options.mac_bundle,
            context->options.mac_type,context->options.mac_creator);
        initialize_mac_imports(context);
    }
    clear_argument_list(&mac_import_arguments);
    swap_byte_order(mac_import_glue,sizeof(mac_import_glue));
}
}

/* Retain this provisional registry name: its actual role is .text$_glue,
 * rather than the separate AFP/resource records prepared above. */
void RECON_THISCALL LinkContext::PrepareMacResources(void)
{
    extern OutputModule *output_module;
    DWORD bytes=options.new_glue?mac_glue_count*8+40:mac_glue_count*24+20;
    DWORD padding=0;
    if(incremental_enabled)padding=(WORD)bytes<0xfff0?(WORD)bytes:0xfff0;
    mac_glue_contribution=create_contribution(".text$_glue",bytes+padding,
        0x60100020,0x60100020,output_module,&sections,this);
    mac_glue_contribution->padding=padding;
    if(options.effective_reference_opt || options.fold_identical_comdats)
        attach_comdat(mac_glue_contribution,NULL,1);
}

void RECON_THISCALL LinkContext::PrepareMacSymbols(void)
{
    StringNode *symbol=symbols->FindSymbol("._icsym");
    if(!symbol)return;
    set_symbol_defined(symbol,1,symbols);
    if(!symbol->contribution)
        symbol->contribution=create_contribution(".text",0,0x60100020,0x60100020,
            output_module,&sections,this);
    if(options.effective_reference_opt || options.fold_identical_comdats)
        attach_comdat(symbol->contribution,NULL,1);
}

/* This provisional owner name prepares the TOC contribution and its symbol. */
void RECON_THISCALL LinkContext::ResolveMacImports(void)
{
    DWORD bytes=mac_toc_entry_count*4,padding=0;
    if(bytes>0x10000)link_fatal(NULL,1177);
    if(incremental_enabled) {
        padding=0x10000-bytes;
        if(bytes<padding)padding=bytes;
        bytes+=padding;
    }
    mac_toc=create_contribution(".data",bytes,0xc0300040,0xc0300040,
        output_module,&sections,this);
    mac_toc->padding=padding;
    if(options.effective_reference_opt || options.fold_identical_comdats)
        attach_comdat(mac_toc,NULL,1);
    define_input_symbol(mac_toc_symbol,mac_toc,get_mac_toc_bias(),-2,0,0,
        output_module,symbols);
}

/* This provisional name owns the function-information contribution. */
void RECON_THISCALL LinkContext::ResolveMacExports(void)
{
    mac_function_info=create_contribution(".data",(BYTE)mac_export_state*16+8,
        0xc0300040,0xc0300040,output_module,&sections,this);
    if(options.effective_reference_opt || options.fold_identical_comdats)
        attach_comdat(mac_function_info,NULL,1);
    mac_function_symbol->reserved36=0x40;
    mac_function_symbol->attributes|=0x10;
    define_input_symbol(mac_function_symbol,mac_function_info,0,-2,0,0,
        output_module,symbols);
}

extern "C" {
void *__stdcall zero_allocate(unsigned);
DWORD __stdcall mac_contribution_padding(DWORD);
/* Separate physical entry439e80, not generic relocation setup439e50. */
void __stdcall initialize_mac_relocation_padding(void *,DWORD,DWORD,DWORD)
{ __asm { int 3 } }
DWORD __stdcall prepare_mac_export_table(LinkContext *);
DWORD mac_import_library_count; /*494570,24-byte library records*/
DWORD mac_import_symbol_count;  /*4945a0,four-byte symbol records*/
DWORD mac_library_name_bytes;   /*494568*/
DWORD mac_symbol_name_bytes;    /*494590,subsequently a string cursor*/
DWORD mac_import_symbols_offset; /*49453c*/
DWORD mac_relocation_header_offset; /*494540; instruction offset aliases header4944fc*/
DWORD mac_loader_string_cursor; /*494514; strings/hash aliases header494500/504*/
struct MacRelocationRecord {
    DWORD offset,encoded,count,kind,argument;
};
typedef char MacRelocationRecordLayout[(sizeof(MacRelocationRecord)==20 &&
    offsetof(MacRelocationRecord,kind)==12 &&
    offsetof(MacRelocationRecord,argument)==16)?1:-1];
MacRelocationRecord *mac_relocation_records,*mac_relocation_cursor; /*494598/494554*/
DWORD mac_relocation_record_count; /*4946b8, actual appended count; not planned494510*/
Contribution *mac_loader_contribution; /*4944c0*/
}

/* The caller ignores EAX; the original also leaves the computed size there.
 * This owner retains its provisional void declaration without a return claim. */
void RECON_THISCALL LinkContext::PrepareMacSections(void)
{
    DWORD libraries=mac_import_library_count*24;
    DWORD imports=mac_import_symbol_count*4;
    DWORD exports,relocations,padding=0,strings,size;
    mac_import_symbols_offset=56+libraries;
    exports=prepare_mac_export_table(this);
    mac_relocation_header_offset=mac_import_symbols_offset+imports;
    mac_relocation_instruction_offset=mac_relocation_header_offset+12;
    mac_relocation_records=(MacRelocationRecord *)zero_allocate(
        (mac_relocation_count+1)*sizeof(MacRelocationRecord));
    mac_relocation_cursor=mac_relocation_records;
    relocations=mac_relocation_count*8;
    if(incremental_enabled)padding=(WORD)relocations<0xfff8?(WORD)relocations:0xfff8;
    relocations+=padding;
    if(incremental_enabled)
        initialize_mac_relocation_padding(reserved264,mac_relocation_header_offset,
            mac_relocation_instruction_offset,relocations);
    mac_loader_strings_offset=mac_relocation_instruction_offset+relocations;
    mac_loader_string_cursor=mac_loader_strings_offset;
    strings=mac_symbol_name_bytes+mac_library_name_bytes+2;
    mac_symbol_name_bytes=mac_library_name_bytes;
    mac_export_hash_offset=mac_loader_strings_offset+strings;
    size=68+libraries+imports+relocations+strings+exports;
    mac_loader_contribution=create_contribution(".ppcldr",size,0x42000040,0x42000040,
        output_module,&sections,this);
    if(mac_file_padding) {
        mac_loader_contribution->padding=mac_contribution_padding(size);
        mac_loader_contribution->raw_size+=mac_loader_contribution->padding;
    }
    if(options.effective_reference_opt || options.fold_identical_comdats)
        attach_comdat(mac_loader_contribution,NULL,1);
}

#define READ_MAC_EXPORT(data,bytes) do { \
    if(file_access_statistics)record_file_access(); \
    file=file_slots[fd]; \
    if(file->flags & 8)mapped_read(file,(data),(bytes)); \
    else buffered_read(file,(data),(bytes)); \
} while(0)
extern "C" {
extern BYTE file_access_statistics;
extern char mac_shared_module_name[];
#pragma intrinsic(strcmp)
BYTE __stdcall open_mac_export_section(int *descriptor)
{
    IMAGE_SECTION_HEADER section;
    FileRecord *file;
    DWORD error;
    if(!mac_shared_module_name[0])return 0;
    *descriptor=open_argument_file(mac_shared_module_name,0x8000,0);
    error=0;
    seek_file(*descriptor,20,FILE_BEGIN,&error);
    do {
        int fd=*descriptor;
        READ_MAC_EXPORT(&section,sizeof(section));
    } while(strcmp((const char *)section.Name,".ppcshl"));
    error=0;
    seek_file(*descriptor,section.PointerToRawData,FILE_BEGIN,&error);
    return (BYTE)(strcmp((const char *)section.Name,".ppcshl")==0);
}
DWORD __stdcall mac_export_hash_power(LONG count)
{
    DWORD power=0;
    do {
        if(count/(1L<<power)<10)break;
        ++power;
    } while(power<13);
    return power<10?power+1:power;
}
void __stdcall add_mac_export(LinkContext *,StringNode *,const char *,const char *,DWORD);
DWORD __stdcall mac_export_name_hash(const char *name,DWORD limit)
{
    DWORD hash=0,length=0;
    while(*name) {
        hash=(hash*2-(DWORD)((LONG)hash>>16))^(BYTE)*name++;
        ++length;
        if(!--limit)break;
    }
    return ((((DWORD)((LONG)hash>>16)&0xffff)^(hash&0xffff))+(length<<16));
}
/* Actual220B nodes, written only by add_mac_export. */
struct MacExportEntry {
    char name[200];
    DWORD hash;
    StringNode *entry,*symbol;
    BYTE is_function,reserved[3];
    MacExportEntry *next;
};
typedef char MacExportEntryLayout[(sizeof(MacExportEntry)==220 &&
    offsetof(MacExportEntry,symbol)==208 && offsetof(MacExportEntry,next)==216)?1:-1];
MacExportEntry *mac_export_buckets; /*494544*/
/* Header aliases own494508/49450c; no duplicate power/count storage. */
extern ComdatRoot *comdat_roots;
StringNode *__stdcall record_incremental_entry(const char *,Contribution *,LinkContext *,DWORD);
#pragma intrinsic(strlen,strcpy,strcat)
void __stdcall add_mac_export(LinkContext *context,StringNode *symbol,
    const char *name,const char *export_name,DWORD power)
{
    MacExportEntry *entry;
    DWORD length,hash,bucket;
    char *entry_name;
    StringNode *nonexport_entry;
    if(context->options.effective_reference_opt || context->options.fold_identical_comdats)
        add_comdat_root(NULL,symbol,NULL,&comdat_roots);
    if(*export_name=='_')++export_name;
    length=strlen(export_name);
    hash=mac_export_name_hash(export_name,length);
    bucket=((hash>>power)^hash)&((1UL<<power)-1);
    entry=mac_export_buckets+bucket;
    if(entry->symbol) {
        while(entry->next)entry=entry->next;
        entry->next=(MacExportEntry *)zero_allocate(sizeof(MacExportEntry));
        entry=entry->next;
    }
    strncpy(entry->name,export_name,200);
    entry->hash=hash;
    entry->symbol=symbol;
    if(!(symbol->attributes&2) && (symbol->symbol.Type&0x30)==0x20) {
        entry->entry=record_incremental_entry(name,symbol->contribution,context,0);
        entry->entry->attributes|=0x80;
        entry->is_function=1;
        if(symbol->symbol.Type&0x8000) {
            entry_name=(char *)allocate_memory(strlen(name)+6);
            strcpy(entry_name,"__nep");
            strcat(entry_name,name);
            nonexport_entry=context->symbols->FindSymbol(entry_name);
            release_memory(entry_name);
            if(context->options.effective_reference_opt || context->options.fold_identical_comdats)
                add_comdat_root(NULL,nonexport_entry,NULL,&comdat_roots);
            entry->symbol=nonexport_entry;
        }
    }
    mac_symbol_name_bytes+=length+1;
}
DWORD __stdcall prepare_mac_export_table(LinkContext *context)
{
    int fd;
    MacSharedLibraryHeader header;
    char name[201],*export_name;
    FileRecord *file;
    DWORD count,power,slots,index;
    StringNode *symbol;
    if(!open_mac_export_section(&fd))return 0;
    READ_MAC_EXPORT(&header,sizeof(header));
    count=header.named_count;
    if(!count)link_warning(NULL,4549,header.library);
    power=mac_export_hash_power((LONG)count);
    slots=1UL<<(power&31);
    mac_export_buckets=(MacExportEntry *)zero_allocate(220*(slots+1));
    mac_export_hash_power_value=power;
    mac_export_count=count;
    context->symbols->ReleaseLookupIndex();
    for(index=0;index<count;++index) {
        READ_MAC_EXPORT(name,200);
        name[200]=0;
        export_name=strchr(name,'!');
        if(export_name)*export_name++=0;
        else export_name=name;
        symbol=context->symbols->FindSymbol(name);
        if((symbol->flags&1) && symbol->symbol.SectionNumber!=-2)
            add_mac_export(context,symbol,name,export_name,power);
    }
    close_argument_file(fd,1);
    return count*4+slots*4+count*10;
}
}
#undef READ_MAC_EXPORT

extern "C" {
DWORD mac_jump_entry_count; /*4944bc, separate from TOC and glue counts*/
Contribution *mac_jump_contribution; /*494528*/
void __stdcall initialize_hash_enumerator(HashState *);
void __stdcall end_hash_enumerator(HashState *);
void __stdcall define_mac_jump_symbols(LinkContext *context)
{
    StringTable *symbols=context->symbols;
    DWORD count=count_hash_symbols(symbols->hash.state),index;
    StringNode *symbol;
    initialize_hash_enumerator(symbols->hash.state);
    for(index=0;index<count;++index) {
        symbol=symbols->NextSymbol();
        if(symbol->attributes&0x20)
            define_input_symbol(symbol,mac_jump_contribution,symbol->symbol.Value,
                -2,0,0,output_module,symbols);
    }
    end_hash_enumerator(symbols->hash.state);
}
}
void RECON_THISCALL LinkContext::PrepareMacJumpTables(void)
{
    DWORD bytes=mac_jump_entry_count*8,padding=0;
    if(incremental_enabled)padding=(WORD)bytes<0xfff8?(WORD)bytes:0xfff8;
    mac_jump_contribution=create_contribution(".data",bytes+padding,
        0xc0000040,0xc0000040,output_module,&sections,this);
    mac_jump_contribution->padding=padding;
    if(options.effective_reference_opt || options.fold_identical_comdats)
        attach_comdat(mac_jump_contribution,NULL,1);
    define_mac_jump_symbols(this);
}

/* The provisional registry name groups readable executable sections into .text;
 * it does not create debug payload or qualify any PDB path. */
void RECON_THISCALL LinkContext::PrepareMacDebugSections(void)
{
    SectionEnumerator enumerator;
    OutputSection *text=create_output_section(NULL,".text",0x60000020,
        &sections,&optional_header);
    initialize_section_enumerator(&enumerator,&sections);
    while(next_output_section(&enumerator)) {
        OutputSection *section=enumerator.current;
        if((section->characteristics&0x60000020)==0x60000020)
            merge_section_groups(section,text);
    }
    end_section_enumerator(&enumerator);
}

/* Group initialized readable/writable data using the existing sorted merge. */
void RECON_THISCALL LinkContext::PrepareMacRuntimeSections(void)
{
    SectionEnumerator enumerator;
    if(!data_section)return;
    initialize_section_enumerator(&enumerator,&sections);
    while(next_output_section(&enumerator)) {
        OutputSection *section=enumerator.current;
        if((section->characteristics&0xc0000040)==0xc0000040)
            merge_section_groups(section,data_section);
    }
    end_section_enumerator(&enumerator);
}

extern "C" {
extern int output_file;
/* File-cache writes are inline in the target; reload the physical globals
 * after each real seek/access child before consuming sizes and destinations. */
#define ZERO_MAC_CONTRIBUTION(contribution) do { \
    error=0; \
    seek_file(output_file,(contribution)->file_offset,FILE_BEGIN,&error); \
    bytes=(contribution)->raw_size; \
    fd=output_file; \
    if(file_access_statistics)record_file_access(); \
    file=file_slots[fd]; \
    if(file->flags&8)mapped_write(file,zeros,bytes); \
    else buffered_write(file,zeros,bytes); \
} while(0)
/* Provisional registry name retained. This routine initializes the five
 * generated Mac contribution spans in the output file, not symbol values. */
void __cdecl set_mac_symbol_addresses(void)
{
    DWORD size,bytes,error;
    BYTE *zeros;
    int fd;
    FileRecord *file;
    if(incremental_reuse)return;
    size=mac_toc->raw_size;
    if(size<=mac_function_info->raw_size)size=mac_function_info->raw_size;
    if(size<=mac_jump_contribution->raw_size)size=mac_jump_contribution->raw_size;
    if(size<=mac_glue_contribution->raw_size)size=mac_glue_contribution->raw_size;
    if(size<=mac_loader_contribution->raw_size)size=mac_loader_contribution->raw_size;
    zeros=(BYTE *)zero_allocate(size);
    ZERO_MAC_CONTRIBUTION(mac_toc);
    ZERO_MAC_CONTRIBUTION(mac_function_info);
    ZERO_MAC_CONTRIBUTION(mac_jump_contribution);
    ZERO_MAC_CONTRIBUTION(mac_glue_contribution);
    ZERO_MAC_CONTRIBUTION(mac_loader_contribution);
    release_memory(zeros);
}
#undef ZERO_MAC_CONTRIBUTION
}

extern "C" {
BYTE mac_sections_collected; /*4946bc*/
OutputSection **mac_output_sections; /*4946ac; allocated count plus one slot*/
DWORD mac_output_section_count; /*494550*/
DWORD mac_code_base,mac_data_base; /*4946b4/49459c*/
/* Resource sections begin with the actual two-character ";;" prefix.
 * The complete body computes a full integer boolean, consumed as AL here. */
int __stdcall is_mac_loadable_section(OutputSection *section)
{
    return strncmp(section->name,";;",strlen(";;"))!=0;
}
void __stdcall set_mac_section_addresses(SectionList *sections)
{
    SectionEnumerator enumerator;
    DWORD count=1,index;
    OutputSection *section,*temporary;
    BYTE eligible,swapped;
    if(mac_sections_collected)return;
    initialize_section_enumerator(&enumerator,sections);
    while(next_output_section(&enumerator)) {
        section=enumerator.current;
        if(section->rva && section->virtual_size)++count;
    }
    mac_output_sections=(OutputSection **)allocate_memory(count*4);
    count=0;
    initialize_section_enumerator(&enumerator,sections);
    while(next_output_section(&enumerator)) {
        section=enumerator.current;
        if(!section->rva || !section->virtual_size)continue;
        eligible=1;
        if((section->characteristics&0x60000020)==0x60000020)
            mac_code_base=section->rva;
        else if((section->characteristics&0xefffffff)==0xc0000040)
            mac_data_base=section->rva;
        else if(is_mac_loadable_section(section) &&
                (section->characteristics&0x42000040)!=0x42000040) {
            link_warning(NULL,4543,section->name);
            eligible=0;
        }
        if(eligible)mac_output_sections[count++]=section;
    }
    mac_output_section_count=count;
    /* The original assumes at least one eligible section: its unsigned
     * count-1 loop bound is retained, not repaired for an empty table. */
    do {
        swapped=0;
        for(index=0;index<mac_output_section_count-1;++index) {
            if(mac_output_sections[index]->rva<mac_output_sections[index+1]->rva) {
                temporary=mac_output_sections[index];
                mac_output_sections[index]=mac_output_sections[index+1];
                mac_output_sections[index+1]=temporary;
                swapped=1;
            }
        }
    } while(swapped);
    mac_sections_collected=1;
}
}

extern "C" {
/* The real import producer allocates these12B/16B linked owners, separate
 * from the24B serialized library record. */
struct MacImportSymbolNode {
    MacImportSymbolNode *next;
    StringNode *symbol;
    DWORD index;
};
struct MacImportLibraryNode {
    const char *library;
    MacImportedLibrary *record;
    MacImportSymbolNode *symbols;
    MacImportLibraryNode *next;
};
extern char short_symbol_name[9];
MacImportLibraryNode *mac_import_libraries; /*4946b0*/
typedef char MacImportOwnersLayout[(sizeof(MacImportSymbolNode)==12 &&
    offsetof(MacImportSymbolNode,symbol)==4 && offsetof(MacImportSymbolNode,index)==8 &&
    sizeof(MacImportLibraryNode)==16 && offsetof(MacImportLibraryNode,next)==12)?1:-1];
void __stdcall append_mac_import_symbol(LinkContext *context,MacImportLibraryNode *library,
    MacImportSymbolNode *symbol)
{
    MacImportSymbolNode **position=&library->symbols;
    const char *name;
    WORD kind;
    while(*position)position=&(*position)->next;
    *position=symbol;
    symbol->next=NULL;
    name=get_symbol_name(symbol->symbol,context->symbols->hash.string_storage,&kind,NULL);
    if(kind==1)name=strncpy(short_symbol_name,name,8);
    if(*name=='_')++name;
    mac_symbol_name_bytes+=strlen(name)+1;
}
void __stdcall add_mac_import_library(LinkContext *context,const char *name,
    MacImportSymbolNode *symbol)
{
    MacImportLibraryNode **position=&mac_import_libraries;
    MacImportLibraryNode *library;
    MacImportedLibrary *record;
    while(*position) {
        if(!strcmp((*position)->library,name))break;
        position=&(*position)->next;
    }
    library=*position;
    if(!library) {
        record=(MacImportedLibrary *)allocate_link_memory(1,sizeof(MacImportedLibrary));
        library=(MacImportLibraryNode *)allocate_link_memory(1,sizeof(MacImportLibraryNode));
        library->library=duplicate_link_string(name);
        library->record=record;
        *position=library;
        ++mac_import_library_count;
        mac_library_name_bytes+=strlen(library->library)+1;
    }
    ++library->record->symbol_count;
    append_mac_import_symbol(context,library,symbol);
    ++mac_import_symbol_count;
}
void __stdcall append_mac_map_symbol(StringNode *) { __asm { int 3 } }
char *__stdcall format_contribution_module(Contribution *,char *);
void __stdcall report_duplicate_symbol(LinkOptions *,const char *,const char *,const char *);
BYTE __stdcall add_mac_import(const unsigned char *name,const unsigned char *library,
    Contribution *contribution,LinkContext *context)
{
    StringNode *symbol=context->symbols->Lookup(name,NULL);
    char previous_name[512],module_name[528];
    MacImportSymbolNode *imported;
    DWORD bytes;
    if(name[0]=='.' && name[1]!='.') {
        set_symbol_defined(symbol,1,context->symbols);
        symbol->contribution=contribution;
        return 0;
    }
    if(symbol->flags&1) {
        if(!symbol->contribution)strcpy(previous_name,"a previous module");
        else format_contribution_module(symbol->contribution,previous_name);
        report_duplicate_symbol(&context->options,
            format_contribution_module(contribution,module_name),
            (const char *)name,previous_name);
        return 1;
    }
    symbol->attributes|=2;
    symbol->contribution=contribution;
    set_symbol_defined(symbol,1,context->symbols);
    if(library) {
        imported=(MacImportSymbolNode *)allocate_link_memory(1,sizeof(MacImportSymbolNode));
        imported->symbol=symbol;
        add_mac_import_library(context,(const char *)library,imported);
        if(!(symbol->attributes&0x10)) {
            bytes=mac_toc_entry_count*4;
            symbol->reserved36=(WORD)(bytes<0x8000?bytes:0x7ffc-bytes);
            ++mac_toc_entry_count;
            ++mac_relocation_count;
            ++fixup_count;
            symbol->attributes|=0x10;
            if(context->options.write_map)append_mac_map_symbol(symbol);
        }
        ++context->mac_glue_count;
    }
    return 1;
}

/* Provisional name retained: assigns contiguous import-symbol ordinals. */
void __cdecl prepare_mac_resource_map(void)
{
    MacImportLibraryNode *library=mac_import_libraries;
    MacImportSymbolNode *symbol;
    DWORD index=0;
    while(library) {
        symbol=library->symbols;
        while(symbol) {
            symbol->index=index++;
            symbol=symbol->next;
        }
        library=library->next;
    }
}
}

extern "C" {
void __stdcall prepare_mac_incremental_entry(StringNode *,LinkContext *) { __asm { int 3 } }
StringNode *__stdcall lookup_mac_entry_symbol(const char *,LinkContext *,DWORD);
void __stdcall record_debug_fixup(WORD,WORD,DWORD,DWORD);
void __stdcall append_mac_relocation(DWORD,DWORD,DWORD);
void __stdcall write_mac_entry(const char *name,StringNode *symbol,DWORD offset,
    LinkContext *context,DWORD physical_flag)
{
    StringNode *entry;
    DWORD relocation,toc,error;
    int fd;
    FileRecord *file;
    if(!(BYTE)physical_flag) {
        if((symbol->attributes&2) || !(symbol->attributes&0x100))return;
        entry=lookup_mac_entry_symbol(name,context,1);
    } else entry=symbol;
    if(entry->attributes&0x40)return;
    relocation=mac_jump_contribution->rva-mac_data_base+entry->symbol.Value;
    toc=mac_toc_symbol->final_value-mac_data_base;
    if(context->options.debug_formats&8) {
        DWORD address=mac_jump_contribution->rva+entry->symbol.Value;
        record_debug_fixup(2,0,address,offset+mac_code_base);
        record_debug_fixup(2,1,address+4,mac_toc_symbol->final_value);
    }
    error=0;
    seek_file(output_file,mac_jump_contribution->file_offset+entry->symbol.Value,
        FILE_BEGIN,&error);
    swap_byte_order(&offset,4);
    fd=output_file;
    if(file_access_statistics)record_file_access();
    file=file_slots[fd];
    if(file->flags&8)mapped_write(file,&offset,4);
    else buffered_write(file,&offset,4);
    swap_byte_order(&toc,4);
    fd=output_file;
    if(file_access_statistics)record_file_access();
    file=file_slots[fd];
    if(file->flags&8)mapped_write(file,&toc,4);
    else buffered_write(file,&toc,4);
    if(!(entry->attributes&0x400)) {
        append_mac_relocation(relocation,5,0);
        entry->attributes|=0x400;
    }
    entry->attributes|=0x40;
}
void __stdcall set_mac_entry_point(StringNode *entry,LinkContext *context)
{
    DWORD offset;
    const char *name;
    WORD kind;
    if(entry && entry->contribution) {
        offset=entry->symbol.Value-mac_code_base+entry->contribution->rva;
        if(incremental_reuse)prepare_mac_incremental_entry(entry,context);
        name=get_symbol_name(entry,context->symbols->hash.string_storage,&kind,NULL);
        if(kind==1)name=strncpy(short_symbol_name,name,8);
        write_mac_entry(name,entry,offset,context,0);
    }
    if(context->mac_initializer_name) {
        entry=context->symbols->Lookup(context->mac_initializer_name,NULL);
        offset=entry->contribution->rva+entry->symbol.Value-mac_code_base;
        if(incremental_reuse)prepare_mac_incremental_entry(entry,context);
        write_mac_entry((const char *)context->mac_initializer_name,entry,offset,context,0);
    }
    if(context->mac_terminator_name) {
        entry=context->symbols->Lookup(context->mac_terminator_name,NULL);
        offset=entry->contribution->rva+entry->symbol.Value-mac_code_base;
        if(incremental_reuse)prepare_mac_incremental_entry(entry,context);
        write_mac_entry((const char *)context->mac_terminator_name,entry,offset,context,0);
    }
}
}

extern "C" {
DWORD mac_export_key_offset,mac_export_record_offset; /*494564/494524*/
DWORD __stdcall write_mac_export_key(DWORD key,DWORD offset)
{
    DWORD error=0,keys;
    int fd;
    FileRecord *file;
    swap_byte_order(&key,4);
    keys=mac_export_key_offset;
    seek_file(output_file,mac_loader_contribution->file_offset+offset+keys,
        FILE_BEGIN,&error);
    fd=output_file;
    if(file_access_statistics)record_file_access();
    file=file_slots[fd];
    if(file->flags&8)mapped_write(file,&key,4);
    else buffered_write(file,&key,4);
    return 4;
}
DWORD __stdcall write_mac_export_name(const char *name)
{
    DWORD length=strlen(name),offset,strings,error=0;
    int fd;
    FileRecord *file;
    offset=mac_symbol_name_bytes;
    strings=mac_loader_string_cursor;
    seek_file(output_file,mac_loader_contribution->file_offset+offset+strings,
        FILE_BEGIN,&error);
    fd=output_file;
    if(file_access_statistics)record_file_access();
    file=file_slots[fd];
    if(file->flags&8)mapped_write(file,name,length+1);
    else buffered_write(file,name,length+1);
    offset=mac_symbol_name_bytes;
    mac_symbol_name_bytes=offset+length+1;
    return offset;
}
MacImportSymbolNode *__stdcall find_mac_import_symbol(LinkContext *,StringNode *)
{ __asm { int 3 } return NULL; }
#pragma pack(push,1)
struct MacLoaderExportRecord {
    BYTE kind,name_offset[3];
    DWORD value;
    WORD section;
};
#pragma pack(pop)
typedef char MacExportRecordLayout[(sizeof(MacLoaderExportRecord)==10 &&
    offsetof(MacLoaderExportRecord,value)==4 &&
    offsetof(MacLoaderExportRecord,section)==8)?1:-1];
void __stdcall write_mac_export_record(LinkContext *context,MacExportEntry *entry)
{
    MacLoaderExportRecord record;
    DWORD name,offset,error;
    StringNode *symbol;
    Contribution *contribution;
    OutputSection *section;
    MacImportSymbolNode *imported;
    const char *symbol_name;
    WORD kind;
    FileRecord *file;
    int fd;
    name=byte_swap_dword(write_mac_export_name(entry->name));
    record.name_offset[0]=(BYTE)(name>>8);
    record.name_offset[1]=(BYTE)(name>>16);
    record.name_offset[2]=(BYTE)(name>>24);
    symbol=entry->symbol;
    if(entry->is_function) {
        contribution=symbol->contribution;
        while(contribution && (contribution->characteristics&0x4000))
            contribution=contribution->associated_contribution;
        offset=symbol->symbol.Value;
        if(contribution)
            offset+=contribution->rva-contribution->group->section->rva;
        write_mac_entry(entry->name,entry->entry,offset,context,1);
        symbol=entry->entry;
        contribution=symbol->contribution;
        while(contribution && (contribution->characteristics&0x4000))
            contribution=contribution->associated_contribution;
        record.kind=2;
        record.value=symbol->symbol.Value-contribution->group->section->rva+
            contribution->rva;
        record.section=(WORD)(contribution->group->section->index-1);
    } else if(symbol->attributes&2) {
        imported=find_mac_import_symbol(context,symbol);
        symbol=entry->symbol;
        record.kind=(symbol->symbol.Type&0x30)==0x20?2:1;
        record.value=imported->index;
        record.section=(WORD)-3;
        symbol_name=get_symbol_name(symbol,context->symbols->hash.string_storage,
            &kind,NULL);
        if(kind==1)symbol_name=strncpy(short_symbol_name,symbol_name,8);
        link_warning(NULL,4101,symbol_name);
    } else {
        contribution=symbol->contribution;
        offset=symbol->symbol.Value;
        while(contribution && (contribution->characteristics&0x4000))
            contribution=contribution->associated_contribution;
        if(contribution) {
            section=contribution->group->section;
            offset+=contribution->rva-section->rva;
            record.kind=(symbol->symbol.Type&0x30)==0x20?0:1;
            record.value=offset;
            record.section=(WORD)(section->index-1);
        } else {
            record.kind=1;
            record.value=offset;
            record.section=(WORD)-2;
        }
    }
    swap_byte_order(&record.value,4);
    swap_byte_order(&record.section,2);
    offset=mac_export_record_offset;
    error=0;
    seek_file(output_file,mac_loader_contribution->file_offset+offset,FILE_BEGIN,&error);
    fd=output_file;
    if(file_access_statistics)record_file_access();
    file=file_slots[fd];
    if(file->flags&8)mapped_write(file,&record,10);
    else buffered_write(file,&record,10);
    mac_export_record_offset+=10;
}
DWORD __stdcall write_mac_export_bucket(DWORD bucket,DWORD offset,LinkContext *context)
{
    MacExportEntry *entry=mac_export_buckets+bucket;
    if(entry->symbol) {
        while(entry) {
            offset+=write_mac_export_key(entry->hash,offset);
            write_mac_export_record(context,entry);
            entry=entry->next;
        }
    }
    return offset;
}
void __stdcall write_mac_export_hash_slot(DWORD offset,DWORD count,DWORD first_key)
{
    DWORD slot=(count<<18)|((first_key>>2)&0x3ffff);
    DWORD error=0,hashes;
    int fd;
    FileRecord *file;
    swap_byte_order(&slot,4);
    hashes=mac_export_hash_offset;
    seek_file(output_file,mac_loader_contribution->file_offset+offset+hashes,
        FILE_BEGIN,&error);
    fd=output_file;
    if(file_access_statistics)record_file_access();
    file=file_slots[fd];
    if(file->flags&8)mapped_write(file,&slot,4);
    else buffered_write(file,&slot,4);
}
/* Provisional name retained: emit bucket keys/records and nonempty hash slots. */
void __stdcall prepare_mac_data_headers(LinkContext *context)
{
    DWORD slots,bucket,offset,previous=0,count;
    if(!mac_export_count)return;
    slots=1UL<<(mac_export_hash_power_value&31);
    mac_export_key_offset=mac_export_hash_offset+slots*4;
    mac_export_record_offset=mac_export_key_offset+mac_export_count*4;
    for(bucket=0;bucket<slots;++bucket) {
        offset=write_mac_export_bucket(bucket,previous,context);
        count=(offset-previous)/4;
        if(count)write_mac_export_hash_slot(bucket*4,count,previous);
        previous=offset;
    }
}
}

extern "C" {
/* Separate physical template4673b8, not an alias into mac_import_glue. */
DWORD mac_glue_extension[]={
    0x800c0000UL, /* lwz r0,0(r12) */
    0x90410014UL, /* stw r2,20(r1) */
    0x7c0903a6UL, /* mtctr r0 */
    0x804c0004UL, /* lwz r2,4(r12) */
    0x4e800420UL  /* bctr */
};
/* Provisional name retained: append the new-glue dispatch extension. */
void __stdcall prepare_mac_debug_information(LinkContext *context)
{
    StringNode *symbol;
    FileRecord *file;
    int fd;
    DWORD error=0;
    if(!context->options.new_glue)return;
    if(!incremental_enabled && !context->mac_glue_count)return;
    swap_byte_order(mac_glue_extension,sizeof(mac_glue_extension));
    symbol=context->symbols->Lookup((const unsigned char *)"__glueExtension",NULL);
    set_symbol_defined(symbol,1,context->symbols);
    symbol->contribution=mac_glue_contribution;
    symbol->symbol.Value=context->mac_glue_bytes_used;
    symbol->final_value=mac_glue_contribution->rva+context->mac_glue_bytes_used;
    seek_file(output_file,mac_glue_contribution->file_offset+context->mac_glue_bytes_used,
        FILE_BEGIN,&error);
    fd=output_file;
    if(file_access_statistics)record_file_access();
    file=file_slots[fd];
    if(file->flags&8)mapped_write(file,mac_glue_extension,sizeof(mac_glue_extension));
    else buffered_write(file,mac_glue_extension,sizeof(mac_glue_extension));
    context->mac_glue_bytes_used+=20;
}
}

extern "C" {
void __stdcall pdb_define_section(const char *,DWORD,DWORD);
void __stdcall prepare_mac_pass_two(LinkContext *context)
{
    StringTable *symbols=context->symbols;
    DWORD count=count_hash_symbols(symbols->hash.state),index,section_carrier;
    StringNode *symbol;
    Contribution *contribution;
    OutputSection *section;
    const char *name;
    WORD kind;
    initialize_hash_enumerator(symbols->hash.state);
    for(index=0;index<count;++index) {
        symbol=symbols->NextSymbol();
        if(!(symbol->attributes&0x20))continue;
        contribution=symbol->contribution;
        while(contribution && (contribution->characteristics&0x4000))
            contribution=contribution->associated_contribution;
        section=contribution->group->section;
        symbol->final_value=symbol->symbol.Value+contribution->rva;
        if(pdb_enabled) {
            name=get_symbol_name(symbol,symbols->hash.string_storage,&kind,NULL);
            if(kind==1)name=strncpy(short_symbol_name,name,8);
            /*451359..451363 forwards the defined RVA high16 with the index
             * replacing only low16; this is transport, not a PDB interpretation. */
            section_carrier=(section->rva&0xffff0000)|section->index;
            pdb_define_section(name,section_carrier,symbol->final_value-section->rva);
        }
    }
    end_hash_enumerator(symbols->hash.state);
}
}

extern "C" {
extern ContributionGroup *mac_procedure_group;
void __stdcall record_debug_fixup(WORD,WORD,DWORD,DWORD);
DWORD __stdcall mac_section_relative_address(DWORD address,BYTE *is_code)
{
    DWORD index;
    OutputSection *section;
    for(index=0;index<mac_output_section_count;++index) {
        section=mac_output_sections[index];
        if(address>=section->rva) {
            if(is_code)*is_code=(BYTE)((section->characteristics&0x60000020)==0x60000020);
            return address-section->rva;
        }
    }
    /* The original leaves the optional output byte untouched on no match. */
    return address;
}
void __stdcall append_mac_relocation(DWORD offset,DWORD kind,DWORD argument)
{
    mac_relocation_cursor->offset=offset;
    mac_relocation_cursor->kind=kind&255;
    mac_relocation_cursor->encoded=0;
    mac_relocation_cursor->count=0;
    mac_relocation_cursor->argument=argument;
    ++mac_relocation_cursor;
    ++mac_relocation_record_count;
}
void __stdcall prepare_mac_procedure_data(LinkContext *,DWORD) { __asm { int 3 } }
struct MacFunctionInformation {
    DWORD reserved00[2];
    DWORD procedure_offset,procedure_count,reserved10,text_size;
};
typedef char MacFunctionInformationSize[(sizeof(MacFunctionInformation)==24)?1:-1];
#define WRITE_MAC_HEADER(data,bytes) do { \
    fd=output_file; \
    if(file_access_statistics)record_file_access(); \
    file=file_slots[fd]; \
    if(file->flags&8)mapped_write(file,(data),(bytes)); \
    else buffered_write(file,(data),(bytes)); \
} while(0)
/* Provisional registry name retained: finish TOC/function information and
 * its real relocation records, rather than serialize the final PEF header. */
void __stdcall finish_mac_headers(LinkContext *context)
{
    MacFunctionInformation information;
    OutputSection *text;
    DWORD value,error;
    BYTE is_code;
    FileRecord *file;
    int fd;
    if(context->options.debug_formats&8)
        record_debug_fixup(2,0,mac_toc_symbol->final_value+64,
            mac_function_symbol->final_value);
    error=0;
    seek_file(output_file,get_mac_toc_bias()+mac_toc->file_offset+64,FILE_BEGIN,&error);
    /* The original uses a dead formal slot for this byte output; the byte
     * is never consumed here, including the helper's no-match route. */
    value=mac_section_relative_address(mac_function_symbol->final_value,&is_code);
    swap_byte_order(&value,4);
    WRITE_MAC_HEADER(&value,4);
    mac_function_symbol->attributes|=0x201;
    append_mac_relocation(mac_toc_symbol->final_value-mac_data_base+64,1,0);
    information.reserved00[0]=0;
    information.reserved00[1]=0;
    if(mac_procedure_group) {
        if(context->options.debug_formats&8)
            record_debug_fixup(2,2,mac_function_info->rva+8,mac_procedure_group->rva);
        information.procedure_offset=mac_procedure_group->rva-mac_data_base;
        information.procedure_count=incremental_enabled?context->mac_procedure_count:
            mac_procedure_group->size/20;
        append_mac_relocation(mac_function_info->rva-mac_data_base+8,1,0);
        if(!incremental_enabled)
            prepare_mac_procedure_data(context,mac_procedure_group->size);
    } else {
        information.procedure_offset=0;
        information.procedure_count=0;
    }
    information.reserved10=0;
    text=find_matching_output_section(NULL,".text",0x60100020,
        &context->sections,&context->optional_header);
    information.text_size=text->raw_size;
    swap_byte_order(&information,sizeof(information));
    error=0;
    seek_file(output_file,mac_function_info->file_offset,FILE_BEGIN,&error);
    WRITE_MAC_HEADER(&information,sizeof(information));
    if(context->options.debug_formats&8)
        record_debug_fixup(2,2,mac_function_info->rva+16,mac_code_base);
    append_mac_relocation(mac_function_info->rva-mac_data_base+16,2,0);
}
#undef WRITE_MAC_HEADER
}

extern "C" {
extern DWORD mac_current_import_version;
extern LONG mac_old_api_version;
extern InputArguments weak_mac_libraries,weak_mac_imports;
StringNode *__stdcall lookup_mac_entry_symbol(const char *name,LinkContext *context,
    DWORD physical_flag)
{
    char *buffer=(char *)allocate_memory(strlen(name)+2);
    StringNode *symbol;
    strcpy(buffer,".");
    strcat(buffer,name);
    if((BYTE)physical_flag)symbol=context->symbols->Lookup((const unsigned char *)buffer,NULL);
    else symbol=context->symbols->FindSymbol(buffer);
    release_memory(buffer);
    return symbol;
}
void __stdcall release_mac_import_versions(MacImportVersions *);
int __cdecl compare_mac_relocation_records(const void *left,const void *right)
{
    DWORD a=((const MacRelocationRecord *)left)->offset;
    DWORD b=((const MacRelocationRecord *)right)->offset;
    if(a<b)return -1;
    return a>b?1:0;
}
struct MacRelocationHeader {
    WORD section,reserved;
    DWORD count,offset;
};
typedef char MacRelocationHeaderLayout[(sizeof(MacRelocationHeader)==12 &&
    offsetof(MacRelocationHeader,count)==4)?1:-1];
void __stdcall write_mac_relocation_stream(DWORD offset)
{
    MacRelocationHeader header;
    MacRelocationRecord *record;
    DWORD error=0,total,index=0,written=0,encoded,count,address,next;
    FileRecord *file;
    int fd;
    seek_file(output_file,mac_loader_contribution->file_offset+offset,FILE_BEGIN,&error);
    total=mac_relocation_record_count;
    mac_relocation_cursor=mac_relocation_records;
    while(index<total) {
        record=mac_relocation_cursor;
        encoded=written;
        count=1;
        address=record->offset;
        switch(record->kind) {
        case 1:
            encoded=(address&0x1fffffff)|0x20000000;
            if(!incremental_enabled) {
                --total;
                while(index<total && record[1].kind==1) {
                    next=record[1].offset;
                    if(next!=address+4)break;
                    ++count;
                    ++index;
                    ++mac_relocation_cursor;
                    record=mac_relocation_cursor;
                    address=next;
                    if(count>=63)break;
                }
            }
            break;
        case 2:encoded=(address&0x1fffffff)|0x80000000;break;
        case 5:encoded=(address&0x1fffffff)|0xc0000000;break;
        case 8:
            encoded=(address&0x1fffffff)|0x60000000;
            count=((MacImportSymbolNode *)record->argument)->index;
            break;
        default:
            /* The target's sparse switch retains the running emitted count
             * in this value slot for its other kind values. */
            break;
        }
        record->encoded=encoded;
        mac_relocation_cursor->count=count;
        fd=output_file;
        if(file_access_statistics)record_file_access();
        file=file_slots[fd];
        if(file->flags&8)mapped_write(file,&mac_relocation_cursor->encoded,8);
        else buffered_write(file,&mac_relocation_cursor->encoded,8);
        ++written;
        ++index;
        ++mac_relocation_cursor;
        total=mac_relocation_record_count;
    }
    header.section=byte_swap_word(1);
    header.reserved=0;
    header.count=byte_swap_dword(written);
    header.offset=0;
    error=0;
    seek_file(output_file,mac_loader_contribution->file_offset+mac_relocation_header_offset,
        FILE_BEGIN,&error);
    fd=output_file;
    if(file_access_statistics)record_file_access();
    file=file_slots[fd];
    if(file->flags&8)mapped_write(file,&header,sizeof(header));
    else buffered_write(file,&header,sizeof(header));
}
#define WRITE_MAC_LOADER(data,bytes) do { \
    fd=output_file; \
    if(file_access_statistics)record_file_access(); \
    file=file_slots[fd]; \
    if(file->flags&8)mapped_write(file,(data),(bytes)); \
    else buffered_write(file,(data),(bytes)); \
} while(0)
/* These are the exact byte-sized matched/used flags; neighboring carried
 * words and unconsumed bytes retain their prior owners and values. */
#define MAC_ARGUMENT_USED(argument) (((BYTE *)&(argument)->flags)[1])
#define MAC_VERSION_USED(version) (((BYTE *)&(version)->reserved)[1])
void __stdcall finish_mac_entry_point(StringNode *entry,LinkContext *context)
{
    DWORD version,strings,relocations,error,offset,length,index,first_symbol,next_symbol;
    DWORD name_offset;
    WORD kind;
    StringNode *symbol;
    Contribution *contribution;
    MacImportLibraryNode *library;
    MacImportSymbolNode *imported;
    MacImportVersion *bound;
    InputArgument *argument;
    MacImportedSymbol import_record;
    const char *name;
    FileRecord *file;
    int fd;
    if(context->file_header.Characteristics&IMAGE_FILE_DLL) {
        context->optional_header.MajorImageVersion=(WORD)(mac_current_import_version>>16);
        context->optional_header.MinorImageVersion=(WORD)mac_current_import_version;
        version=mac_old_api_version==-1?0:(DWORD)mac_old_api_version;
        context->optional_header.MajorSubsystemVersion=(WORD)(version>>16);
        context->optional_header.MinorSubsystemVersion=(WORD)version;
    }
    if(entry) {
        if(entry->attributes&0x100) {
            name=get_symbol_name(entry,context->symbols->hash.string_storage,&kind,NULL);
            if(kind==1)name=strncpy(short_symbol_name,name,8);
            entry=lookup_mac_entry_symbol(name,context,1);
        }
        contribution=entry->contribution;
        while(contribution && (contribution->characteristics&0x4000))
            contribution=contribution->associated_contribution;
        if(contribution) {
            mac_loader_header.main_section=1;
            mac_loader_header.main_offset=entry->symbol.Value+contribution->rva-mac_data_base;
        }
    } else {
        mac_loader_header.main_section=-1;
        mac_loader_header.main_offset=0;
    }
    if(context->mac_initializer_name) {
        symbol=lookup_mac_entry_symbol((const char *)context->mac_initializer_name,context,1);
        contribution=symbol->contribution;
        while(contribution && (contribution->characteristics&0x4000))
            contribution=contribution->associated_contribution;
        mac_loader_header.initializer_section=1;
        mac_loader_header.initializer_offset=symbol->symbol.Value+contribution->rva-mac_data_base;
    } else {
        mac_loader_header.initializer_section=-1;
        mac_loader_header.initializer_offset=0;
    }
    if(context->mac_terminator_name) {
        symbol=lookup_mac_entry_symbol((const char *)context->mac_terminator_name,context,1);
        contribution=symbol->contribution;
        while(contribution && (contribution->characteristics&0x4000))
            contribution=contribution->associated_contribution;
        mac_loader_header.terminator_section=1;
        mac_loader_header.terminator_offset=symbol->symbol.Value+contribution->rva-mac_data_base;
    } else {
        mac_loader_header.terminator_section=-1;
        mac_loader_header.terminator_offset=0;
    }
    mac_loader_header.library_count=mac_import_library_count;
    mac_loader_header.import_symbol_count=mac_import_symbol_count;
    mac_loader_header.relocation_section_count=1;
    relocations=mac_relocation_instruction_offset;
    strings=mac_loader_string_cursor;
    error=0;
    seek_file(output_file,mac_loader_contribution->file_offset+strings,FILE_BEGIN,&error);
    offset=0;
    for(library=mac_import_libraries;library;library=library->next) {
        library->record->name_offset=offset;
        name=library->library;
        length=strlen(name)+1;
        WRITE_MAC_LOADER(name,length);
        offset+=length;
    }
    swap_byte_order(&mac_loader_header,sizeof(mac_loader_header));
    error=0;
    seek_file(output_file,mac_loader_contribution->file_offset,FILE_BEGIN,&error);
    WRITE_MAC_LOADER(&mac_loader_header,sizeof(mac_loader_header));
    first_symbol=0;
    for(library=mac_import_libraries;library;library=library->next) {
        argument=weak_mac_libraries.first;
        for(index=0;(WORD)index<weak_mac_libraries.count;++index,argument=argument->next) {
            if(!(MAC_ARGUMENT_USED(argument)&1) && !_stricmp(argument->name,library->library)) {
                MAC_ARGUMENT_USED(argument)|=1;
                library->record->options=0x40;
                break;
            }
        }
        bound=mac_library_current_versions.first;
        for(index=0;(WORD)index<mac_library_current_versions.count;++index,bound=bound->next) {
            if(!(MAC_VERSION_USED(bound)&1) && !_stricmp(bound->library,library->library)) {
                library->record->current_version=bound->version;
                MAC_VERSION_USED(bound)|=1;
                break;
            }
        }
        bound=mac_library_old_code_versions.first;
        for(index=0;(WORD)index<mac_library_old_code_versions.count;++index,bound=bound->next) {
            if(!(MAC_VERSION_USED(bound)&1) && !_stricmp(bound->library,library->library)) {
                library->record->old_version=bound->version;
                MAC_VERSION_USED(bound)|=1;
                break;
            }
        }
        library->record->first_symbol=first_symbol;
        next_symbol=first_symbol+library->record->symbol_count;
        swap_byte_order(library->record,20);
        WRITE_MAC_LOADER(library->record,sizeof(*library->record));
        first_symbol=next_symbol;
    }
    mac_import_symbols_offset+=mac_loader_contribution->file_offset;
    for(library=mac_import_libraries;library;library=library->next) {
        for(imported=library->symbols;imported;imported=imported->next) {
            name=get_symbol_name(imported->symbol,context->symbols->hash.string_storage,&kind,NULL);
            if(kind==1)name=strncpy(short_symbol_name,name,8);
            if(*name=='_')++name;
            name_offset=write_mac_export_name(name);
            swap_byte_order(&name_offset,4);
            import_record.name_offset[0]=(BYTE)(name_offset>>8);
            import_record.name_offset[1]=(BYTE)(name_offset>>16);
            import_record.name_offset[2]=(BYTE)(name_offset>>24);
            import_record.kind=(imported->symbol->symbol.Type&0x30)==0x20?2:1;
            argument=weak_mac_imports.first;
            for(index=0;(WORD)index<weak_mac_imports.count;++index,argument=argument->next) {
                if(!(MAC_ARGUMENT_USED(argument)&1) && !strcmp(name,argument->name)) {
                    MAC_ARGUMENT_USED(argument)|=1;
                    import_record.kind|=0x80;
                    break;
                }
            }
            error=0;
            seek_file(output_file,mac_import_symbols_offset,FILE_BEGIN,&error);
            WRITE_MAC_LOADER(&import_record,sizeof(import_record));
            mac_import_symbols_offset+=4;
        }
    }
    argument=weak_mac_imports.first;
    for(index=0;(WORD)index<weak_mac_imports.count;++index,argument=argument->next)
        if(!(argument->flags&0x100))link_warning(NULL,4544,argument->name);
    argument=weak_mac_libraries.first;
    for(index=0;(WORD)index<weak_mac_libraries.count;++index,argument=argument->next)
        if(!(argument->flags&0x100))link_warning(NULL,4545,argument->name);
    bound=mac_library_current_versions.first;
    for(index=0;(WORD)index<mac_library_current_versions.count;++index,bound=bound->next)
        if(!(bound->reserved&0x100))link_warning(NULL,4545,bound->library);
    bound=mac_library_old_code_versions.first;
    for(index=0;(WORD)index<mac_library_old_code_versions.count;++index,bound=bound->next)
        if(!(bound->reserved&0x100))link_warning(NULL,4545,bound->library);
    release_mac_import_versions(&mac_library_current_versions);
    release_mac_import_versions(&mac_library_old_code_versions);
    release_mac_import_versions(&mac_current_version_history);
    release_mac_import_versions(&mac_old_code_version_history);
    release_mac_import_versions(&mac_old_api_version_history);
    qsort(mac_relocation_records,mac_relocation_record_count,sizeof(MacRelocationRecord),
        compare_mac_relocation_records);
    write_mac_relocation_stream(relocations);
    /* The original+1f4 DWORD retains this list pointer for later Mac output. */
    context->reserved1f4=(DWORD)mac_import_libraries;
}
#undef MAC_VERSION_USED
#undef MAC_ARGUMENT_USED
#undef WRITE_MAC_LOADER
}

extern "C" {
char *__stdcall make_mac_entry_name(char *,const char *) { __asm { int 3 } return NULL; }
/* Provisional name retained: this creates the dot-prefixed Mac jump entry,
 * independently of whether the link itself is incremental. */
StringNode *__stdcall record_incremental_entry(const char *name,Contribution *contribution,
    LinkContext *context,DWORD physical_flag)
{
    char *buffer=(char *)allocate_memory(strlen(name)+7);
    StringNode *symbol;
    strcpy(buffer,".");
    strcat(buffer,name);
    symbol=context->symbols->Lookup((const unsigned char *)buffer,NULL);
    /* All32bits are physically forwarded; this complete consumer reads BYTE. */
    if((BYTE)physical_flag) {
        set_symbol_defined(symbol,1,context->symbols);
        symbol->contribution=contribution;
        buffer=make_mac_entry_name(buffer,name);
        symbol=context->symbols->Lookup((const unsigned char *)buffer,NULL);
    }
    if(!(symbol->attributes&0x20) && !(symbol->attributes&0x100)) {
        set_symbol_defined(symbol,1,context->symbols);
        symbol->symbol.SectionNumber=-2;
        symbol->symbol.Value=mac_jump_entry_count*8;
        ++mac_jump_entry_count;
        ++mac_relocation_count;
        fixup_count+=2;
        symbol->attributes|=0x20;
    }
    symbol->attributes|=0x100;
    release_memory(buffer);
    return symbol;
}
}
