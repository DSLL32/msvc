/* Named original boundaries. Each body stays an INT3 until reconstructed. */
#include "full-link-functions.h"
#include <recon.h>
DWORD RECON_THISCALL LinkContext::RangeCodeBytes(DWORD start,DWORD limit,Contribution **last) { __asm { int 3 } return 0; }
extern "C" void __cdecl finish_alpha_branches(void) { __asm { int 3 } }
extern "C" LONG __stdcall seek_file(int file,long offset,int origin,DWORD *upper) { __asm { int 3 } return 0; }
extern "C" unsigned __stdcall buffered_read(FileRecord *file,void *buffer,unsigned bytes) { __asm { int 3 } return 0; }
extern "C" unsigned __stdcall buffered_write(FileRecord *file,const void *buffer,unsigned bytes) { __asm { int 3 } return 0; }
extern "C" unsigned __stdcall mapped_write(FileRecord *file,const void *buffer,unsigned bytes) { __asm { int 3 } return 0; }
extern "C" void __cdecl record_file_access(void) { __asm { int 3 } }
extern "C" void * __stdcall map_file_range(int file,long offset,unsigned bytes) { __asm { int 3 } return 0; }
extern "C" Contribution * __stdcall create_contribution(const char *name,DWORD bytes,DWORD flags,DWORD section_flags,OutputModule *module,SectionList *sections,LinkContext *context) { __asm { int 3 } return 0; }
extern "C" OutputSection * __stdcall find_output_section(const char *name,SectionList *sections) { __asm { int 3 } return 0; }
extern "C" void __stdcall merge_section_groups(OutputSection *source,OutputSection *destination) { __asm { int 3 } }
extern "C" void __stdcall merge_output_section(OutputSection *source,OutputSection *destination) { __asm { int 3 } }
extern "C" void __stdcall move_contribution_to_end(Contribution *contribution) { __asm { int 3 } }
extern "C" void __stdcall move_group_to_end(ContributionGroup *group) { __asm { int 3 } }
extern "C" void __stdcall initialize_pdb(BYTE mppc32_types,const char *name) { __asm { int 3 } }
extern "C" void __cdecl close_pdb(void) { __asm { int 3 } }
extern "C" void __cdecl commit_pdb(void) { __asm { int 3 } }
extern "C" void __stdcall initialize_pdb_image(const char *image) { __asm { int 3 } }
extern "C" char * __stdcall derive_pdb_name(const char *image,const char *specified) { __asm { int 3 } return 0; }
extern "C" void __cdecl prepare_delay_imports(void) { __asm { int 3 } }
extern "C" void __cdecl link_contribution_fatal(Contribution *contribution,unsigned message,...) { __asm { int 3 } }
extern "C" unsigned __stdcall count_hash_symbols(HashState *table) { __asm { int 3 } return 0; }
void RECON_THISCALL LinkContext::FoldIdenticalComdats(void) { __asm { int 3 } }
void RECON_THISCALL LinkContext::PrintFoldedComdats(void) { __asm { int 3 } }
extern "C" void __stdcall allocate_pdb_module_table(DWORD count) { __asm { int 3 } }
extern "C" char * __stdcall derive_old_image_name(LinkContext *context) { __asm { int 3 } return 0; }
extern "C" BYTE __stdcall read_old_image(const char *name,BYTE quiet,LinkContext *image,void *information) { __asm { int 3 } return 0; }
extern "C" void __stdcall adjust_incremental_symbols(LinkContext *context) { __asm { int 3 } }
Contribution * RECON_THISCALL LinkContext::CreateIncrementalContribution(void) { __asm { int 3 } return 0; }
extern "C" void __stdcall write_mac_incremental_table(LinkContext *context,DWORD offset) { __asm { int 3 } }
extern "C" void __stdcall allocate_mac_incremental_table(DWORD count,BYTE fixed) { __asm { int 3 } }
DWORD RECON_THISCALL LinkContext::BuildDosExtension(void **buffer) { __asm { int 3 } return 0; }
void RECON_THISCALL LinkContext::ApplyMerges(void) { __asm { int 3 } }
void RECON_THISCALL LinkContext::PrepareMacDebugSections(void) { __asm { int 3 } }
void RECON_THISCALL LinkContext::PrepareMacRuntimeSections(void) { __asm { int 3 } }
void RECON_THISCALL LinkContext::PrepareNativeRelocations(void) { __asm { int 3 } }
BYTE RECON_THISCALL LinkContext::CanMergeReadOnlyData(void) { __asm { int 3 } return 0; }
void RECON_THISCALL LinkContext::MergeExceptionSections(void) { __asm { int 3 } }
extern "C" DWORD __stdcall section_data_size(OutputSection *section) { __asm { int 3 } return 0; }
void RECON_THISCALL LinkContext::ResolveEntryPoint(void) { __asm { int 3 } }
extern "C" unsigned __stdcall count_debug_modules(void *libraries) { __asm { int 3 } return 0; }
WORD RECON_THISCALL LinkContext::CountOutputSections(void) { __asm { int 3 } return 0; }
extern "C" void __stdcall finish_image_layout(LinkContext *context,void *padding) { __asm { int 3 } }
void RECON_THISCALL LinkContext::PrepareSectionLayout(void) { __asm { int 3 } }
void RECON_THISCALL LinkContext::LayoutSections(DWORD *rva,DWORD *file_offset,DWORD *line_count) { __asm { int 3 } }
extern "C" void __cdecl update_gp_bounds(void) { __asm { int 3 } }
void RECON_THISCALL LinkContext::WriteSections(DWORD *line_offset) { __asm { int 3 } }
extern "C" void __stdcall pad_debug_contribution(Contribution *contribution) { __asm { int 3 } }
extern "C" DWORD __stdcall align_image_base(DWORD base) { __asm { int 3 } return 0; }
void RECON_THISCALL LinkContext::ApplySectionAttributes(void) { __asm { int 3 } }
extern "C" void __cdecl prepare_import_sections(void) { __asm { int 3 } }
void RECON_THISCALL LinkContext::ResolveLocalImports(Contribution **contribution,IncludeSymbol **aliases) { __asm { int 3 } }
void RECON_THISCALL LinkContext::ResolveUndefined(void) { __asm { int 3 } }
void RECON_THISCALL LinkContext::CollectRelocationSymbols(DWORD *first_count,void **first,DWORD *second_count,void **second) { __asm { int 3 } }
void RECON_THISCALL LinkContext::FinalizeOrder(void) { __asm { int 3 } }
void RECON_THISCALL LinkContext::FinishNativeSections(void) { __asm { int 3 } }
extern "C" BYTE __stdcall emit_symbol_class(BYTE symbol_class,LinkOptions *options) { __asm { int 3 } return 0; }
void RECON_THISCALL LinkContext::WriteRelocationSymbols(DWORD first_count,void **first,DWORD second_count,void **second) { __asm { int 3 } }
void RECON_THISCALL LinkContext::WriteLocalImports(Contribution *contribution,IncludeSymbol **aliases) { __asm { int 3 } }
extern "C" void __stdcall write_coff_symbols(LinkContext *context,DWORD image_rva) { __asm { int 3 } }
void RECON_THISCALL LinkContext::EmitDebugDirectories(DWORD start,DWORD original_offset,DWORD lines,DWORD end,DWORD line_offset) { __asm { int 3 } }
void RECON_THISCALL LinkContext::FinishImage(void) { __asm { int 3 } }
void RECON_THISCALL LinkContext::PassTwo(void) { __asm { int 3 } }
void RECON_THISCALL LinkContext::PrepareMacJumpTables(void) { __asm { int 3 } }
void RECON_THISCALL LinkContext::ResolveMacImports(void) { __asm { int 3 } }
void RECON_THISCALL LinkContext::ResolveMacExports(void) { __asm { int 3 } }
void RECON_THISCALL LinkContext::PrepareMacResources(void) { __asm { int 3 } }
void RECON_THISCALL LinkContext::PrepareMacSections(void) { __asm { int 3 } }
extern "C" void __stdcall set_mac_entry_point(StringNode *entry,LinkContext *context) { __asm { int 3 } }
extern "C" void __cdecl prepare_mac_resource_map(void) { __asm { int 3 } }
extern "C" void __stdcall prepare_mac_data_headers(LinkContext *context) { __asm { int 3 } }
extern "C" void __stdcall finish_mac_entry_point(StringNode *entry,LinkContext *context) { __asm { int 3 } }
extern "C" void __stdcall set_mac_section_addresses(SectionList *sections) { __asm { int 3 } }
void RECON_THISCALL LinkContext::PrepareMacSymbols(void) { __asm { int 3 } }
extern "C" void __stdcall finish_mac_headers(LinkContext *context) { __asm { int 3 } }
extern "C" void __cdecl set_mac_symbol_addresses(void) { __asm { int 3 } }
extern "C" void __stdcall prepare_mac_debug_information(LinkContext *context) { __asm { int 3 } }
void RECON_THISCALL LinkContext::PrepareAuxiliarySections(void) { __asm { int 3 } }
void RECON_THISCALL LinkAuxiliaryData::PrepareOutput(void) { __asm { int 3 } }
void RECON_THISCALL LinkAuxiliaryData::FinishOutput(LinkContext *context) { __asm { int 3 } }
extern "C" void __cdecl finish_ppc_branches(void) { __asm { int 3 } }
extern "C" void __stdcall clear_argument_list(InputArguments *arguments) { __asm { int 3 } }
extern "C" void __stdcall apply_section_merge(OutputSection *section) { __asm { int 3 } }
void RECON_THISCALL StringTable::SortSymbols(void) { __asm { int 3 } }
extern "C" char * __stdcall replace_extension(const char *file,const char *extension) { __asm { int 3 } return 0; }
void RECON_THISCALL LinkContext::UpdateChecksum(void) { __asm { int 3 } }
extern "C" void __cdecl finish_symbol_tables(void) { __asm { int 3 } }
extern "C" const char * __stdcall get_symbol_name(StringNode *symbol,const void *strings,WORD *kind,BYTE *is_import) { __asm { int 3 } return 0; }
StringNode * RECON_THISCALL StringTable::LookupSymbol(WORD kind,const char *name,BYTE *created) { __asm { int 3 } return 0; }
StringNode * RECON_THISCALL StringTable::FindSymbol(const char *name) { __asm { int 3 } return 0; }
extern "C" void __stdcall set_symbol_defined(StringNode *symbol,BYTE defined,StringTable *symbols) { __asm { int 3 } }
extern "C" void __cdecl release_comdat_tables(void) { __asm { int 3 } }
extern "C" void __stdcall attach_comdat(Contribution *contribution,const char *name,BYTE linker_defined) { __asm { int 3 } }
extern "C" void __stdcall add_comdat_root(Contribution *contribution,StringNode *symbol,OutputModule *module,ComdatRoot **roots) { __asm { int 3 } }
extern "C" void __stdcall add_special_comdat_roots(StringTable *symbols) { __asm { int 3 } }
extern "C" void __stdcall eliminate_unused_comdats(ComdatRoot *roots,StringTable *symbols) { __asm { int 3 } }
extern "C" void __cdecl finish_comdat_elimination(void) { __asm { int 3 } }
extern "C" void __cdecl print_removed_comdats(void) { __asm { int 3 } }
void RECON_THISCALL LinkContext::WriteVxdModuleTable(int file) { __asm { int 3 } }
void RECON_THISCALL LinkContext::WriteVxdNames(int file) { __asm { int 3 } }
