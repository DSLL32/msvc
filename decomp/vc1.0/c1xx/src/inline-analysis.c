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

unsigned char inline_saved_slot_count; /* original0x91848 */
unsigned short inline_previous_region; /* original0x91858 */

/* Original0x446d9..0x448c3, stdcall4. Enter a control region, link it
   beneath the current region and save the tracked parameter reference bytes. */
void __stdcall begin_qc_control(unsigned char mode)
{
    InlineRegion *region;
    InlineScope *scope;
    unsigned i;
    ++inline_region_count;
    ++inline_scope_depth;
    if (inline_scope_depth < 8) {
        if (inline_region_count >= 255) return;
        scope = &inline_scopes[inline_scope_depth];
        scope->region = inline_current_region;
        region = &inline_regions[inline_region_count];
        region->slot_count = inline_slot_count;
        inline_saved_slot_count = inline_slot_count;
        if (inline_regions[inline_current_region].child == 0xffff)
            inline_regions[inline_current_region].child = inline_region_count;
        else inline_regions[inline_tail_region].next = inline_region_count;
        region->state07 = 0;
        region->state06 = region->state07;
        region->state0c = 0;
        region->state08 = region->state0c;
        region->child = 0xffff;
        region->next = region->child;
        region->kind = mode;
        inline_previous_region = inline_current_region;
        inline_current_region = inline_region_count;
        for (i = 0; i < inline_slot_count; ++i) {
            scope->prefix[i] = (unsigned char)inline_parameters[i]->references;
            inline_parameters[i]->references = 0;
        }
        scope->flags = 0;
        write_ir_record(0x93, 0);
    } else if (inline_region_count < 255) {
        region = &inline_regions[inline_region_count];
        region->next = region->child = 0xffff;
        region->slot_count = region->kind = region->state06 = region->state07 = 0xff;
        region->state08 = region->state0c = ~0U;
    }
}
