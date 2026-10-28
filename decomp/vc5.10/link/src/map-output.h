#ifndef LINK_MAP_OUTPUT_H
#define LINK_MAP_OUTPUT_H
#include "full-link.h"
#include <recon.h>

typedef struct MacMapSymbol {
    struct MacMapSymbol *next;
    StringNode *symbol;
} MacMapSymbol;
typedef char MacMapSymbolLayout[(sizeof(MacMapSymbol)==8 &&
    offsetof(MacMapSymbol,next)==0 && offsetof(MacMapSymbol,symbol)==4)?1:-1];
#ifdef __cplusplus
extern "C" {
#endif
extern MacMapSymbol *map_thunk_symbols;
void __stdcall append_mac_map_symbol(StringNode *);
#ifdef __cplusplus
}
#endif
#endif
