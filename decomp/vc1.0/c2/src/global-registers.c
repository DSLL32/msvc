
/* Global allocation values and connected definition ranges. */
#include <stdlib.h>
#include "global-registers.h"
#include "register-variables.h"
#include "work-expressions.h"
#include "work-optimizer.h"
#include "work-prepare.h"
#include "function-input.h"
#include "records.h"
#include "control-input.h"
#include "tree-rewrite.h"
#include "tree-input.h"
#include "machine.h"
#include <recon.h>
extern int optimize_precision, optimize_alias;
AllocationValue *allocation_values[ALLOCATION_VALUE_BUCKETS]; /* 0007b9f0..0007ba70 */
BitSet *allocation_live_definitions; /* 0007b9ec */
BitSet *allocation_candidate_registers; /* 0007b9e0 */
BitSet *allocation_observable_registers; /* 0007b9dc */
BitSet *allocation_initial_definitions; /* 0007b9e4 */
BitSet *allocation_observable_definitions; /* 0007b9d8 */
BitSet *allocation_all_definitions; /* 0007b9d4 */
int allocation_region_checked; /* 0007b9c4 */
int allocation_last_definition; /* 0007b9cc */
/* Original [00062100,00062480). Build definition ranges, give eligible
   components virtual registers, rewrite references, then solve their liveness
   and range history before the final global register allocation passes. */
void finish_global_work(void)
{
    WorkItem *work;
    Expression *expression;
    AllocationValue *value;
    AllocationRange *range;
    Node *node;
    SymbolHeader *symbol;
    unsigned bucket, type, attributes, number, identifier;
    for (bucket = 0; bucket < ALLOCATION_VALUE_BUCKETS; ++bucket)
        allocation_values[bucket] = 0;
    initialize_global_values();
    allocation_live_definitions = 0;
    allocation_last_definition = 0;
    for (work = current_work; work; work = work->next) {
        current_expression_work = work;
        allocation_region_checked = 0;
        allocation_live_definitions = assign_bitset(allocation_live_definitions, work->flow_sets[4]);
        for (expression = work->body.expressions; expression; expression = expression->next) {
            work_expression = expression;
            if (expression->source_location) current_source_location = expression->source_location;
            node = expression->tree;
            if (node->opcode == 0x56) current_source_filename = node->right.pointer;
            collect_allocation_ranges(node);
        }
    }
    reset_global_register_map();
    allocation_candidate_registers = 0;
    allocation_observable_registers = 0;
    for (bucket = 0; bucket < ALLOCATION_VALUE_BUCKETS; ++bucket) {
        for (value = allocation_values[bucket]; value; value = value->next) {
            node = value->node;
            type = node->attributes.words.type;
            if (type & 0x1000) {
                if (!is_nonnull_address(node) || optimize_precision) continue;
            } else if ((type & 255) > 4) continue;
            symbol = node->right.symbol;
            if (symbol && symbol->kind != 19) {
                attributes = ((TemporarySymbol *)symbol)->attributes;
                if ((attributes & 0x10000) || (attributes & 15) == 6 ||
                    (!(node->attributes.words.flags & 1) && (attributes & 0x4000))) continue;
            }
            for (range = value->ranges; range; range = range->next) {
                number = next_tree_register | 0x4000;
                ++next_tree_register;
                if (range->flags & 2) continue;
                range->number = (unsigned short)number;
                identifier = number & 0x3fff;
                value->registers = bitset_include(value->registers, identifier);
                allocation_candidate_registers = bitset_include(allocation_candidate_registers, identifier);
                if (is_global_work_value(node, 1))
                    allocation_observable_registers = bitset_include(allocation_observable_registers, identifier);
                type = node->attributes.words.type;
                if (type & 0x1000) record_float_register((unsigned short)(next_tree_register - 1), node->right.symbol);
                else if ((type & 255) == 8) ++next_tree_register;
            }
        }
    }
    for (work = current_work; work; work = work->next) {
        current_expression_work = work;
        release_bitset(work->flow_sets[1]);
        release_bitset(work->flow_sets[3]);
        release_bitset(work->flow_sets[0]);
        work->flow_sets[1] = work->flow_sets[3] = work->flow_sets[0] = 0;
        work->pass.selection.uses = work->pass.selection.definitions = 0;
        allocation_live_definitions = assign_bitset(allocation_live_definitions, work->flow_sets[4]);
        for (expression = work->body.expressions; expression; expression = expression->next) {
            work_expression = expression;
            rewrite_allocation_registers(expression->tree);
        }
    }
    solve_allocation_liveness(current_work);
    for (work = current_work; work; work = work->next) {
        release_bitset(work->flow_sets[2]);
        release_bitset(work->flow_sets[1]);
        work->flow_sets[2] = work->flow_sets[1] = 0;
    }
    solve_allocation_history(current_work);
    for (work = current_work; work; work = work->next) {
        work->pass.selection.uses = intersect_bitset(work->pass.selection.uses, work->flow_sets[2]);
        work->pass.selection.definitions = intersect_bitset(work->pass.selection.definitions, work->flow_sets[1]);
    }
    work = current_work->next;
    if (work) work->pass.selection.uses = bitset_union(work->pass.selection.uses, spilled_parameter_registers);
    exclude_exception_registers(current_work);
    assign_global_registers();
    extend_global_register_ranges();
    record_global_register_ranges();
    release_bitset(allocation_initial_definitions);
    release_bitset(allocation_observable_definitions);
    release_bitset(allocation_all_definitions);
    release_bitset(allocation_live_definitions);
}

