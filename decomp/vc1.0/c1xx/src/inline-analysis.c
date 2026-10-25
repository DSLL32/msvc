/* Shared qc state encountered by address-taking. */
#include <stdlib.h>
#include "inline-analysis.h"
#include "type-sizes.h"
#include <recon.h>
unsigned short inline_base_region; /* original 0x912f0 */
int inline_scope_depth; /* original 0x912f8 */
InlineScope *inline_scopes; /* original 0x912fc */
unsigned inline_parameter_slots[10]; /* original 0x91304..0x9132c */
unsigned char inline_slot_count; /* original 0x91354 */

void allocate_inline_storage(void);

/* Original0x44630..0x44635. The original tail entry shares the ordinary
   no-argument cdecl contract with its storage allocator. */
void initialize_inline_queries(void)
{
    allocate_inline_storage();
}

InlineRegion *inline_regions; /* original0x912f4 */

/* Original0x450b7..0x4510f. Allocate the255 region rows and eight scope
   records, clearing the complete owned allocations before per-function setup. */
void allocate_inline_storage(void)
{
    unsigned i;
    inline_regions = malloc(255 * sizeof(InlineRegion));
    inline_scopes = malloc(8 * sizeof(InlineScope));
    if (!inline_regions || !inline_scopes) fatal_error(60);
    for (i = 0; i < 255 * sizeof(InlineRegion); ++i)
        ((unsigned char *)inline_regions)[i] = 0;
    for (i = 0; i < 8 * sizeof(InlineScope); ++i)
        ((unsigned char *)inline_scopes)[i] = 0;
}

unsigned short inline_region_count; /* original0x91300 */
unsigned short inline_tail_region; /* original0x91850 */
unsigned inline_control_state; /* original0x91854 */
unsigned short inline_current_region; /* original0x9185c */

/* Original0x44635..0x446c1. Each function starts at region zero with
   explicit missing-link/slot sentinels and one active scope record. */
void initialize_function_inline_analysis(void)
{
    inline_current_region = 0;
    inline_tail_region = 0;
    inline_region_count = 0;
    inline_base_region = 0;
    inline_scope_depth = -1;
    inline_slot_count = 0;
    inline_control_state = 0;
    inline_regions[0].child = 0xffff;
    inline_regions[0].next = inline_regions[0].child;
    inline_regions[0].state07 = 0xff;
    inline_regions[0].state06 = inline_regions[0].state07;
    ++inline_scope_depth;
    inline_scopes[inline_scope_depth].region = 0;
}

CppSymbol *inline_parameters[10]; /* original0x9132c..0x91354 */

/* Original0x44d77..0x44e28, stdcall4. Track eligible word parameters
   only while the fixed slot/scope/region tables can represent their state. */
void __stdcall analyze_inline_parameter(CppSymbol *parameter)
{
    CType *type = parameter->type;
    int eligible;
    if (type->related) {
        if (parameter->attributes & 0x400) return;
        if (member_pointer_is_aggregate(type)) return;
        eligible = pointer_kind_size(type->related->attributes) == 2;
    } else eligible = (type->flags & 15) == 2 && !(type->flags & 0x400);
    if (!eligible || (type->attributes & 0x2000) || inline_slot_count >= 10 ||
        inline_scope_depth >= 8 || inline_region_count >= 255) return;
    inline_parameter_slots[inline_slot_count] = parameter->index;
    inline_parameters[inline_slot_count] = parameter;
    write_ir_record(0x91, 0);
    ++inline_slot_count;
}
