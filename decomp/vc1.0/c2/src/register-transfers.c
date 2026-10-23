/* Transfers between canonical memory homes and global virtual registers. */
#include "register-transfers.h"
#include "register-variables.h"
#include "work-prepare.h"
#include "work-optimizer.h"
#include "expression-fields.h"
#include "function-input.h"
#include "flow-edges.h"
#include <recon.h>

AllocationBinding *allocation_register_bindings; /* 0007b9bc */
WorkItem **allocation_definition_blocks; /* 0007b9c0 */
/* Original [000613f0,00061bc0). Insert transfers between memory homes and
   the virtual registers assigned to connected definition ranges. The two
   arena-one indices associate registers with ranges and definitions with
   call-containing blocks. Floating/special types use a separate path. */
void assign_global_registers(void)
{
    BitSet *observable = 0, *observable_definitions = 0, *special = 0, *hinted = 0;
    BitSet *scratch, *reaching, *local, *common, *stores, *loads, *outside, *pinned;
    WorkItem *work, *owner;
    FlowLink *edge;
    Expression *expression;
    AllocationValue *value;
    AllocationRange *range;
    AllocationBinding *binding;
    unsigned bucket, identifier;
    int member;
    scratch = ensure_bitset(0, 0);
    reaching = ensure_bitset(0, 0);
    allocation_register_bindings = arena_alloc(1, next_tree_register * sizeof(AllocationBinding) + 0x50);
    allocation_definition_blocks = arena_alloc(1, next_allocation_definition * sizeof(WorkItem *) + 0x28);
    for (bucket = 0; bucket < ALLOCATION_VALUE_BUCKETS; ++bucket) {
        for (value = allocation_values[bucket]; value; value = value->next) {
            for (range = value->ranges; range; range = range->next) {
                if (!range->number) continue;
                identifier = range->number & 0x3fff;
                if (value->node->attributes.words.type & 0x3000) {
                    special = bitset_include(special, identifier);
                    continue;
                }
                if (range->flags & 1) hinted = bitset_include(hinted, identifier);
                allocation_register_bindings[identifier].value = value;
                allocation_register_bindings[identifier].range = range;
                if (is_global_work_value(value->node, 1)) {
                    observable = bitset_include(observable, identifier);
                    observable_definitions = bitset_union(observable_definitions, range->definitions);
                }
            }
        }
    }
    observable_definitions = intersect_bitset(observable_definitions, allocation_all_definitions);
    for (work = current_work; work; work = work->next) {
        release_bitset(work->flow_sets[3]);
        work->flow_sets[3] = 0;
        for (expression = work->body.expressions; expression; expression = expression->next)
            if (expression->clobbered_definitions)
                work->flow_sets[3] = bitset_union(work->flow_sets[3], expression->clobbered_definitions);
        if (work->flow_sets[3]) {
            for (member = next_bitset_member(work->flow_sets[3]); member != -1; member = next_bitset_member(0))
                allocation_definition_blocks[member] = work;
        }
    }
    for (work = current_work; work; work = work->next) {
        reaching = assign_bitset(reaching, work->flow_sets[4]);
        reaching = intersect_bitset(reaching, allocation_all_definitions);
        assign_union_bitset(scratch, work->flow_sets[0], work->pass.selection.definitions);
        scratch = subtract_bitset(scratch, work->pass.selection.uses);
        scratch = subtract_bitset(scratch, special);
        for (member = next_bitset_member(scratch); member != -1; member = next_bitset_member(0)) {
            if (member <= 31) continue;
            binding = &allocation_register_bindings[member];
            if (bitsets_overlap(binding->range->definitions, reaching))
                emit_global_register_transfer(0x24, binding->range->number, binding->value->node, work, 0);
        }
        for (edge = work->predecessors; edge; edge = edge->next) {
            assign_difference_bitset(scratch, work->pass.selection.uses, edge->work->pass.selection.definitions);
            scratch = subtract_bitset(scratch, special);
            for (member = next_bitset_member(scratch); member != -1; member = next_bitset_member(0)) {
                if (member > 31) {
                    binding = &allocation_register_bindings[member];
                    split_register_transfer_edge(binding->range, binding->value->node, work, edge);
                }
            }
        }
    }
    release_bitset(reaching);
    for (identifier = 0; identifier < next_tree_register; ++identifier) {
        binding = &allocation_register_bindings[identifier];
        if (!binding->range) continue;
        for (member = next_bitset_member(binding->range->definitions); member != -1; member = next_bitset_member(0)) {
            owner = allocation_definition_blocks[member];
            if (owner)
                emit_global_register_transfer(0x24, binding->range->number, binding->value->node, owner,
                    bitset_contains(owner->pass.selection.definitions, identifier) ? 3 : 0);
        }
    }
    assign_difference_bitset(scratch, hinted, special);
    if (!is_empty_bitset(scratch)) {
        local = 0;
        for (work = current_work; work; work = work->next) {
            local = assign_bitset(local, scratch);
            local = intersect_bitset(local, work->flow_sets[0]);
            for (member = next_bitset_member(local); member != -1; member = next_bitset_member(0)) {
                if (member > 31) {
                    binding = &allocation_register_bindings[member];
                    if (binding->range->flags & 1)
                        mark_register_definition_ends(work, binding->value, binding->range);
                }
            }
        }
        release_bitset(local);
    }
    for (work = current_work; work; work = work->next) {
        pinned = 0;
        common = assign_bitset(0, work->pass.selection.definitions);
        for (edge = work->successors; edge; edge = edge->next)
            common = intersect_bitset(common, edge->work->pass.selection.uses);
        stores = assign_bitset(0, work->flow_sets[0]);
        stores = subtract_bitset(stores, hinted);
        stores = subtract_bitset(stores, special);
        stores = intersect_bitset(stores, observable);
        stores = subtract_bitset(stores, common);
        for (member = next_bitset_member(stores); member != -1; member = next_bitset_member(0)) {
            if (member <= 31) continue;
            binding = &allocation_register_bindings[member];
            for (expression = work->body.expressions; expression; expression = expression->next) {
                if ((node_shapes[expression->tree->opcode] & NODE_WRITES_LEFT) &&
                    expression->tree->left.node->opcode == 0xa2 &&
                    binding->range->number == (int)(short)expression->tree->left.node->left.value) {
                    emit_global_register_transfer(0x25, binding->range->number, binding->value->node, work, 1);
                    pinned = bitset_include(pinned, member);
                    break;
                }
            }
        }
        loads = assign_bitset(0, work->pass.selection.uses);
        loads = subtract_bitset(loads, common);
        loads = subtract_bitset(loads, special);
        loads = intersect_bitset(loads, observable);
        loads = subtract_bitset(loads, pinned);
        outside = assign_bitset(0, work->flow_sets[4]);
        outside = subtract_bitset(outside, allocation_all_definitions);
        for (member = next_bitset_member(loads); member != -1; member = next_bitset_member(0)) {
            if (member <= 31) continue;
            binding = &allocation_register_bindings[member];
            if (bitsets_overlap(binding->range->definitions, outside))
                emit_global_register_transfer(0x25, binding->range->number, binding->value->node, work, 0);
        }
        release_bitset(pinned);
        local = 0;
        if (work->flags & 8) {
            for (expression = work->body.expressions;
                 expression && !expression->clobbered_definitions; expression = expression->next) {
                if (!(node_shapes[expression->tree->opcode] & NODE_WRITES_LEFT) ||
                    expression->tree->left.node->opcode != 0xa2) continue;
                identifier = (unsigned short)expression->tree->left.node->left.value & 0x3fff;
                binding = &allocation_register_bindings[identifier];
                if (binding->range && is_global_work_value(binding->value->node, 1) &&
                    !(binding->range->flags & 1) && !bitset_contains(local, identifier)) {
                    local = bitset_include(local, identifier);
                    emit_global_register_transfer(0x25, binding->range->number, binding->value->node, work, 2);
                }
            }
            assign_difference_bitset(scratch, work->pass.selection.uses, local);
            scratch = intersect_bitset(scratch, observable);
            scratch = subtract_bitset(scratch, special);
            scratch = subtract_bitset(scratch, hinted);
            for (member = next_bitset_member(scratch); member != -1; member = next_bitset_member(0)) {
                if (member <= 31) continue;
                binding = &allocation_register_bindings[member];
                if (bitsets_overlap(binding->range->definitions, outside))
                    emit_global_register_transfer(0x25, binding->range->number, binding->value->node, work, 2);
            }
            /* These temporaries are released only on the native flag-eight
               path; keep its ownership and free-list order. */
            release_bitset(common);
            release_bitset(loads);
            release_bitset(0);
            release_bitset(outside);
            release_bitset(local);
            release_bitset(stores);
        }
    }
    release_bitset(scratch);
    release_bitset(observable_definitions);
    release_bitset(observable);
    release_bitset(special);
}