AllocationRange *free_allocation_ranges; /* 0007ba70 */
unsigned next_allocation_definition; /* 0007b9d0 */
/* Original [00062c60,00063000). Seed direct memory values, assign a distinct
   definition identity to each store/call effect, then derive block kill and
   reaching-definition sets. The temporary definition-to-value index belongs
   to arena four and is discarded before solving global reachability. */
void initialize_global_values(void)
{
    WorkValue *cursor = (WorkValue *)-1;
    AllocationValue *value, **owners;
    WorkItem *work;
    Expression *expression;
    Node *node;
    SymbolHeader *symbol;
    BitSet *expanded;
    int member;
    unsigned bucket, identifier;
    free_allocation_ranges = 0;
    next_allocation_definition = 0;
    allocation_initial_definitions = 0;
    while ((node = next_work_value_node(&cursor)) != 0) {
        if (node->opcode != 0x30 || node->left.node->opcode != 0x26 ||
            node->left.node->right.symbol != node->right.symbol) continue;
        symbol = node->right.symbol;
        if (symbol && symbol->kind != 19 && (((TemporarySymbol *)symbol)->attributes & 0x2000)) continue;
        if (!optimize_alias && work_has_indirect_values && is_global_work_value(node, 1)) continue;
        find_allocation_value(node, 1);
    }
    limit_global_allocation_values(current_work);
    allocation_all_definitions = 0;
    for (work = current_work; work; work = work->next) {
        release_bitset(work->flow_sets[ALLOCATION_DEFINITION_IN]);
        work->flow_sets[ALLOCATION_DEFINITION_IN] = 0;
        release_bitset(work->flow_sets[ALLOCATION_DEFINITION_KILLS]);
        work->flow_sets[ALLOCATION_DEFINITION_KILLS] = 0;
        for (expression = work->body.expressions; expression; expression = expression->next) {
            node = expression->tree;
            if (node_shapes[node->opcode] & NODE_WRITES_LEFT) {
                value = find_allocation_value(node, 0);
                if (value) {
                    if (node->opcode == 0x32 && !(work->flags & 1))
                        work->flow_sets[ALLOCATION_DEFINITION_KILLS] = bitset_include(work->flow_sets[ALLOCATION_DEFINITION_KILLS], next_allocation_definition);
                    node->info.words.identifier = (short)next_allocation_definition;
                    identifier = next_allocation_definition++;
                    value->definitions = bitset_include(value->definitions, identifier);
                }
            }
            if (allocation_expression_has_call(node)) {
                expression->clobbered_definitions = new_call_definitions();
                work->flow_sets[ALLOCATION_DEFINITION_KILLS] = bitset_union(work->flow_sets[ALLOCATION_DEFINITION_KILLS], expression->clobbered_definitions);
                work->flags |= 8;
            } else expression->clobbered_definitions = 0;
        }
    }
    allocation_all_definitions = bitset_union(allocation_all_definitions, allocation_initial_definitions);
    allocation_observable_definitions = 0;
    for (bucket = 0; bucket < ALLOCATION_VALUE_BUCKETS; ++bucket)
        for (value = allocation_values[bucket]; value; value = value->next)
            if (is_global_work_value(value->node, 1))
                allocation_observable_definitions = bitset_union(allocation_observable_definitions, value->definitions);
    /* The original reserves ten extra pointer slots beyond the actual IDs. */
    owners = arena_alloc(4, next_allocation_definition * sizeof(*owners) + 0x28);
    for (bucket = 0; bucket < ALLOCATION_VALUE_BUCKETS; ++bucket) {
        for (value = allocation_values[bucket]; value; value = value->next) {
            for (member = next_bitset_member(value->definitions); member != -1; member = next_bitset_member(0))
                owners[member] = value;
        }
    }
    for (work = current_work; work; work = work->next) {
        expanded = ensure_bitset(0, 0);
        for (member = next_bitset_member(work->flow_sets[ALLOCATION_DEFINITION_KILLS]); member != -1; member = next_bitset_member(0))
            expanded = bitset_union(expanded, owners[member]->definitions);
        work->flow_sets[ALLOCATION_DEFINITION_KILLS] = bitset_union(work->flow_sets[ALLOCATION_DEFINITION_KILLS], expanded);
        release_bitset(expanded);
        release_bitset(work->flow_sets[ALLOCATION_DEFINITION_GEN]);
        work->flow_sets[ALLOCATION_DEFINITION_GEN] = 0;
        for (expression = work->body.expressions; expression; expression = expression->next) {
            node = expression->tree;
            if (expression->clobbered_definitions) {
                subtract_bitset(work->flow_sets[ALLOCATION_DEFINITION_GEN], allocation_observable_definitions);
                work->flow_sets[ALLOCATION_DEFINITION_GEN] = bitset_union(work->flow_sets[ALLOCATION_DEFINITION_GEN], expression->clobbered_definitions);
            }
            if (node_shapes[node->opcode] & NODE_WRITES_LEFT) {
                value = find_allocation_value(node, 0);
                if (value) {
                    if (node->opcode == 0x32 && !(work->flags & 1))
                        subtract_bitset(work->flow_sets[ALLOCATION_DEFINITION_GEN], value->definitions);
                    work->flow_sets[ALLOCATION_DEFINITION_GEN] = bitset_include(work->flow_sets[ALLOCATION_DEFINITION_GEN], (unsigned)(int)node->info.words.identifier);
                }
            }
        }
        if (work->flow_sets[ALLOCATION_DEFINITION_IN]) release_bitset(work->flow_sets[ALLOCATION_DEFINITION_IN]);
        work->flow_sets[ALLOCATION_DEFINITION_IN] = ensure_bitset(0, -1);
        if (work->flow_sets[ALLOCATION_DEFINITION_OUT]) release_bitset(work->flow_sets[ALLOCATION_DEFINITION_OUT]);
        work->flow_sets[ALLOCATION_DEFINITION_OUT] = ensure_bitset(0, -1);
    }
    reset_arena(4);
    work = current_work;
    work->flow_sets[ALLOCATION_DEFINITION_GEN] = bitset_union(work->flow_sets[ALLOCATION_DEFINITION_GEN], allocation_initial_definitions);
    solve_definition_reachability(current_work);
}

