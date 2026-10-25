#ifndef LINK_FULL_LINK_FUNCTIONS_H
#define LINK_FULL_LINK_FUNCTIONS_H
#include <windows.h>
#include "link-driver.h"
#include "files.h"
#include "options.h"
#include <recon.h>
#ifdef __cplusplus
extern "C" {
#endif
void __cdecl finish_alpha_branches(void);
unsigned __stdcall buffered_read(FileRecord *file,void *buffer,unsigned bytes);
unsigned __stdcall buffered_write(FileRecord *file,const void *buffer,unsigned bytes);
unsigned __stdcall mapped_read(FileRecord *file,void *buffer,unsigned bytes);
unsigned __stdcall mapped_write(FileRecord *file,const void *buffer,unsigned bytes);
void __cdecl record_file_access(void);
void * __stdcall map_file_range(int file,long offset,unsigned bytes);
void __cdecl initialize_contribution_pools(void);
void __stdcall initialize_output_modules(OutputModule **module);
char *__stdcall duplicate_link_string(const char *text);
ContributionGroup * __stdcall find_contribution_group(OutputSection *section,const char *name);
Contribution * __stdcall create_contribution(const char *name,DWORD bytes,DWORD flags,DWORD section_flags,OutputModule *module,SectionList *sections,LinkContext *context);
OutputSection * __stdcall find_output_section(const char *name,SectionList *sections);
void __stdcall merge_section_groups(OutputSection *source,OutputSection *destination);
void __stdcall merge_output_section(OutputSection *source,OutputSection *destination);
void __stdcall move_contribution_to_end(Contribution *contribution);
void __stdcall move_group_to_end(ContributionGroup *group);
void __stdcall initialize_section_enumerator(SectionEnumerator *enumerator,SectionList *sections);
BYTE __stdcall next_output_section(SectionEnumerator *enumerator);
void __stdcall end_section_enumerator(SectionEnumerator *enumerator);
void __stdcall initialize_pdb(BYTE mppc32_types,const char *name);
void __cdecl close_pdb(void);
void __cdecl commit_pdb(void);
void __stdcall initialize_pdb_image(const char *image);
char * __stdcall derive_pdb_name(const char *image,const char *specified);
void __cdecl prepare_delay_imports(void);
void __cdecl link_contribution_fatal(Contribution *contribution,unsigned message,...);
unsigned __stdcall count_hash_symbols(HashState *table);
void __stdcall allocate_pdb_module_table(DWORD count);
char * __stdcall derive_old_image_name(LinkContext *context);
BYTE __stdcall read_old_image(const char *name,BYTE quiet,LinkContext *image,void *information);
void __stdcall adjust_incremental_symbols(LinkContext *context);
void __stdcall write_mac_incremental_table(LinkContext *context,DWORD offset);
void __stdcall allocate_mac_incremental_table(DWORD count,BYTE fixed);
DWORD __stdcall section_data_size(OutputSection *section);
unsigned __stdcall count_debug_modules(void *libraries);
void __stdcall finish_image_layout(LinkContext *context,void *padding);
void __cdecl update_gp_bounds(void);
void __stdcall pad_debug_contribution(Contribution *contribution);
DWORD __stdcall align_image_base(DWORD base);
void __cdecl prepare_import_sections(void);
BYTE __stdcall emit_symbol_class(BYTE symbol_class,LinkOptions *options);
void __stdcall write_coff_symbols(LinkContext *context,DWORD image_rva);
void __stdcall set_mac_entry_point(StringNode *entry,LinkContext *context);
void __cdecl prepare_mac_resource_map(void);
void __stdcall prepare_mac_data_headers(LinkContext *context);
void __stdcall finish_mac_entry_point(StringNode *entry,LinkContext *context);
void __stdcall set_mac_section_addresses(SectionList *sections);
void __stdcall finish_mac_headers(LinkContext *context);
void __cdecl set_mac_symbol_addresses(void);
void __stdcall prepare_mac_debug_information(LinkContext *context);
void __cdecl finish_ppc_branches(void);
void __stdcall clear_argument_list(InputArguments *arguments);
void __stdcall apply_section_merge(OutputSection *section);
char * __stdcall replace_extension(const char *file,const char *extension);
void __cdecl finish_symbol_tables(void);
const char * __stdcall get_symbol_name(StringNode *symbol,const void *strings,WORD *kind,BYTE *is_import);
void __stdcall set_symbol_defined(StringNode *symbol,BYTE defined,StringTable *symbols);
void __cdecl initialize_comdats(void);
void __cdecl release_comdat_tables(void);
void __stdcall attach_comdat(Contribution *contribution,const char *name,BYTE linker_defined);
void __stdcall add_comdat_root(Contribution *contribution,StringNode *symbol,Contribution *owner,ComdatRoot **roots);
void __stdcall add_special_comdat_roots(StringTable *symbols);
void __stdcall eliminate_unused_comdats(ComdatRoot *roots,StringTable *symbols);
void __cdecl finish_comdat_elimination(void);
void __cdecl print_removed_comdats(void);
void __stdcall clear_buffer(ByteBuffer *);
int __stdcall open_argument_file(const char *,int,int);
void __stdcall close_argument_file(int,int);
#ifdef __cplusplus
}
#endif
#endif
