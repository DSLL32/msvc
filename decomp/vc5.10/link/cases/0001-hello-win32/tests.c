/* Narrow live-state check for the real hello invocation, before object loading. */
#include <recon-tool.h>
#include <recon-native.h>
#include <string.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/wait.h>
#include "../../src/input.h"
#include "../../src/memory.h"
#include "../../src/comdat.h"
#include "../../src/initializers.h"
#include "../../src/files.h"
#include "../../src/options.h"
#include "../../src/pass-one.h"
#include "../../src/full-link-functions.h"
#include "../../src/sections.h"
#include "../../harness/startup.h"
#include <recon.h>

extern MemoryArena *section_header_arena;

static void __stdcall check_section_headers(LinkContext *context,void *parent,
    OutputModule *module,DWORD flags)
{
    unsigned i;
    BYTE *bytes;
    int pass=context && !parent && module && !flags;
    if (!pass) recon_native_entry_finish(0);
    pass=module->library==context->command_line_library &&
        module->library->module_head==module && module->section_count==7 &&
        !strcmp(module->name,"hello.obj") && !module->section_headers &&
        module->contribution_info && section_header_arena &&
        !section_header_arena->previous && section_header_arena->used==280 &&
        section_header_arena->remaining==4096-280 &&
        section_header_arena->base==(BYTE *)module->contribution_info;
    if (!pass) recon_native_entry_finish(0);
    bytes=(BYTE *)module->contribution_info;
    for (i=0;i<280;++i) if (bytes[i]) recon_native_entry_finish(0);
    recon_native_entry_finish(1);
}
static void invoke_hello(void *unused)
{
    char *args[]={"LINK.EXE","/nologo","/incremental:no","/out:hello.exe",
        "hello.obj","kernel32.lib",NULL};
    (void)unused;
    recon_link_invoke(6,args);
}
static int section_headers(void)
{
    return recon_native_at_entry(load_object_module,check_section_headers,
        invoke_hello,NULL)==1;
}
/* C harness spelling of the genuine native member entry: ECX module, one
   stack output-pointer argument. No adapter enters the application graph. */
extern IMAGE_SECTION_HEADER *RECON_THISCALL native_read_section_headers(
    OutputModule *,IMAGE_SECTION_HEADER **)
    __asm__("_ZN12OutputModule18ReadSectionHeadersEPP20IMAGE_SECTION_HEADER");
extern OutputLibrary *current_input_library;
static void RECON_THISCALL check_section_header_call(OutputModule *module,
    IMAGE_SECTION_HEADER **headers)
{
    recon_native_entry_finish(module && headers &&
        module->library==current_input_library &&
        module->library->module_head==module && module->section_count==7 &&
        module->contribution_info && !module->section_headers &&
        sizeof(*headers)==4);
}
static int section_header_call(void)
{
    return recon_native_at_entry(native_read_section_headers,check_section_header_call,
        invoke_hello,NULL)==1;
}
extern BYTE *module_section_map;
extern IMAGE_SECTION_HEADER *module_section_headers;
extern unsigned module_section_capacity;
extern IMAGE_SYMBOL *__stdcall read_module_symbols(DWORD,DWORD,BYTE);
static void __stdcall check_section_header_read(DWORD offset,DWORD count,BYTE force_copy)
{
    static const WORD relocations[7]={0,8,0,1,13,0,1};
    static const DWORD reloc_offsets[7]={0,543,0,653,967,0,1125};
    static const DWORD raw_offsets[7]={300,383,623,637,663,1097,1109};
    OutputModule *module=current_input_library ? current_input_library->module_head : NULL;
    FileRecord *file=file_slots[current_input_file];
    Contribution *parts;
    unsigned i;
    int pass=module && offset==1135 && count==37 && !force_copy &&
        module_section_capacity==500 && module_section_headers &&
        module->section_map==(SectionFileMap *)module_section_map &&
        file && (file->flags & 8) && file->size==2262 && file->file_size==300 &&
        file->view && !memcmp(module_section_headers,(BYTE *)file->view+20,280);
    if (!pass) recon_native_entry_finish(0);
    parts=(Contribution *)module->contributions;
    for (i=0;i<7;++i) {
        if (parts[i].relocation_count!=relocations[i] ||
            parts[i].line_number_count ||
            parts[i].relocation_offset!=reloc_offsets[i] ||
            parts[i].input_raw_offset!=raw_offsets[i] ||
            module->section_map[i].virtual_address ||
            module->section_map[i].line_number_offset)
            recon_native_entry_finish(0);
    }
    recon_native_entry_finish(1);
}
static int section_header_read(void)
{
    return recon_native_at_entry(read_module_symbols,check_section_header_read,
        invoke_hello,NULL)==1;
}
extern const char *default_phase;
extern BYTE module_symbols_active,module_symbols_mapped;
extern ByteBuffer symbol_read_buffer;
extern void *__stdcall map_file_range(int,LONG,unsigned);
static void __stdcall check_symbol_read_mapping(int file,LONG offset,unsigned bytes)
{
    recon_native_entry_finish(file==current_input_file && offset==1135 && bytes==666 &&
        !strcmp(default_phase,"ReadSymbolTable") && module_symbols_active==1 &&
        !module_symbols_mapped && !symbol_read_buffer.data &&
        !symbol_read_buffer.used && !symbol_read_buffer.capacity &&
        file_slots[file]->file_size==300);
}
static int symbol_read_mapping(void)
{
    return recon_native_at_entry(map_file_range,check_symbol_read_mapping,
        invoke_hello,NULL)==1;
}
extern char *__stdcall read_module_string_table(const char *,DWORD,DWORD *);
static void __stdcall check_symbol_mapping(const char *name,DWORD offset,DWORD *size)
{
    FileRecord *file=file_slots[current_input_file];
    recon_native_entry_finish(name && !strcmp(name,"hello.obj") && offset==1801 && size &&
        file && file->view && file->size==2262 && file->file_size==300 &&
        module_symbols_active==1 && module_symbols_mapped==1 &&
        !symbol_read_buffer.data && !strcmp(default_phase,"ReadSymbolTable") &&
        map_file_range(current_input_file,1135,666)==(BYTE *)file->view+1135);
}
static int symbol_mapping(void)
{
    return recon_native_at_entry(read_module_string_table,check_symbol_mapping,
        invoke_hello,NULL)==1;
}
extern BYTE module_strings_active,module_strings_mapped;
extern ByteBuffer string_read_buffer;
extern char *current_module_strings;
extern StringNode **current_module_symbols;
extern DWORD coff_string_size;
extern void __stdcall add_input_section(LinkContext *,OutputModule *,DWORD,
    IMAGE_SECTION_HEADER *,IMAGE_SYMBOL *,InputArguments *);
static void __stdcall check_string_mapping(LinkContext *context,OutputModule *module,
    DWORD index,IMAGE_SECTION_HEADER *header,IMAGE_SYMBOL *symbols,InputArguments *pending)
{
    FileRecord *file=file_slots[current_input_file];
    unsigned i;
    int pass=context && module==current_input_library->module_head && index==1 &&
        header==module_section_headers && symbols==(IMAGE_SYMBOL *)((BYTE *)file->view+1135) &&
        current_module_strings==(char *)file->view+1801 &&
        *(DWORD *)current_module_strings==461 && !current_module_strings[460] &&
        module_strings_active==1 && module_strings_mapped==1 &&
        module_symbols_active==1 && module_symbols_mapped==1 &&
        !string_read_buffer.data && !string_read_buffer.capacity &&
        current_module_symbols && pending && file->file_size==300;
    if (!pass) recon_native_entry_finish(0);
    for (i=0;i<37;++i) if (current_module_symbols[i]) recon_native_entry_finish(0);
    for (i=0;i<sizeof(*pending);++i) if (((BYTE *)pending)[i]) recon_native_entry_finish(0);
    recon_native_entry_finish(1);
}
static int string_mapping(void)
{
    return recon_native_at_entry(add_input_section,check_string_mapping,
        invoke_hello,NULL)==1;
}
extern char *__stdcall get_input_section_name(IMAGE_SECTION_HEADER *,const char *);
static void __stdcall check_section_name_call(IMAGE_SECTION_HEADER *header,const char *strings)
{
    FileRecord *file=file_slots[current_input_file];
    recon_native_entry_finish(header==module_section_headers &&
        !memcmp(header->Name,".drectve",8) && strings==current_module_strings &&
        strings==(char *)file->view+1801 && *(DWORD *)strings==461);
}
static int section_name_call(void)
{
    return recon_native_at_entry(get_input_section_name,check_section_name_call,
        invoke_hello,NULL)==1;
}
extern char input_section_name[9];
extern Contribution *__stdcall create_contribution(const char *,DWORD,DWORD,DWORD,
    OutputModule *,SectionList *,LinkContext *);
static void __stdcall check_named_contribution(const char *name,DWORD bytes,DWORD flags,
    DWORD section_flags,OutputModule *module,SectionList *sections,LinkContext *context)
{
    recon_native_entry_finish(name==input_section_name && !strcmp(name,".drectve") &&
        input_section_name[8]==0 && bytes==module_section_headers[0].SizeOfRawData &&
        flags==module_section_headers[0].Characteristics && section_flags==flags &&
        module==current_input_library->module_head && context && sections==&context->sections);
}
static int named_contribution(void)
{
    return recon_native_at_entry(create_contribution,check_named_contribution,
        invoke_hello,NULL)==1;
}
extern void __stdcall split_section_name(const char *,const char **);
static void __stdcall check_section_name_split(const char *name,const char **base)
{
    OutputModule *module=current_input_library->module_head;
    recon_native_entry_finish(name==input_section_name && !strcmp(name,".drectve") &&
        base && sizeof(*base)==4 && module && !module->contribution_count && module->section_count==7);
}
static int section_name_split(void)
{
    return recon_native_at_entry(split_section_name,check_section_name_split,
        invoke_hello,NULL)==1;
}
extern void __stdcall mark_incremental_contribution(Contribution *);
static void __stdcall check_first_contribution(Contribution *contribution)
{
    OutputModule *module=current_input_library->module_head;
    ContributionGroup *group=contribution->group;
    Contribution expected;
    memset(&expected,0,sizeof(expected));
    expected.characteristics=0x100a00;
    expected.raw_size=83;
    expected.module=module;
    expected.group=group;
    expected.input_raw_offset=300;
    recon_native_entry_finish(contribution==(Contribution *)module->contributions &&
        module->contribution_count==1 && group && group->contribution_count==1 &&
        group->first==contribution && group->last==contribution &&
        !strcmp(group->name,".drectve") && !strcmp(group->section->name,".drectve") &&
        !memcmp(contribution,&expected,sizeof(expected)));
}
static int first_contribution(void)
{
    return recon_native_at_entry(mark_incremental_contribution,check_first_contribution,
        invoke_hello,NULL)==1;
}
extern void __stdcall read_section_directives(LinkContext *,Contribution *,const char *,InputArguments *);
extern BYTE incremental_enabled,arch_data_ignored;
static void __stdcall check_directive_reader_call(LinkContext *context,Contribution *contribution,
    const char *name,InputArguments *pending)
{
    unsigned i;
    OutputModule *module=current_input_library->module_head;
    if (!context || contribution!=(Contribution *)module->contributions ||
        !name || strcmp(name,".drectve") || !pending || incremental_enabled ||
        arch_data_ignored || contribution->characteristics!=0x100a00 ||
        contribution->group->first!=contribution || module->contribution_count!=1)
        recon_native_entry_finish(0);
    for (i=0;i<sizeof(*pending);++i) if (((BYTE *)pending)[i]) recon_native_entry_finish(0);
    recon_native_entry_finish(1);
}
static int directive_reader_call(void)
{
    return recon_native_at_entry(read_section_directives,check_directive_reader_call,
        invoke_hello,NULL)==1;
}
extern void __stdcall process_module_directives(LinkContext *,Contribution *,char *);
static void __stdcall check_directive_buffer(LinkContext *context,Contribution *contribution,
    char *directives)
{
    static const char expected[]="-defaultlib:user32.lib -defaultlib:gdi32.lib -defaultlib:LIBC -defaultlib:OLDNAMES ";
    FileRecord *file=file_slots[current_input_file];
    unsigned i;
    if (!context || context->ignore_directives ||
        contribution!=(Contribution *)current_input_library->module_head->contributions ||
        !directives || !file || file->file_size!=383 || contribution->raw_size!=83 ||
        directives[83]) recon_native_entry_finish(0);
    for (i=0;i<83;++i) {
        char byte=((char *)file->view)[300+i];
        if (directives[i]!=(byte ? byte : ' ')) recon_native_entry_finish(0);
    }
    recon_native_entry_finish(strstr(directives,expected)!=NULL);
}
static int directive_buffer(void)
{
    return recon_native_at_entry(process_module_directives,check_directive_buffer,
        invoke_hello,NULL)==1;
}
extern void __stdcall collect_module_directives(LinkContext *,Contribution *,InputArguments *,char *);
static void __stdcall check_directive_collection(LinkContext *context,Contribution *contribution,
    InputArguments *arguments,char *directives)
{
    recon_native_entry_finish(context && contribution &&
        contribution==(Contribution *)current_input_library->module_head->contributions &&
        arguments==&option_arguments && !arguments->first && !arguments->last && !arguments->count &&
        directives && strlen(directives)==83 &&
        !memcmp(directives,(BYTE *)file_slots[current_input_file]->view+300,83));
}
static int directive_collection(void)
{
    return recon_native_at_entry(collect_module_directives,check_directive_collection,
        invoke_hello,NULL)==1;
}
extern char *__stdcall next_directive_token(char *,BYTE *);
static void __stdcall check_first_directive_token(char *text,BYTE *quoted)
{
    recon_native_entry_finish(text && !quoted && strlen(text)==83 &&
        !memcmp(text,(BYTE *)file_slots[current_input_file]->view+300,83) &&
        !option_arguments.first && !option_arguments.last && !option_arguments.count);
}
static int first_directive_token(void)
{
    return recon_native_at_entry(next_directive_token,check_first_directive_token,
        invoke_hello,NULL)==1;
}
extern char *__stdcall format_contribution_module(Contribution *,char *);
extern char *directive_cursor;
static void __stdcall check_collected_directives(Contribution *contribution,char *name)
{
    static const char *const expected[]={"defaultlib:user32.lib","defaultlib:gdi32.lib",
        "defaultlib:LIBC","defaultlib:OLDNAMES"};
    InputArgument *argument=option_arguments.first;
    unsigned i;
    if (!contribution || contribution!=(Contribution *)current_input_library->module_head->contributions ||
        !name || option_arguments.count!=4 || directive_cursor)
        recon_native_entry_finish(0);
    for (i=0;i<4;++i) {
        if (!argument || strcmp(argument->name,expected[i]) ||
            argument->original_name!=argument->name || argument->switch_info)
            recon_native_entry_finish(0);
        if (i==3 && option_arguments.last!=argument) recon_native_entry_finish(0);
        argument=argument->next;
    }
    recon_native_entry_finish(argument==NULL);
}
static int collected_directives(void)
{
    return recon_native_at_entry(format_contribution_module,check_collected_directives,
        invoke_hello,NULL)==1;
}
extern char *__stdcall format_module_name(OutputModule *,char *);
extern OutputModule *output_module;
static void __stdcall check_module_name_call(OutputModule *module,char *name)
{
    recon_native_entry_finish(module && module!=output_module &&
        module==current_input_library->module_head && module->library==current_input_library &&
        (module->library->flags & 16) && !strcmp(module->name,"hello.obj") && name &&
        option_arguments.count==4);
}
static int module_name_call(void)
{
    return recon_native_at_entry(format_module_name,check_module_name_call,
        invoke_hello,NULL)==1;
}
extern void __stdcall add_default_library(const char *,LinkPointerList *);
static void __stdcall check_module_filename(const char *library,LinkPointerList *list)
{
    char buffer[512];
    unsigned i;
    OutputModule *module=current_input_library->module_head;
    memset(buffer,0xa5,sizeof(buffer));
    if (!library || strcmp(library,"user32.lib") || !list ||
        library!=option_arguments.first->name+11 ||
        format_module_name(module,buffer)!=buffer || strcmp(buffer,"hello.obj"))
        recon_native_entry_finish(0);
    for (i=10;i<sizeof(buffer);++i) if ((BYTE)buffer[i]!=0xa5) recon_native_entry_finish(0);
    recon_native_entry_finish(1);
}
static int module_filename(void)
{
    return recon_native_at_entry(add_default_library,check_module_filename,
        invoke_hello,NULL)==1;
}
extern BYTE __stdcall library_name_matches(const char *,DefaultLibrary *);
static void __stdcall check_default_library_entry(const char *name,DefaultLibrary *library)
{
    InputArgument *argument=option_arguments.first;
    recon_native_entry_finish(name && !strcmp(name,"gdi32.lib") && library &&
        !library->next && library->name && !strcmp(library->name,"user32.lib") &&
        library->flags==8 && !library->owner && argument && argument->next &&
        library->name!=argument->name+11 && name==argument->next->name+11 &&
        !incremental_enabled);
}
static int default_library_entry(void)
{
    return recon_native_at_entry(library_name_matches,check_default_library_entry,
        invoke_hello,NULL)==1;
}
extern BYTE __stdcall read_comdat_section(LinkContext *,Contribution *,IMAGE_SYMBOL *,DWORD,char **);
static void __stdcall check_first_comdat(LinkContext *context,Contribution *contribution,
    IMAGE_SYMBOL *symbols,DWORD index,char **name)
{
    OutputModule *module=current_input_library->module_head;
    Contribution *parts=(Contribution *)module->contributions;
    FileRecord *file=file_slots[current_input_file];
    Contribution expected;
    memset(&expected,0,sizeof(expected));
    expected.characteristics=module_section_headers[1].Characteristics;
    expected.raw_size=160;
    expected.module=module;
    expected.group=parts[1].group;
    expected.relocation_count=8;
    expected.relocation_offset=543;
    expected.input_raw_offset=383;
    recon_native_entry_finish(context && index==2 && name && !*name &&
        contribution==parts+1 && symbols==(IMAGE_SYMBOL *)((BYTE *)file->view+1135) &&
        (expected.characteristics & 0x1000) && module->contribution_count==2 &&
        !memcmp(parts+1,&expected,sizeof(expected)) && expected.group &&
        !strcmp(expected.group->name,".text") && expected.group->first==parts+1 &&
        expected.group->last==parts+1 && !strcmp(expected.group->section->name,".text") &&
        !strcmp(parts[0].group->name,".drectve") && option_arguments.count==4);
}
static int first_comdat(void)
{
    return recon_native_at_entry(read_comdat_section,check_first_comdat,
        invoke_hello,NULL)==1;
}
extern DWORD *__stdcall build_section_symbol_ranges(Contribution *,IMAGE_SYMBOL *);
extern OutputModule *comdat_symbol_module;
extern DWORD *section_symbol_ranges;
static void __stdcall check_section_symbol_ranges(Contribution *contribution,IMAGE_SYMBOL *symbols)
{
    OutputModule *module=current_input_library->module_head;
    Contribution *parts=(Contribution *)module->contributions;
    FileRecord *file=file_slots[current_input_file];
    recon_native_entry_finish(contribution==parts+1 &&
        contribution->module==module && comdat_symbol_module==module &&
        !section_symbol_ranges && module->section_count==7 && module->symbol_count==37 &&
        symbols==(IMAGE_SYMBOL *)((BYTE *)file->view+1135) &&
        contribution->raw_size==160 && (contribution->characteristics & 0x1000) &&
        contribution->group && !strcmp(contribution->group->name,".text") &&
        contribution->group->first==contribution && contribution->group->last==contribution);
}
static int section_symbol_ranges_call(void)
{
    return recon_native_at_entry(build_section_symbol_ranges,check_section_symbol_ranges,
        invoke_hello,NULL)==1;
}
extern const char *__stdcall get_symbol_name(StringNode *,const void *,WORD *,BYTE *);
extern char *current_module_strings;
static void __stdcall check_comdat_symbol_name(StringNode *symbol,const void *strings,
    WORD *kind,BYTE *is_import)
{
    static const DWORD expected[14]={3,0xffffffff,5,7,11,13,18,0xffffffff,
        20,22,30,32,35,0xffffffff};
    OutputModule *module=current_input_library->module_head;
    FileRecord *file=file_slots[current_input_file];
    IMAGE_SYMBOL *symbols=(IMAGE_SYMBOL *)((BYTE *)file->view+1135);
    recon_native_entry_finish((IMAGE_SYMBOL *)symbol==symbols+7 &&
        symbol->symbol.N.Name.Short==0 && symbol->symbol.N.Name.Long==4 &&
        symbol->symbol.SectionNumber==2 && symbol->symbol.StorageClass==2 &&
        strings==current_module_strings && strings==(BYTE *)file->view+1801 &&
        kind && !is_import && comdat_symbol_module==module && section_symbol_ranges &&
        !memcmp(section_symbol_ranges,expected,sizeof(expected)) &&
        ((Contribution *)module->contributions)[1].comdat_selection==0);
}
static int comdat_symbol_name(void)
{
    return recon_native_at_entry(get_symbol_name,check_comdat_symbol_name,
        invoke_hello,NULL)==1;
}
extern StringNode *RECON_THISCALL native_lookup_symbol(StringTable *,WORD,const char *,BYTE *)
    __asm__("_ZN11StringTable12LookupSymbolEtPKcPh");
