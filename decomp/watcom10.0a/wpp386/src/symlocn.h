#ifndef WPP_SYMLOCN_H
#define WPP_SYMLOCN_H
#include "cgio.h"
#include "symbol.h"
#include <recon.h>
#pragma pack(1)
typedef struct sym_token_locn { CGIO_LOCATION tl; unsigned dwh; } SYM_TOKEN_LOCN;
#pragma pack()
typedef char SymbolLocationSize[(sizeof(SYM_TOKEN_LOCN)==16)?1:-1];
SYM_TOKEN_LOCN *SymbolLocnAlloc(void);
void SymbolLocnDefine(CGIO_LOCATION *location,SYMBOL sym);
#endif
