#include <windows.h>
#include "input.h"
#include <recon.h>

/* Named pending callees: declarations retain actual stack cleanup, no body credit. */
IMAGE_SYMBOL *__stdcall next_coff_symbol(IMAGE_SYMBOL **cursor)
{ __asm { int 3 } return NULL; }
void __stdcall mark_symbol_bit(DWORD *bits,DWORD index)
{ __asm { int 3 } }
void __stdcall collect_weak_external(StringTable *table,IMAGE_SYMBOL **cursor,
    IMAGE_SYMBOL *raw,StringNode *symbol,OutputModule *module,BYTE *parent,
    BYTE created,DWORD flags)
{ __asm { int 3 } }
int __stdcall has_incremental_symbol(void *table,StringNode *symbol)
{ __asm { int 3 } return 0; }
int __stdcall update_incremental_symbol(const char *name,IMAGE_SYMBOL *raw,
    StringNode *symbol,BYTE created)
{ __asm { int 3 } return 0; }
void __stdcall process_mac_common(LinkContext *context,StringNode *symbol,
    IMAGE_SYMBOL *raw,const char *name,BYTE created,OutputModule *module)
{ __asm { int 3 } }
void __stdcall remember_common_symbol(void *list,StringNode *symbol)
{ __asm { int 3 } }
void __stdcall clear_incremental_undefined(StringNode *symbol,LinkContext *context)
{ __asm { int 3 } }
void __stdcall process_mac_undefined(LinkContext *context,StringNode *symbol,
    IMAGE_SYMBOL *raw,const char *name,BYTE created,OutputModule *module)
{ __asm { int 3 } }
void __stdcall define_input_symbol(StringNode *symbol,Contribution *contribution,
    DWORD value,SHORT section,WORD type,DWORD flags,OutputModule *module,StringTable *table)
{ __asm { int 3 } }
void __stdcall prepare_module_imports(OutputModule *module)
{ __asm { int 3 } }