static StringNode *RECON_THISCALL check_comdat_lookup(StringTable *table,WORD kind,
    const char *name,BYTE *created)
{
    recon_native_entry_finish(table && table->hash.state && !table->undefined_first &&
        table->undefined_last==&table->undefined_first && kind==0 &&
        name==current_module_strings+4 && !strcmp(name,"_HelloWindowProc@16") &&
        created && !*created &&
        comdat_symbol_module==current_input_library->module_head);
    return NULL;
}
static int comdat_lookup(void)
{
    return recon_native_at_entry(native_lookup_symbol,check_comdat_lookup,
        invoke_hello,NULL)==1;
}
extern const char *__stdcall string_key_name(const void *,const void *);
static HashEntry *__stdcall check_comdat_hash_lookup(const char *name,HashState *state,
    BYTE create,const void *strings,BYTE *created)
{
    const DWORD *words=(const DWORD *)state;
    unsigned i;
    int pass=name==current_module_strings+4 && !strcmp(name,"_HelloWindowProc@16") &&
        state && create==1 && strings && !*(const void * const *)strings && created && !*created;
    if (!pass) recon_native_entry_finish(0);
    pass=words[0]==0 && words[1]==1024 && words[2]==1024 && words[3]==0 &&
        words[4]==0 && words[5]==128 && words[6]==1024 && words[7]==0 &&
        state->key_name==string_key_name && state->enumerator==NULL &&
        !state->incremental_mode && state->directory && state->directory[0] &&
        state->directory[0][0];
    if (!pass) recon_native_entry_finish(0);
    for(i=0;i<1024;++i) if (((void **)state->directory[0][0])[i]) recon_native_entry_finish(0);
    recon_native_entry_finish(1);
    return NULL;
}
static int comdat_hash_lookup(void)
{
    return recon_native_at_entry(lookup_hash,check_comdat_hash_lookup,
        invoke_hello,NULL)==1;
}
extern DWORD __stdcall intern_symbol_string(StringTable *,const char *);
extern DWORD symbol_record_size;
static DWORD __stdcall check_intern_symbol(StringTable *table,const char *name)
{
    HashState *state=table ? table->hash.state : NULL;
    HashEntry **buckets;
    HashEntry *entry;
    const BYTE *bytes;
    unsigned i;
    int pass=state && state->entries==1 && state->split_bucket==0 &&
        state->base_buckets==1024 && state->bucket_count==1024 &&
        !table->hash.string_storage && !table->undefined_first && table->undefined_last==&table->undefined_first &&
        name==current_module_strings+4 && !strcmp(name,"_HelloWindowProc@16") &&
        symbol_record_size==60 && !incremental_enabled;
    if (!pass) recon_native_entry_finish(0);
    buckets=(HashEntry **)state->directory[0][0];
    entry=buckets[277];
    if (!entry || entry->next || !entry->value || !state->bucket_arena ||
        state->bucket_arena->base!=(BYTE *)buckets ||
        state->bucket_arena->base+4096!=(BYTE *)entry ||
        state->bucket_arena->used!=4104 || state->bucket_arena->remaining!=4088 ||
        state->bucket_arena->previous)
        recon_native_entry_finish(0);
    for(i=0;i<1024;++i) if(i!=277 && buckets[i]) recon_native_entry_finish(0);
    bytes=(const BYTE *)entry->value;
    for(i=0;i<60;++i) if(bytes[i]!=(i==16?2:0)) recon_native_entry_finish(0);
    recon_native_entry_finish(1);
    return 0;
}
static int intern_symbol(void)
{
    return recon_native_at_entry(intern_symbol_string,check_intern_symbol,
        invoke_hello,NULL)==1;
}
extern void *__stdcall allocate_symbol_string_block(unsigned);
extern BYTE *symbol_string_pool;
extern DWORD symbol_string_pool_remaining,symbol_string_pool_previous;
extern DWORD symbol_string_pool_size;
static void *__stdcall check_string_tree_pool(unsigned bytes)
{
    recon_native_entry_finish(bytes==12 && !symbol_string_pool &&
        !symbol_string_pool_remaining && !symbol_string_pool_previous &&
        symbol_string_pool_size==4096);
    return NULL;
}
static int string_tree_pool(void)
{
    return recon_native_at_entry(allocate_symbol_string_block,check_string_tree_pool,
        invoke_hello,NULL)==1;
}
extern void __stdcall reserve_byte_buffer(ByteBuffer *,unsigned);
static void __stdcall check_string_buffer_reserve(ByteBuffer *buffer,unsigned bytes)
{
    HashTable *hash=(HashTable *)((BYTE *)buffer-4);
    unsigned i;
    if (bytes!=0x4000 || !buffer || buffer->data || buffer->used || buffer->capacity ||
        hash->string_tree!=(StringTreeNode *)symbol_string_pool ||
        !symbol_string_pool || symbol_string_pool_size!=4096 ||
        symbol_string_pool_remaining!=4084 || symbol_string_pool_previous!=0 ||
        !hash->state || hash->state->entries!=1)
        recon_native_entry_finish(0);
    for(i=0;i<4096;++i) if(symbol_string_pool[i]) recon_native_entry_finish(0);
    recon_native_entry_finish(1);
}
static int string_buffer_reserve(void)
{
    return recon_native_at_entry(reserve_byte_buffer,check_string_buffer_reserve,
        invoke_hello,NULL)==1;
}
static unsigned __stdcall check_symbol_string_append(ByteBuffer *buffer,const void *bytes,
    unsigned count)
{
    HashTable *hash;
    StringTreeNode *node=(StringTreeNode *)symbol_string_pool;
    if (!buffer) recon_native_entry_finish(0);
    hash=(HashTable *)((BYTE *)buffer-4);
    recon_native_entry_finish(buffer->data && buffer->used==4 && buffer->capacity==0x4000 &&
        hash->string_tree==node && node && node->offset==4 && !node->left && !node->right &&
        bytes==current_module_strings+4 && count==20 &&
        !memcmp(bytes,"_HelloWindowProc@16",20) && symbol_string_pool_remaining==4084);
    /* The newly realloc'd string buffer is uninitialized. No unused bytes,
     * including the reserved four-byte length prefix, are read or compared. */
    return 0;
}
static int symbol_string_append(void)
{
    return recon_native_at_entry(append_bytes,check_symbol_string_append,
        invoke_hello,NULL)==1;
}
extern void __stdcall set_symbol_defined(StringNode *,BYTE,StringTable *);
static void __stdcall check_symbol_definition(StringNode *symbol,BYTE defined,StringTable *table)
{
    HashEntry *entry;
    BYTE expected[60];
    StringTreeNode *node=(StringTreeNode *)symbol_string_pool;
    if (!symbol || !table || !table->hash.state || defined ||
        table->hash.state->entries!=1 || !table->hash.strings.data ||
        table->hash.strings.used!=24 || table->hash.strings.capacity!=0x4000 ||
        table->hash.string_tree!=node || !node || node->offset!=4 || node->left || node->right)
        recon_native_entry_finish(0);
    entry=((HashEntry **)table->hash.state->directory[0][0])[277];
    if (!entry || entry->value!=symbol || entry->next) recon_native_entry_finish(0);
    memset(expected,0,sizeof(expected));
    expected[4]=4;
    expected[16]=2;
    expected[48]=1;
    recon_native_entry_finish(!memcmp(symbol,expected,sizeof(expected)) &&
        !memcmp(table->hash.strings.data+4,"_HelloWindowProc@16",20));
    /* Buffer prefix0..3 and tail24..capacity are still uninitialized. */
}
static int symbol_definition(void)
{
    return recon_native_at_entry(set_symbol_defined,check_symbol_definition,
        invoke_hello,NULL)==1;
}
static void __stdcall check_definition_transition(StringNode *symbol,BYTE defined,StringTable *table)
{
    BYTE expected[60];
    if (!symbol || !table || defined || table->undefined_first ||
        table->undefined_last!=&table->undefined_first || symbol->flags!=1)
        recon_native_entry_finish(0);
    /* The hook restores the real entry before redirecting here. Invoke that
     * exact function with its genuine live inputs and inspect its normal return. */
    set_symbol_defined(symbol,defined,table);
    if (table->undefined_first!=symbol || table->undefined_last!=&symbol->undefined_next ||
        symbol->undefined_previous!=&table->undefined_first || symbol->undefined_next ||
        symbol->auxiliary || symbol->flags)
        recon_native_entry_finish(0);
    memset(expected,0,sizeof(expected));
    expected[4]=4; expected[16]=2;
    *(StringNode ***)(expected+0x1c)=&table->undefined_first;
    if (memcmp(symbol,expected,60)) recon_native_entry_finish(0);
    set_symbol_defined(symbol,0,table);
    recon_native_entry_finish(table->undefined_first==symbol &&
        table->undefined_last==&symbol->undefined_next &&
        symbol->undefined_previous==&table->undefined_first && !symbol->undefined_next &&
        !memcmp(symbol,expected,60));
}
static int definition_transition(void)
{
    return recon_native_at_entry(set_symbol_defined,check_definition_transition,
        invoke_hello,NULL)==1;
}
extern void *__stdcall get_contribution_info(Contribution *);
static void *__stdcall check_folded_contribution(Contribution *contribution)
{
    OutputModule *module=current_input_library->module_head;
    Contribution expected;
    Contribution *parts=(Contribution *)module->contributions;
    memset(&expected,0,sizeof(expected));
    expected.characteristics=module_section_headers[1].Characteristics;
    expected.raw_size=160;
    expected.module=module;
    expected.group=parts[1].group;
    expected.relocation_count=8;
    expected.relocation_offset=543;
    expected.input_raw_offset=383;
    expected.comdat_selection=1;
    recon_native_entry_finish(contribution==parts+1 &&
        !memcmp(contribution,&expected,sizeof(expected)) && expected.group &&
        expected.group->first==contribution && expected.group->last==contribution &&
        !strcmp(expected.group->name,".text") &&
        buffer4.base && buffer4.remaining==0 && buffer4.capacity==1024);
    return NULL;
}
static int folded_contribution(void)
{
    return recon_native_at_entry(get_contribution_info,check_folded_contribution,
        invoke_hello,NULL)==1;
}
extern DWORD __stdcall align_section_size(DWORD,DWORD);
static DWORD __stdcall check_fold_record(DWORD bytes,DWORD flags)
{
    OutputModule *module=current_input_library->module_head;
    Contribution *contribution=(Contribution *)module->contributions+1;
    ContributionInfo expected[7];
    memset(expected,0,sizeof(expected));
    expected[1].fold_candidate=contribution;
    recon_native_entry_finish(bytes==1 && flags==module_section_headers[1].Characteristics &&
        module->contribution_info && module->contribution_info!=(void *)module->section_headers &&
        !memcmp(module->contribution_info,expected,sizeof(expected)) &&
        section_header_arena && section_header_arena->base==(BYTE *)module->contribution_info &&
        section_header_arena->used==280 && buffer4.remaining==1 && buffer4.capacity==1024 &&
        ((ContributionInfo **)buffer4.base)[0]==module->contribution_info+1 &&
        module->contribution_info[1].fold_candidate==contribution &&
        contribution->comdat_selection==1 && !contribution->comdat_checksum);
    return 0;
}
static int fold_record(void)
{
    return recon_native_at_entry(align_section_size,check_fold_record,
        invoke_hello,NULL)==1;
}
extern void __stdcall attach_comdat(Contribution *,const char *,BYTE);
static void __stdcall check_comdat_attachment(Contribution *contribution,const char *name,BYTE linker_defined)
{
    OutputModule *module=current_input_library->module_head;
    Contribution *parts=(Contribution *)module->contributions;
    Contribution expected;
    memset(&expected,0,sizeof(expected));
    expected.characteristics=module_section_headers[1].Characteristics;
    expected.raw_size=160;
    expected.module=module;
    expected.group=parts[1].group;
    expected.relocation_count=8;
    expected.relocation_offset=543;
    expected.input_raw_offset=383;
    expected.comdat_selection=1;
    recon_native_entry_finish(!linker_defined && contribution==parts+1 &&
        !memcmp(contribution,&expected,sizeof(expected)) && expected.group &&
        expected.group->alignment==16 && name==current_module_strings+4 &&
        !strcmp(name,"_HelloWindowProc@16") && buffer4.remaining==1 &&
        module->contribution_info[1].fold_candidate==contribution &&
        ((ContributionInfo **)buffer4.base)[0]==module->contribution_info+1);
}
static int comdat_attachment(void)
{
    return recon_native_at_entry(attach_comdat,check_comdat_attachment,
        invoke_hello,NULL)==1;
}
extern char *__stdcall duplicate_comdat_name(const char *);
extern MemoryPool *comdat_names;
extern MemoryArena *comdat_reference_arena;
extern Contribution *last_comdat;
static char *__stdcall check_comdat_name_copy(const char *name)
{
    OutputModule *module=current_input_library->module_head;
    Contribution *contribution=(Contribution *)module->contributions+1;
    ContributionInfo expected[7];
    unsigned i;
    if (!comdat_reference_arena || comdat_reference_arena->previous ||
        comdat_reference_arena->used!=96 || comdat_reference_arena->remaining!=4000 ||
        !comdat_reference_arena->base || comdat_names || last_comdat)
        recon_native_entry_finish(0);
    memset(expected,0,sizeof(expected));
    expected[1].reference_count=8;
    expected[1].references=comdat_reference_arena->base;
    expected[1].fold_candidate=contribution;
    if (name!=current_module_strings+4 || strcmp(name,"_HelloWindowProc@16") ||
        memcmp(module->contribution_info,expected,sizeof(expected)))
        recon_native_entry_finish(0);
    for(i=0;i<96;++i) if(comdat_reference_arena->base[i]) recon_native_entry_finish(0);
    recon_native_entry_finish(1);
    return NULL;
}
static int comdat_name_copy(void)
{
    return recon_native_at_entry(duplicate_comdat_name,check_comdat_name_copy,
        invoke_hello,NULL)==1;
}
extern void *__stdcall allocate_pool_memory(MemoryPool **,unsigned);
static void *__stdcall check_name_pool_call(MemoryPool **pool,unsigned bytes)
{
    recon_native_entry_finish(pool==&comdat_names && !*pool && bytes==20 &&
        comdat_reference_arena && comdat_reference_arena->used==96 && !last_comdat);
    return NULL;
}
static int name_pool_call(void)
{
    return recon_native_at_entry(allocate_pool_memory,check_name_pool_call,
        invoke_hello,NULL)==1;
}
extern void __stdcall read_section_relocations(LinkContext *,Contribution *,IMAGE_SYMBOL *,OutputModule *);
static void __stdcall check_relocation_reader(LinkContext *context,Contribution *contribution,
    IMAGE_SYMBOL *symbols,OutputModule *module)
{
    ContributionInfo expected[7];
    FileRecord *file=file_slots[current_input_file];
    const BYTE *payload;
    unsigned i;
    if (!context || context->file_header.Machine!=0x14c ||
        module!=current_input_library->module_head ||
        contribution!=(Contribution *)module->contributions+1 ||
        symbols!=(IMAGE_SYMBOL *)((BYTE *)file->view+1135) ||
        !comdat_names || comdat_names->previous || comdat_names->used!=20 ||
        comdat_names->remaining!=8172 || last_comdat!=contribution)
        recon_native_entry_finish(0);
    payload=(const BYTE *)(comdat_names+1);
    memset(expected,0,sizeof(expected));
    expected[1].reference_count=8;
    expected[1].references=comdat_reference_arena->base;
    expected[1].name=(const char *)payload;
    expected[1].fold_candidate=contribution;
    if (memcmp(module->contribution_info,expected,sizeof(expected)) ||
        memcmp(payload,"_HelloWindowProc@16",20) ||
        payload==(const BYTE *)current_module_strings+4)
        recon_native_entry_finish(0);
    for(i=20;i<8192;++i) if(payload[i]) recon_native_entry_finish(0);
    recon_native_entry_finish(1);
}
static int relocation_reader(void)
{
    return recon_native_at_entry(read_section_relocations,check_relocation_reader,
        invoke_hello,NULL)==1;
}
extern IMAGE_RELOCATION *RECON_THISCALL native_read_relocations(Contribution *,DWORD *)
    __asm__("_ZN12Contribution15ReadRelocationsEPm");