unsigned last_allocation_initial_definition; /* 0007b9c8 */
/* Original [000630d0,000631c0). Address-operand identity is the key: stores
   and loads with the same canonical address share a value and its definition
   set. Create an initial definition when the value first enters this table. */
AllocationValue *__fastcall find_allocation_value(Node *node, int create)
{
    AllocationValue **bucket, *value;
    TemporarySymbol *symbol;
    unsigned attributes, identifier;
    bucket = &allocation_values[shallow_tree_hash(node->left.node) & 31];
    for (value = *bucket; value; value = value->next)
        if (value->node->left.node == node->left.node) return value;
    if (!create) return 0;
    value = arena_alloc(2, sizeof(AllocationValue));
    value->node = node;
    value->ranges = 0;
    value->definitions = 0;
    value->last_block = 0;
    symbol = (TemporarySymbol *)node->right.symbol;
    if (symbol && symbol->symbol.kind == 1 &&
        ((attributes = symbol->attributes) & 0x70) == 0x10 && symbol->symbol.name &&
        !(attributes & 0x10000) && (attributes & 15) != 6)
        value->diagnostic_flags |= ALLOCATION_CHECK_INITIALIZATION;
    else value->diagnostic_flags &= ~ALLOCATION_CHECK_INITIALIZATION;
    value->diagnostic_flags &= ~ALLOCATION_REPORTED_POSSIBLE_UNINITIALIZED;
    value->next = *bucket;
    *bucket = value;
    allocation_initial_definitions = bitset_include(allocation_initial_definitions, next_allocation_definition);
    identifier = next_allocation_definition;
    last_allocation_initial_definition = identifier;
    ++next_allocation_definition;
    value->definitions = bitset_include(0, identifier);
    return value;
}

