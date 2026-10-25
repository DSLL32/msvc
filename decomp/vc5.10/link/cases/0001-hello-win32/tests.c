/* Narrow live-state check for the real hello invocation, before object loading. */
#include <recon-tool.h>
#include <recon-native.h>
#include <string.h>
#include "../../src/input.h"
#include "../../src/memory.h"
#include "../../src/initializers.h"
#include "../../src/files.h"
#include "../../src/options.h"
#include "../../harness/startup.h"
#include <recon.h>

extern MemoryArena *section_header_arena;

static void __stdcall check_section_headers(LinkContext *context,void *parent,
    OutputModule *module,WORD flags)
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
extern DWORD *current_module_symbol_indices;
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
        current_module_symbol_indices && pending && file->file_size==300;
    if (!pass) recon_native_entry_finish(0);
    for (i=0;i<37;++i) if (current_module_symbol_indices[i]) recon_native_entry_finish(0);
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
        state->key_name==string_key_name && state->reserved24==0 &&
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
const ReconTest hello_tests[]={
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
    {"fold-record",1,fold_record}
};
