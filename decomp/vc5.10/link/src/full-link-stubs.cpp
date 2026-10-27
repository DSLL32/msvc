/* Named original boundaries. Each body stays an INT3 until reconstructed. */
#include "full-link-functions.h"
#include <recon.h>
extern "C" void __cdecl finish_alpha_branches(void) { __asm { int 3 } }
extern "C" void __cdecl record_file_access(void) { __asm { int 3 } }
extern "C" void __stdcall move_contribution_to_end(Contribution *contribution) { __asm { int 3 } }
extern "C" void __stdcall initialize_pdb(BYTE mppc32_types,const char *name) { __asm { int 3 } }
extern "C" void __cdecl close_pdb(void) { __asm { int 3 } }
extern "C" void __cdecl commit_pdb(void) { __asm { int 3 } }
extern "C" void __stdcall initialize_pdb_image(const char *image) { __asm { int 3 } }
extern "C" char * __stdcall derive_pdb_name(const char *image,const char *specified) { __asm { int 3 } return 0; }
extern "C" void __cdecl link_contribution_fatal(Contribution *contribution,unsigned message,...) { __asm { int 3 } }
extern "C" void __stdcall allocate_pdb_module_table(DWORD count) { __asm { int 3 } }
extern "C" void __stdcall adjust_incremental_symbols(LinkContext *context) { __asm { int 3 } }
Contribution * RECON_THISCALL LinkContext::CreateIncrementalContribution(void) { __asm { int 3 } return 0; }
extern "C" void __stdcall write_mac_incremental_table(LinkContext *context,DWORD offset) { __asm { int 3 } }
extern "C" void __stdcall allocate_mac_incremental_table(DWORD count,BYTE fixed) { __asm { int 3 } }
void RECON_THISCALL LinkContext::PrepareMacDebugSections(void) { __asm { int 3 } }
void RECON_THISCALL LinkContext::PrepareMacRuntimeSections(void) { __asm { int 3 } }
void RECON_THISCALL LinkContext::PrepareNativeRelocations(void) { __asm { int 3 } }
extern "C" DWORD __stdcall mips_code_padding(Contribution *contribution) { __asm { int 3 } return 0; }
extern "C" void __stdcall add_alpha_branch_island(Contribution *last,DWORD rva,DWORD count) { __asm { int 3 } }
extern "C" void __stdcall add_ppc_branch_island(Contribution *last,DWORD rva,DWORD count) { __asm { int 3 } }
extern "C" void __stdcall initialize_relocation_padding(void *state,DWORD pages,DWORD base,DWORD extra) { __asm { int 3 } }

extern "C" void __cdecl update_gp_bounds(void) { __asm { int 3 } }

void RECON_THISCALL LinkContext::CollectRelocationSymbols(DWORD *first_count,void **first,DWORD *second_count,void **second) { __asm { int 3 } }
void RECON_THISCALL LinkContext::WriteRelocationSymbols(DWORD first_count,void **first,DWORD second_count,void **second) { __asm { int 3 } }
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
extern "C" void __cdecl finish_ppc_branches(void) { __asm { int 3 } }
extern "C" char * __stdcall replace_extension(const char *file,const char *extension) { __asm { int 3 } return 0; }
