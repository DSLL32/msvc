
/* Global allocation values and connected definition ranges. */
#include <stdlib.h>
#include "global-registers.h"
#include "work-expressions.h"
#include "work-optimizer.h"
#include "work-prepare.h"
#include "function-input.h"
#include "control-input.h"
#include "tree-rewrite.h"
#include "tree-input.h"
#include "machine.h"
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
        value->diagnostic_flags |= 1;
    else value->diagnostic_flags &= ~1;
    value->diagnostic_flags &= ~2;
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