extern DWORD total_relocation_count;
static IMAGE_RELOCATION *RECON_THISCALL check_relocation_load(Contribution *contribution,
    DWORD *count)
{
    OutputModule *module=current_input_library->module_head;
    Contribution *expected=(Contribution *)module->contributions+1;
    recon_native_entry_finish(contribution==expected && count &&
        contribution->relocation_count==8 && contribution->relocation_offset==543 &&
        !total_relocation_count && !module->base_relocation_size &&
        sizeof(*count)==4);
    return NULL;
}
static int relocation_load(void)
{
    return recon_native_at_entry(native_read_relocations,check_relocation_load,
        invoke_hello,NULL)==1;
}
extern DWORD __stdcall read_x86_relocations(LinkContext *,Contribution *,IMAGE_SYMBOL *,IMAGE_RELOCATION *,DWORD,DWORD);
extern BYTE relocation_data_active,relocations_mapped;
extern ByteBuffer relocation_buffer;
static DWORD __stdcall check_x86_relocation_read(LinkContext *context,Contribution *contribution,
    IMAGE_SYMBOL *symbols,IMAGE_RELOCATION *relocations,DWORD count,DWORD index)
{
    OutputModule *module=current_input_library->module_head;
    FileRecord *file=file_slots[current_input_file];
    recon_native_entry_finish(context && context->file_header.Machine==0x14c &&
        contribution==(Contribution *)module->contributions+1 &&
        symbols==(IMAGE_SYMBOL *)((BYTE *)file->view+1135) &&
        relocations==(IMAGE_RELOCATION *)((BYTE *)file->view+543) &&
        sizeof(*relocations)==10 && count==8 && index==2 &&
        total_relocation_count==8 && relocation_data_active==1 && relocations_mapped==1 &&
        !relocation_buffer.data && !relocation_buffer.used && !relocation_buffer.capacity &&
        !module->base_relocation_size);
    return 0;
}
static int x86_relocation_read(void)
{
    return recon_native_at_entry(read_x86_relocations,check_x86_relocation_read,
        invoke_hello,NULL)==1;
}
static BYTE __stdcall check_x86_classify(Contribution *contribution,WORD type,DWORD mode,BYTE vxd)
{
    OutputModule *module=current_input_library->module_head;
    ContributionInfo *info=module->contribution_info+1;
    recon_native_entry_finish(contribution==(Contribution *)module->contributions+1 &&
        type==6 && !mode && !vxd && total_relocation_count==8 &&
        relocation_data_active==1 && relocations_mapped==1 &&
        buffer4.remaining==1 && ((ContributionInfo **)buffer4.base)[0]==info &&
        !info->fold_signature);
    return 0;
}
static int x86_classify(void)
{
    return recon_native_at_entry(classify_x86_relocation,check_x86_classify,
        invoke_hello,NULL)==1;
}
extern void __stdcall record_relocation_reference(LinkContext *,Contribution *,IMAGE_SYMBOL *,IMAGE_RELOCATION *);
static void __stdcall check_relocation_reference(LinkContext *context,Contribution *contribution,
    IMAGE_SYMBOL *symbols,IMAGE_RELOCATION *relocation)
{
    OutputModule *module=current_input_library->module_head;
    FileRecord *file=file_slots[current_input_file];
    ContributionInfo *info=module->contribution_info+1;
    unsigned i;
    if (!context || !context->options.fold_identical_comdats ||
        contribution!=(Contribution *)module->contributions+1 ||
        symbols!=(IMAGE_SYMBOL *)((BYTE *)file->view+1135) ||
        relocation!=(IMAGE_RELOCATION *)((BYTE *)file->view+543) ||
        relocation->VirtualAddress!=39 || relocation->SymbolTableIndex!=17 ||
        relocation->Type!=6 || info->reference_count!=8 || info->fold_signature ||
        info->references!=comdat_reference_arena->base || comdat_reference_arena->used!=96)
        recon_native_entry_finish(0);
    for(i=0;i<96;++i) if(comdat_reference_arena->base[i]) recon_native_entry_finish(0);
    recon_native_entry_finish(1);
}
static int relocation_reference(void)
{
    return recon_native_at_entry(record_relocation_reference,check_relocation_reference,
        invoke_hello,NULL)==1;
}
extern void *__stdcall add_comdat_reference(DWORD,Contribution *,Contribution *);
static void *__stdcall check_comdat_reference(DWORD symbol_index,Contribution *parent,
    Contribution *child)
{
    OutputModule *module=current_input_library->module_head;
    FileRecord *file=file_slots[current_input_file];
    IMAGE_SYMBOL *symbols=(IMAGE_SYMBOL *)((BYTE *)file->view+1135);
    ContributionInfo *info=module->contribution_info+1;
    unsigned i;
    if (symbol_index!=17 || parent!=(Contribution *)module->contributions+1 ||
        child || symbols[symbol_index].StorageClass!=IMAGE_SYM_CLASS_EXTERNAL ||
        info->references_used || info->reference_count!=8 ||
        info->references!=comdat_reference_arena->base || comdat_reference_arena->used!=96)
        recon_native_entry_finish(0);
    for(i=0;i<96;++i) if(comdat_reference_arena->base[i]) recon_native_entry_finish(0);
    {
        Contribution *distinct_child=(Contribution *)module->contributions+2;
        ComdatReference *first=(ComdatReference *)add_comdat_reference(symbol_index,parent,NULL);
        ComdatReference *second=(ComdatReference *)add_comdat_reference(0x13579bdf,parent,distinct_child);
        DWORD first_raw[3],second_raw[3];
        /* Unwritten words retain the fresh arena zero prestate checked above. */
        memcpy(first_raw,first,sizeof(first_raw));
        memcpy(second_raw,second,sizeof(second_raw));
        recon_native_entry_finish(first==(ComdatReference *)info->references &&
            second==first+1 && first_raw[0]==0 && first_raw[1]==17 && first_raw[2]==0 &&
            second_raw[0]==(DWORD)distinct_child && second_raw[1]==0 && second_raw[2]==0 &&
            second_raw[0]!=0x13579bdf && info->references_used==2);
    }
    return NULL;
}
static int comdat_reference(void)
{
    return recon_native_at_entry(add_comdat_reference,check_comdat_reference,
        invoke_hello,NULL)==1;
}
extern void __stdcall release_relocations(IMAGE_RELOCATION *);
static void __stdcall check_completed_references(IMAGE_RELOCATION *relocations)
{
    static const DWORD indices[8]={17,16,15,14,13,10,9,8};
    OutputModule *module=current_input_library->module_head;
    FileRecord *file=file_slots[current_input_file];
    Contribution *contribution=(Contribution *)module->contributions+1;
    ContributionInfo expected[7];
    ComdatReference references[8];
    unsigned i;
    memset(expected,0,sizeof(expected));
    memset(references,0,sizeof(references));
    expected[1].references_used=8;
    expected[1].reference_count=8;
    expected[1].references=comdat_reference_arena->base;
    expected[1].name=(char *)(comdat_names+1);
    expected[1].fold_signature=0xa81a7f01;
    expected[1].fold_candidate=contribution;
    for (i=0;i<8;++i) references[i].symbol_index=indices[i];
    recon_native_entry_finish(relocations==(IMAGE_RELOCATION *)((BYTE *)file->view+543) &&
        !memcmp(module->contribution_info,expected,sizeof(expected)) &&
        !memcmp(comdat_reference_arena->base,references,sizeof(references)) &&
        comdat_reference_arena->used==96 && comdat_reference_arena->remaining==4000 &&
        total_relocation_count==8 && relocation_data_active==1 && relocations_mapped==1 &&
        !module->base_relocation_size);
}
static int completed_references(void)
{
    return recon_native_at_entry(release_relocations,check_completed_references,
        invoke_hello,NULL)==1;
}
extern void __stdcall process_module_symbols(LinkContext *,OutputModule *,void *,IMAGE_SYMBOL *,DWORD,BYTE);
static void __stdcall check_sections_complete(LinkContext *context,OutputModule *module,void *parent,
    IMAGE_SYMBOL *symbols,DWORD flags,BYTE first_pass)
{
    FileRecord *file=file_slots[current_input_file];
    recon_native_entry_finish(context && module==current_input_library->module_head &&
        !parent && symbols==(IMAGE_SYMBOL *)((BYTE *)file->view+1135) && !flags && !first_pass &&
        !relocation_data_active && relocations_mapped==1 && total_relocation_count==21 &&
        module->section_count==7 && module->contribution_info[1].references_used==8 &&
        module->contribution_info[1].fold_signature==0xa81a7f01);
}
static int sections_complete(void)
{
    return recon_native_at_entry(process_module_symbols,check_sections_complete,
        invoke_hello,NULL)==1;
}
extern IMAGE_SYMBOL *__stdcall next_coff_symbol(IMAGE_SYMBOL **);
static IMAGE_SYMBOL *__stdcall check_symbol_cursor(IMAGE_SYMBOL **cursor)
{
    OutputModule *module=current_input_library->module_head;
    FileRecord *file=file_slots[current_input_file];
    unsigned i;
    if(!cursor || *cursor!=(IMAGE_SYMBOL *)((BYTE *)file->view+1135) ||
        module->symbol_count!=37 || module->compiler_id || !current_module_symbols ||
        relocation_data_active || total_relocation_count!=21)
        recon_native_entry_finish(0);
    for(i=0;i<37;++i) if(current_module_symbols[i])recon_native_entry_finish(0);
    {
        IMAGE_SYMBOL *before=*cursor;
        /* Shared hook restores the genuine entry before this typed callback. */
        IMAGE_SYMBOL *result=next_coff_symbol(cursor);
        if(result!=before || *cursor!=before+1 || result->StorageClass!=103)
            recon_native_entry_finish(0);
    }
    recon_native_entry_finish(1);
    return NULL;
}
static int symbol_cursor(void)
{
    return recon_native_at_entry(next_coff_symbol,check_symbol_cursor,invoke_hello,NULL)==1;
}
extern BYTE __stdcall emit_symbol_class(BYTE,LinkOptions *);
static BYTE __stdcall check_symbol_class(BYTE symbol_class,LinkOptions *options)
{
    LinkContext *context;
    if(!options)recon_native_entry_finish(0);
    context=(LinkContext *)((BYTE *)options - offsetof(LinkContext,options));
    if(symbol_class!=103 || options->debug_formats || options->debug_level ||
        context->command_line_library!=current_input_library || !context->symbols ||
        current_input_library->module_head->symbol_count!=37)recon_native_entry_finish(0);
    {
        LinkOptions before=*options;
        BYTE result=emit_symbol_class(symbol_class,options);
        recon_native_entry_finish(!result && !memcmp(options,&before,sizeof(before)));
    }
    return 0;
}
static int symbol_class_call(void)
{
    return recon_native_at_entry(emit_symbol_class,check_symbol_class,invoke_hello,NULL)==1;
}
extern void __stdcall define_input_symbol(StringNode *,Contribution *,DWORD,SHORT,WORD,
    DWORD,OutputModule *,StringTable *);
