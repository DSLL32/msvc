#include "omf.h"
#include <recon.h>

void __cdecl omf_error(const char * a0,...) { __asm { int 3 } }
void __stdcall read_omf_extdefs(DWORD a0,DWORD a1) { __asm { int 3 } }
void __cdecl read_omf_lidata(void) { __asm { int 3 } }
void __stdcall read_omf_comdefs(DWORD a0) { __asm { int 3 } }
void __stdcall read_omf_lpubdefs(DWORD a0) { __asm { int 3 } }
void __cdecl read_omf_linnum(void) { __asm { int 3 } }
void __cdecl read_omf_lheadr(void) { __asm { int 3 } }
void __cdecl read_omf_comdat(void) { __asm { int 3 } }
void __cdecl read_omf_alias(void) { __asm { int 3 } }
void __cdecl read_omf_bakpat(void) { __asm { int 3 } }
void __cdecl read_omf_linsym(void) { __asm { int 3 } }




void __stdcall remember_omf_target_offset(DWORD symbol,DWORD offset) { __asm { int 3 } }
DWORD __stdcall lookup_omf_target_offset(DWORD symbol,DWORD offset) { __asm { int 3 } return 0; }

void __stdcall write_omf_indirect_section_data(DWORD section,DWORD offset,char *const *sources,DWORD count) { __asm { int 3 } }

void __stdcall append_omf_function_auxiliary(DWORD count,DWORD section,DWORD start,DWORD firstline,DWORD length,DWORD lastline) { __asm { int 3 } }

void __stdcall append_omf_line(DWORD section,DWORD offset,DWORD line) { __asm { int 3 } }
