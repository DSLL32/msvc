#ifndef CV510_NATIVE_WIDEN_ABI_H
#define CV510_NATIVE_WIDEN_ABI_H
#include "widen.h"
#include <recon.h>

/* Host C representation of the captured DLL's genuine thiscall interface.
 * The separate C++ exerciser calls via real virtual declarations in widen.h. */
typedef void (RECON_THISCALL *WidenDestroy)(WidenTi *);
typedef void (RECON_THISCALL *WidenUnknown)(WidenTi *);
typedef int (RECON_THISCALL *WidenRecord)(WidenTi *, void *, const void *, unsigned *);
typedef int (RECON_THISCALL *WidenSymbols)(WidenTi *, WidenOutput *, const void *, unsigned, unsigned);
typedef struct WidenVTable {
    WidenDestroy destroy;
    WidenUnknown reserved1, reserved2;
    WidenRecord record;
    WidenUnknown reserved4, reserved5, reserved6, reserved7;
    WidenSymbols query, emit;
} WidenVTable;
struct WidenTi { const WidenVTable *vtable; };
#endif
