#ifndef INLINE_ANALYSIS_H
#define INLINE_ANALYSIS_H
#include "global-ir.h"
#include <recon.h>
/* Original qc records rank two parameter slots per control region.
   The0xff0-byte allocator and16-byte index stride establish255 rows. */
typedef struct InlineRegion {
    unsigned short next_sibling, first_child;
    unsigned char parameter_count, flags, left_slot, right_slot;
    unsigned left_symbol_index, right_symbol_index;
} InlineRegion;
typedef char InlineRegionSizeMustBe16[(sizeof(InlineRegion) == 16) ? 1 : -1];
typedef char InlineRegionSymbolOffsetMustBe8[
    (offsetof(InlineRegion, left_symbol_index) == 8) ? 1 : -1];
typedef char InlineRegionRightSymbolOffsetMustBe12[
    (offsetof(InlineRegion, right_symbol_index) == 12) ? 1 : -1];
extern InlineRegion *inline_regions;
/* Each active scope saves the ten bounded parameter usage bytes. */
typedef struct InlineScope {
    unsigned char parameter_usage[10];
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
