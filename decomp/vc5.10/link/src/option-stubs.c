/* Numeric option conversion and pending helpers retain their own ABIs. */
#include <stdio.h>
#include "options.h"
#include <recon.h>
BYTE __stdcall switch_number(SwitchInfo *info,WORD index,DWORD *value)
{
    char trailing;
    /* The physical index slot is DWORD; the original zero-extends its WORD
     * before indexing. A second successful assignment rejects trailing text. */
    return sscanf(info->values[index].text,"%li%c",(LONG *)value,&trailing)==1;
}
void __stdcall add_delay_library(const char *name) { __asm { int 3 } }
void __stdcall remove_delay_library(const char *name) { __asm { int 3 } }
BYTE __stdcall has_delay_library(const char *name) { __asm { int 3 } return 0; }
void __cdecl enable_warning_errors(void) { __asm { int 3 } }
void __stdcall disable_warning(unsigned number) { __asm { int 3 } }
void __stdcall record_incremental_include(void *owner,StringNode *symbol) { __asm { int 3 } }
void __stdcall copy_repro_file(const char *name,BYTE copy_object) { __asm { int 3 } }
void __stdcall add_mac_data(const unsigned char *name,int enabled,DWORD id) { __asm { int 3 } }
void __stdcall set_mac_initializer(LinkContext *context,const char *name) { __asm { int 3 } }
void __stdcall set_mac_terminator(LinkContext *context,const char *name) { __asm { int 3 } }
BYTE __stdcall add_mac_import(const unsigned char *name,const unsigned char *library,DirectiveModule *module,LinkContext *context) { __asm { int 3 } return 0; }
void __stdcall set_mac_import_versions(const char *library,const char *current,const char *old_code,const char *old_api) { __asm { int 3 } }
extern BYTE order_file_enabled;
void __cdecl enable_order_file(void) { order_file_enabled=1; }
BYTE __stdcall find_argument(const char *name,InputArguments *list) { __asm { int 3 } return 0; }
