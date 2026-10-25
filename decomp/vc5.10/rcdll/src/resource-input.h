#ifndef RCDLL_RESOURCE_INPUT_H
#define RCDLL_RESOURCE_INPUT_H
#include <stdio.h>
#include <stddef.h>
#include "rcdll.h"
#include <recon.h>
extern DWORD rc_resource_status,rc_resource_file_index,rc_resource_line;
extern FILE *rc_resource_input_file;
extern WCHAR rc_resource_character;
unsigned __cdecl rc_next_resource_character(void);
unsigned __cdecl rc_read_resource_character(void);
unsigned __cdecl rc_read_resource_code_unit(void);
typedef struct RcResourceToken {
    DWORD unknown0,line,file_index,unknown12;
    WORD value;
    BYTE kind,unknown19;
    WCHAR text[248],filename[260];
    WORD unknown1036,unknown1038;
} RcResourceToken;
typedef char RcResourceTokenLayout[
    sizeof(RcResourceToken)==1040 && offsetof(RcResourceToken,value)==16 &&
    offsetof(RcResourceToken,text)==20 && offsetof(RcResourceToken,filename)==516 &&
    offsetof(RcResourceToken,unknown1036)==1036 ? 1 : -1];
extern RcResourceToken rc_resource_token;
void __stdcall rc_copy_resource_token(RcResourceToken *,const RcResourceToken *);
unsigned __stdcall rc_scan_resource_token(unsigned);
void __stdcall rc_resource_input_fatal(unsigned);
void __stdcall rc_read_apstudio_directive(int,DWORD);
void *__stdcall rc_reserve_resource_bytes(WORD);
#endif
