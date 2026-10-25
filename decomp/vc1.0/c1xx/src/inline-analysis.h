#ifndef INLINE_ANALYSIS_H
#define INLINE_ANALYSIS_H
#include "global-ir.h"
#include <recon.h>
/* Original qc control records use16-byte rows. The allocator's0xff0-byte
   block and later region-index shifts establish255 real rows. Numeric state
   names remain provisional until their individual consumers are reconstructed. */
typedef struct InlineRegion {
    unsigned short next, child;
    unsigned char slot_count, kind, state06, state07;
    unsigned state08, state0c;
} InlineRegion;
typedef char InlineRegionSizeMustBe16[(sizeof(InlineRegion) == 16) ? 1 : -1];
typedef char InlineRegionStateOffsetMustBe8[
    (offsetof(InlineRegion, state08) == 8) ? 1 : -1];
extern InlineRegion *inline_regions;
/* Encountered scope prefix, through its region and flags. */
typedef struct InlineScope {
    unsigned char prefix[10];
    unsigned short region;
    unsigned char flags, reserved;
} InlineScope;
typedef char InlineScopeSizeMustBe14[(sizeof(InlineScope) == 14) ? 1 : -1];
typedef char InlineScopeRegionOffsetMustBe10[(offsetof(InlineScope, region) == 10) ? 1 : -1];
typedef char InlineScopeFlagsOffsetMustBe12[(offsetof(InlineScope, flags) == 12) ? 1 : -1];
extern unsigned short inline_base_region;
extern int inline_scope_depth;
extern InlineScope *inline_scopes;
extern unsigned inline_parameter_slots[10];
extern unsigned char inline_slot_count;
int __stdcall find_inline_parameter_slot(unsigned *slots, unsigned count, unsigned symbol_index);
#endif