/* Original [000632c0,00063410). Bound observable-value allocation work by
   the number of blocks and expressions containing calls. When necessary,
   retain the most frequently loaded values among at most 500 candidates. */
void __fastcall limit_global_allocation_values(WorkItem *work)
{
    WorkItem *block;
    Expression *expression;
    AllocationValue *value, *previous, dummy, **ranked;
    int blocks = 0, calls = 0, count = 0, limit, i;
    unsigned bucket;
    for (block = work; block; block = block->next) {
        ++blocks;
        for (expression = block->body.expressions; expression; expression = expression->next)
            calls += count_global_allocation_loads(expression->tree);
    }
    if (!calls) return;
    limit = (0x100000 / blocks) / calls;
    ranked = arena_alloc(4, 500 * sizeof(*ranked));
    for (bucket = 0; bucket < ALLOCATION_VALUE_BUCKETS; ++bucket) {
        for (value = allocation_values[bucket]; value; value = value->next) {
            if (is_global_work_value(value->node, 1)) {
                if (count >= 500) break;
                ranked[count++] = value;
            }
        }
    }
    if (count > limit) {
        qsort(ranked, count, sizeof(*ranked), compare_allocation_load_counts);
        for (i = limit; i < count; ++i) ranked[i]->global_load_count = 0;
        for (bucket = 0; bucket < ALLOCATION_VALUE_BUCKETS; ++bucket) {
            dummy.next = allocation_values[bucket];
            previous = &dummy;
            for (value = dummy.next; value; value = value->next) {
                if (!is_global_work_value(value->node, 1) || value->global_load_count)
                    previous = value;
                else previous->next = value->next;
            }
            allocation_values[bucket] = dummy.next;
        }
        reset_arena(4);
    }
}

/* Original [00063640,000636c0). Count observable loads for the pruning
   score and return whether this tree contains an ordinary call. Multiple
   calls in one tree still contribute one to the caller's expression count. */
unsigned __fastcall count_global_allocation_loads(Node *node)
{
    AllocationValue *value;
    unsigned calls;
    if ((node_shapes[node->opcode] & NODE_ARITY) == NODE_LEAF) return 0;
    calls = count_global_allocation_loads(node->left.node);
    if (node_shapes[node->opcode] & NODE_HAS_SECOND)
        calls |= count_global_allocation_loads(node->second.node);
    if (node->opcode == 0x30 && is_global_work_value(node, 1)) {
        value = find_allocation_value(node, 0);
        if (value) ++value->global_load_count;
    }
    if (node->opcode == 0x3e) return 1;
    return calls;
}

/* Original [00063240,000632c0). Ordinary calls and intrinsic db clobber
   observable allocation values. Traverse the left subtree recursively and
   continue through the second operand without growing this stack frame. */
int __fastcall allocation_expression_has_call(Node *node)
{
    for (;;) {
        if ((node_shapes[node->opcode] & NODE_ARITY) == NODE_LEAF) return 0;
        if (node->opcode == 0x3e ||
            (node->opcode == 0x40 && node->left.node->left.value == 0xdb)) return 1;
        if (allocation_expression_has_call(node->left.node)) return 1;
        if (!(node_shapes[node->opcode] & NODE_HAS_SECOND)) return 0;
        node = node->second.node;
    }
}

/* Original [000631c0,00063240). Each call gets a fresh definition for every
   observable tracked value. Retain those IDs on both the owning value and
   this expression's effect set, then add the effect to the global set. */
BitSet *new_call_definitions(void)
{
    AllocationValue *value;
    BitSet *definitions = 0;
    unsigned bucket, identifier;
    for (bucket = 0; bucket < ALLOCATION_VALUE_BUCKETS; ++bucket) {
        for (value = allocation_values[bucket]; value; value = value->next) {
            if (is_global_work_value(value->node, 1)) {
                value->definitions = bitset_include(value->definitions, next_allocation_definition);
                identifier = next_allocation_definition++;
                definitions = bitset_include(definitions, identifier);
            }
        }
    }
    allocation_all_definitions = bitset_union(allocation_all_definitions, definitions);
    return definitions;
}