extern BYTE incremental_enabled;
extern DWORD undefined_symbols;
static void __stdcall check_input_symbol_definition(StringNode *symbol,Contribution *contribution,
    DWORD value,SHORT section,WORD type,DWORD flags,OutputModule *module,StringTable *table)
{
    StringNode **previous,*next;
    StringNode **last;
    DWORD before_flags,before_undefined;
    if (!symbol || !table || module!=current_input_library->module_head ||
        contribution!=(Contribution *)module->contributions+1 ||
        contribution->module!=module || value || section!=2 || type!=0x20 || flags ||
        incremental_enabled || strcmp(contribution->group->section->name,".text") ||
        (symbol->flags & 1)) recon_native_entry_finish(0);
    previous=symbol->undefined_previous;
    next=symbol->undefined_next;
    last=table->undefined_last;
    before_flags=symbol->flags;
    before_undefined=undefined_symbols;
    if (!previous || *previous!=symbol) recon_native_entry_finish(0);
    define_input_symbol(symbol,contribution,value,section,type,flags,module,table);
    recon_native_entry_finish(symbol->symbol.Value==value &&
        symbol->symbol.SectionNumber==section && symbol->symbol.Type==type &&
        symbol->input_flags==0 && symbol->contribution==contribution &&
        symbol->flags==((before_flags & ~0x40c0U)|1) &&
        !symbol->undefined_previous && !symbol->undefined_next && *previous==next &&
        (!next || next->undefined_previous==previous) &&
        table->undefined_last==(last==&symbol->undefined_next ? previous:last) &&
        undefined_symbols==before_undefined-((before_flags & 0x40c0)!=0));
}
static int input_symbol_definition(void)
{
    return recon_native_at_entry(define_input_symbol,check_input_symbol_definition,invoke_hello,NULL)==1;
}
static void __stdcall check_symbol_reference(StringNode *symbol,void *owner)
{
    DWORD flags;
    IMAGE_SYMBOL before;
    if (!symbol || owner!=current_input_library->module_head || incremental_enabled ||
        (symbol->flags & 0x1001) || symbol->reference_owner)
        recon_native_entry_finish(0);
    flags=symbol->flags;
    before=symbol->symbol;
    record_symbol_reference(symbol,owner);
    recon_native_entry_finish(symbol->reference_owner==owner && symbol->flags==flags &&
        !memcmp(&symbol->symbol,&before,sizeof(before)));
}
static int symbol_reference(void)
{
    return recon_native_at_entry(record_symbol_reference,check_symbol_reference,invoke_hello,NULL)==1;
}
extern void __stdcall end_section_enumerator(SectionEnumerator *);
static void __stdcall check_module_contribution_enumerator(ModuleContributionEnumerator *enumerator,
    OutputModule *module)
{
    if (!enumerator || module!=current_input_library->module_head || module->section_count!=7)
        recon_native_entry_finish(0);
    initialize_module_contribution_enumerator(enumerator,module);
    if (!(enumerator->next==next_module_contribution &&
        enumerator->end==(void (__stdcall *)(void *))end_section_enumerator &&
        enumerator->module==module && !enumerator->current && !enumerator->index))
        recon_native_entry_finish(0);
    {
        unsigned i;
        for(i=0;i<7;++i) {
            if(!enumerator->next(enumerator) || enumerator->index!=i+1 ||
                enumerator->current!=(Contribution *)module->contributions+i)
                recon_native_entry_finish(0);
        }
        recon_native_entry_finish(!enumerator->next(enumerator) && !enumerator->current &&
            enumerator->index==7 && enumerator->module==module);
    }
}
static int module_contribution_enumerator(void)
{
    return recon_native_at_entry(initialize_module_contribution_enumerator,
        check_module_contribution_enumerator,invoke_hello,NULL)==1;
}
static void __stdcall check_reference_enumerator(ComdatReferenceEnumerator *enumerator,
    ContributionInfo *block)
{
    OutputModule *module=current_input_library->module_head;
    if(!enumerator || block!=module->contribution_info+1 || block->references_used!=8 ||
        block->reference_count!=8 || block->references!=comdat_reference_arena->base ||
        ((ComdatReference *)block->references)[0].symbol_index!=17)
        recon_native_entry_finish(0);
    initialize_comdat_reference_enumerator(enumerator,block);
    /* The unused padding WORD is uninitialized on each side and unobserved. */
    if (!(enumerator->next==next_comdat_reference &&
        enumerator->end==(void (__stdcall *)(void *))end_section_enumerator &&
        enumerator->block==block && !enumerator->current && !enumerator->index))
        recon_native_entry_finish(0);
    {
        unsigned i;
        for (i=0;i<8;++i) {
            if (!enumerator->next(enumerator) || enumerator->index!=i+1 ||
                enumerator->current!=(ComdatReference *)block->references+i)
                recon_native_entry_finish(0);
        }
        recon_native_entry_finish(!enumerator->next(enumerator) && !enumerator->current &&
            enumerator->block==block && enumerator->index==8);
    }
}
static int reference_enumerator(void)
{
    return recon_native_at_entry(initialize_comdat_reference_enumerator,
        check_reference_enumerator,invoke_hello,NULL)==1;
}
extern void __stdcall release_module_strings(char *);
extern void __stdcall release_module_symbols(IMAGE_SYMBOL *);
extern char *current_module_strings;
static void __stdcall check_bound_comdat_references(char *strings)
{
    static const DWORD indices[8]={17,16,15,14,13,10,9,8};
    OutputModule *module=current_input_library->module_head;
    ComdatReference *references=(ComdatReference *)module->contribution_info[1].references;
    unsigned i;
    if(strings!=current_module_strings || !strings || module->section_map ||
        module->contribution_info[1].references_used!=8) recon_native_entry_finish(0);
    for(i=0;i<8;++i) {
        if(references[i].contribution || references[i].flags ||
            !current_module_symbols[indices[i]] ||
            references[i].symbol!=current_module_symbols[indices[i]])
            recon_native_entry_finish(0);
    }
    if (!module_strings_active || !module_symbols_active) recon_native_entry_finish(0);
    release_module_strings(strings);
    if (module_strings_active || !module_symbols_active ||
        strings!=current_module_strings || *(DWORD *)strings!=461)
        recon_native_entry_finish(0);
    {
        IMAGE_SYMBOL *symbols=(IMAGE_SYMBOL *)((BYTE *)file_slots[current_input_file]->view+1135);
        release_module_symbols(symbols);
        recon_native_entry_finish(!module_symbols_active && !module_strings_active &&
            symbols->StorageClass==IMAGE_SYM_CLASS_FILE && *(DWORD *)strings==461);
    }
}
static int bound_comdat_references(void)
{
    return recon_native_at_entry(release_module_strings,check_bound_comdat_references,
        invoke_hello,NULL)==1;
}
extern void __stdcall cache_file_record(FileRecord *);
extern FileRecord *cached_files,*cached_file_tail,*pending_files;
extern unsigned cached_file_count;
extern int output_file;
static void __stdcall check_cached_file_transition(FileRecord *record)
{
    if (!(record==file_slots[current_input_file] &&
        current_input_file==14 && record->index==14 && !strcmp(record->name,"hello.obj") &&
        record->flags==9 && record->size==2262 && record->mapping_size==2262 &&
        record->view && record->file && !record->previous &&
        cached_files==record && cached_file_tail==record && cached_file_count==1 &&
        pending_files==file_slots[output_file] && !pending_files->previous && file_window_limit==14))
        recon_native_entry_finish(0);
    {
        FileRecord expected=*record;
        expected.flags |= 4;
        cache_file_record(record);
        recon_native_entry_finish(!memcmp(record,&expected,sizeof(expected)) &&
            cached_file_count==1 && cached_files==record && cached_file_tail==record &&
            pending_files==file_slots[output_file]);
    }
}
static int cached_file_transition(void)
{
    return recon_native_at_entry(cache_file_record,check_cached_file_transition,invoke_hello,NULL)==1;
}
extern StringNode *RECON_THISCALL native_find_entry_symbol(StringTable *,const char *)
    __asm__("_ZN11StringTable10FindSymbolEPKc");
static StringNode *RECON_THISCALL check_entry_symbol_lookup(StringTable *table,const char *name)
{
    if (!(table && table->hash.state && table->hash.state->entries &&
        table->hash.state->key_name==string_key_name && table->hash.strings.data &&
        table->hash.strings.used>=4 && name && !strcmp(name,"_main") &&
        (table->hash.state->entries<<4)/table->hash.state->bucket_count<=48))
        recon_native_entry_finish(0);
    {
        HashState state=*table->hash.state;
        StringTable before=*table;
        StringNode *result=native_find_entry_symbol(table,name);
        recon_native_entry_finish(!result && !memcmp(table,&before,sizeof(before)) &&
            !memcmp(table->hash.state,&state,sizeof(state)));
    }
    return NULL;
}
static int entry_symbol_lookup(void)
{
    return recon_native_at_entry(native_find_entry_symbol,check_entry_symbol_lookup,
        invoke_hello,NULL)==1;
}
extern BYTE __stdcall infer_entry_point(LinkContext *);
static BYTE __stdcall check_gui_subsystem(LinkContext *context)
{
    StringNode *symbol;
    OutputModule *module=current_input_library->module_head;
    if(!context || context->optional_header.Subsystem!=IMAGE_SUBSYSTEM_WINDOWS_GUI ||
        context->file_header.Machine!=IMAGE_FILE_MACHINE_I386 || !context->symbols)
        recon_native_entry_finish(0);
    symbol=native_find_entry_symbol(context->symbols,"_WinMain@16");
    recon_native_entry_finish(symbol && (symbol->flags & 1) &&
        symbol->symbol.Type==0x20 && symbol->symbol.SectionNumber==5 &&
        symbol->contribution==(Contribution *)module->contributions+4);
    return 0;
}
static int gui_subsystem(void)
{
    return recon_native_at_entry(infer_entry_point,check_gui_subsystem,invoke_hello,NULL)==1;
}
extern BYTE __stdcall infer_subsystem(LinkContext *);
extern StringNode *entry_symbol;
extern const char *entry_point_name;
static BYTE __stdcall check_entry_symbol_predicate(LinkContext *context)
{
    StringNode *symbol=native_find_entry_symbol(context->symbols,"_WinMain@16");
    BYTE result;
    if (!symbol || !(symbol->flags & 1)) recon_native_entry_finish(0);
    /* These are different original owners: the resolved symbol suppresses
       inference even when no /ENTRY spelling has been installed. */
    entry_symbol=symbol;
    entry_point_name=NULL;
    result=infer_subsystem(context);
    recon_native_entry_finish(result==0 && entry_symbol==symbol &&
        entry_point_name==NULL &&
        context->optional_header.Subsystem==IMAGE_SUBSYSTEM_WINDOWS_GUI);
    return 0;
}
static int entry_symbol_predicate(void)
{
    return recon_native_at_entry(infer_subsystem,check_entry_symbol_predicate,
        invoke_hello,NULL)==1;
}
extern BYTE __stdcall resolve_initial_entry(LinkContext *);
static BYTE __stdcall check_inferred_startup(LinkContext *context)
{
    recon_native_entry_finish(context && !context->entry_name && !entry_symbol &&
        entry_point_name && !strcmp(entry_point_name,"WinMainCRTStartup") &&
        context->optional_header.Subsystem==IMAGE_SUBSYSTEM_WINDOWS_GUI &&
        !(context->file_header.Characteristics & IMAGE_FILE_DLL));
    return 0;
}
static int inferred_startup(void)
{
    return recon_native_at_entry(resolve_initial_entry,check_inferred_startup,
        invoke_hello,NULL)==1;
}
extern StringNode *__stdcall resolve_entry_symbol(LinkContext *);
static StringNode *__stdcall check_initial_entry_copy(LinkContext *context)
{
    recon_native_entry_finish(context && context->entry_name && entry_point_name &&
        context->entry_name!=entry_point_name &&
        !strcmp(context->entry_name,"WinMainCRTStartup") &&
        !strcmp(entry_point_name,"WinMainCRTStartup") && !entry_symbol);
    return NULL;
}
static int initial_entry_copy(void)
{
    return recon_native_at_entry(resolve_entry_symbol,check_initial_entry_copy,
        invoke_hello,NULL)==1;
}
extern StringNode *RECON_THISCALL native_lookup_entry_symbol(StringTable *,const unsigned char *,BYTE *)
    __asm__("_ZN11StringTable6LookupEPKhPh");
static StringNode *RECON_THISCALL check_decorated_entry(StringTable *table,
    const unsigned char *name,BYTE *created)
{
    recon_native_entry_finish(table && table->hash.state &&
        table->hash.state->key_name==string_key_name && !created &&
        name && !strcmp((const char *)name,"_WinMainCRTStartup") &&
        entry_point_name && !strcmp(entry_point_name,"WinMainCRTStartup") &&
        name!=(const unsigned char *)entry_point_name && !entry_symbol);
    return NULL;
}
static int decorated_entry(void)
{
    return recon_native_at_entry(native_lookup_entry_symbol,check_decorated_entry,
        invoke_hello,NULL)==1;
}
extern void __stdcall search_libraries(LinkContext *);
static void __stdcall check_startup_undefined(LinkContext *context)
{
    StringNode *found=native_find_entry_symbol(context->symbols,"_WinMainCRTStartup");
    recon_native_entry_finish(found && found==entry_symbol && !(found->flags & 1) &&
        !found->contribution && found->symbol.StorageClass==IMAGE_SYM_CLASS_EXTERNAL &&
        found->undefined_previous && *found->undefined_previous==found &&
        context->entry_name && !strcmp(context->entry_name,"WinMainCRTStartup") &&
        entry_point_name && !strcmp(entry_point_name,"WinMainCRTStartup"));
}
static int startup_undefined(void)
{
    return recon_native_at_entry(search_libraries,check_startup_undefined,
        invoke_hello,NULL)==1;
}
extern void __stdcall ensure_library_loaded(LinkContext *,OutputLibrary *);
static void __stdcall check_library_search_input(LinkContext *context,OutputLibrary *library)
{
    recon_native_entry_finish(context && library && library!=context->command_line_library &&
        context->pointer_list.first==context->command_line_library &&
        context->command_line_library->next==library &&
        library->name && !strcmp(library->name,"R:\\toolchains\\5.10\\lib\\kernel32.lib") &&
        !(library->flags & 4) && !library->index_data[3] && entry_symbol &&
        !(entry_symbol->flags & 1) &&
        context->optional_header.Subsystem==IMAGE_SUBSYSTEM_WINDOWS_GUI);
}
static int library_search_input(void)
{
    return recon_native_at_entry(ensure_library_loaded,check_library_search_input,
        invoke_hello,NULL)==1;
}
static void __stdcall check_library_reader_input(OutputLibrary *library,LinkContext *context)
{
    FileRecord *record=file_slots[current_input_file];
    recon_native_entry_finish(context && library &&
        library==context->command_line_library->next && library->name &&
        !library->index_data[3] && record && record->index==current_input_file &&
        record->name && !strcmp(record->name,library->name) && record->file &&
        record->view && record->size &&
        current_input_library==context->command_line_library && entry_symbol);
}
static int library_reader_input(void)
{
    return recon_native_at_entry(read_library_index,check_library_reader_input,
        invoke_hello,NULL)==1;
}
extern void RECON_THISCALL native_library_read_index(OutputLibrary *,LinkContext *)
    __asm__("_ZN13OutputLibrary9ReadIndexEP11LinkContext");
static void RECON_THISCALL check_library_index_position(OutputLibrary *library,LinkContext *context)
{
    FileRecord *record=file_slots[current_input_file];
    recon_native_entry_finish(context && library==context->command_line_library->next &&
        library->name && !library->index_data[3] && record && record->view &&
        (record->flags & 8) && record->file_size==8 && record->size>=68 &&
        !memcmp(record->view,"!<arch>\n",8));
}
static int library_index_position(void)
{
    return recon_native_at_entry(native_library_read_index,check_library_index_position,
        invoke_hello,NULL)==1;
}
extern DWORD archive_member_size;
extern IMAGE_ARCHIVE_MEMBER_HEADER *__cdecl read_archive_member_header(void);
static IMAGE_ARCHIVE_MEMBER_HEADER *__cdecl check_archive_header_call(void)
{
    FileRecord *record=file_slots[current_input_file];
    recon_native_entry_finish(archive_member_offset==8 && archive_member_size==0 &&
        record && record->view && record->file_size==8 &&
        !memcmp(record->view,"!<arch>\n",8));
    return NULL;
}
static int archive_header_call(void)
{
    archive_member_size=0x13579bdf;
    return recon_native_at_entry(read_archive_member_header,check_archive_header_call,
        invoke_hello,NULL)==1;
}
extern DWORD __stdcall archive_big_endian_dword(const void *);
static DWORD __stdcall check_archive_count_bytes(const void *value)
{
    FileRecord *record=file_slots[current_input_file];
    recon_native_entry_finish(value && record && record->view &&
        record->file_size==72 && archive_member_offset==68 && archive_member_size==37818 &&
        !memcmp(value,(const BYTE *)record->view+68,4) && *(const DWORD *)value==0x49050000);
    return 0;
}
static int archive_count_bytes(void)
{
    return recon_native_at_entry(archive_big_endian_dword,check_archive_count_bytes,
        invoke_hello,NULL)==1;
}
extern void RECON_THISCALL native_check_library_machine(LinkContext *,const char *,WORD)
    __asm__("_ZN11LinkContext12CheckMachineEPKct");
