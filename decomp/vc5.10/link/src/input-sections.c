#include <windows.h>
#include <string.h>
#include <stdio.h>
#include <mbstring.h>
#include "input.h"
#include "memory.h"
#include "sections.h"
#include "full-link-functions.h"
#include <recon.h>

extern char *current_module_strings;
extern BYTE file_access_statistics;
extern BYTE mac_target,incremental_enabled,incremental_reuse,pdb_enabled,mac_file_padding;
extern OutputSection *import_section,*import_thunk_section,*import_descriptor_section;
extern OutputSection *architecture_section,*debug_section;
extern ComdatRoot *comdat_roots;
extern int selected_tool_kind;
typedef struct ModuleListNode { OutputModule *module; struct ModuleListNode *next; } ModuleListNode;
ModuleListNode *precompiled_modules;
void __stdcall add_module_to_list(ModuleListNode **,OutputModule *);
char *__stdcall get_input_section_name(IMAGE_SECTION_HEADER *,const char *);
DWORD __stdcall align_section_size(DWORD,DWORD);
void __stdcall remember_mac_module_name(const char *);
void __stdcall mark_incremental_contribution(Contribution *);
BYTE __stdcall read_comdat_section(LinkContext *,Contribution *,IMAGE_SYMBOL *,DWORD,char **);
void __stdcall read_section_directives(LinkContext *,Contribution *,const char *,InputArguments *);
void *__stdcall add_comdat_reference(StringNode *,Contribution *,Contribution *);
void __stdcall read_section_relocations(LinkContext *,Contribution *,IMAGE_SYMBOL *,OutputModule *);
DWORD __stdcall mac_contribution_padding(DWORD);

