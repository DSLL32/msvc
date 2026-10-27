#ifndef LINK_WEAK_SYMBOLS_H
#define LINK_WEAK_SYMBOLS_H
#include "full-link.h"
#include <recon.h>

/* Original47f02c owns this list; records are12 bytes, separately allocated. */
typedef struct WeakDefault {
    StringNode *symbol,*default_symbol;
    struct WeakDefault *next;
} WeakDefault;
typedef char WeakDefaultSize[(sizeof(WeakDefault)==12)?1:-1];
#ifdef __cplusplus
extern "C" {
#endif
extern WeakDefault *weak_defaults;
extern DWORD weak_external_count;
void __stdcall remember_weak_default(StringNode *,StringNode *);
StringNode *__stdcall find_weak_default(StringNode *);
#ifdef __cplusplus
}
#endif
#endif