static void RECON_THISCALL check_archive_index_tables(LinkContext *context,const char *name,WORD machine)
{
    OutputLibrary *library=context->command_line_library->next;
    FileRecord *record=file_slots[current_input_file];
    const BYTE *input=(const BYTE *)record->view;
    char *cursor=(char *)library->index_data[2];
    char *end=cursor+32402;
    unsigned i;
    if (!(library && name==library->name && machine==IMAGE_FILE_MACHINE_I386 &&
        library->flags==1 && library->archive_symbol_count==1353 &&
        record->size==635638 && record->file_size==75854 &&
        archive_member_offset==37946 && archive_member_size==37828 &&
        library->index_data[0] && library->index_data[1] && cursor &&
        library->index_data[3] && !library->index_data[4] &&
        !memcmp((DWORD *)library->index_data[0]+1,input+37950,678*4) &&
        !memcmp(library->index_data[1],input+40666,1353*2) &&
        !memcmp(cursor,input+43372,32402))) recon_native_entry_finish(0);
    /* The unused leading offset DWORD is original allocator prestate; do not
       compare it. Every written pointer must own its actual copied name. */
    for (i=0;i<1353;++i) {
        if (((char **)library->index_data[3])[i]!=cursor) recon_native_entry_finish(0);
        while (cursor<end && *cursor) ++cursor;
        if (cursor==end) recon_native_entry_finish(0);
        ++cursor;
    }
    recon_native_entry_finish(1);
}
static int archive_index_tables(void)
{
    return recon_native_at_entry(native_check_library_machine,check_archive_index_tables,
        invoke_hello,NULL)==1;
}
extern void RECON_THISCALL native_library_search_symbols(OutputLibrary *,LinkContext *,BYTE *,BYTE *)
    __asm__("_ZN13OutputLibrary13SearchSymbolsEP11LinkContextPhS2_");
static void RECON_THISCALL check_library_search_outputs(OutputLibrary *library,
    LinkContext *context,BYTE *changed,BYTE *keep_searching)
{
    recon_native_entry_finish(context && library==context->command_line_library->next &&
        context->file_header.Machine==IMAGE_FILE_MACHINE_I386 &&
        changed && keep_searching && changed!=keep_searching &&
        *changed==0 && *keep_searching==1 && library->archive_symbol_count==1353 &&
        library->index_data[3] && entry_symbol && !(entry_symbol->flags & 1));
}
static int library_search_outputs(void)
{
    return recon_native_at_entry(native_library_search_symbols,check_library_search_outputs,
        invoke_hello,NULL)==1;
}
static void __stdcall check_library_file_restore(LinkContext *context,OutputLibrary *library)
{
    int saved=current_input_file;
    if (!(file_slots[14] && !strcmp(file_slots[14]->name,"hello.obj") &&
        library==context->command_line_library->next && !library->index_data[3]))
        recon_native_entry_finish(0);
    current_input_file=14;
    ensure_library_loaded(context,library);
    if (current_input_file!=14 || !library->index_data[3]) recon_native_entry_finish(0);
    current_input_file=saved;
    recon_native_entry_finish(1);
}
static int library_file_restore(void)
{
    return recon_native_at_entry(ensure_library_loaded,check_library_file_restore,
        invoke_hello,NULL)==1;
}
extern void __stdcall initialize_undefined_enumerator(void *,StringTable *);
static void __stdcall check_undefined_enumerator_input(void *output,StringTable *table)
{
    recon_native_entry_finish(output && table && table->undefined_first &&
        table->hash.state && table->hash.state->key_name==string_key_name &&
        entry_symbol && !(entry_symbol->flags & 1) &&
        native_find_entry_symbol(table,"_WinMainCRTStartup")==entry_symbol);
}
static int undefined_enumerator_input(void)
{
    return recon_native_at_entry(initialize_undefined_enumerator,
        check_undefined_enumerator_input,invoke_hello,NULL)==1;
}
typedef struct TestUndefinedEnumerator {
    BYTE (__stdcall *next)(struct TestUndefinedEnumerator *);
    void (__stdcall *end)(struct TestUndefinedEnumerator *);
    StringNode *next_symbol,*current;
} TestUndefinedEnumerator;
extern BYTE __stdcall next_undefined(TestUndefinedEnumerator *);
static BYTE __stdcall check_undefined_enumerator(TestUndefinedEnumerator *e)
{
    StringNode *symbol;
    unsigned count=0;
    if (!(e && e->next==next_undefined &&
        (void *)e->end==(void *)end_section_enumerator && e->next_symbol &&
        !(e->next_symbol->flags & 1) && e->next_symbol->undefined_previous &&
        *e->next_symbol->undefined_previous==e->next_symbol)) recon_native_entry_finish(0);
    /* current is own unwritten prestate until the first genuine step. */
    symbol=e->next_symbol;
    while (symbol) {
        StringNode *following=symbol->undefined_next;
        if (++count>8192 || !next_undefined(e) || e->current!=symbol ||
            e->next_symbol!=following) recon_native_entry_finish(0);
        symbol=following;
    }
    recon_native_entry_finish(count && !next_undefined(e) && !e->current && !e->next_symbol);
    return 0;
}
static int undefined_enumerator(void)
{
    return recon_native_at_entry(next_undefined,check_undefined_enumerator,
        invoke_hello,NULL)==1;
}
extern BYTE RECON_THISCALL native_find_symbol_member(OutputLibrary *,LinkContext *,StringNode *,const char *,DWORD *)
    __asm__("_ZN13OutputLibrary16FindSymbolMemberEP11LinkContextP10StringNodePKcPm");
static BYTE RECON_THISCALL check_find_symbol_member_input(OutputLibrary *library,
    LinkContext *context,StringNode *symbol,const char *name,DWORD *offset)
{
    WORD kind;
    const char *expected;
    if (!(context && library==context->command_line_library->next && symbol &&
        !(symbol->flags & 0x4140) && !symbol->symbol.SectionNumber && !symbol->symbol.Value &&
        name && offset==&archive_member_offset)) recon_native_entry_finish(0);
    expected=get_symbol_name(symbol,context->symbols->hash.string_storage,&kind,NULL);
    recon_native_entry_finish(expected && (kind==1 ? !memcmp(name,expected,8) : !strcmp(name,expected)));
    return 0;
}
static int find_symbol_member_input(void)
{
    return recon_native_at_entry(native_find_symbol_member,check_find_symbol_member_input,
        invoke_hello,NULL)==1;
}
extern int __cdecl compare_archive_symbol_names(const void *,const void *);
static int __cdecl check_archive_compare_inputs(const void *key,const void *element)
{
    OutputLibrary *library=current_input_library->next;
    if (!(library && library->archive_symbol_count==1353 &&
        element==(const char **)library->index_data[3]+676 &&
        key && *(const char *const *)key &&
        !strcmp(*(const char *const *)key,"__imp__PostQuitMessage@4") &&
        !strcmp(*(const char *const *)element,"__imp__CloseConsoleHandle@4")))
        recon_native_entry_finish(0);
    recon_native_entry_finish(compare_archive_symbol_names(key,element)==1 &&
        compare_archive_symbol_names(element,key)==-1 && compare_archive_symbol_names(key,key)==0);
    return 0;
}
static int archive_compare_inputs(void)
{
    return recon_native_at_entry(compare_archive_symbol_names,check_archive_compare_inputs,
        invoke_hello,NULL)==1;
}
extern OutputLibrary *__stdcall load_next_default_library(LinkPointerList *);
static OutputLibrary *__stdcall check_default_library_input(LinkPointerList *list)
{
    static const char *const names[]={"user32.lib","gdi32.lib","LIBC","OLDNAMES"};
    DefaultLibrary *library;
    unsigned i;
    if (!(list && list->first==current_input_library && !list->suppress_default_libraries &&
        list->last==(void **)&current_input_library->next->next &&
        current_input_library->next->flags==1)) recon_native_entry_finish(0);
    library=list->default_libraries;
    for (i=0;i<4;++i) {
        if (!library || strcmp(library->name,names[i]) || library->flags!=8 || library->owner)
            recon_native_entry_finish(0);
        library=library->next;
    }
    recon_native_entry_finish(library==NULL);
    return NULL;
}
static int default_library_input(void)
{
    return recon_native_at_entry(load_next_default_library,check_default_library_input,
        invoke_hello,NULL)==1;
}
extern OutputLibrary *__stdcall find_loaded_default_library(const char *,LinkPointerList *);
static OutputLibrary *__stdcall check_default_library_attempt(const char *name,LinkPointerList *list)
{
    DefaultLibrary *first=list->default_libraries;
    if (!(first && name==first->name && !strcmp(name,"user32.lib") &&
        first->flags==0x28 && !first->owner && first->next && first->next->flags==8 &&
        list->first==current_input_library && current_input_library->next &&
        current_input_library->next->flags==1 && !current_input_library->next->next))
        recon_native_entry_finish(0);
    recon_native_entry_finish(!find_loaded_default_library(name,list) &&
        find_loaded_default_library("kernel32",list)==current_input_library->next &&
        find_loaded_default_library("R:\\toolchains\\5.10\\lib\\kernel32.lib",list)==
            current_input_library->next && first->flags==0x28 && !first->owner);
    return NULL;
}
static int default_library_attempt(void)
{
    return recon_native_at_entry(find_loaded_default_library,check_default_library_attempt,
        invoke_hello,NULL)==1;
}
extern const char *RECON_THISCALL native_library_member_name(OutputLibrary *,const IMAGE_ARCHIVE_MEMBER_HEADER *)
    __asm__("_ZN13OutputLibrary10MemberNameEPK27IMAGE_ARCHIVE_MEMBER_HEADER");
static const char *RECON_THISCALL check_library_member_header(OutputLibrary *library,
    const IMAGE_ARCHIVE_MEMBER_HEADER *header)
{
    FileRecord *record=file_slots[current_input_file];
    recon_native_entry_finish(library==current_input_library->next->next &&
        (library->flags & 0xb)==0xb && library->name && record && record->view &&
        !strcmp(record->name,library->name) && header && archive_member_offset>=60 &&
        archive_member_size>=20 && record->file_size==archive_member_offset &&
        (DWORD)record->size>=archive_member_offset+archive_member_size &&
        !memcmp(header,(const BYTE *)record->view+archive_member_offset-60,60) &&
        header->EndHeader[0]=='`' && header->EndHeader[1]=='\n');
    return NULL;
}
static int library_member_header(void)
{
    return recon_native_at_entry(native_library_member_name,check_library_member_header,
        invoke_hello,NULL)==1;
}
extern const char *__stdcall archive_member_name(const IMAGE_ARCHIVE_MEMBER_HEADER *,const char *);
static const char *__stdcall check_archive_name_input(const IMAGE_ARCHIVE_MEMBER_HEADER *header,
    const char *long_names)
{
    FileRecord *record=file_slots[current_input_file];
    OutputLibrary *library=current_input_library->next->next;
    char expected[17],*slash;
    const char *actual;
    memcpy(expected,header->Name,16);
    expected[16]=0;
    slash=strrchr(expected,'/');
    if (!slash) { recon_native_entry_finish(0); return NULL; }
    *slash=0;
    actual=archive_member_name(header,long_names);
    recon_native_entry_finish(actual && !strcmp(actual,expected) &&
        archive_member_name(header,long_names)==actual && header && record && record->view &&
        !memcmp(header,(const BYTE *)record->view+archive_member_offset-60,60) &&
        long_names==(const char *)library->index_data[4] && !long_names &&
        header->Name[0]!='/' && memchr(header->Name,'/',16)!=NULL);
    return NULL;
}
static int archive_name_last_separator(void)
{
    IMAGE_ARCHIVE_MEMBER_HEADER header;
    const char *name;
    memset(&header,' ',sizeof(header));
    memcpy(header.Name,"a/b/",4);
    name=archive_member_name(&header,NULL);
    return name && !strcmp(name,"a/b") && !memcmp(header.Name,"a/b/",4);
}
static int archive_name_input(void)
{
    return recon_native_at_entry(archive_member_name,check_archive_name_input,
        invoke_hello,NULL)==1;
}
extern BYTE __stdcall select_delay_load_library(const char *);
static BYTE __stdcall check_delay_selection(const char *name)
{
    recon_native_entry_finish(name && *name && !delay_libraries.first &&
        !delay_libraries.count && !select_delay_load_library(name));
    return 0;
}
static int delay_selection(void)
{
    return recon_native_at_entry(select_delay_load_library,check_delay_selection,
        invoke_hello,NULL)==1;
}
extern IMAGE_SECTION_HEADER *module_section_headers;
extern unsigned module_section_capacity;
extern IMAGE_SECTION_HEADER *__stdcall find_section_by_name(IMAGE_SECTION_HEADER *,
    DWORD,const char *,BYTE *);
static IMAGE_SECTION_HEADER *__stdcall check_member_sections(IMAGE_SECTION_HEADER *sections,
    DWORD count,const char *name,BYTE *duplicate)
{
    FileRecord *record=file_slots[current_input_file];
    const IMAGE_FILE_HEADER *header=(const IMAGE_FILE_HEADER *)
        ((const BYTE *)record->view+archive_member_offset);
    DWORD start=archive_member_offset+20+header->SizeOfOptionalHeader;
    recon_native_entry_finish(sections==module_section_headers && count &&
        count==header->NumberOfSections && count<=module_section_capacity &&
        name && !strcmp(name,".idata$5") && duplicate && !*duplicate &&
        record->file_size==start+40*count &&
        !memcmp(sections,(const BYTE *)record->view+start,40*count));
    return NULL;
}
static int member_sections(void)
{
    return recon_native_at_entry(find_section_by_name,check_member_sections,
        invoke_hello,NULL)==1;
}
extern ComdatRoot *comdat_roots;
extern void __stdcall add_comdat_root(const char *,StringNode *,Contribution *,ComdatRoot **);
static void __stdcall check_comdat_root(const char *name,StringNode *symbol,
    Contribution *owner,ComdatRoot **roots)
{
    MemoryPool *pool=comdat_names;
    ComdatRoot *previous,*expected;
    unsigned used;
    if (name || symbol || !owner || roots!=&comdat_roots || !pool ||
        pool->remaining<12) { recon_native_entry_finish(0); return; }
    previous=*roots;
    used=pool->used;
    expected=(ComdatRoot *)((BYTE *)(pool+1)+used);
    add_comdat_root(name,symbol,owner,roots);
    recon_native_entry_finish(comdat_names==pool && pool->used==used+12 &&
        *roots==expected && expected->owner==owner && expected->kind==1 &&
        expected->next==previous);
}
static int comdat_root(void)
{
    return recon_native_at_entry(add_comdat_root,check_comdat_root,invoke_hello,NULL)==1;
}
static Contribution class_probe_contribution;
static OutputModule class_probe_module;
static ContributionInfo class_probe_info;
static LinkContext class_probe_context;
static IMAGE_SYMBOL class_probe_symbol;
static IMAGE_RELOCATION class_probe_relocation;
extern void __cdecl link_contribution_fatal(Contribution *,unsigned,...);
static void __cdecl check_undefined_static(Contribution *contribution,unsigned message,...)
{
    va_list arguments;
    const char *name;
    va_start(arguments,message);
    name=va_arg(arguments,const char *);
    va_end(arguments);
    recon_native_entry_finish(contribution==&class_probe_contribution && message==1186 &&
        class_probe_symbol.StorageClass==IMAGE_SYM_CLASS_UNDEFINED_STATIC &&
        name && !strcmp(name,"probe"));
}
static void invoke_relocation_classes(void *unused)
{
    class_probe_symbol.StorageClass=IMAGE_SYM_CLASS_LABEL;
    if (read_x86_relocations(&class_probe_context,&class_probe_contribution,
        &class_probe_symbol,&class_probe_relocation,1,1)!=1) return;
    class_probe_symbol.StorageClass=IMAGE_SYM_CLASS_UNDEFINED_STATIC;
    read_x86_relocations(&class_probe_context,&class_probe_contribution,
        &class_probe_symbol,&class_probe_relocation,1,1);
}
static int relocation_storage_classes(void)
{
    memset(&class_probe_contribution,0,sizeof(class_probe_contribution));
    memset(&class_probe_module,0,sizeof(class_probe_module));
    memset(&class_probe_context,0,sizeof(class_probe_context));
    memset(&class_probe_info,0,sizeof(class_probe_info));
    memset(&class_probe_symbol,0,sizeof(class_probe_symbol));
    memset(&class_probe_relocation,0,sizeof(class_probe_relocation));
    class_probe_module.contributions=&class_probe_contribution;
    class_probe_module.section_count=1;
    class_probe_module.contribution_info=&class_probe_info;
    class_probe_contribution.module=&class_probe_module;
    memcpy(class_probe_symbol.N.ShortName,"probe",6);
    class_probe_relocation.Type=IMAGE_REL_I386_DIR32;
    return recon_native_at_entry(link_contribution_fatal,check_undefined_static,
        invoke_relocation_classes,NULL)==1;
}
extern void __stdcall remember_common_symbol(CommonSymbol **,StringNode *);
static void __stdcall check_common_symbol_list(CommonSymbol **list,StringNode *symbol)
{
    CommonSymbol *previous,*first,*second;
    StringNode distinct;
    if (!list || !symbol) { recon_native_entry_finish(0); return; }
    previous=*list;
    remember_common_symbol(list,symbol);
    first=*list;
    remember_common_symbol(list,&distinct);
    second=*list;
    recon_native_entry_finish(first && second && first!=second &&
        first->symbol==symbol && first->next==previous && second->symbol==&distinct &&
        second->next==first && offsetof(OutputModule,common_symbols)==0x54);
}
static int common_symbol_list(void)
{
    return recon_native_at_entry(remember_common_symbol,check_common_symbol_list,
        invoke_hello,NULL)==1;
}
/* The real 20-byte member receiver and thiscall1/RET4 entry, expressed in
 * native test C without adding an application adapter. */
