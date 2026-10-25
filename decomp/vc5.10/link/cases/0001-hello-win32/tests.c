/* Narrow live-state check for the real hello invocation, before object loading. */
#include <recon-tool.h>
#include <recon-native.h>
#include <string.h>
#include "../../src/input.h"
#include "../../src/memory.h"
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
        module->saved_section_headers && section_header_arena &&
        !section_header_arena->previous && section_header_arena->used==280 &&
        section_header_arena->remaining==4096-280 &&
        section_header_arena->base==(BYTE *)module->saved_section_headers;
    if (!pass) recon_native_entry_finish(0);
    bytes=(BYTE *)module->saved_section_headers;
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
        module->saved_section_headers && !module->section_headers &&
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
    {"collected-directives",1,collected_directives}
};