/* Original [00063000,000630d0). Solve out = (in - kills) | gen forwards.
   The synthetic entry's gen set supplies initial definitions; all other
   incoming sets are unions of their predecessors' outgoing definitions. */
void __fastcall solve_definition_reachability(WorkItem *head)
{
    WorkItem *work;
    FlowLink *predecessor;
    BitSet *incoming;
    int changed;
    for (work = head; work; work = work->next) {
        clear_bitset(work->flow_sets[ALLOCATION_DEFINITION_IN]);
        work->flow_sets[ALLOCATION_DEFINITION_OUT] = ensure_bitset(work->flow_sets[ALLOCATION_DEFINITION_OUT], 1);
        transfer_bitset(work->flow_sets[ALLOCATION_DEFINITION_OUT], work->flow_sets[ALLOCATION_DEFINITION_IN],
            work->flow_sets[ALLOCATION_DEFINITION_KILLS], work->flow_sets[ALLOCATION_DEFINITION_GEN]);
    }
    incoming = ensure_bitset(0, 1);
    do {
        changed = 0;
        for (work = head->next; work; work = work->next) {
            clear_bitset(incoming);
            for (predecessor = work->predecessors; predecessor; predecessor = predecessor->next)
                bitset_union(incoming, predecessor->work->flow_sets[ALLOCATION_DEFINITION_OUT]);
            if (!equal_bitsets(incoming, work->flow_sets[ALLOCATION_DEFINITION_IN])) {
                assign_bitset(work->flow_sets[ALLOCATION_DEFINITION_IN], incoming);
                changed = 1;
                transfer_bitset(work->flow_sets[ALLOCATION_DEFINITION_OUT], work->flow_sets[ALLOCATION_DEFINITION_IN],
                    work->flow_sets[ALLOCATION_DEFINITION_KILLS], work->flow_sets[ALLOCATION_DEFINITION_GEN]);
            }
        }
    } while (changed);
    release_bitset(incoming);
}

int allocation_use_is_unconditional; /* 0007e848: lazy per-block diagnostic cache */
/* Original [00062960,00062c60). Connected reaching-definition sets form
   allocation ranges. Merge every prior component intersecting this access,
   extend its numbered-block bounds and preserve the two allocation hints.
   Named locals also retain the native definite/possible uninitialized-use
   diagnostics and their once-per-block/once-per-value suppression state. */
void __fastcall collect_allocation_ranges(Node *node)
{
    AllocationValue *value;
    AllocationRange **link, *range, *matched;
    BitSet *definitions;
    int block, first, second, definite;
    unsigned flags;
    if ((node_shapes[node->opcode] & NODE_ARITY) == NODE_LEAF) return;
    collect_allocation_ranges(node->left.node);
    if (node_shapes[node->opcode] & NODE_HAS_SECOND)
        collect_allocation_ranges(node->second.node);
    if (work_expression->tree == node && work_expression->clobbered_definitions) {
        subtract_bitset(allocation_live_definitions, allocation_observable_definitions);
        allocation_live_definitions = bitset_union(allocation_live_definitions, work_expression->clobbered_definitions);
    }
    if (!(node_shapes[node->opcode] & NODE_MEMORY_ACCESS)) return;
    value = find_allocation_value(node, 0);
    if (!value) return;
    if (node_shapes[node->opcode] & NODE_WRITES_LEFT) {
        if (node->opcode == 0x32 && !(current_expression_work->flags & 1))
            subtract_bitset(allocation_live_definitions, value->definitions);
        allocation_live_definitions = bitset_include(allocation_live_definitions, (unsigned)(int)node->info.words.identifier);
        allocation_last_definition = (int)node->info.words.identifier;
    }
    definitions = assign_bitset(0, allocation_live_definitions);
    intersect_bitset(definitions, value->definitions);
    matched = 0;
    link = &value->ranges;
    while ((range = *link) != 0) {
        if (bitsets_overlap(range->definitions, definitions)) {
            block = current_expression_work->number;
            if (range->first_block > block) range->first_block = block;
            if (range->last_block < block) range->last_block = block;
            if (matched) {
                if (matched->first_block > range->first_block) matched->first_block = range->first_block;
                if (matched->last_block < range->last_block) matched->last_block = range->last_block;
                bitset_union(matched->definitions, range->definitions);
                matched->flags |= range->flags & 3;
                *link = range->next;
                release_bitset(range->definitions);
                release_allocation_range(range);
                continue;
            }
            matched = range;
        }
        link = &range->next;
    }
    if (!matched) {
        matched = new_allocation_range();
        matched->last_block = matched->first_block = current_expression_work->number;
        matched->definitions = 0;
        matched->flags &= ~2U;
        matched->flags &= ~1U;
        matched->next = value->ranges;
        value->ranges = matched;
    }
    matched->definitions = bitset_union(matched->definitions, definitions);
    flags = node->attributes.words.flags;
    if (flags & 4) matched->flags |= 1;
    if (flags & 0x800) matched->flags |= 2;
    if ((value->diagnostic_flags & ALLOCATION_CHECK_INITIALIZATION) && value->last_block != current_expression_work->number) {
        value->last_block = current_expression_work->number;
        if (node->opcode != 0x32) {
            first = next_bitset_member(definitions);
            if (first <= (int)last_allocation_initial_definition) {
                second = next_bitset_member(0);
                definite = second == -1;
                if (!definite && allocation_last_definition <= second &&
                    (allocation_last_definition != second || (node_shapes[node->opcode] & NODE_WRITES_LEFT))) {
                    if (!allocation_region_checked) {
                        allocation_use_is_unconditional = !current_expression_work->region ||
                            allocation_use_dominates_backedges(current_expression_work);
                        allocation_region_checked = 1;
                    }
                    definite = allocation_use_is_unconditional != 0;
                }
                if (definite) {
                    compiler_warning(1, 0x2bc, format_function_name(node->right.symbol));
                    value->diagnostic_flags &= ~ALLOCATION_CHECK_INITIALIZATION;
                } else if (!(value->diagnostic_flags & ALLOCATION_REPORTED_POSSIBLE_UNINITIALIZED)) {
                    compiler_warning(4, 0x2bd, format_function_name(node->right.symbol));
                    value->diagnostic_flags |= ALLOCATION_REPORTED_POSSIBLE_UNINITIALIZED;
                }
            }
        }
    }
    release_bitset(definitions);
}