typedef struct DefinitionBufferState {
    BYTE *data,*end;
    int capacity;
    void (__stdcall *relocate)(void *,void *,void *);
    void *context;
} DefinitionBufferState;
extern int RECON_THISCALL native_definition_grow(DefinitionBufferState *,int)
    __asm__("_ZN17DefinitionStorageIhE4GrowEi");
extern void RECON_THISCALL native_definition_destroy(DefinitionBufferState *)
    __asm__("_ZN17DefinitionStorageIhED2Ev");
static int definition_buffer_allocation(void)
{
    DefinitionBufferState owner={NULL,NULL,0,NULL,NULL};
    ReconCppAllocation *allocation;
    BYTE *base;
    unsigned i;
    if(sizeof(owner)!=20 || !native_definition_grow(&owner,4096) ||
       !owner.data || owner.end!=owner.data || owner.capacity!=8192 ||
       owner.relocate || owner.context) return 0;
    for(allocation=recon_cpp_allocations;allocation;allocation=allocation->next)
        if(allocation->base==owner.data && allocation->live)break;
    if(!allocation || allocation->requested!=8192)return 0;
    for(i=0;i<8192;++i)if(owner.data[i])return 0;
    base=owner.data;
    native_definition_destroy(&owner);
    return !allocation->live && owner.data==base && owner.end==base &&
        !owner.capacity && !owner.relocate && !owner.context;
}

static int subsystem_versions(void)
{
    static const WORD inputs[]={1,2,3,7,9,0,4,6};
    static const WORD majors[]={4,4,4,19,2,0,0,0};
    static const WORD minors[]={0,0,0,90,0,0,0,0};
    LinkContext context,expected;
    unsigned i;
    for(i=0;i<sizeof(inputs)/sizeof(inputs[0]);++i) {
        memset(&context,0x69,sizeof(context));
        context.optional_header.Subsystem=inputs[i];
        expected=context;
        expected.optional_header.MajorSubsystemVersion=majors[i];
        expected.optional_header.MinorSubsystemVersion=minors[i];
        set_subsystem_version(&context);
        if(memcmp(&context,&expected,sizeof(context)))return 0;
    }
    return 1;
}

static void __cdecl check_default_library_warning(const char *file,unsigned message,...)
{
    const char *name;
    va_list args;
    va_start(args,message);
    name=va_arg(args,const char *);
    va_end(args);
    recon_native_entry_finish(!file && message==4098 && name && !strcmp(name,"blocked.lib"));
}
static void invoke_default_library_warning(void *list)
{
    warn_missing_default_libraries((LinkPointerList *)list);
}
static int missing_default_library_warning(void)
{
    DefaultLibrary item={NULL,"blocked.lib",0x48,NULL};
    LinkPointerList list={NULL,NULL,1,{0,0,0},&item};
    warn_missing_default_libraries(&list);
    list.suppress_default_libraries=0;
    item.flags=4;
    warn_missing_default_libraries(&list);
    item.flags=8;
    warn_missing_default_libraries(&list);
    item.flags=0x40;
    warn_missing_default_libraries(&list);
    item.flags=0x48;
    return recon_native_at_entry(link_warning,check_default_library_warning,
        invoke_default_library_warning,&list)==1;
}

static void __cdecl check_unused_delay_warning(const char *file,unsigned message,...)
{
    const char *first,*second;
    va_list args;
    va_start(args,message);
    first=va_arg(args,const char *);
    second=va_arg(args,const char *);
    va_end(args);
    recon_native_entry_finish(!file && message==4199 && first==second && first &&
        !strcmp(first,"unused.dll"));
}
static void invoke_unused_delay_warning(void *unused)
{
    (void)unused;
    warn_unused_delay_libraries();
}
static int unused_delay_warning(void)
{
    InputArgument unused={"unused.dll",NULL,NULL,0,0,NULL};
    InputArgument used={"used.dll",NULL,NULL,0,0x12340200,&unused};
    delay_libraries.first=NULL;
    delay_libraries.count=2;
    warn_unused_delay_libraries();
    delay_libraries.first=&used;
    delay_libraries.count=1;
    warn_unused_delay_libraries();
    delay_libraries.count=2;
    return recon_native_at_entry(link_warning,check_unused_delay_warning,
        invoke_unused_delay_warning,NULL)==1;
}

extern const char *output_file_name;
static int old_image_name(void)
{
    const char *saved=output_file_name;
    InputArguments saved_files=link_files;
    char *name;
    int pass;
    output_file_name="hello.exe";
    name=derive_old_image_name(NULL);
    pass=name && !strcmp(name,"hello.ilk") && name!=output_file_name;
    release_memory(name);
    output_file_name=NULL;
    link_files.count=0;
    pass=pass && derive_old_image_name(NULL)==NULL;
    link_files=saved_files;
    output_file_name=saved;
    return pass;
}

static int missing_old_image(void)
{
    LinkContext image,before;
    struct _stat information,old_information;
    BYTE result;
    memset(&image,0x5a,sizeof(image));
    before=image;
    memset(&information,0x6b,sizeof(information));
    old_information=information;
    result=read_old_image("hello.ilk",0,&image,&information);
    return result==0 && !memcmp(&image,&before,sizeof(image)) &&
        !memcmp(&information,&old_information,sizeof(information));
}

extern char short_symbol_name[9];
extern void __stdcall place_common_symbol(LinkContext *,StringNode *);
static void __stdcall check_common_placement(LinkContext *context,StringNode *symbol)
{
    StringNode probe;
    LinkContext copy;
    Contribution *contribution;
    (void)symbol;
    if(!context) { recon_native_entry_finish(0); return; }
    copy=*context;
    copy.options.gp_size=0;
    copy.options.effective_reference_opt=1;
    memset(&probe,0,sizeof(probe));
    memcpy(probe.symbol.N.ShortName,"ALIAS123",8);
    probe.symbol.Value=17;
    probe.flags=2;
    memcpy(short_symbol_name,"sentinel",8);
    short_symbol_name[8]=0;
    place_common_symbol(&copy,&probe);
    contribution=probe.contribution;
    recon_native_entry_finish(contribution && !probe.symbol.Value &&
        contribution->raw_size==17 &&
        contribution->characteristics==0xc0600080 &&
        !strcmp(contribution->group->name,".bss") &&
        !memcmp(short_symbol_name,"ALIAS123",8) && !short_symbol_name[8]);
}
static int common_placement(void)
{
    return recon_native_at_entry(place_common_symbol,check_common_placement,
        invoke_hello,NULL)==1;
}

extern void RECON_THISCALL native_resolve_local_imports(LinkContext *,Contribution **,IncludeSymbol **)
    __asm__("_ZN11LinkContext19ResolveLocalImportsEPP12ContributionPP13IncludeSymbol");
static void RECON_THISCALL check_local_import_outputs(LinkContext *context,
    Contribution **contribution,IncludeSymbol **aliases)
{
    LinkContext before;
    if(!context || !contribution || !aliases || (void *)contribution==(void *)aliases) {
        recon_native_entry_finish(0); return;
    }
    before=*context;
    native_resolve_local_imports(context,contribution,aliases);
    recon_native_entry_finish(!*contribution && !*aliases &&
        !memcmp(context,&before,sizeof(before)));
}
static int local_import_outputs(void)
{
    return recon_native_at_entry(native_resolve_local_imports,check_local_import_outputs,
        invoke_hello,NULL)==1;
}

static int merged_group_order(void)
{
    OutputSection source,destination;
    ContributionGroup left[3],right[2],*group;
    ContributionGroup *expected[5];
    unsigned i;
    memset(&source,0,sizeof(source));
    memset(&destination,0,sizeof(destination));
    memset(left,0,sizeof(left));
    memset(right,0,sizeof(right));
    left[0].name="a"; left[1].name="b"; left[2].name="d";
    right[0].name="b"; right[1].name="c";
    for(i=0;i<3;++i) {
        left[i].section=&source;
        if(i<2)left[i].next=&left[i+1];
    }
    right[0].section=right[1].section=&destination;
    right[0].next=&right[1];
    source.groups=left; destination.groups=right;
    expected[0]=left; expected[1]=right; expected[2]=left+1;
    expected[3]=right+1; expected[4]=left+2;
    merge_section_groups(&source,&destination);
    if(source.groups || source.merge_target!=&destination)return 0;
    for(i=0,group=destination.groups;i<5;++i,group=group->next) {
        if(group!=expected[i] || group->section!=&destination)return 0;
    }
    if(group)return 0;
    merge_section_groups(&source,&destination);
    return destination.groups==left && left[0].next==right;
}
extern InputArguments section_attributes,section_merges;
static void __cdecl check_distinct_section_options(void)
{
    recon_native_entry_finish(!section_attributes.first && !section_attributes.count &&
        section_merges.first && section_merges.count);
}
static int distinct_section_options(void)
{
    return recon_native_at_entry(prepare_import_sections,check_distinct_section_options,
        invoke_hello,NULL)==1;
}

extern void __stdcall sort_import_group(ContributionGroup *);
static void __stdcall check_import_alignment(ContributionGroup *group)
{
    ContributionGroup *descriptors,*null_descriptor;
    descriptors=find_contribution_group(import_descriptor_section,".idata$2");
    null_descriptor=find_contribution_group(import_descriptor_section,".idata$3");
    recon_native_entry_finish(group && !strcmp(group->name,".idata$4") &&
        group->first && descriptors && null_descriptor &&
        descriptors->alignment==4 && null_descriptor->alignment==4);
}
static int import_alignment(void)
{
    return recon_native_at_entry(sort_import_group,check_import_alignment,
        invoke_hello,NULL)==1;
}

extern BYTE __stdcall sort_import_tail(Contribution *);
static BYTE __stdcall check_sorted_imports(Contribution *current)
{
    Contribution *item;
    ContributionGroup *group=current?current->group:NULL;
    const char *previous=NULL;
    unsigned count=0;
    if(!group || !current->rva || strcmp(group->name,".idata$4")) {
        recon_native_entry_finish(0); return 0;
    }
    for(item=group->first;item && count<=group->contribution_count;item=item->next) {
        if(item->group!=group || !item->module || !item->module->name ||
            (previous && strcmp(previous,item->module->name)>0)) {
            recon_native_entry_finish(0); return 0;
        }
        previous=item->module->name;
        ++count;
    }
    recon_native_entry_finish(!item && count==group->contribution_count && count>1);
    return 0;
}
static int sorted_imports(void)
{
    return recon_native_at_entry(sort_import_tail,check_sorted_imports,
        invoke_hello,NULL)==1;
}

static int import_module_tail(void)
{
    OutputModule modules[4];
    Contribution members[4];
    ContributionGroup group;
    unsigned i;
    memset(modules,0,sizeof(modules));
    memset(members,0,sizeof(members));
    memset(&group,0,sizeof(group));
    for(i=0;i<4;++i) {
        modules[i].name=i<3?"same":"later";
        members[i].module=&modules[i];
        members[i].group=&group;
        if(i<3)members[i].next=&members[i+1];
    }
    group.first=members;
    group.last=members+3;
    if(sort_import_tail(members)!=1 || group.first!=members+1 ||
        members[1].next!=members+2 || members[2].next!=members ||
        members[0].next!=members+3 || members[3].next || group.last!=members+3)return 0;
    return sort_import_tail(members)==0 && group.first==members+1 &&
        members[0].next==members+3;
}

extern OutputSection *relocation_section;
extern BYTE *base_relocations,*next_base_relocation,*base_relocations_end;
extern BYTE no_base_relocations;
extern void RECON_THISCALL native_finalize_order(LinkContext *)
    __asm__("_ZN11LinkContext13FinalizeOrderEv");
static void RECON_THISCALL check_relocation_allocation(LinkContext *context)
{
    DWORD count;
    BYTE *old_base=base_relocations,*old_next=next_base_relocation,*old_end=base_relocations_end;
    BYTE old_empty=no_base_relocations;
    if(!context || context->is_vxd) {
        recon_native_entry_finish(0); return;
    }
    count=context->optional_header.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size;
    native_finalize_order(context);
    if(!relocation_section || strcmp(relocation_section->name,".reloc")) {
        recon_native_entry_finish(0); return;
    }
    if(context->options.fixed_image) {
        recon_native_entry_finish(relocation_section->raw_size==0 &&
            base_relocations==old_base && next_base_relocation==old_next &&
            base_relocations_end==old_end && no_base_relocations==old_empty &&
            context->optional_header.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size==count);
        return;
    }
    recon_native_entry_finish(relocation_section->raw_size==1 &&
        base_relocations && next_base_relocation==base_relocations &&
        context->optional_header.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size==(count?count:1) &&
        (DWORD)(base_relocations_end-base_relocations)==(count?count:1)*12 &&
        (count || no_base_relocations));
}
static int relocation_allocation(void)
{
    return recon_native_at_entry(native_finalize_order,check_relocation_allocation,
        invoke_hello,NULL)==1;
}

static int appended_section_groups(void)
{
    OutputSection source,destination;
    ContributionGroup first,second;
    memset(&source,0,sizeof(source));
    memset(&destination,0,sizeof(destination));
    memset(&first,0,sizeof(first));
    memset(&second,0,sizeof(second));
    first.name="z"; second.name="a";
    first.section=&destination; second.section=&source;
    destination.groups=&first; source.groups=&second;
    merge_output_section(&source,&destination);
    if(destination.groups!=&first || first.next!=&second || second.next ||
        second.section!=&destination || source.groups || source.merge_target!=&destination)return 0;
    merge_output_section(&source,&destination);
    return destination.groups==&first && first.next==&second && !second.next;
}

static int named_group_transfer(void)
{
    OutputSection source,destination,alias;
    ContributionGroup a,b,c,old_b,d;
    memset(&source,0,sizeof(source)); memset(&destination,0,sizeof(destination));
    memset(&alias,0,sizeof(alias)); memset(&a,0,sizeof(a)); memset(&b,0,sizeof(b));
    memset(&c,0,sizeof(c)); memset(&old_b,0,sizeof(old_b)); memset(&d,0,sizeof(d));
    a.name="a"; b.name="b"; c.name="c"; old_b.name="b"; d.name="d";
    a.next=&b; b.next=&c; old_b.next=&d;
    a.section=b.section=c.section=&source;
    old_b.section=d.section=&destination;
    source.groups=&a; destination.groups=&old_b;
    alias.merge_target=&destination;
    move_named_group("b",&source,&alias);
    if(source.groups!=&a || a.next!=&c || c.next ||
        destination.groups!=&old_b || old_b.next!=&b || b.next!=&d || d.next ||
        b.section!=&destination || alias.groups || alias.merge_target!=&destination)return 0;
    move_named_group("missing",&source,&alias);
    return source.groups==&a && a.next==&c && old_b.next==&b && b.next==&d;
}

static int prepended_group(void)
{
    OutputSection section;
    ContributionGroup a,b,c;
    memset(&section,0,sizeof(section));
    memset(&a,0,sizeof(a)); memset(&b,0,sizeof(b)); memset(&c,0,sizeof(c));
    section.groups=&a; a.next=&b; b.next=&c;
    a.section=b.section=c.section=&section;
    move_group_to_end(&c);
    if(section.groups!=&c || c.next!=&a || a.next!=&b || b.next ||
        a.section!=&section || b.section!=&section || c.section!=&section)return 0;
    move_group_to_end(&c);
    return section.groups==&c && c.next==&a && a.next==&b && !b.next;
}

extern WORD RECON_THISCALL native_count_sections(LinkContext *)
    __asm__("_ZN11LinkContext19CountOutputSectionsEv");
