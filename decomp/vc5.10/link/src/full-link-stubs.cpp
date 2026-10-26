/* Named original boundaries. Each body stays an INT3 until reconstructed. */
#include "full-link-functions.h"
#include <recon.h>
DWORD RECON_THISCALL LinkContext::RangeCodeBytes(DWORD start,DWORD limit,Contribution **last) { __asm { int 3 } return 0; }
extern "C" void __cdecl finish_alpha_branches(void) { __asm { int 3 } }
extern "C" unsigned __stdcall buffered_read(FileRecord *file,void *buffer,unsigned bytes) { __asm { int 3 } return 0; }
extern "C" unsigned __stdcall buffered_write(FileRecord *file,const void *buffer,unsigned bytes) { __asm { int 3 } return 0; }
extern "C" void __cdecl record_file_access(void) { __asm { int 3 } }
extern "C" void __stdcall move_contribution_to_end(Contribution *contribution) { __asm { int 3 } }
extern "C" void __stdcall initialize_pdb(BYTE mppc32_types,const char *name) { __asm { int 3 } }
extern "C" void __cdecl close_pdb(void) { __asm { int 3 } }
extern "C" void __cdecl commit_pdb(void) { __asm { int 3 } }
extern "C" void __stdcall initialize_pdb_image(const char *image) { __asm { int 3 } }
extern "C" char * __stdcall derive_pdb_name(const char *image,const char *specified) { __asm { int 3 } return 0; }
extern "C" void __cdecl link_contribution_fatal(Contribution *contribution,unsigned message,...) { __asm { int 3 } }
void RECON_THISCALL LinkContext::PrintFoldedComdats(void) { __asm { int 3 } }
extern "C" void __stdcall allocate_pdb_module_table(DWORD count) { __asm { int 3 } }
extern "C" void __stdcall adjust_incremental_symbols(LinkContext *context) { __asm { int 3 } }
Contribution * RECON_THISCALL LinkContext::CreateIncrementalContribution(void) { __asm { int 3 } return 0; }
extern "C" void __stdcall write_mac_incremental_table(LinkContext *context,DWORD offset) { __asm { int 3 } }
extern "C" void __stdcall allocate_mac_incremental_table(DWORD count,BYTE fixed) { __asm { int 3 } }
void RECON_THISCALL LinkContext::PrepareMacDebugSections(void) { __asm { int 3 } }
void RECON_THISCALL LinkContext::PrepareMacRuntimeSections(void) { __asm { int 3 } }
void RECON_THISCALL LinkContext::PrepareNativeRelocations(void) { __asm { int 3 } }
extern "C" DWORD __stdcall section_data_size(OutputSection *section) { __asm { int 3 } return 0; }
extern "C" unsigned __stdcall count_debug_modules(void *libraries) { __asm { int 3 } return 0; }
extern "C" DWORD __stdcall mips_code_padding(Contribution *contribution) { __asm { int 3 } return 0; }
extern "C" void __stdcall add_alpha_branch_island(Contribution *last,DWORD rva,DWORD count) { __asm { int 3 } }
extern "C" void __stdcall add_ppc_branch_island(Contribution *last,DWORD rva,DWORD count) { __asm { int 3 } }
extern "C" void __stdcall initialize_relocation_padding(void *state,DWORD pages,DWORD base,DWORD extra) { __asm { int 3 } }

extern "C" void __cdecl update_gp_bounds(void) { __asm { int 3 } }
extern "C" void __stdcall adjust_rom_section_header(OutputSection *section,IMAGE_SECTION_HEADER *header,LinkContext *context) { __asm { int 3 } }
extern "C" void __stdcall pad_debug_contribution(Contribution *contribution) { __asm { int 3 } }
void RECON_THISCALL LinkContext::CollectRelocationSymbols(DWORD *first_count,void **first,DWORD *second_count,void **second) { __asm { int 3 } }
void RECON_THISCALL LinkContext::WriteRelocationSymbols(DWORD first_count,void **first,DWORD second_count,void **second) { __asm { int 3 } }
extern "C" void __stdcall write_coff_symbols(LinkContext *context,DWORD image_rva) { __asm { int 3 } }
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
void RECON_THISCALL LinkContext::ApplyOrderFile(void) { __asm { int 3 } }
extern "C" void __cdecl finish_ppc_branches(void) { __asm { int 3 } }
extern "C" void __stdcall apply_section_merge(OutputSection *section) { __asm { int 3 } }
extern "C" char * __stdcall replace_extension(const char *file,const char *extension) { __asm { int 3 } return 0; }
void RECON_THISCALL LinkContext::UpdateChecksum(void) { __asm { int 3 } }
extern "C" void __cdecl print_removed_comdats(void) { __asm { int 3 } }
void RECON_THISCALL LinkContext::WriteVxdModuleTable(int file) { __asm { int 3 } }
void RECON_THISCALL LinkContext::WriteVxdNames(int file) { __asm { int 3 } }
