#ifndef WPP_LINKAGE_H
#define WPP_LINKAGE_H
#include "symbol.h"
#include <recon.h>
typedef struct linkage_name *LINKAGE;
extern LINKAGE CurrLinkage,CppLinkage,CLinkage;
int LinkageIsC(SYMBOL sym);
#endif