/* Original [00063480,000634b0). Recycle a 24-byte range when available.
   Only the virtual register is initialized here; the collector owns its
   bounds, definition set, allocation hints and insertion into a value. */
AllocationRange *new_allocation_range(void)
{
    AllocationRange *range = free_allocation_ranges;
    if (range) free_allocation_ranges = range->next;
    else range = arena_alloc(2, sizeof(AllocationRange));
    range->number = 0;
    return range;
}

/* Original [000627a0,00062960). Select the connected component containing
   each direct access's live definitions. Save the canonical address in detail
   and replace the address operand with its assigned virtual-register leaf;
   collect referenced, upward-exposed and killed register sets per block. */
void __fastcall rewrite_allocation_registers(Node *node)
{
    AllocationValue *value;
    AllocationRange *range;
    Node *address;
    unsigned number, identifier;
    if (node->opcode == 0x3c) rewrite_allocation_registers(node->left.node);
    if ((node_shapes[node->opcode] & NODE_ARITY) == NODE_LEAF) return;
    rewrite_allocation_registers(node->left.node);
    if (node_shapes[node->opcode] & NODE_HAS_SECOND)
        rewrite_allocation_registers(node->second.node);
    if (work_expression->tree == node && work_expression->clobbered_definitions) {
        subtract_bitset(allocation_live_definitions, allocation_observable_definitions);
        allocation_live_definitions = bitset_union(allocation_live_definitions, work_expression->clobbered_definitions);
        current_expression_work->flow_sets[ALLOCATION_REGISTER_KILLS] = bitset_union(
            current_expression_work->flow_sets[ALLOCATION_REGISTER_KILLS], allocation_observable_registers);
    }
    if (!(node_shapes[node->opcode] & NODE_MEMORY_ACCESS) || node->left.node->opcode != 0x26) return;
    value = find_allocation_value(node, 0);
    if (!value) return;
    if (node->opcode == 0x32) {
        if (!(current_expression_work->flags & 1)) {
            subtract_bitset(allocation_live_definitions, value->definitions);
            current_expression_work->flow_sets[ALLOCATION_REGISTER_KILLS] = bitset_union(
                current_expression_work->flow_sets[ALLOCATION_REGISTER_KILLS], value->registers);
        }
        allocation_live_definitions = bitset_include(allocation_live_definitions, (unsigned)(int)node->info.words.identifier);
    }
    for (range = value->ranges; range; range = range->next)
        if (bitsets_overlap(range->definitions, allocation_live_definitions)) break;
    if (!range) compiler_internal_error("@(#)nglobreg.c:1.35", 0x5bd);
    number = range->number;
    if (!number) return;
    identifier = number & 0x3fff;
    if (!bitset_contains(current_expression_work->flow_sets[ALLOCATION_REGISTER_KILLS], identifier))
        current_expression_work->flow_sets[ALLOCATION_REGISTER_UPWARD_USES] = bitset_include(
            current_expression_work->flow_sets[ALLOCATION_REGISTER_UPWARD_USES], identifier);
    current_expression_work->flow_sets[ALLOCATION_REGISTER_REFERENCES] = bitset_include(
        current_expression_work->flow_sets[ALLOCATION_REGISTER_REFERENCES], identifier);
    address = node->left.node;
    node->detail.node = address;
    node->left.node = intern_leaf(0xa2, 0, (unsigned)address->attributes.words.type, number);
}