/* Original [00061bc0,00061d40). An opcode-4f directive carries a load or
   store operation, its virtual register and value type. Place it near the
   first access, after the last write, or at the requested pinned boundary. */
void __fastcall emit_global_register_transfer(unsigned operation, unsigned number, Node *value, WorkItem *work, int placement)
{
    Node *directive, *before;
    Expression *expression;
    unsigned opcode;
    directive = new_node(0x4f);
    directive->left.value = number;
    directive->detail.control.code = (short)operation;
    directive->attributes.words.type = value->attributes.words.type;
    if (placement == 1) {
        before = 0;
        for (expression = work->body.expressions; expression; expression = expression->next) {
            opcode = expression->tree->opcode;
            if (opcode == 0x38 || opcode == 0x3a || opcode == 0x3c)
                before = expression->tree;
            else if (operation == 0x25 && tree_uses_virtual_register(number, expression->tree) == 1)
                before = expression->next ? expression->next->tree : 0;
        }
        work->body.expressions = insert_tree_expression(work->body.expressions, before, directive);
        return;
    }
    if (placement == 3) {
        directive->attributes.words.flags |= 0x80;
        work->body.expressions = insert_tree_expression(work->body.expressions, 0, directive);
        return;
    }
    if (placement == 2) directive->attributes.words.flags |= 0x80;
    else if (placement != 0) return;
    before = 0;
    for (expression = work->body.expressions; expression; expression = expression->next) {
        opcode = expression->tree->opcode;
        if (opcode != 0x4f && opcode != 0x29) {
            before = expression->tree;
            break;
        }
    }
    if (operation == 0x24) {
        for (; expression; expression = expression->next) {
            opcode = expression->tree->opcode;
            if (opcode == 0x38 || opcode == 0x3a || opcode == 0x3c ||
                tree_uses_virtual_register(number, expression->tree)) {
                before = expression->tree;
                break;
            }
        }
    }
    work->body.expressions = insert_tree_expression(work->body.expressions, before, directive);
}

