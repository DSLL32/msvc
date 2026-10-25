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
unsigned short inline_last_region; /* original0x91850 */
unsigned inline_jump_count; /* original0x91854 */
unsigned short inline_current_region; /* original0x9185c */

/* Original0x44635..0x446c1. Each function starts at region zero with
   explicit missing-link/slot sentinels and one active scope record. */
void initialize_function_inline_analysis(void)
{
    inline_current_region = 0;
    inline_last_region = 0;
    inline_region_count = 0;
    inline_base_region = 0;
    inline_scope_depth = -1;
    inline_slot_count = 0;
    inline_jump_count = 0;
    inline_regions[0].first_child = 0xffff;
    inline_regions[0].next_sibling = inline_regions[0].first_child;
    inline_regions[0].right_slot = 0xff;
    inline_regions[0].left_slot = inline_regions[0].right_slot;
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
unsigned short inline_parent_region; /* original0x91858 */

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
        region->parameter_count = inline_slot_count;
        inline_saved_slot_count = inline_slot_count;
        if (inline_regions[inline_current_region].first_child == 0xffff)
            inline_regions[inline_current_region].first_child = inline_region_count;
        else inline_regions[inline_last_region].next_sibling = inline_region_count;
        region->right_slot = 0;
        region->left_slot = region->right_slot;
        region->right_symbol_index = 0;
        region->left_symbol_index = region->right_symbol_index;
        region->first_child = 0xffff;
        region->next_sibling = region->first_child;
        region->flags = mode;
        inline_parent_region = inline_current_region;
        inline_current_region = inline_region_count;
        for (i = 0; i < inline_slot_count; ++i) {
            scope->parameter_usage[i] = (unsigned char)inline_parameters[i]->references;
            inline_parameters[i]->references = 0;
        }
        scope->flags = 0;
        write_ir_record(0x93, 0);
    } else if (inline_region_count < 255) {
        region = &inline_regions[inline_region_count];
        region->next_sibling = region->first_child = 0xffff;
        region->parameter_count = region->flags = region->left_slot = region->right_slot = 0xff;
        region->left_symbol_index = region->right_symbol_index = ~0U;
    }
}

CppSymbol *inline_unused_parameter; /* original0x98048, only references is used */
typedef char InlineUnusedSymbolSizeMustBe52[(sizeof(CppSymbol) == 52) ? 1 : -1];

/* Original0x448c3..0x44d77. Select the two worthwhile parameter slots,
   then restore the enclosing scopes' saturating reference counts. */
void end_qc_control(void)
{
    InlineRegion *region;
    InlineScope *scope;
    CppSymbol *parameter;
    unsigned i, total;
    unsigned short maximum, threshold;
    int depth;
    if (inline_scope_depth >= 8 || inline_region_count >= 255) {
        --inline_region_count;
        --inline_scope_depth;
        return;
    }
    region = &inline_regions[inline_current_region];
    scope = &inline_scopes[inline_scope_depth];
    if (inline_jump_count) {
        region->left_slot = region->right_slot = 0xff;
        region->flags |= 6;
        for (i = 0; i < inline_slot_count; ++i) {
            parameter = inline_parameters[i];
            if (!scope->parameter_usage[i] && parameter->references) parameter->references = 1;
            else parameter->references = scope->parameter_usage[i];
        }
        inline_slot_count = 0;
    } else {
        region->flags |= scope->flags;
        if (!inline_unused_parameter) inline_unused_parameter = allocate_permanent(sizeof(CppSymbol));
        inline_unused_parameter->references = 0;
        for (i = 0; i < inline_slot_count; ++i) {
            if (inline_parameter_slots[i] == 0xffff) {
                total = 0;
                for (depth = inline_scope_depth; depth >= 0; --depth)
                    total += inline_scopes[depth].parameter_usage[i];
                parameter = inline_parameters[i];
                if (!scope->parameter_usage[i] && (parameter->references || total))
                    parameter->references = 1;
                else parameter->references = scope->parameter_usage[i];
                inline_parameters[i] = inline_unused_parameter;
            }
        }
        if (region->left_slot == 0xff) {
            for (i = 0; i < inline_slot_count; ++i) {
                parameter = inline_parameters[i];
                if (!scope->parameter_usage[i] && parameter->references) parameter->references = 1;
                else parameter->references = scope->parameter_usage[i];
            }
            inline_slot_count = 0;
        } else {
            region->left_slot = region->right_slot = 0xff;
            if (inline_slot_count) {
                region->left_slot = 0;
                threshold = region->first_child == 0xffff ? 2 : 8;
                maximum = 0;
                region->left_symbol_index = 0;
                for (i = 0; i < inline_slot_count; ++i) {
                    if (inline_parameters[i]->references > maximum) {
                        maximum = inline_parameters[i]->references;
                        region->left_slot = (unsigned char)i;
                        region->left_symbol_index = inline_parameter_slots[i];
                    }
                }
                if (maximum < threshold) {
                    region->left_slot = 0xff;
                    region->left_symbol_index = 0;
                } else if (inline_slot_count >= 2) {
                    maximum = 0;
                    for (i = 0; i < inline_slot_count; ++i) {
                        if (i != region->left_slot && inline_parameters[i]->references >= maximum) {
                            region->right_slot = (unsigned char)i;
                            region->right_symbol_index = inline_parameter_slots[i];
                            maximum = inline_parameters[i]->references;
                        }
                    }
                    if (maximum < threshold) {
                        region->right_slot = 0xff;
                        region->right_symbol_index = 0;
                    }
                }
            }
        }
    }
    for (i = 0; i < inline_slot_count; ++i) {
        parameter = inline_parameters[i];
        if (!scope->parameter_usage[i] && parameter->references) parameter->references = 1;
        else parameter->references = scope->parameter_usage[i];
    }
    if (region->left_slot != 0xff) inline_parameters[region->left_slot]->references += 4;
    if (region->right_slot != 0xff) inline_parameters[region->right_slot]->references += 4;
    write_ir_record(0x94, 0);
    inline_last_region = inline_current_region;
    inline_slot_count = inline_regions[inline_current_region].parameter_count;
    inline_saved_slot_count = inline_regions[inline_parent_region].parameter_count;
    inline_current_region = scope->region;
    --inline_scope_depth;
    inline_parent_region = inline_scopes[inline_scope_depth].region;
}

