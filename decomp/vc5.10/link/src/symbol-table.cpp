/* The symbol table's existing nonvirtual methods use the original thiscall. */
#include <windows.h>
#include <string.h>
#include "input.h"
#include "full-link-functions.h"
#include "memory.h"
#include <recon.h>

extern "C" {
extern BYTE incremental_enabled;
extern char short_symbol_name[9];
/* Default record width at4672dc. The unvisited supports_incremental routine
 * can select52; it must update this same storage when reconstructed. */
DWORD symbol_record_size=60;
DWORD __stdcall append_symbol_string(StringTable *,const char *);
DWORD __stdcall intern_symbol_string(StringTable *,const char *);
}
StringNode *RECON_THISCALL StringTable::LookupSymbol(WORD kind,const char *name,BYTE *created)
{
    HashEntry *entry;
    StringNode *symbol;
    BYTE added=0;
    if (kind==1) name=strncpy(short_symbol_name,name,8);
    entry=lookup_hash(name,hash.state,1,&hash.string_storage,&added);
    if (added) {
        entry->value=allocate_link_memory(1,symbol_record_size);
        if (created) *created=1;
        symbol=(StringNode *)entry->value;
        /* The original copies an all-zero IMAGE_SYMBOL template before setting
         * the external class. Spell the encountered typed fields directly. */
        symbol->symbol.N.Name.Short=0;
        symbol->symbol.N.Name.Long=0;
        symbol->symbol.Value=0;
        symbol->symbol.SectionNumber=0;
        symbol->symbol.Type=0;
        symbol->symbol.StorageClass=IMAGE_SYM_CLASS_EXTERNAL;
        symbol->symbol.NumberOfAuxSymbols=0;
        if (!kind) {
            if (incremental_enabled) {
                DWORD offset=append_symbol_string(this,name);
                ((StringNode *)entry->value)->symbol.N.Name.Long=offset;
            } else {
                DWORD offset=intern_symbol_string(this,name);
                ((StringNode *)entry->value)->symbol.N.Name.Long=offset;
            }
        } else strncpy((char *)((StringNode *)entry->value)->symbol.N.ShortName,name,8);
        symbol=(StringNode *)entry->value;
        symbol->contribution=NULL;
        symbol->flags |= 1;
        set_symbol_defined(symbol,0,this);
    }
    return (StringNode *)entry->value;
}
extern "C" DWORD __stdcall append_symbol_string(StringTable *table,const char *name)
{ __asm { int 3 } return 0; }