/* Original [00063520,00063590). Test virtual-register leaves, short-circuit
   the first operand and continue through the second operand when present. */
int __fastcall tree_uses_virtual_register(unsigned number, Node *tree)
{
    for (;;) {
        if ((node_shapes[tree->opcode] & NODE_ARITY) == NODE_LEAF)
            return tree->opcode == 0xa2 && (unsigned)(int)tree->left.word == number;
        if (tree_uses_virtual_register(number, tree->left.node)) return 1;
        if (!(node_shapes[tree->opcode] & NODE_HAS_SECOND)) return 0;
        tree = tree->second.node;
    }
}

/* Original [00061f90,00062020). Include blocks where a range is live even
   when that block contains no direct reference to its virtual register. */
void extend_global_register_ranges(void)
{
    WorkItem *work;
    AllocationBinding *binding;
    BitSet *live = ensure_bitset(0, 0);
    int member, block;
    for (work = current_work; work; work = work->next) {
        block = work->number;
        if (block <= 0) continue;
        assign_union_bitset(live, work->pass.selection.uses, work->pass.selection.definitions);
        for (member = next_bitset_member(live); member != -1; member = next_bitset_member(0)) {
            if (member <= 31) continue;
            binding = &allocation_register_bindings[member];
            if (!binding->value) continue;
            if (block < binding->range->first_block) binding->range->first_block = block;
            if (binding->range->last_block < block) binding->range->last_block = block;
        }
    }
    release_bitset(live);
}

/* Original [00062020,00062100). Transfer each scalar range's numbered
   block bounds and canonical memory home into the register-variable map. */
void record_global_register_ranges(void)
{
    AllocationValue *value;
    AllocationRange *range;
    SymbolHeader *symbol;
    unsigned offset, bucket;
    for (bucket = 0; bucket < ALLOCATION_VALUE_BUCKETS; ++bucket) {
        for (value = allocation_values[bucket]; value; value = value->next) {
            if (value->node->attributes.words.type & 0x3000) continue;
            if (!get_stack_address(value->node->left.node, &offset, &symbol)) {
                offset = value->node->left.node->left.value;
                symbol = value->node->left.node->right.symbol;
            }
            for (range = value->ranges; range; range = range->next) {
                if (range->number)
                    record_register_variable_range(range->number, (short)range->first_block, (short)range->last_block,
                        is_global_work_value(value->node, 1), is_parameter_memory_value(value->node),
                        value->node->attributes.words.type & 255, symbol, offset);
            }
        }
    }
}

/* Original [0001a590,0001a5d0). A local/parameter symbol or a frame
   temporary supplies a directly representable stack address. */
int __fastcall get_stack_address(Node *address, unsigned *offset, SymbolHeader **result)
{
    SymbolHeader *symbol = address->right.symbol;
    unsigned storage;
    if (symbol->kind == 1) {
        storage = ((TemporarySymbol *)symbol)->attributes & 0x70;
        if (storage != 0x10 && storage != 0x30) return 0;
    } else if (symbol->kind != 19) return 0;
    *offset = address->left.value;
    *result = symbol;
    return 1;
}

/* Original [00058630,00058660). Only ordinary named parameter storage,
   with the native zero subtype, carries this register-variable flag. */
