#ifndef INLINE_ANALYSIS_H
#define INLINE_ANALYSIS_H
#include "global-ir.h"
#include <recon.h>
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