static int counted_sections(void)
{
    LinkContext context;
    OutputSection sections[7];
    ContributionGroup group;
    Contribution ignored,live;
    OutputSection *old_debug=debug_section,*old_procedure=procedure_section;
    OutputSection *old_relocation=relocation_section;
    unsigned i;
    int pass;
    memset(&context,0,sizeof(context)); memset(sections,0,sizeof(sections));
    memset(&group,0,sizeof(group)); memset(&ignored,0,sizeof(ignored));
    memset(&live,0,sizeof(live));
    for(i=0;i<6;++i)sections[i].next=sections+i+1;
    context.sections.first=sections;
    sections[0].raw_size=1;
    sections[1].raw_size=1; sections[1].characteristics=0x800;
    debug_section=sections+2;
    sections[3].groups=&group; group.first=&ignored;
    ignored.raw_size=4; ignored.characteristics=0x800; ignored.next=&live;
    live.raw_size=1;
    procedure_section=sections+4; sections[4].raw_size=1;
    relocation_section=sections+5; sections[5].raw_size=1;
    pass=native_count_sections(&context)==4;
    context.options.rom=1;
    pass=pass && native_count_sections(&context)==2;
    debug_section=old_debug; procedure_section=old_procedure;
    relocation_section=old_relocation;
    return pass;
}

extern DWORD RECON_THISCALL native_dos_extension(LinkContext *,void **)
    __asm__("_ZN11LinkContext17BuildDosExtensionEPPv");
static int encoded_compiler_ids(void)
{
    LinkContext context;
    OutputLibrary library;
    OutputModule modules[3];
    DWORD expected[14]={0x536e61cc,136,136,136,138,137,137,138,
        0x68636952,136,0,0,0,0};
    void *buffer=NULL;
    DWORD size;
    int pass;
    memset(&context,0,sizeof(context)); memset(&library,0,sizeof(library));
    memset(modules,0,sizeof(modules));
    modules[0].compiler_id=1; modules[0].next=modules+1;
    modules[1].compiler_id=2; modules[1].next=modules+2;
    modules[2].compiler_id=1;
    library.module_head=modules;
    context.pointer_list.first=&library; context.dos_image_size=128;
    size=native_dos_extension(&context,&buffer);
    pass=size==sizeof(expected) && buffer && !memcmp(buffer,expected,sizeof(expected));
    release_memory(buffer);
    context.options.rom=1;
    buffer=&context;
    return pass && native_dos_extension(&context,&buffer)==0 && !buffer;
}

extern DWORD RECON_THISCALL native_pe_header_size(LinkContext *,DWORD *)
    __asm__("_ZN11LinkContext12PeHeaderSizeEPm");
extern ByteBuffer image_comments;
extern BYTE mapped_debug;
static int pe_header_offsets(void)
{
    LinkContext context;
    DWORD offset,old_used=image_comments.used;
    BYTE old_mapped=mapped_debug;
    int pass;
    memset(&context,0,sizeof(context));
    context.pe_header_offset=176;
    context.file_header.SizeOfOptionalHeader=224;
    context.file_header.NumberOfSections=3;
    image_comments.used=7; mapped_debug=0;
    pass=native_pe_header_size(&context,&offset)==551 && offset==424;
    mapped_debug=1;
    pass=pass && native_pe_header_size(&context,&offset)==591 && offset==424;
    context.options.rom=1;
    pass=pass && native_pe_header_size(&context,&offset)==411 && offset==244;
    image_comments.used=old_used; mapped_debug=old_mapped;
    return pass;
}

extern BYTE mac_target;
extern void __stdcall mark_live_comdat(StringTable *,Contribution *);
static int comdat_cycle(void)
{
    OutputModule module;
    Contribution members[3];
    ContributionInfo info[3];
    ComdatReference references[2];
    BYTE old_mac=mac_target;
    unsigned i;
    int pass;
    memset(&module,0,sizeof(module)); memset(members,0,sizeof(members));
    memset(info,0,sizeof(info)); memset(references,0,sizeof(references));
    module.section_count=3; module.contributions=members; module.contribution_info=info;
    for(i=0;i<3;++i)members[i].module=&module;
    members[0].characteristics=0x40; members[1].characteristics=0x80;
    members[2].characteristics=0x100;
    info[0].references_used=info[0].reference_count=1; info[0].references=references;
    info[1].references_used=info[1].reference_count=1; info[1].references=references+1;
    references[0].contribution=members+1; references[1].contribution=members;
    mac_target=0;
    mark_live_comdat(NULL,members);
    pass=members[0].characteristics==0x2040 && members[1].characteristics==0x2080 &&
        members[2].characteristics==0x100 && references[0].contribution==members+1 &&
        references[1].contribution==members;
    mac_target=old_mac;
    return pass;
}

extern void __stdcall append_import_module(OutputLibrary *,OutputLibrary ***,int *,int *);
extern void __stdcall reset_import_modules(OutputLibrary **,int *);
static int import_library_storage(void)
{
    OutputLibrary owners[4];
    OutputLibrary **libraries=(OutputLibrary **)allocate_memory(8);
    int capacity=2,count=0,pass;
    memset(owners,0,sizeof(owners));
    owners[0].name="first.lib"; owners[1].name="second.lib";
    owners[2].name="third.lib"; owners[3].name="tail.lib";
    append_import_module(owners,&libraries,&capacity,&count);
    append_import_module(owners+1,&libraries,&capacity,&count);
    append_import_module(owners,&libraries,&capacity,&count);
    pass=count==2 && capacity==2 && libraries[0]==owners && libraries[1]==owners+1;
    append_import_module(owners+2,&libraries,&capacity,&count);
    pass=pass && count==3 && capacity==4 && libraries[0]==owners &&
        libraries[1]==owners+1 && libraries[2]==owners+2;
    if(capacity==4) {
        libraries[3]=owners+3;
        reset_import_modules(libraries,&count);
        pass=pass && count==0 && !libraries[0] && !libraries[1] && !libraries[2] &&
            libraries[3]==owners+3;
    } else pass=0;
    release_memory(libraries);
    return pass;
}
static void __stdcall check_import_library(OutputLibrary *library,OutputLibrary ***libraries,
    int *capacity,int *count)
{
    ContributionGroup *group=find_contribution_group(import_section,".idata$4");
    recon_native_entry_finish(group && group->first && group->first->module &&
        library==group->first->module->library && library && libraries && *libraries &&
        capacity && *capacity==5 && count && !*count);
}
static int actual_import_library(void)
{
    return recon_native_at_entry(append_import_module,check_import_library,
        invoke_hello,NULL)==1;
}

extern BYTE __stdcall compare_fold_references(ContributionInfo *,ContributionInfo *,LinkContext *,DWORD);
static BYTE __stdcall check_fold_recursion_marker(ContributionInfo *first,ContributionInfo *second,
    LinkContext *context,DWORD depth)
{
    recon_native_entry_finish(first && second && first!=second && context && depth==2 &&
        first->comparison_depth==1 && second->comparison_depth==1 &&
        first->fold_candidate && second->fold_candidate &&
        first->fold_candidate!=second->fold_candidate &&
        first->fold_candidate->group==second->fold_candidate->group &&
        first->fold_candidate->characteristics==second->fold_candidate->characteristics &&
        first->fold_candidate->comdat_selection==second->fold_candidate->comdat_selection);
    return 0;
}
static int fold_recursion_marker(void)
{
    return recon_native_at_entry(compare_fold_references,check_fold_recursion_marker,
        invoke_hello,NULL)==1;
}

extern MemoryPool *fold_changes;
extern BYTE __stdcall compare_fold_symbols(IMAGE_SYMBOL *,const char *,IMAGE_SYMBOL *,const char *,LinkContext *);
static int fold_pool_span(const void *pointer,DWORD bytes)
{
    MemoryPool *block;
    DWORD start,address=(DWORD)pointer;
    for(block=fold_changes;block;block=block->previous) {
        start=(DWORD)(block+1);
        if(address>=start && bytes<=block->used && address-start<=block->used-bytes)return 1;
    }
    return 0;
}
static BYTE __stdcall check_fold_input_cache(IMAGE_SYMBOL *first,const char *first_strings,
    IMAGE_SYMBOL *second,const char *second_strings,LinkContext *context)
{
    OutputLibrary *library;
    OutputModule *module;
    ModuleIndexRecord *cache;
    DWORD start,bytes,address;
    int found_first=0,found_second=0;
    if(!context || !buffer8.base || !fold_changes)recon_native_entry_finish(0);
    for(library=(OutputLibrary *)context->pointer_list.first;library;library=library->next) {
        for(module=library->module_head;module;module=module->next) {
            if(module->module_index>=buffer8.remaining)recon_native_entry_finish(0);
            cache=(ModuleIndexRecord *)buffer8.base+module->module_index;
            if(!cache->symbols)continue;
            start=(DWORD)cache->symbols; bytes=module->symbol_count*18;
            address=(DWORD)first;
            if(address>=start && address-start<bytes && !((address-start)%18) &&
                cache->strings==first_strings && fold_pool_span(cache->symbols,bytes) &&
                fold_pool_span(cache->strings,0))found_first=1;
            address=(DWORD)second;
            if(address>=start && address-start<bytes && !((address-start)%18) &&
                cache->strings==second_strings && fold_pool_span(cache->symbols,bytes) &&
                fold_pool_span(cache->strings,0))found_second=1;
        }
    }
    recon_native_entry_finish(found_first && found_second);
    return 0;
}
static int fold_input_cache(void)
{
    return recon_native_at_entry(compare_fold_symbols,check_fold_input_cache,
        invoke_hello,NULL)==1;
}

extern WORD output_section_count;
static int section_partition_order(void)
{
    OutputSection nodes[6];
    SectionList list;
    WORD old_count=output_section_count;
    unsigned i;
    int pass;
    memset(nodes,0,sizeof(nodes));
    for(i=0;i<5;++i)nodes[i].next=nodes+i+1;
    nodes[0].characteristics=nodes[2].characteristics=0x40;
    nodes[1].characteristics=nodes[3].characteristics=0x80;
    nodes[4].characteristics=0x20;
    list.first=nodes; list.last=&nodes[5].next;
    output_section_count=6;
    partition_sections(&list,0xc0,0x40);
    pass=list.first==nodes && nodes[0].next==nodes+2 && nodes[2].next==nodes+1 &&
        nodes[1].next==nodes+3 && nodes[3].next==nodes+4 && nodes[4].next==nodes+5 &&
        !nodes[5].next && list.last==&nodes[5].next;
    partition_sections(&list,0x20,0x20);
    pass=pass && list.first==nodes+4 && nodes[4].next==nodes && nodes[0].next==nodes+2 &&
        nodes[2].next==nodes+1 && nodes[1].next==nodes+3 && nodes[3].next==nodes+5 &&
        !nodes[5].next && list.last==&nodes[5].next;
    partition_sections(&list,0x100,0x100);
    pass=pass && list.first==nodes+4 && nodes[4].next==nodes && nodes[0].next==nodes+2 &&
        nodes[2].next==nodes+1 && nodes[1].next==nodes+3 && nodes[3].next==nodes+5 &&
        !nodes[5].next && list.last==&nodes[5].next;
    output_section_count=old_count;
    return pass;
}

extern void RECON_THISCALL native_layout_sections(LinkContext *,DWORD *,DWORD *,DWORD *)
    __asm__("_ZN11LinkContext14LayoutSectionsEPmS0_S0_");
extern BYTE small_section_alignment,pdb_enabled,incremental_enabled,mac_target;
static int small_alignment_owner(void)
{
    IMAGE_OPTIONAL_HEADER header;
    DWORD flags;
    BYTE old=small_section_alignment;
    int pass;
    memset(&header,0,sizeof(header));
    header.Subsystem=IMAGE_SUBSYSTEM_WINDOWS_GUI;
    small_section_alignment=0;
    flags=IMAGE_SCN_CNT_UNINITIALIZED_DATA|IMAGE_SCN_MEM_READ|IMAGE_SCN_MEM_WRITE;
    adjust_output_section_flags(&flags,".bss",&header);
    pass=(flags & (IMAGE_SCN_CNT_INITIALIZED_DATA|IMAGE_SCN_CNT_UNINITIALIZED_DATA))==
        IMAGE_SCN_CNT_UNINITIALIZED_DATA;
    small_section_alignment=1;
    flags=IMAGE_SCN_CNT_UNINITIALIZED_DATA|IMAGE_SCN_MEM_READ|IMAGE_SCN_MEM_WRITE;
    adjust_output_section_flags(&flags,".bss",&header);
    pass=pass && (flags & (IMAGE_SCN_CNT_INITIALIZED_DATA|IMAGE_SCN_CNT_UNINITIALIZED_DATA))==
        IMAGE_SCN_CNT_INITIALIZED_DATA;
    small_section_alignment=old;
    return pass;
}

static int section_layout_offsets(void)
{
    LinkContext context;
    OutputSection sections[3];
    ContributionGroup groups[3];
    Contribution contributions[5];
    OutputSection *old_read_only=read_only_section;
    BYTE old_pdb=pdb_enabled,old_incremental=incremental_enabled;
    BYTE old_mac=mac_target,old_bss=small_section_alignment;
    DWORD rva=0x1000,file=0x200,lines=0;
    unsigned i;
    int pass;
    memset(&context,0,sizeof(context));
    memset(sections,0,sizeof(sections));
    memset(groups,0,sizeof(groups));
    memset(contributions,0,sizeof(contributions));
    context.file_header.Machine=0x14c;
    context.file_header.NumberOfSections=3;
    context.optional_header.SectionAlignment=4096;
    context.optional_header.FileAlignment=512;
    context.sections.first=sections;
    context.sections.last=&sections[2].next;
    sections[0].name=".text"; sections[0].characteristics=0x20;
    sections[1].name=".data"; sections[1].characteristics=0x40;
    sections[2].name=".bss"; sections[2].characteristics=0x80;
    for(i=0;i<3;++i) {
        sections[i].groups=groups+i;
        if(i<2)sections[i].next=sections+i+1;
        groups[i].name=sections[i].name;
        groups[i].alignment=1;
        groups[i].section=sections+i;
    }
    groups[0].first=contributions; groups[0].last=contributions+1;
    groups[1].first=contributions+2; groups[1].last=contributions+3;
    groups[2].first=groups[2].last=contributions+4;
    contributions[0].next=contributions+1;
    contributions[2].next=contributions+3;
    for(i=0;i<5;++i) {
        contributions[i].group=groups+(i/2);
        contributions[i].characteristics=0x100000 | (i<2?0x20:i==2?0x40:0x80);
    }
    contributions[0].raw_size=13;
    contributions[1].raw_size=7; contributions[1].characteristics=0x500020;
    contributions[2].raw_size=5;
    contributions[3].raw_size=11;
    contributions[4].raw_size=17;
    read_only_section=sections+1;
    pdb_enabled=incremental_enabled=mac_target=small_section_alignment=0;
    native_layout_sections(&context,&rva,&file,&lines);
    pass=rva==0x4000 && file==0x600 && !lines &&
        context.optional_header.BaseOfData==0x2000 &&
        context.optional_header.SizeOfCode==512 &&
        context.optional_header.SizeOfInitializedData==512 &&
        context.optional_header.SizeOfUninitializedData==512 &&
        context.file_header.NumberOfSections==3 &&
        contributions[0].rva==0x1000 && contributions[0].raw_size==16 &&
        contributions[0].padding==3 && contributions[1].rva==0x1010 &&
        contributions[1].file_offset==0x210 && groups[0].size==23 &&
        sections[0].virtual_size==23 && sections[0].raw_size==512 &&
        sections[0].reserved08==0x217 && sections[0].initialized_file_size==512 &&
        sections[1].rva==0x2000 && sections[1].virtual_size==16 &&
        sections[1].reserved08==0x405 && sections[1].initialized_file_size==512 &&
        sections[2].rva==0x3000 && sections[2].virtual_size==17 &&
        !sections[2].file_offset && !sections[2].initialized_file_size;
    read_only_section=old_read_only;
    pdb_enabled=old_pdb; incremental_enabled=old_incremental;
    mac_target=old_mac; small_section_alignment=old_bss;
    return pass;
}