int __fastcall is_parameter_memory_value(Node *node)
{
    SymbolHeader *symbol = node->right.symbol;
    return symbol && symbol->kind == 1 && !symbol->field0d &&
        (((TemporarySymbol *)symbol)->attributes & 0x70) == 0x30;
}

/* Original [00063510,00063520). Resolve the allocation value, then use
   its unchanged canonical address as the memory home of a register. */
Node *__fastcall register_memory_address(unsigned short number)
{
    AllocationValue *value = find_register_allocation(number, 0);
    return value->node->left.node;
}

/* Original [000634c0,00063510). The private selector returns either a
   connected range or its owning value. Register numbers are compared in
   their native short width; an unknown number is an internal error. */
void *__fastcall find_register_allocation(unsigned short number, int want_range)
{
    AllocationValue *value;
    AllocationRange *range;
    unsigned bucket;
    for (bucket = 0; bucket < ALLOCATION_VALUE_BUCKETS; ++bucket) {
        for (value = allocation_values[bucket]; value; value = value->next) {
            for (range = value->ranges; range; range = range->next) {
                if (range->number == number) {
                    if (want_range) return range;
                    return value;
                }
            }
        }
    }
    compiler_internal_error("@(#)nglobreg.c:1.35", 0x68a);
    return 0;
}

int __fastcall redirect_assembly_label(Node *node, SymbolHeader *old_label, SymbolHeader *new_label);
/* Original [00061d40,00061f8d). A critical predecessor edge needs its
   own register reload. Reuse a transfer block when possible; otherwise split
   before the destination and redirect every explicit incoming label reference. */
void __fastcall split_register_transfer_edge(AllocationRange *range, Node *value,
    WorkItem *work, FlowLink *edge)
{
    WorkItem *predecessor = edge->work, *transfer, *previous;
    Expression *expression, *scan;
    Node *label, *node, *entry;
    SymbolHeader *target;
    unsigned number;
    expression = work->body.expressions;
    while (expression && expression->tree->opcode != 0x29) expression = expression->next;
    target = expression->tree->right.symbol;
    if ((predecessor->flags & 4) || !predecessor->successors->next) transfer = predecessor;
    else {
        previous = work->previous;
        if (!(previous->flags & 4)) {
            expression = previous->body.expressions;
            for (scan = expression; scan; scan = scan->next)
                if (scan->tree->opcode == 0x3a || scan->tree->opcode == 0x3c) break;
            if (!scan) {
                node = new_node(0x3a);
                node->right.symbol = target;
                insert_tree_expression(expression, 0, node);
            }
        }
        transfer = new_work_item();
        transfer->previous = work->previous;
        transfer->next = work;
        work->previous->next = transfer;
        work->previous = transfer;
        transfer->flags |= 4;
        transfer->number = -1;
        transfer->pass.selection.definitions = assign_bitset(0, work->pass.selection.uses);
        intersect_bitset(transfer->pass.selection.definitions, predecessor->pass.selection.definitions);
        transfer->pass.selection.uses = assign_bitset(0, transfer->pass.selection.definitions);
        label = new_flow_label_node();
        transfer->body.expressions = new_expression();
        transfer->body.expressions->tree = label;
        node = new_node(0x3a);
        node->right.symbol = target;
        insert_tree_expression(transfer->body.expressions, 0, node);
        transfer->region = work->region;
        if (transfer->region && transfer->region->header == work) transfer->region->header = transfer;
        for (expression = predecessor->body.expressions; expression; expression = expression->next) {
            node = expression->tree;
            switch (node->opcode) {
            case 0x38: case 0x3a:
                if (node->right.symbol == target) node->right.symbol = label->right.symbol;
                break;
            case 0x3c:
                if (node->right.symbol == target) node->right.symbol = label->right.symbol;
                for (entry = node->second.node; entry->opcode != 0x4c; entry = entry->second.node)
                    if (entry->right.symbol == target) entry->right.symbol = label->right.symbol;
                break;
            case 0x45:
                compiler_internal_error("@(#)nglobreg.c:1.35", 0x733);
                break;
            case 0x4f:
                if (node->detail.control.code == 0x16)
                    redirect_assembly_label(node, target, label->right.symbol);
                break;
            }
        }
        add_flow_predecessor(transfer, predecessor);
        add_flow_successor(predecessor, transfer);
        remove_flow_reference(&predecessor->successors, work);
        edge->work = transfer;
        add_flow_successor(transfer, work);
        transfer->flags |= work->flags & 1;
    }
    number = range->number;
    emit_global_register_transfer(0x24, number, value, transfer, 0);
    transfer->pass.selection.definitions = bitset_include(transfer->pass.selection.definitions, number & 0x3fff);
}
