#include <string.h>
#include "c1.h"
#include <recon-tool.h>
#include <recon.h>
extern BYTE *__cdecl grow_macro_text(BYTE *, BYTE *, BYTE **);
static int macro_commit_growth(void)
{
    BYTE *base = (BYTE *)VirtualAlloc(NULL,0x500000,MEM_RESERVE,PAGE_READWRITE);
    BYTE *limit, *cursor;
    unsigned i;
    int ok = 1;
    if (!base || VirtualAlloc(base,0x4000,MEM_COMMIT,PAGE_READWRITE) != base) return 0;
    memset(base,0xa5,0x4000);
    limit = base + 0x3ffb;
    cursor = limit;
    if (grow_macro_text(cursor,base,&limit) != cursor || limit != base+0x7ffb) ok = 0;
    for (i = 0; i < 0x4000; ++i) if (base[i] != 0xa5) ok = 0;
    base[0x7ffa] = 0x3c;
    cursor = base+0x10004;
    if (grow_macro_text(cursor,base,&limit) != cursor || limit != base+0x1fffb ||
        base[0x7ffa] != 0x3c || base[0x10004]) ok = 0;
    if (!VirtualFree(base,0x20000,MEM_DECOMMIT) || !VirtualFree(base,0,MEM_RELEASE)) ok = 0;
    return ok;
}
extern int c1_test_intrinsic_lookup_relations(void);
const ReconTest intrinsic_tests[2] = {
 {"macro commits retain the existing prefix and grow until the requested cursor fits",1,macro_commit_growth},
 {"intrinsic lookup preserves canonical selectors, exact case and no-match results",1,c1_test_intrinsic_lookup_relations}
};