void reconcile_inline_regions(void);

/* Original0x446c1..0x446d4. Resolve the function's completed region tree
   before publishing its inline-query global record. */
void finish_inline_function(void)
{
    reconcile_inline_regions();
    write_global_record(15, function_symbol);
}

int inline_regions_changed; /* original0x9184c */
void __stdcall propagate_inline_regions(unsigned short index);

/* Original0x44e28..0x44e66. Repeat the tree reconciliation at most ten
   times, retaining the original fatal diagnostic if it does not converge. */
void reconcile_inline_regions(void)
{
    int pass;
    for (pass = 0; pass <= 9; ++pass) {
        inline_regions_changed = 0;
        propagate_inline_regions(0);
        if (!inline_regions_changed) return;
    }
    if (inline_regions_changed) fatal_error(1, pch_diagnostic_name, 751);
}

int __stdcall align_inline_slots(unsigned short first, unsigned short second);

/* Original0x44e66..0x44fdc, stdcall4. Reconcile child slot ordering,
   inherit missing choices and propagate eligible child choices upward. */
void __stdcall propagate_inline_regions(unsigned short index)
{
    InlineRegion *region = &inline_regions[index];
    InlineRegion *child;
    unsigned short child_index = region->first_child;
    unsigned slot;
    unsigned char *parent_slot, *child_slot;
    unsigned *parent_symbol, *child_symbol;
    while (child_index != 0xffff) {
        align_inline_slots(index, child_index);
        child = &inline_regions[child_index];
        if (child->left_slot == 0xff) {
            child->left_slot = region->left_slot;
            child->left_symbol_index = region->left_symbol_index;
        }
        if (child->right_slot == 0xff) {
            child->right_slot = region->right_slot;
            child->right_symbol_index = region->right_symbol_index;
        }
        if (child->left_slot == child->right_slot) {
            child->right_slot = 0xff;
            child->right_symbol_index = 0;
        }
        if (child->next_sibling != 0xffff)
            align_inline_slots(child_index, child->next_sibling);
        propagate_inline_regions(child_index);
        if (index && !(region->flags & 2)) {
            for (slot = 0; slot < 2; ++slot) {
                if (!slot) {
                    parent_slot = &region->left_slot;
                    child_slot = &child->left_slot;
                    parent_symbol = &region->left_symbol_index;
                    child_symbol = &child->left_symbol_index;
                } else {
                    parent_slot = &region->right_slot;
                    child_slot = &child->right_slot;
                    parent_symbol = &region->right_symbol_index;
                    child_symbol = &child->right_symbol_index;
                }
                if (*parent_slot == 0xff && child->parameter_count > *child_slot) {
                    inline_regions_changed = 1;
                    *parent_slot = *child_slot;
                    *parent_symbol = *child_symbol;
                }
            }
        }
        child_index = child->next_sibling;
    }
}

/* Original0x44fdc..0x4505d, stdcall8. Swap the second region's two
   choices when either known slot crosses the first region's ordering. */
int __stdcall align_inline_slots(unsigned short first, unsigned short second)
{
    InlineRegion *left = &inline_regions[first];
    InlineRegion *right = &inline_regions[second];
    unsigned char slot;
    unsigned symbol;
    if ((right->right_slot != 0xff && left->left_slot == right->right_slot) ||
        (right->left_slot != 0xff && left->right_slot == right->left_slot)) {
        slot = right->left_slot;
        right->left_slot = right->right_slot;
        right->right_slot = slot;
        symbol = right->left_symbol_index;
        right->left_symbol_index = right->right_symbol_index;
        right->right_symbol_index = symbol;
        return 0;
    }
    return -1;
}