static int mapped_output_cursor(void)
{
    FileRecord record;
    BYTE storage[24],expected[24];
    static const BYTE first[]={0x41,0x00,0x7f,0x82,0xff};
    static const BYTE second[]={0x22,0x33};
    int pass;
    memset(&record,0,sizeof(record));
    memset(storage,0xa5,sizeof(storage));
    memset(expected,0xa5,sizeof(expected));
    record.view=storage; record.mapping_size=sizeof(storage);
    record.file_size=3; record.size=4; record.flags=0x1234;
    memcpy(expected+3,first,sizeof(first));
    pass=mapped_write(&record,first,sizeof(first))==sizeof(first) &&
        record.file_size==8 && record.size==8 &&
        !memcmp(storage,expected,sizeof(storage));
    record.file_size=4;
    memcpy(expected+4,second,sizeof(second));
    pass=pass && mapped_write(&record,second,sizeof(second))==sizeof(second) &&
        record.file_size==6 && record.size==8 &&
        !memcmp(storage,expected,sizeof(storage));
    pass=pass && mapped_write(&record,second,0)==0 &&
        record.file_size==6 && record.size==8 && record.flags==0x1234 &&
        record.view==storage && record.mapping_size==sizeof(storage) &&
        !memcmp(storage,expected,sizeof(storage));
    return pass;
}

static void RECON_THISCALL check_import_hint_alignment(LinkContext *context,
    DWORD *rva,DWORD *offset,DWORD *lines)
{
    ContributionGroup *group=find_contribution_group(read_only_section,".idata$6");
    Contribution *item;
    unsigned count=0;
    int pass=context && rva && offset && lines && group &&
        group->alignment==2 && import_section!=procedure_section;
    if(group)for(item=group->first;item;item=item->next) {
        if(item->characteristics & 0x800)continue;
        pass=pass && (item->characteristics & 0x700000)==0x200000 &&
            item->module && item->group==group;
        ++count;
    }
    recon_native_entry_finish(pass && count>40);
}
static int import_hint_alignment(void)
{
    return recon_native_at_entry(native_layout_sections,check_import_hint_alignment,
        invoke_hello,NULL)==1;
}

extern void RECON_THISCALL native_process_output(OutputModule *,LinkContext *,DWORD)
    __asm__("_ZN12OutputModule13ProcessOutputEP11LinkContextm");
extern const char *current_object_name;
static void RECON_THISCALL check_defined_output_state(OutputModule *module,
    LinkContext *context,DWORD state)
{
    FileRecord *file;
    if(!context || !module || !context->command_line_library || current_input_file<0)
        recon_native_entry_finish(0);
    file=file_slots[current_input_file];
    recon_native_entry_finish(module==context->command_line_library->module_head &&
        !strcmp(module->name,"hello.obj") && !strcmp(current_phase,"hello.obj") &&
        !current_object_name && file && file->size==2262 &&
        !strcmp(file->name,"hello.obj") &&
        state==(((DWORD)module & 0xffffff00)|1));
}
static int defined_output_state(void)
{
    return recon_native_at_entry(native_process_output,check_defined_output_state,
        invoke_hello,NULL)==1;
}

extern void __stdcall process_output_symbol(LinkContext *,OutputModule *,IMAGE_SYMBOL *,
    IMAGE_SYMBOL **,DWORD *,DWORD *,DWORD);
extern DWORD emitted_symbol_count,coff_symbol_offset,module_debug_index;
extern char *current_module_strings;
extern BYTE skip_module_debug_symbols;
static void __stdcall check_module_output_symbols(LinkContext *context,OutputModule *module,
    IMAGE_SYMBOL *symbol,IMAGE_SYMBOL **cursor,DWORD *index,DWORD *slot,DWORD state)
{
    FileRecord *input,*output;
    int pass=context && module && symbol && cursor && index;
    if(!pass)recon_native_entry_finish(0);
    input=file_slots[current_input_file];
    output=file_slots[output_file];
    recon_native_entry_finish(module==context->command_line_library->module_head &&
        !strcmp(module->name,"hello.obj") && module->input_symbols==symbol &&
        *cursor==symbol+1 && *index==0 && !slot && module->symbol_count>0 &&
        current_module_strings && module->section_map &&
        !module_debug_index && !skip_module_debug_symbols && input && output &&
        input->file_size==(LONG)module->symbol_offset &&
        output->file_size==(LONG)(coff_symbol_offset+emitted_symbol_count*18) &&
        state==(((DWORD)module & 0xffffff00)|1));
}
static int module_output_symbols(void)
{
    return recon_native_at_entry(process_output_symbol,check_module_output_symbols,
        invoke_hello,NULL)==1;
}

extern void __stdcall process_file_output_symbol(LinkContext *,IMAGE_SYMBOL *,DWORD,DWORD *);
static void __stdcall check_file_output_symbol(LinkContext *context,IMAGE_SYMBOL *symbol,
    DWORD index,DWORD *replacement)
{
    OutputModule *module=context ? context->command_line_library->module_head : NULL;
    recon_native_entry_finish(module && symbol==module->input_symbols &&
        symbol->StorageClass==IMAGE_SYM_CLASS_FILE && symbol->SectionNumber==-2 &&
        symbol->NumberOfAuxSymbols && !index && replacement && *replacement==0xffffffff);
}
static int file_output_symbol(void)
{
    return recon_native_at_entry(process_file_output_symbol,check_file_output_symbol,
        invoke_hello,NULL)==1;
}

extern StringNode *__stdcall process_external_output_symbol(LinkContext *,IMAGE_SYMBOL *);
static void __stdcall check_local_output_values(LinkContext *context,IMAGE_SYMBOL *raw)
{
    OutputModule *module=context->command_line_library->module_head;
    FileRecord *input=file_slots[current_input_file];
    IMAGE_SYMBOL *original;
    unsigned index=0,checked=0;
    if(!module || !input || !(input->flags & 8) || !input->view ||
       raw<=module->input_symbols || raw>=module->input_symbols+module->symbol_count)
        recon_native_entry_finish(0);
    original=(IMAGE_SYMBOL *)((BYTE *)input->view+module->symbol_offset);
    while(module->input_symbols+index<raw) {
        IMAGE_SYMBOL *source=original+index,*current=module->input_symbols+index;
        if(source->StorageClass==IMAGE_SYM_CLASS_STATIC ||
           source->StorageClass==IMAGE_SYM_CLASS_LABEL ||
           source->StorageClass==IMAGE_SYM_CLASS_UNDEFINED_LABEL ||
           source->StorageClass==IMAGE_SYM_CLASS_UNDEFINED_STATIC) {
            if(source->SectionNumber>0) {
                Contribution *contribution=(Contribution *)module->contributions+
                    source->SectionNumber-1;
                while(contribution && (contribution->characteristics & 0x4000))
                    contribution=contribution->associated_contribution;
                if(!contribution || current->Value!=source->Value+contribution->rva ||
                   current->SectionNumber!=contribution->group->section->index)
                    recon_native_entry_finish(0);
            } else if(current->Value)recon_native_entry_finish(0);
            ++checked;
        }
        index+=source->NumberOfAuxSymbols+1;
    }
    recon_native_entry_finish(checked>0 && module->input_symbols+index==raw &&
        raw->StorageClass==IMAGE_SYM_CLASS_EXTERNAL);
}
static int local_output_values(void)
{
    return recon_native_at_entry(process_external_output_symbol,check_local_output_values,
        invoke_hello,NULL)==1;
}

static void __stdcall check_contribution_bytes(Contribution *contribution,void *relocations,
    DWORD count,void *data,void *symbols,LinkContext *context,DWORD *indices)
{
    FileRecord *input=file_slots[current_input_file],*output=file_slots[output_file];
    int pass=contribution && context && input && output && relocations && count && !indices;
    if(!pass)recon_native_entry_finish(0);
    pass=contribution->module==context->command_line_library->module_head &&
        symbols==contribution->module->input_symbols && count==contribution->relocation_count &&
        (input->flags & 8) && (output->flags & 8) &&
        data==(BYTE *)output->view+contribution->file_offset &&
        !memcmp(data,(BYTE *)input->view+contribution->input_raw_offset,
            contribution->raw_size-contribution->padding);
    recon_native_entry_finish(pass);
}
static int contribution_bytes(void)
{
    return recon_native_at_entry(process_x86_relocations,check_contribution_bytes,
        invoke_hello,NULL)==1;
}

extern void RECON_THISCALL native_write_common_symbols(OutputModule *,BYTE)
    __asm__("_ZN12OutputModule18WriteCommonSymbolsEh");
static int common_list_stop(void)
{
    OutputModule module;
    StringNode first,second;
    CommonSymbol head,tail;
    memset(&module,0,sizeof(module));
    memset(&first,0,sizeof(first));
    memset(&second,0,sizeof(second));
    head.symbol=&first;head.next=&tail;
    tail.symbol=&second;tail.next=NULL;
    second.flags=2;
    module.common_symbols=&head;
    /* A continued traversal would reach the genuine BSS trap. */
    native_write_common_symbols(&module,0);
    return module.common_symbols==&head && head.next==&tail && second.flags==2;
}

/* Regression for the former32-byte diagnostic-prefix overwrite. Both native
 * guards must stop longer writes; ordinary formatter destinations stay valid. */
extern void RECON_THISCALL native_load_input_argument(LinkContext *,InputArgument *,OutputLibrary *)
    __asm__("_ZN11LinkContext17LoadInputArgumentEP13InputArgumentP13OutputLibrary");
extern int native_host_close(int) __asm__("close");
static int diagnostic_prefix_bounds(void)
{
    char longer[sizeof(current_phase)+1],other[128];
    OutputModule module;
    OutputLibrary library;
    unsigned which;
    memset(longer,'x',sizeof(current_phase));longer[sizeof(current_phase)]=0;
    memset(&module,0,sizeof(module));memset(&library,0,sizeof(library));
    module.name=longer;module.library=&library;library.flags=16;
    if(sizeof(current_phase)!=41 || format_module_name(&module,other)!=other || strcmp(other,longer))return 0;
    library.flags=0;library.name="updated.lib";module.name="replacement-long-object.obj";
    if(format_module_name(&module,current_phase)!=current_phase ||
       strcmp(current_phase,"updated.lib(replacement-long-object.obj)"))return 0;
    module.name=longer;library.flags=16;
    for(which=0;which<2;++which) {
        int status;
        pid_t child=fork();
        if(child<0)return 0;
        if(!child) {
            InputArgument argument;
            native_host_close(2);
            if(which)format_module_name(&module,current_phase);
            else {
                memset(&argument,0,sizeof(argument));argument.name=longer;
                native_load_input_argument(NULL,&argument,NULL);
            }
            _exit(1);
        }
        if(waitpid(child,&status,0)!=child || !WIFEXITED(status) || WEXITSTATUS(status)!=125)return 0;
    }
    return 1;
}
const ReconTest hello_tests[]={
    {"diagnostic-prefix-bounds",1,diagnostic_prefix_bounds},
    {"common-list-stop",1,common_list_stop},
    {"contribution-bytes",1,contribution_bytes},
    {"local-output-values",1,local_output_values},
    {"file-output-symbol",1,file_output_symbol},
    {"module-output-symbols",1,module_output_symbols},
    {"defined-output-state",1,defined_output_state},
    {"import-hint-alignment",1,import_hint_alignment},
    {"mapped-output-cursor",1,mapped_output_cursor},
    {"section-layout-offsets",1,section_layout_offsets},
    {"small-alignment-owner",1,small_alignment_owner},
    {"section-partition-order",1,section_partition_order},
    {"fold-input-cache",1,fold_input_cache},
    {"fold-recursion-marker",1,fold_recursion_marker},
    {"import-library-storage",1,import_library_storage},
    {"actual-import-library",1,actual_import_library},
    {"comdat-cycle",1,comdat_cycle},
    {"pe-header-offsets",1,pe_header_offsets},
    {"encoded-compiler-ids",1,encoded_compiler_ids},
    {"counted-sections",1,counted_sections},
    {"prepended-group",1,prepended_group},
    {"named-group-transfer",1,named_group_transfer},
    {"appended-section-groups",1,appended_section_groups},
    {"relocation-allocation",1,relocation_allocation},
    {"import-module-tail",1,import_module_tail},
    {"sorted-imports",1,sorted_imports},
    {"import-alignment",1,import_alignment},
    {"merged-group-order",1,merged_group_order},
    {"distinct-section-options",1,distinct_section_options},
    {"local-import-outputs",1,local_import_outputs},
    {"common-placement",1,common_placement},
    {"missing-old-image",1,missing_old_image},
    {"old-image-name",1,old_image_name},
    {"unused-delay-warning",1,unused_delay_warning},
    {"missing-default-library-warning",1,missing_default_library_warning},
    {"subsystem-versions",1,subsystem_versions},
    {"definition-buffer-allocation",1,definition_buffer_allocation},
    {"common-symbol-list",1,common_symbol_list},
    {"relocation-storage-classes",1,relocation_storage_classes},
    {"comdat-root",1,comdat_root},
    {"member-sections",1,member_sections},
    {"delay-selection",1,delay_selection},
    {"archive-name-input",1,archive_name_input},
    {"archive-name-last-separator",1,archive_name_last_separator},
    {"library-member-header",1,library_member_header},
    {"default-library-attempt",1,default_library_attempt},
    {"default-library-input",1,default_library_input},
    {"archive-compare-inputs",1,archive_compare_inputs},
    {"find-symbol-member-input",1,find_symbol_member_input},
    {"undefined-enumerator",1,undefined_enumerator},
    {"undefined-enumerator-input",1,undefined_enumerator_input},
    {"library-file-restore",1,library_file_restore},
    {"library-search-outputs",1,library_search_outputs},
    {"archive-index-tables",1,archive_index_tables},
    {"archive-count-bytes",1,archive_count_bytes},
    {"archive-header-call",1,archive_header_call},
    {"library-index-position",1,library_index_position},
    {"library-reader-input",1,library_reader_input},
    {"library-search-input",1,library_search_input},
    {"startup-undefined",1,startup_undefined},
    {"decorated-entry",1,decorated_entry},
    {"initial-entry-copy",1,initial_entry_copy},
    {"inferred-startup",1,inferred_startup},
    {"entry-symbol-predicate",1,entry_symbol_predicate},
    {"gui-subsystem",1,gui_subsystem},
    {"entry-symbol-lookup",1,entry_symbol_lookup},
    {"cached-file-transition",1,cached_file_transition},
    {"bound-comdat-references",1,bound_comdat_references},
    {"reference-enumerator",1,reference_enumerator},
    {"module-contribution-enumerator",1,module_contribution_enumerator},
    {"symbol-reference-owner",1,symbol_reference},
    {"input-symbol-definition",1,input_symbol_definition},
    {"symbol-class-call",1,symbol_class_call},
    {"symbol-cursor-call",1,symbol_cursor},
    {"section-header-allocation",1,section_headers},
    {"loader-section-header-call",1,section_header_call},
    {"section-header-read",1,section_header_read},
    {"symbol-read-mapping",1,symbol_read_mapping},
    {"symbol-mapped-range",1,symbol_mapping},
    {"string-mapped-range",1,string_mapping},
    {"section-name-call",1,section_name_call},
    {"named-contribution",1,named_contribution},
    {"section-name-split",1,section_name_split},
    {"first-contribution",1,first_contribution},
    {"directive-reader-call",1,directive_reader_call},
    {"directive-buffer",1,directive_buffer},
    {"directive-collection",1,directive_collection},
    {"first-directive-token",1,first_directive_token},
    {"collected-directives",1,collected_directives},
    {"module-name-call",1,module_name_call},
    {"module-filename",1,module_filename},
    {"default-library-entry",1,default_library_entry},
    {"first-comdat",1,first_comdat},
    {"section-symbol-ranges-call",1,section_symbol_ranges_call},
    {"comdat-symbol-name",1,comdat_symbol_name},
    {"comdat-symbol-lookup",1,comdat_lookup},
    {"comdat-hash-lookup",1,comdat_hash_lookup},
    {"intern-symbol",1,intern_symbol},
    {"string-tree-pool",1,string_tree_pool},
    {"string-buffer-reserve",1,string_buffer_reserve},
    {"symbol-string-append",1,symbol_string_append},
    {"symbol-definition",1,symbol_definition},
    {"symbol-definition-transition",1,definition_transition},
    {"folded-contribution",1,folded_contribution},
    {"fold-record",1,fold_record},
    {"comdat-attachment",1,comdat_attachment},
    {"comdat-name-copy",1,comdat_name_copy},
    {"name-pool-call",1,name_pool_call},
    {"relocation-reader",1,relocation_reader},
    {"relocation-load",1,relocation_load},
    {"x86-relocation-read",1,x86_relocation_read},
    {"x86-classifier",1,x86_classify},
    {"relocation-reference",1,relocation_reference},
    {"comdat-reference",1,comdat_reference},
    {"completed-references",1,completed_references},
    {"sections-complete",1,sections_complete}
};
