/* Named original boundaries. Each body stays an INT3 until reconstructed. */
#include "full-link-functions.h"
#include <recon.h>
extern "C" void __cdecl record_file_access(void) { __asm { int 3 } }

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


extern "C" void __stdcall add_ppc_branch_island(Contribution *last,DWORD rva,DWORD count) { __asm { int 3 } }
extern "C" void __stdcall initialize_relocation_padding(void *state,DWORD pages,DWORD base,DWORD extra) { __asm { int 3 } }











extern "C" void __cdecl finish_ppc_branches(void) { __asm { int 3 } }
extern "C" char * __stdcall replace_extension(const char *file,const char *extension) { __asm { int 3 } return 0; }
