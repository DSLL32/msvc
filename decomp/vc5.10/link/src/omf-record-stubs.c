#include "omf.h"
#include <recon.h>

void __cdecl omf_error(const char * a0,...) { __asm { int 3 } }
void __cdecl read_omf_lidata(void) { __asm { int 3 } }

void __cdecl read_omf_lheadr(void) { __asm { int 3 } }
void __stdcall append_omf_section_data(DWORD section,DWORD offset,const void *bytes,DWORD count) { __asm { int 3 } }
void __cdecl read_omf_alias(void) { __asm { int 3 } }
void __cdecl read_omf_bakpat(void) { __asm { int 3 } }
void __cdecl read_omf_linsym(void) { __asm { int 3 } }




void __stdcall remember_omf_target_offset(DWORD symbol,DWORD offset) { __asm { int 3 } }
DWORD __stdcall lookup_omf_target_offset(DWORD symbol,DWORD offset) { __asm { int 3 } return 0; }

void __stdcall write_omf_indirect_section_data(DWORD section,DWORD offset,char *const *sources,DWORD count) { __asm { int 3 } }
