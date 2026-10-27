/* Mac PowerPC machine setup. The generated PPC glue is output data, never
 * executed by this native linker. */
#include <windows.h>
#include <time.h>
#include <string.h>
#include "link.h"
#include "input.h"
#include "options.h"
#include "memory.h"
#include "pass-one.h"
#include "sections.h"
#include "full-link-functions.h"
#include "mac-format.h"
#include <recon.h>

extern "C" {
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
            (DirectiveModule *)contribution,context))
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
        context->reserved1e8=0;
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
DWORD mac_relocation_header_offset,mac_relocation_instruction_offset; /*494540/4944fc*/
DWORD mac_loader_strings_offset,mac_loader_string_cursor,mac_export_hash_offset; /*494500/514/504*/
BYTE *mac_relocation_records,*mac_relocation_cursor; /*494598/494554,same allocation*/
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
    mac_relocation_records=(BYTE *)zero_allocate((mac_relocation_count+1)*20);
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
BYTE __stdcall open_mac_export_section(int *) { __asm { int 3 } return 0; }
DWORD __stdcall mac_export_hash_power(LONG) { __asm { int 3 } return 0; }
void __stdcall add_mac_export(LinkContext *,StringNode *,const char *,const char *,DWORD) { __asm { int 3 } }
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
DWORD mac_export_hash_power_value,mac_export_count; /*494508/49450c*/
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
