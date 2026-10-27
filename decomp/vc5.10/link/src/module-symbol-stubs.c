#include <windows.h>
#include "input.h"
#include <recon.h>

/* Named pending callees: declarations retain actual stack cleanup, no body credit. */
int __stdcall has_incremental_symbol(void *table,StringNode *symbol)
{ __asm { int 3 } return 0; }
int __stdcall update_incremental_symbol(const char *name,IMAGE_SYMBOL *raw,
    StringNode *symbol,BYTE created)
{ __asm { int 3 } return 0; }
void __stdcall process_mac_common(LinkContext *context,StringNode *symbol,
    IMAGE_SYMBOL *raw,const char *name,BYTE created,OutputModule *module)
{ __asm { int 3 } }
void __stdcall clear_incremental_undefined(StringNode *symbol,LinkContext *context)
{ __asm { int 3 } }
void __stdcall process_mac_undefined(LinkContext *context,StringNode *symbol,
    IMAGE_SYMBOL *raw,const char *name,BYTE created,OutputModule *module)
{ __asm { int 3 } }