/* Original [000626d0,000627a0). Solve backward register liveness:
   live-in = (live-out - killed) | upward-exposed uses. */
void __fastcall solve_allocation_liveness(WorkItem *head)
{
    WorkItem *work;
    FlowLink *successor;
    BitSet *outgoing;
    int changed;
    for (work = head; work; work = work->next) {
        clear_bitset(work->pass.selection.definitions);
        work->pass.selection.uses = ensure_bitset(work->pass.selection.uses, 1);
        transfer_bitset(work->pass.selection.uses, work->pass.selection.definitions,
            work->flow_sets[ALLOCATION_REGISTER_KILLS], work->flow_sets[ALLOCATION_REGISTER_UPWARD_USES]);
    }
    outgoing = ensure_bitset(0, 1);
    do {
        changed = 0;
        for (work = global_exit_work->previous; work; work = work->previous) {
            clear_bitset(outgoing);
            for (successor = work->successors; successor; successor = successor->next)
                bitset_union(outgoing, successor->work->pass.selection.uses);
            if (!equal_bitsets(outgoing, work->pass.selection.definitions)) {
                work->pass.selection.definitions = assign_bitset(work->pass.selection.definitions, outgoing);
                changed = 1;
                transfer_bitset(work->pass.selection.uses, work->pass.selection.definitions,
                    work->flow_sets[ALLOCATION_REGISTER_KILLS], work->flow_sets[ALLOCATION_REGISTER_UPWARD_USES]);
            }
        }
    } while (changed);
    release_bitset(outgoing);
}

/* Original [00062600,000626d0). Propagate the set of registers referenced
   on any incoming path. The upward-use slot is now the history-out set. */
void __fastcall solve_allocation_history(WorkItem *head)
{
    WorkItem *work;
    FlowLink *predecessor;
    BitSet *incoming;
    int changed;
    for (work = head->next; work; work = work->next) {
        clear_bitset(work->flow_sets[ALLOCATION_REGISTER_HISTORY_IN]);
        work->flow_sets[ALLOCATION_REGISTER_HISTORY_OUT] = ensure_bitset(work->flow_sets[ALLOCATION_REGISTER_HISTORY_OUT], 1);
        assign_union_bitset(work->flow_sets[ALLOCATION_REGISTER_HISTORY_OUT],
            work->flow_sets[ALLOCATION_REGISTER_HISTORY_IN], work->flow_sets[ALLOCATION_REGISTER_REFERENCES]);
    }
    incoming = ensure_bitset(0, 1);
    do {
        changed = 0;
        for (work = head->next; work; work = work->next) {
            clear_bitset(incoming);
            for (predecessor = work->predecessors; predecessor; predecessor = predecessor->next)
                bitset_union(incoming, predecessor->work->flow_sets[ALLOCATION_REGISTER_HISTORY_OUT]);
            if (!equal_bitsets(incoming, work->flow_sets[ALLOCATION_REGISTER_HISTORY_IN])) {
                work->flow_sets[ALLOCATION_REGISTER_HISTORY_IN] = assign_bitset(work->flow_sets[ALLOCATION_REGISTER_HISTORY_IN], incoming);
                changed = 1;
                assign_union_bitset(work->flow_sets[ALLOCATION_REGISTER_HISTORY_OUT],
                    work->flow_sets[ALLOCATION_REGISTER_HISTORY_IN], work->flow_sets[ALLOCATION_REGISTER_REFERENCES]);
            }
        }
    } while (changed);
    release_bitset(incoming);
}

BitSet *allocation_excluded_registers; /* 0007b9e8: borrowed during restoration */
/* Original [00062480,00062600). Values live across a protected exception
   edge or region must retain memory homes. Remove their virtual registers
   and restore the saved address operands before subsequent allocation. */