#pragma intrinsic(strcmp,strlen)
void __stdcall add_input_section(LinkContext *context,OutputModule *module,DWORD index,
    IMAGE_SECTION_HEADER *header,IMAGE_SYMBOL *symbols,InputArguments *pending)
{
    char *name=get_input_section_name(header,current_module_strings);
    char module_name[512];
    DWORD flags,alignment,padding;
    Contribution *contribution;
    ContributionGroup *group;
    OutputSection *section;
    char *comdat_name=NULL;
    if ((context->file_header.Machine==0x166 || context->file_header.Machine==0x168) &&
        !(header->Characteristics & 0x02000000) && strncmp(name,".idata",6) &&
        (header->Characteristics & 0x1e0)!=0x100 &&
        align_section_size(1,header->Characteristics)<4) {
        format_module_name(module,module_name);
        if (incremental_enabled && !incremental_reuse)
            header->Characteristics &= 0xff8ffff7;
        else header->Characteristics &= 0xff8fffff;
        header->Characteristics |= 0x300000;
    }
    flags=header->Characteristics;
    if (mac_target && !strcmp(name,".ppcshl")) remember_mac_module_name(module->name);
    contribution=create_contribution(name,header->SizeOfRawData,flags,flags,module,
        &context->sections,context);
    group=contribution->group;
    if (!(context->options.debug_level==3 && (context->options.debug_formats & 2)) &&
        (group==debug_symbol_group || group==debug_type_group || group==debug_precompiled_group))
        contribution->characteristics |= 0x800;
    if (!(context->options.debug_level && (context->options.debug_formats & 4)) &&
        group==debug_fixup_group) contribution->characteristics |= 0x800;
    mark_incremental_contribution(contribution);
    if (pdb_enabled && contribution->group==debug_precompiled_group)
        add_module_to_list(&precompiled_modules,module);
    if ((contribution->characteristics & 0x1000) &&
        !read_comdat_section(context,contribution,symbols,index,&comdat_name)) {
        contribution->characteristics |= 0x800;
        return;
    }
    read_section_directives(context,contribution,name,pending);
    if (selected_tool_kind==2 || (contribution->characteristics & 0x800)) return;
    group=contribution->group;
    section=group->section;
    if (section==import_section) {
        flags=(flags & 0xffbfffff)|0x300000;
        contribution->characteristics=flags;
    }
    alignment=align_section_size(1,flags);
    if (group->alignment<(BYTE)alignment) {
        group->alignment=(BYTE)alignment;
        if (!mac_target && context->optional_header.SectionAlignment<alignment)
            link_contribution_fatal(contribution,1164,(int)(SHORT)index,alignment);
    }
    if (context->options.effective_reference_opt || context->options.fold_identical_comdats) {
        attach_comdat(contribution,comdat_name,0);
        if (!(contribution->characteristics & 0x1000) ||
            (section==architecture_section && !_strnicmp(name,".arch$a",7)))
            add_comdat_root(NULL,NULL,contribution,&comdat_roots);
        else if (contribution->associated_contribution)
            add_comdat_reference(NULL,contribution->associated_contribution,contribution);
    }
    section=contribution->group->section;
    if (incremental_enabled && !incremental_reuse && section!=debug_section &&
        section!=import_section && (module->library->flags & 16) &&
        strcmp(section->name,".idata")) {
        if (contribution->characteristics & 0x20) {
            padding=(DWORD)(contribution->raw_size*25)/100;
            if (padding==1) padding=0;
        } else padding=(DWORD)(contribution->raw_size*(section==small_data_section ? 10 : 20))/100;
        contribution->padding=padding;
        contribution->raw_size+=padding;
    }
    section=contribution->group->section;
    if (section!=debug_section) read_section_relocations(context,contribution,symbols,module);
    section=contribution->group->section;
    if (mac_target && mac_file_padding && section!=debug_section && section!=import_section &&
        (contribution->module->library->flags & 16)) {
        padding=mac_contribution_padding(contribution->raw_size);
        contribution->padding=padding;
        contribution->raw_size+=padding;
    }
}
/* Shared mutable name storage; byte8 remains the original zero terminator. */
char input_section_name[9];
char *__stdcall get_input_section_name(IMAGE_SECTION_HEADER *header,const char *strings)
{
    LONG offset;
    strncpy(input_section_name,(const char *)header->Name,8);
    if (input_section_name[0]=='/' && sscanf(input_section_name+1,"%ld",&offset)==1)
        return (char *)strings+offset;
    return input_section_name;
}
DWORD __stdcall align_section_size(DWORD size,DWORD flags)
{
    DWORD alignment,mask;
    if (flags & 8) return size; /* IMAGE_SCN_TYPE_NO_PAD */
    alignment=flags & 0x700000;
    if (!alignment || alignment==0x500000) mask=15;
    else if (alignment==0x200000) mask=1;
    else if (alignment==0x300000) mask=3;
    else if (alignment==0x400000) mask=7;
    else if (alignment==0x600000) mask=31;
    else if (alignment==0x700000) mask=63;
    else return size;
    if (size & mask) size=(size & ~mask)+mask+1;
    return size;
}
void __stdcall remember_mac_module_name(const char *name) { __asm { int 3 } }
extern WORD warning_level;
extern BYTE arch_data_ignored;
void __stdcall mark_incremental_contribution(Contribution *contribution)
{
    if (incremental_enabled && contribution->group->section==architecture_section) {
        if (warning_level>1)
            arch_data_ignored |= (contribution->raw_size-contribution->padding)!=0;
        contribution->characteristics |= 0x800;
    }
}
void __stdcall collect_module_directives(LinkContext *,Contribution *,InputArguments *,char *);
void __stdcall process_module_directives(LinkContext *,Contribution *,char *);
void __stdcall read_section_directives(LinkContext *context,Contribution *contribution,
    const char *name,InputArguments *pending)
{
    OutputModule *module;
    DWORD offset,error=0,index;
    FileRecord *file;
    char *directives;
    if (!(contribution->characteristics & 0x200) || context->ignore_directives ||
        strcmp(name,".drectve")) return;
    directives=(char *)allocate_memory(contribution->raw_size+1);
    module=contribution->module;
    if (module->library->flags & 16) offset=0;
    else if (module->flags & 0x40) offset=((DWORD *)module->optional_data)[3];
    else offset=(DWORD)module->identity;
    seek_file(current_input_file,offset+contribution->input_raw_offset,FILE_BEGIN,&error);
    if (file_access_statistics) record_file_access();
    file=file_slots[current_input_file];
    if (file->flags & 8) mapped_read(file,directives,contribution->raw_size);
    else buffered_read(file,directives,contribution->raw_size);
    for (index=0;index<contribution->raw_size;++index)
        if (!directives[index]) directives[index]=' ';
    directives[contribution->raw_size]=0;
    if (incremental_reuse) collect_module_directives(context,contribution,pending,directives);
    else process_module_directives(context,contribution,directives);
    release_memory(directives);
}
void *__stdcall add_comdat_reference(StringNode *symbol,Contribution *parent,Contribution *child) { __asm { int 3 } return NULL; }
DWORD __stdcall mac_contribution_padding(DWORD size) { __asm { int 3 } return 0; }
void __stdcall add_module_to_list(ModuleListNode **list,OutputModule *module) { __asm { int 3 } }
char *__stdcall next_directive_token(char *,BYTE *);
void __stdcall remember_module_directive(const char *,StringTable *,OutputModule *);
void __stdcall collect_module_directives(LinkContext *context,Contribution *contribution,
    InputArguments *pending,char *directives)
{
    OutputModule *module=contribution->module;
    char *token=next_directive_token(directives,NULL);
    while (token) {
        if (*token=='/' || *token=='-') {
            if (token[1]=='?') ++token;
            token=duplicate_argument(token+1);
            if (incremental_enabled && !incremental_reuse && (module->library->flags & 16))
                remember_module_directive(token,context->strings,module);
            append_argument(pending,token);
        }
        token=next_directive_token(NULL,NULL);
    }
}
char *directive_cursor;
char *__stdcall next_directive_token(char *text,BYTE *quoted)
{
    char *start,*end;
    if (quoted) *quoted=0;
    if (text) directive_cursor=text;
    while (*directive_cursor && _ismbcspace((unsigned char)*directive_cursor)) ++directive_cursor;
    if (!*directive_cursor) {
        directive_cursor=NULL;
        return NULL;
    }
    start=directive_cursor;
    while (*directive_cursor && !_ismbcspace((unsigned char)*directive_cursor)) {
        if (*directive_cursor=='"') {
            memmove(directive_cursor,directive_cursor+1,strlen(directive_cursor+1)+1);
            end=(char *)_mbschr((unsigned char *)directive_cursor,'"');
            if (end) {
                memmove(end,end+1,strlen(end+1)+1);
                directive_cursor=end;
                if (quoted) *quoted=1;
            }
        } else ++directive_cursor;
    }
    if (*directive_cursor) *directive_cursor++=0;
    if (!*start) return next_directive_token(NULL,quoted);
    return start;
}
void __stdcall remember_module_directive(const char *text,StringTable *strings,OutputModule *module) { __asm { int 3 } }
char *__stdcall format_contribution_module(Contribution *,char *);
void __stdcall process_module_directives(LinkContext *context,Contribution *contribution,
    char *directives)
{
    char module_name[512];
    const char *name;
    option_arguments.first=NULL;
    option_arguments.last=NULL;
    option_arguments.count=0;
    collect_module_directives(context,contribution,&option_arguments,directives);
    name=format_contribution_module(contribution,module_name);
    /* The dispatcher's historical interface view consumes owner at offset18h,
       the Contribution.module field. It does not describe a second object. */
    configure_link_options(context,(DirectiveModule *)contribution,name);
}
extern OutputModule *output_module;
char *__stdcall format_contribution_module(Contribution *contribution,char *name)
{
    if (contribution->module==output_module) return NULL;
    return format_module_name(contribution->module,name);
}