void __fastcall exclude_exception_registers(WorkItem *head)
{
    WorkItem *work;
    FlowLink *successor;
    Expression *expression;
    AllocationValue *value;
    AllocationRange *range;
    BitSet *excluded = 0, *crossing = 0;
    unsigned bucket;
    for (work = head; work; work = work->next) {
        for (successor = work->successors; successor; successor = successor->next) {
            if (successor->flags & 1) {
                crossing = assign_bitset(crossing, work->pass.selection.definitions);
                intersect_bitset(crossing, successor->work->pass.selection.uses);
                excluded = bitset_union(excluded, crossing);
            }
        }
        if (work->region && (work->region->flags & 1)) {
            excluded = bitset_union(excluded, work->pass.selection.uses);
            excluded = bitset_union(excluded, work->pass.selection.definitions);
        }
    }
    release_bitset(crossing);
    if (!excluded) return;
    for (bucket = 0; bucket < ALLOCATION_VALUE_BUCKETS; ++bucket) {
        for (value = allocation_values[bucket]; value; value = value->next) {
            for (range = value->ranges; range; range = range->next) {
                if (bitset_contains(excluded, range->number & 0x3fff)) {
                    range->number = 0;
                    range->flags |= 2;
                }
            }
        }
    }
    allocation_excluded_registers = excluded;
    for (work = head; work; work = work->next) {
        for (expression = work->body.expressions; expression; expression = expression->next)
            restore_excluded_registers(expression->tree);
        work->pass.selection.uses = subtract_bitset(work->pass.selection.uses, allocation_excluded_registers);
        work->pass.selection.definitions = subtract_bitset(work->pass.selection.definitions, allocation_excluded_registers);
        work->flow_sets[ALLOCATION_REGISTER_REFERENCES] = subtract_bitset(work->flow_sets[ALLOCATION_REGISTER_REFERENCES], allocation_excluded_registers);
        /* The original repeats both live-set subtractions here. */
        work->pass.selection.uses = subtract_bitset(work->pass.selection.uses, allocation_excluded_registers);
        work->pass.selection.definitions = subtract_bitset(work->pass.selection.definitions, allocation_excluded_registers);
    }
    release_bitset(excluded);
}


/* Original [00063590,00063604). Restore saved memory addresses for virtual
   registers excluded by protected edges. The packed short is sign-extended
   before clearing bits14/15: negative numbers keep their high sign bits. */
void __fastcall restore_excluded_registers(Node *node)
{
    unsigned number;
    if (node->opcode == 0x3c) restore_excluded_registers(node->left.node);
    if ((node_shapes[node->opcode] & NODE_ARITY) == NODE_LEAF) return;
    restore_excluded_registers(node->left.node);
    if (node_shapes[node->opcode] & NODE_HAS_SECOND)
        restore_excluded_registers(node->second.node);
    if ((node_shapes[node->opcode] & NODE_MEMORY_ACCESS) &&
        node->left.node->opcode == 0xa2) {
        number = (unsigned)(int)node->left.node->left.word & 0xffff3fffU;
        if (bitset_contains(allocation_excluded_registers, number))
            node->left.node = node->detail.node;
    }
}

/* Original [000634b0,000634be). Return a range record to its own pool. */
void __fastcall release_allocation_range(AllocationRange *range)
{
    range->next = free_allocation_ranges;
    free_allocation_ranges = range;
}

FloatRegisterDeclaration *float_register_declarations; /* 00082e74 */
void *__fastcall float_register_heap_allocate(unsigned bytes);
/* Original [0003f210,0003f238). Keep the original nonzeroed heap record
   and its own declaration chain, distinct from range/arena allocation. */
void __fastcall record_float_register(unsigned short number, SymbolHeader *symbol)
{
    FloatRegisterDeclaration *declaration = float_register_heap_allocate(sizeof(*declaration));
    declaration->number = number;
    declaration->symbol = symbol;
    declaration->next = float_register_declarations;
    float_register_declarations = declaration;
}

/* Original [00039590,000395b4). Floating-register/motion storage uses
   ordinary malloc, with the module's own internal-error location on failure. */
void *__fastcall float_register_heap_allocate(unsigned bytes)
{
    void *block = malloc(bytes);
    if (!block) compiler_internal_error("@(#)fltreg.c:1.31", 0x169b);
    return block;
}
