/* Value/reference optimization over global work blocks. */
#include "work-optimizer.h"
#include "options.h"
#include "tree-input.h"
#include "tree-rewrite.h"
extern int floating_frontend, optimize_alias, optimize_weak_inverse;
extern void *output_state;
int work_optimization_active; /* 0007beac: work optimizer / integer folder state */
int work_reference_first_pass; /* 0007bc24: initial versus final reference pass */
/* 0007bc20: free list, not a counter. The allocator at 00060820 reserves
   20-byte records with four copied trees; 000609f0 releases their trees
   and returns records to this list. Define their fields when encountered. */
LoopTreeRecord *free_loop_tree_records;
/* 0007bc48: canonical value hash heads; entry fields are encountered in
   the value interner. These are runtime pointers, initially zero. */
WorkValue *work_value_buckets[WORK_VALUE_BUCKETS];
BitSet *work_memory_values;   /* 0007bd48: indirect and clobberable values */
BitSet *work_global_values;   /* 0007bc34: direct global/clobberable values */
BitSet *work_direct_values;   /* 0007bc38: recognized direct-address values */
BitSet *work_indirect_values; /* 0007bc3c: nodes without direct-symbol flag */
unsigned next_work_value;    /* 0007bc30: value identifiers begin at one */
int work_has_indirect_values; /* 0007ba7c */

/* Original [0005ba70,0005bb20). Initialize value references, perform the
   first rewrite pass, process region values, then rebuild references for
   expression cleanup and the final rewrite pass. */
void __fastcall optimize_work(WorkItem *work)
{
    WorkItem *block;
    FlowRegion *region;
    work_optimization_active = 1;
    initialize_work_optimizer(work);
    backend_stage = 3;
    work_reference_first_pass = 1;
    build_work_references(work);
    clear_work_expression_cache();
    rewrite_work_values(work);
    if (global_optimization_active) {
        free_loop_tree_records = 0;
        compute_work_dominators(work);
        for (block = work; block; block = block->next) {
            region = block->region;
            if (region && region->header == block) collect_region_values(region);
        }
        hoist_region_values(global_flow_regions);
    }
    work_reference_first_pass = 0;
    build_work_references(work);
    rewrite_work_expressions(work);
    rewrite_work_values(work);
    clear_work_expression_cache();
    work_optimization_active = 0;
}

/* Original [0005c170,0005c210). Clear the canonical-value index and
   classified memory-value sets, visit every expression, then collect
   values for each region at its header. */
void __fastcall initialize_work_optimizer(WorkItem *work)
{
    WorkItem *block;
    Expression *expression;
    Node *unused_result;
    FlowRegion *region;
    unsigned i;
    work_memory_values = 0;
    work_global_values = 0;
    work_direct_values = 0;
    work_indirect_values = 0;
    for (i = 0; i < WORK_VALUE_BUCKETS; ++i) work_value_buckets[i] = 0;
    next_work_value = 1;
    work_has_indirect_values = 0;
    for (block = work; block; block = block->next)
        for (expression = block->body.expressions; expression; expression = expression->next)
            initialize_work_value_tree(expression->tree, 0, &unused_result);
    for (block = work; block; block = block->next) {
        region = block->region;
        if (region && region->header == block) collect_region_values(region);
    }
}

/* Original [0005db10,0005dc20). The third parameter is unused, but its
   stack slot is part of this fixed three-argument fastcall ABI (RET 4).
   Native opcode property 0400 marks memory accesses and propagates an
   address-use context to their left operand. */
void __fastcall initialize_work_value_tree(Node *node, unsigned address_context,
    Node **unused_result)
{
    unsigned shape = node_shapes[node->opcode];
    SymbolHeader *symbol;
    Node *child_result = 0;
    if ((shape & NODE_ARITY) == NODE_LEAF) {
        if (node->opcode == 0x26 && !address_context) {
            symbol = node->right.symbol;
            if (symbol && symbol->kind != 19 && (symbol->kind == 1 || symbol->kind == 7))
                ((TemporarySymbol *)symbol)->attributes |= 0x2000;
        }
        return;
    }
    initialize_work_value_tree(node->left.node,
        address_context | ((shape & NODE_MEMORY_ACCESS) ? 0x400 : 0), &child_result);
    if (node_shapes[node->opcode] & NODE_HAS_SECOND)
        initialize_work_value_tree(node->second.node, address_context, &child_result);
    if (node_shapes[node->opcode] & NODE_MEMORY_ACCESS) {
        node->detail.node = intern_work_value(node)->canonical;
        if (!(node->attributes.words.flags & 1)) work_has_indirect_values = 1;
    } else if (node->opcode == 0x55) {
        if (strip_conversions(node)->attributes.words.type & 0x0c00)
            node->detail.node = intern_work_value(node)->canonical;
        else node->detail.node = 0;
    }
}

WorkValue *last_work_value; /* 0007bc40: reverse creation chain */

/* Original [0005d970,0005dad0). Normalize memory expressions to a load,
   index by their symbol, then compare type, addressing flags and address
   trees. The native allocation is 24 bytes; its final two words are not
   initialized by this function. Alias construction initializes the last one. */
WorkValue *__fastcall intern_work_value(Node *node)
{
    Node *value = memory_value_node(scratch_node, node);
    SymbolHeader *symbol = value->right.symbol;
    WorkValue **bucket, *record;
    unsigned hash;
    hash = (symbol->kind == 19 ? symbol->field04 : symbol->identifier) & 63;
    bucket = &work_value_buckets[hash];
    for (record = *bucket; record; record = record->hash_next) {
        node = record->canonical;
        if (value->attributes.words.type == node->attributes.words.type &&
            !((value->attributes.words.flags ^ node->attributes.words.flags) & 3) &&
            trees_equal(value->left.node, node->left.node, 0)) return record;
    }
    if (is_global_work_value(value, 1) || !(value->attributes.words.flags & 1))
        work_memory_values = bitset_include(work_memory_values, next_work_value);
    if (!(value->attributes.words.flags & 1))
        work_indirect_values = bitset_include(work_indirect_values, next_work_value);
    else if (is_nonnull_address(value) && is_address_exposed_value(value))
        work_direct_values = bitset_include(work_direct_values, next_work_value);
    else if (is_global_work_value(value, 1))
        work_global_values = bitset_include(work_global_values, next_work_value);
    record = arena_alloc(2, sizeof(WorkValue));
    node = copy_tree(value);
    record->canonical = node;
    node->info.words.identifier = (short)next_work_value;
    node->detail.node = node;
    record->identifier = next_work_value++;
    record->hash_next = *bucket;
    *bucket = record;
    record->previous = last_work_value;
    last_work_value = record;
    return record;
}

/* Original [0005caa0,0005cad0). Addressing metadata is shared by memory
   operators. Present a non-load operator as a load in the supplied scratch
   node; an existing load is already in canonical form. */
Node *__fastcall memory_value_node(Node *scratch, Node *node)
{
    if (node->opcode == 0x30) return node;
    *scratch = *node;
    scratch->opcode = 0x30;
    return scratch;
}

/* Original [0005ce60,0005cee0). Identify direct symbols whose storage
   class exposes them to global effects. The second argument includes
   anonymous frontend-generated symbols that the stricter test excludes. */
int __fastcall is_global_work_value(Node *node, int include_anonymous)
{
    TemporarySymbol *symbol;
    unsigned storage, attributes;
    if (!(node->attributes.words.flags & 1) || !node->right.symbol ||
        node->right.symbol->kind == 19) return 0;
    symbol = (TemporarySymbol *)node->right.symbol;
    if (current_function->field28 & 0x40) return 1;
    attributes = symbol->attributes;
    storage = (attributes >> 4) & 7;
    if (storage != 2 && storage != 5 && storage != 6) return 0;
    if (!include_anonymous && floating_frontend && symbol->anonymous &&
        ((attributes & 0x780) == 0x80 || (attributes & 0x780) == 0x200)) return 0;
    return 1;
}

/* Original [0005d3c0,0005d3f0). The initializer marks symbols whose
   address escapes its enclosing memory operand with attribute 2000. */
int __fastcall is_address_exposed_value(Node *node)
{
    if (!(node->attributes.words.flags & 1) || !node->right.symbol ||
        node->right.symbol->kind == 19) return 0;
    return (((TemporarySymbol *)node->right.symbol)->attributes & 0x2000) != 0;
}

/* 0007bb94: block-local 32-bucket available reference map. */
WorkReferenceTable work_references;
WorkReferenceTable *free_work_reference_tables; /* 0007bb88 */
int next_work_reference; /* 0007bc28: positive expression identifiers */
BitSet *work_clobbered_values; /* 0007bc44: marked during reference analysis */

/* Original [0005bda0,0005c020). Rebuild per-block available references,
   and record the references reaching each following region header. */
void __fastcall build_work_references(WorkItem *work)
{
    WorkItem *block, *following;
    FlowLink *successor;
    WorkReference *reference, *copy;
    WorkReferenceTable *table;
    BitSet *available, *region_values, *stored_values;
    Node *node;
    unsigned i;
    next_work_reference = 1;
    work_clobbered_values = 0;
    for (block = work; block; block = block->next) {
        block->pass.optimization.successor_count = 0;
        block->pass.optimization.references = 0;
        for (successor = block->successors; successor; successor = successor->next)
            ++block->pass.optimization.successor_count;
        for (i = 0; i < 4; ++i) release_bitset(block->flow_sets[i]);
        for (i = 0; i < 4; ++i) block->flow_sets[i] = 0;
    }
    for (block = work; block; block = block->next) {
        for (i = 0; i < WORK_REFERENCE_BUCKETS; ++i) work_references.buckets[i] = 0;
        merge_predecessor_references(block);
        analyze_block_references(block);
        table = new_work_reference_table();
        block->pass.optimization.references = table;
        available = 0;
        for (i = 0; i < WORK_REFERENCE_BUCKETS; ++i) {
            table->buckets[i] = work_references.buckets[i];
            for (reference = table->buckets[i]; reference; reference = reference->next)
                available = bitset_include(available, (int)reference->node->info.words.identifier);
        }
        block->flow_sets[WORK_REFERENCE_AVAILABLE] = available;
        region_values = stored_values = 0;
        following = block->next;
        if (!following || !following->region || following->region->header != following) continue;
        table = arena_alloc(3, sizeof(WorkReferenceTable));
        for (i = 0; i < WORK_REFERENCE_BUCKETS; ++i)
            for (reference = work_references.buckets[i]; reference; reference = reference->next) {
                node = reference->node;
                region_values = bitset_include(region_values, (int)node->info.words.identifier);
                if (node->opcode == 0x30 && reference->value && !(node->attributes.words.flags & 8)) {
                    copy = arena_alloc(3, sizeof(WorkReference));
                    *copy = *reference;
                    copy->next = table->buckets[i];
                    table->buckets[i] = copy;
                    stored_values = bitset_include(stored_values, (int)node->info.words.identifier);
                }
            }
        for (i = 0; i < WORK_REFERENCE_BUCKETS; ++i)
            for (reference = work_references.buckets[i]; reference; reference = reference->next) {
                node = reference->node;
                if (node->opcode == 0x30 && !reference->value && !(node->attributes.words.flags & 8) &&
                    bitset_contains(stored_values, (int)node->info.words.identifier)) {
                    copy = arena_alloc(3, sizeof(WorkReference));
                    *copy = *reference;
                    copy->next = table->buckets[i];
                    table->buckets[i] = copy;
                }
            }
        following->region->references = table;
        following->region->available_values = region_values;
        release_bitset(stored_values);
    }
}

BitSet *work_killed_values; /* 0007bd4c: canonical values removed from the current map */

/* Original [0005c450,0005c7b0). Intersect incoming availability, retain
   compatible references, and release predecessor tables at their last use. */
void __fastcall merge_predecessor_references(WorkItem *work)
{
    FlowLink *predecessor, *first;
    WorkItem *block;
    WorkReference *reference, *copy, *candidate, **link, *head;
    WorkReferenceTable *table;
    RegionValue *region_value;
    BitSet *available = 0;
    Node *node, *child;
    unsigned i, arity;
    int region_header;
    if (!work->predecessors) return;
    region_header = work->region && work->region->header == work;
    if (region_header && (work->region->flags & 1)) goto consume_predecessors;
    for (predecessor = work->predecessors; predecessor; predecessor = predecessor->next) {
        block = predecessor->work;
        if (region_header && block->number >= work->number) continue;
        if (!available) available = assign_bitset(available, block->flow_sets[WORK_REFERENCE_AVAILABLE]);
        else available = intersect_bitset(available, block->flow_sets[WORK_REFERENCE_AVAILABLE]);
        if ((short)work->region_depth < (short)block->region_depth && block->region)
            available = intersect_bitset(available, block->region->available_values);
    }
    first = work->predecessors;
    while (first && !first->work->pass.optimization.references) first = first->next;
    if (!first) goto consume_predecessors;
    table = first->work->pass.optimization.references;
    for (i = 0; i < WORK_REFERENCE_BUCKETS; ++i)
        for (reference = table->buckets[i]; reference; reference = reference->next) {
            node = reference->node;
            if (!bitset_contains(available, (int)node->info.words.identifier)) continue;
            child = node->left.node;
            if ((node_shapes[child->opcode] & NODE_ARITY) != NODE_LEAF &&
                !bitset_contains(available, (int)child->info.words.identifier)) continue;
            arity = node_shapes[node->opcode] & NODE_ARITY;
            if (arity == NODE_BINARY) {
                child = node->second.node;
                if ((node_shapes[child->opcode] & NODE_ARITY) != NODE_LEAF &&
                    !bitset_contains(available, (int)child->info.words.identifier)) continue;
            }
            if (arity == NODE_CHAIN && !available_chain_operands(node, available)) continue;
            copy = new_work_reference();
            *copy = *reference;
            copy->next = work_references.buckets[i];
            work_references.buckets[i] = copy;
        }
    release_bitset(available);
    for (predecessor = first->next; predecessor; predecessor = predecessor->next) {
        table = predecessor->work->pass.optimization.references;
        if (!table) continue;
        for (i = WORK_REFERENCE_BUCKETS / 2; i < WORK_REFERENCE_BUCKETS; ++i)
            for (reference = work_references.buckets[i]; reference; reference = reference->next) {
                candidate = table->buckets[i];
                while (candidate && !trees_equal(candidate->node, reference->node, 1))
                    candidate = candidate->next;
                if (!candidate) reference->node = 0;
                else if (candidate->node->info.words.identifier != reference->node->info.words.identifier)
                    reference->node = 0;
                else if (reference->value && !trees_equal(candidate->value, reference->value, 1))
                    reference->value = 0;
            }
    }
    for (i = WORK_REFERENCE_BUCKETS / 2; i < WORK_REFERENCE_BUCKETS; ++i) {
        head = work_references.buckets[i];
        link = &head;
        for (reference = head; reference; reference = reference->next) {
            if (!reference->node) *link = reference->next;
            else link = &(*link)->next;
        }
        work_references.buckets[i] = head;
    }
    if (region_header) {
        for (region_value = work->region->candidates; region_value; region_value = region_value->next)
            invalidate_work_reference(region_value->tree);
        flag_killed_work_references();
        release_bitset(work_killed_values);
        work_killed_values = 0;
    }
consume_predecessors:
    for (predecessor = work->predecessors; predecessor; predecessor = predecessor->next) {
        block = predecessor->work;
        if (!--block->pass.optimization.successor_count) {
            release_work_reference_table(block->pass.optimization.references);
            block->pass.optimization.references = 0;
        }
    }
}

PendingWorkEffect *pending_work_effects; /* 0007bd60: effects delayed until tree scan ends */
Expression *work_expression; /* 0007ba74: expression being value-numbered */
Node *enclosing_work_effect; /* 000837f0: nearest assignment/call-like ancestor */

/* Original [0005c250,0005c450). Rewrite and value-number each eligible
   expression, process delayed effects, then retain simple assignment values. */
void __fastcall analyze_block_references(WorkItem *work)
{
    Expression *expression;
    Node *node, *value, *copy;
    PendingWorkEffect *effect, *next;
    WorkReference *reference;
    unsigned original_opcode, effects;
    int changed;
    work_killed_values = 0;
    for (expression = work->body.expressions; expression; expression = expression->next) {
        node = expression->tree;
        if (!(node_shapes[node->opcode] & NODE_REFERENCE_ROOT)) continue;
        if (expression->source_location) current_source_location = expression->source_location;
        original_opcode = node->opcode;
        pending_work_effects = 0;
        work_expression = expression;
        if (original_opcode != 0x32 || !(node->attributes.words.flags & 0x20)) {
            node = substitute_work_values(node, &changed);
            expression->tree = node;
        }
        enclosing_work_effect = 0;
        analyze_reference_tree(node, &effects, &reference);
        for (effect = pending_work_effects; effect; effect = next) {
            next = effect->next;
            invalidate_work_reference(effect->tree);
            release_pending_work_effect(effect);
        }
        flag_killed_work_references();
        clear_bitset(work_killed_values);
        if (node->opcode == 1 || node->opcode == 0x3a) {
            if (original_opcode == 0x38) retarget_empty_work_branch(work, node);
        } else if (node->opcode == 0x32 &&
            (node_shapes[node->second.node->opcode] & NODE_VALUE_EXPRESSION) &&
            node->left.node->info.words.identifier && node->second.node->info.words.identifier &&
            !(work->flags & 1)) {
            copy = clone_node(memory_value_node(scratch_node, node));
            reference = lookup_work_reference(copy, 0);
            value = node->second.node;
            if (reference->node != copy) {
                reference->node = clone_node(reference->node);
                if (!value->info.words.identifier)
                    reference->node->info.words.identifier = (short)next_work_reference++;
            }
            if (value->info.words.identifier > 0)
                reference->node->info.words.identifier = value->info.words.identifier;
            if ((node_shapes[value->opcode] & NODE_ARITY) == NODE_LEAF ||
                (value->opcode == 0x30 && value->left.node->opcode == 0x26)) reference->value = value;
            else reference->value = 0;
        }
    }
    release_bitset(work_killed_values);
    work_killed_values = 0;
}

/* Original [0005c210,0005c250). Recycled 32-bucket tables are cleared;
   a fresh table comes directly from the native arena without extra clearing. */
WorkReferenceTable *new_work_reference_table(void)
{
    WorkReferenceTable *table = free_work_reference_tables;
    unsigned i;
    if (table) {
        free_work_reference_tables = table->free_next;
        for (i = 0; i < WORK_REFERENCE_BUCKETS; ++i) table->buckets[i] = 0;
        return table;
    }
    return arena_alloc(4, sizeof(WorkReferenceTable));
}

WorkReference *free_work_references; /* 0007bb8c: recycled 12-byte reference records */

/* Original [0005c7b0,0005c800). Recycle each reference chain, then use
   the first table word as the table pool link. Bucket heads remain stale
   until the allocator explicitly clears a reused table. */
void __fastcall release_work_reference_table(WorkReferenceTable *table)
{
    WorkReference *reference, *next, *free_head;
    unsigned i;
    if (!table) return;
    free_head = free_work_references;
    for (i = 0; i < WORK_REFERENCE_BUCKETS; ++i) {
        for (reference = table->buckets[i]; reference; reference = next) {
            next = reference->next;
            reference->next = free_head;
            free_head = reference;
        }
        free_work_references = free_head;
    }
    table->free_next = free_work_reference_tables;
    free_work_reference_tables = table;
}

/* Original [0005ccd0,0005ce60). Substitute a still-available reference
   value, folding changed children through the global tree optimizer first. */
Node *__fastcall substitute_work_values(Node *node, int *changed)
{
    int child_changed = 0;
    unsigned shape;
    WorkReference *reference, *value_reference;
    Node *value;
    if ((node_shapes[node->opcode] & NODE_ARITY) == NODE_LEAF) return node;
    node->left.node = substitute_work_values(node->left.node, &child_changed);
    if (node_shapes[node->opcode] & NODE_HAS_SECOND)
        node->second.node = substitute_work_values(node->second.node, &child_changed);
    if (child_changed && node->opcode != 0x30 &&
        (node_shapes[node->opcode] & NODE_ARITY) != NODE_CHAIN) {
        work_optimization_active = 0;
        node = optimize_tree_global(node);
        work_optimization_active = 1;
        *changed = 1;
        if ((node_shapes[node->opcode] & NODE_ARITY) == NODE_LEAF) return node;
    }
    shape = node_shapes[node->opcode];
    if ((node->attributes.words.flags & 0x800) || !(shape & NODE_VALUE_EXPRESSION) ||
        (shape & NODE_SIDE_EFFECT)) return node;
    reference = lookup_work_reference(node, -1);
    if (!reference || !reference->value) return node;
    value = reference->value;
    if ((node_shapes[value->opcode] & NODE_ARITY) != NODE_LEAF) {
        value_reference = lookup_work_reference(value, -2);
        if (!value_reference ||
            value_reference->node->info.words.identifier != value->info.words.identifier ||
            (is_global_work_value(value, 1) &&
             (!optimize_alias || tree_contains_opcode(work_expression->tree, 0x3e)))) {
            reference->value = 0;
            return node;
        }
    }
    release_tree(node);
    *changed = 1;
    return copy_tree(reference->value);
}

/* Original [0005c950,0005caa0). The lower half of the block map indexes
   expressions; loads use the upper half. Negative modes only look up,
   -2 also leaves the node identifier untouched. Mode zero creates a local
   identifier, while positive mode registers a numbered expression/use. */
WorkReference *__fastcall lookup_work_reference(Node *node, int mode)
{
    WorkReference *reference;
    NumberedExpression *expression;
    Node *candidate;
    unsigned hash = node->opcode + (int)node->left.node->info.words.identifier;
    if (node_shapes[node->opcode] & NODE_HAS_SECOND)
        hash += (int)node->second.node->info.words.identifier;
    hash &= 15;
    if (node->opcode == 0x30) hash += 16;
    for (reference = work_references.buckets[hash]; reference; reference = reference->next) {
        candidate = reference->node;
        if (candidate->opcode != node->opcode ||
            candidate->attributes.words.type != node->attributes.words.type ||
            candidate->left.node->info.words.identifier != node->left.node->info.words.identifier)
            continue;
        if ((node_shapes[node->opcode] & NODE_ARITY) == NODE_CHAIN &&
            !equal_chain_value_ids(node, candidate)) continue;
        if ((node_shapes[node->opcode] & NODE_HAS_SECOND) &&
            candidate->second.node->info.words.identifier != node->second.node->info.words.identifier)
            continue;
        if (mode != -2) node->info.words.identifier = candidate->info.words.identifier;
        return reference;
    }
    if (mode < 0) {
        if (mode != -2) node->info.words.identifier = 0;
        return 0;
    }
    reference = new_work_reference();
    if (mode) {
        expression = intern_numbered_expression(node);
        node->info.words.identifier = expression->canonical->info.words.identifier;
        add_expression_use(expression, node);
    } else node->info.words.identifier = (short)next_work_reference++;
    reference->node = node;
    reference->next = work_references.buckets[hash];
    work_references.buckets[hash] = reference;
    return mode ? 0 : reference;
}

Node *work_reference_parent; /* 000837f4: enclosing tree for deferred reference marking */

/* Original [0005cb70,0005ccd0). Number children before their parent,
   defer effect invalidation until the outer scan ends, and mark child
   references when the parent cannot itself reuse an available value. */
Node *__fastcall analyze_reference_tree(Node *node, unsigned *effects, WorkReference **reference)
{
    unsigned child_effects = 0;
    WorkReference *left_reference = 0, *right_reference = 0;
    PendingWorkEffect *effect;
    Node *saved_effect = 0;
    int effect_parent;
    if ((node_shapes[node->opcode] & NODE_ARITY) == NODE_LEAF) return node;
    effect_parent = node->opcode == 0x3e || node->opcode == 0x40 ||
        node->opcode == 0x7a || node->opcode == 0x32;
    if (effect_parent) {
        saved_effect = enclosing_work_effect;
        enclosing_work_effect = node;
    }
    node->left.node = analyze_reference_tree(node->left.node, &child_effects, &left_reference);
    if (node_shapes[node->opcode] & NODE_HAS_SECOND)
        node->second.node = analyze_reference_tree(node->second.node, &child_effects, &right_reference);
    if (node->attributes.words.flags & 0x800) child_effects |= 1;
    *effects |= child_effects & 1;
    if (node_shapes[node->opcode] & NODE_SIDE_EFFECT) {
        effect = new_pending_work_effect();
        effect->next = pending_work_effects;
        effect->tree = node;
        pending_work_effects = effect;
        *effects |= 1;
    }
    node->attributes.words.flags &= 0xebf7;
    if (!(child_effects & 1) && (node_shapes[node->opcode] & NODE_VALUE_EXPRESSION)) {
        *reference = lookup_work_reference(node, 1);
        if (*reference) goto done;
    }
    work_reference_parent = node;
    if (left_reference) mark_work_reference(node->left.node);
    if (right_reference) mark_work_reference(node->second.node);
done:
    if (node->opcode == 0x3e || node->opcode == 0x40 || node->opcode == 0x7a || node->opcode == 0x32)
        enclosing_work_effect = saved_effect;
    return node;
}

/* Original [0005c8c0,0005c900). Fresh records come from arena four;
   recycled records have all three fields cleared before reuse. */
WorkReference *new_work_reference(void)
{
    WorkReference *reference = free_work_references;
    if (reference) {
        free_work_references = reference->next;
        reference->next = 0;
        reference->node = reference->value = 0;
        return reference;
    }
    return arena_alloc(4, sizeof(WorkReference));
}

/* Native 0007ba84 and 0007bd68: independently built structural/identifier
   hash chains for numbered expressions. The upper structural half holds
   unique loads; the lower half interns non-load computations. */
NumberedExpression *numbered_expressions[NUMBERED_EXPRESSION_BUCKETS];
NumberedExpression *numbered_expressions_by_id[NUMBERED_EXPRESSION_BUCKETS];

/* Original [0005dc50,0005dd90). Loads receive unique identifiers; other
   computations share a structural record when operand value IDs agree.
   Signed remainder matches the native identifier-bucket calculation. */
NumberedExpression *__fastcall intern_numbered_expression(Node *node)
{
    int hash, identifier_hash;
    NumberedExpression *expression;
    Node *candidate;
    if (node->opcode == 0x30) hash = next_work_reference % 32 + 32;
    else {
        hash = node->opcode + (int)node->left.node->info.words.identifier;
        if (node_shapes[node->opcode] & NODE_HAS_SECOND)
            hash += (int)node->second.node->info.words.identifier;
        hash &= 31;
        for (expression = numbered_expressions[hash]; expression; expression = expression->hash_next) {
            candidate = expression->canonical;
            if (candidate->opcode != node->opcode ||
                candidate->attributes.words.type != node->attributes.words.type ||
                candidate->left.node->info.words.identifier != node->left.node->info.words.identifier)
                continue;
            if ((node_shapes[node->opcode] & NODE_ARITY) == NODE_CHAIN &&
                !equal_chain_value_ids(node, candidate)) continue;
            if ((node_shapes[node->opcode] & NODE_HAS_SECOND) && node->opcode != 0x32 &&
                candidate->second.node->info.words.identifier != node->second.node->info.words.identifier)
                continue;
            node->info.words.identifier = candidate->info.words.identifier;
            return expression;
        }
    }
    expression = arena_alloc(4, sizeof(NumberedExpression));
    node->info.words.identifier = (short)next_work_reference++;
    expression->canonical = copy_numbered_expression(node);
    expression->temporary_load = 0;
    expression->uses = 0;
    expression->hash_next = numbered_expressions[hash];
    numbered_expressions[hash] = expression;
    identifier_hash = (int)node->info.words.identifier % NUMBERED_EXPRESSION_BUCKETS;
    expression->identifier_next = numbered_expressions_by_id[identifier_hash];
    numbered_expressions_by_id[identifier_hash] = expression;
    return expression;
}

/* Original [0005dde0,0005de60). Reuse registered canonical operand
   trees; operands absent from the numbered index receive ordinary copies.
   A leaf second operand retains the shallow clone's shared pointer. */
Node *__fastcall copy_numbered_expression(Node *node)
{
    Node *copy = clone_node(node);
    Node *child = node->left.node;
    NumberedExpression *expression = 0;
    if ((node_shapes[child->opcode] & NODE_ARITY) != NODE_LEAF)
        expression = find_numbered_expression(child);
    copy->left.node = expression ? expression->canonical : copy_tree(node->left.node);
    if ((node_shapes[node->opcode] & NODE_HAS_SECOND) &&
        (node_shapes[node->second.node->opcode] & NODE_ARITY) != NODE_LEAF) {
        expression = find_numbered_expression(node->second.node);
        copy->second.node = expression ? expression->canonical : copy_tree(node->second.node);
    }
    return copy;
}

/* Original [0005dc20,0005dc50). Record each occurrence of a numbered
   expression so later reference marking can update all of its users. */
void __fastcall add_expression_use(NumberedExpression *expression, Node *node)
{
    ExpressionUse *use = arena_alloc(4, sizeof(ExpressionUse));
    use->tree = node;
    use->next = expression->uses;
    expression->uses = use;
}

/* Original [0005c800,0005c840). References remain in the block map,
   with flag 8 recording dependence on a value killed by the expression. */
void flag_killed_work_references(void)
{
    WorkReference *reference;
    unsigned i;
    for (i = 0; i < WORK_REFERENCE_BUCKETS; ++i)
        for (reference = work_references.buckets[i]; reference; reference = reference->next)
            if (tree_uses_killed_value(reference->node)) reference->node->attributes.words.flags |= 8;
}

/* Original [0005d2e0,0005d360). A load depends on its canonical memory
   value; other nodes inherit dependencies from their operand trees. */
int __fastcall tree_uses_killed_value(Node *node)
{
    for (;;) {
        if ((node_shapes[node->opcode] & NODE_ARITY) == NODE_LEAF) return 0;
        if (node->opcode == 0x30 && bitset_contains(work_killed_values,
            (int)node->detail.node->info.words.identifier)) return 1;
        if (tree_uses_killed_value(node->left.node)) return 1;
        if (!(node_shapes[node->opcode] & NODE_HAS_SECOND)) return 0;
        node = node->second.node;
    }
}

/* Original [0005cee0,0005d070). Final reference marking shares use flags
   across each numbered expression, with native floating-load/conversion
   exceptions and canonical-memory dependency tracking. */
void __fastcall mark_work_reference(Node *node)
{
    NumberedExpression *expression;
    ExpressionUse *use;
    while (!work_reference_first_pass) {
        if ((node_shapes[node->opcode] & NODE_ARITY) == NODE_LEAF || node->opcode == 0x43) return;
        /* Native property 0010 denotes exactly the six comparisons 1f..24. */
        if (node->opcode >= 0x1f && node->opcode <= 0x24) {
            mark_work_reference(node->left.node);
            node = node->second.node;
            continue;
        }
        expression = find_numbered_expression(node);
        if (!expression) compiler_internal_error("@(#)nglobal.c:1.48", 0x302);
        if (node->opcode == 0x30 &&
            (node_shapes[node->left.node->opcode] & NODE_ARITY) == NODE_LEAF &&
            expression->canonical->opcode == 0x30) return;
        if ((node->attributes.words.type & 0x1000) &&
            ((node->opcode == 0x30 && expression->canonical->opcode == 0x30) ||
             (node->opcode == 0x34 && expression->canonical->opcode == 0x34 &&
              enclosing_work_effect && enclosing_work_effect->opcode == 0x7a &&
              work_reference_parent && work_reference_parent->left.node == node &&
              work_reference_parent->opcode == 0x55))) {
            node = node->left.node;
            continue;
        }
        for (use = expression->uses; use; use = use->next)
            use->tree->attributes.words.flags |= 0x420;
        node->attributes.words.flags |= 0x1020;
        if (node->opcode != 0x30 ||
            ((node_shapes[node->left.node->opcode] & NODE_ARITY) != NODE_LEAF &&
             !(node->attributes.words.type & 0x1000)))
            work_clobbered_values = bitset_include(work_clobbered_values, (int)node->info.words.identifier);
        if (bitset_contains(work_clobbered_values, (int)node->info.words.identifier))
            for (use = expression->uses; use; use = use->next)
                if (use->tree->attributes.words.flags & 8) node->attributes.words.flags &= 0xffdf;
        return;
    }
}

PendingWorkEffect *free_pending_work_effects; /* 0007bd58 */

/* Original [0005d070,0005d0a0). Delayed-effect list cells have a separate
   arena-four free list; only reused cells are explicitly cleared here. */
PendingWorkEffect *new_pending_work_effect(void)
{
    PendingWorkEffect *effect = free_pending_work_effects;
    if (effect) {
        free_pending_work_effects = effect->next;
        effect->next = 0;
        effect->tree = 0;
        return effect;
    }
    return arena_alloc(4, sizeof(PendingWorkEffect));
}

/* Original [0005c840,0005c8c0). Remove aliasing loads from the block map,
   mark their canonical memory values killed, and retain the detached cells
   until their arena is reset. */
void __fastcall invalidate_work_reference(Node *node)
{
    WorkReference *head, *reference, **link;
    Node *value;
    unsigned i;
    for (i = WORK_REFERENCE_BUCKETS / 2; i < WORK_REFERENCE_BUCKETS; ++i) {
        head = work_references.buckets[i];
        link = &head;
        for (reference = head; reference; reference = reference->next) {
            if (work_effect_invalidates_value(node, reference->node)) {
                *link = reference->next;
                value = reference->node;
                work_killed_values = bitset_include(work_killed_values,
                    (int)value->detail.node->info.words.identifier);
                value->attributes.words.flags |= 8;
                reference->node = 0;
            } else link = &(*link)->next;
        }
        work_references.buckets[i] = head;
    }
}

/* Original [0005cb60,0005cb70). */
void __fastcall release_pending_work_effect(PendingWorkEffect *effect)
{
    effect->next = free_pending_work_effects;
    free_pending_work_effects = effect;
}

/* Original [0005bb20,0005bb90). Canonical numbered operands may share
   nodes, so release only each indexed root before resetting arena four.
   The native clobbered-set pointer is left unchanged after recycling. */
void clear_work_expression_cache(void)
{
    NumberedExpression *expression;
    unsigned i;
    for (i = 0; i < NUMBERED_EXPRESSION_BUCKETS; ++i) {
        for (expression = numbered_expressions[i]; expression; expression = expression->hash_next)
            release_node(expression->canonical);
        numbered_expressions[i] = 0;
        numbered_expressions_by_id[i] = 0;
    }
    free_pending_work_effects = 0;
    free_work_references = 0;
    free_work_reference_tables = 0;
    reset_arena(4);
    release_bitset(work_clobbered_values);
}

/* Original [0005bb90,0005bd40). Compute memory-value liveness and remove
   eligible dead stores, then remove unavailable region references and clean
   expression lists. The assignment becomes an ordinary addition so cleanup
   retains operand effects without retaining the store itself. */
void __fastcall rewrite_work_values(WorkItem *work)
{
    WorkItem *block;
    WorkReference *reference, *head, **link;
    WorkReferenceTable *table;
    Expression *expression;
    TemporarySymbol *symbol;
    Node *node;
    short identifier;
    unsigned i;
    for (block = work; block; block = block->next)
        if (block->region && block->region->header == block) {
            table = block->region->references;
            for (i = 0; i < WORK_REFERENCE_BUCKETS; ++i)
                for (reference = table->buckets[i]; reference; reference = reference->next)
                    reference->node->attributes.words.flags &= 0xfff7;
        }
    build_work_alias_sets();
    for (block = work; block; block = block->next) summarize_work_memory(block);
    compute_work_memory_liveness(work);
    for (block = work; block; block = block->next)
        for (expression = block->body.expressions; expression; expression = expression->next) {
            node = expression->tree;
            if (node->opcode != 0x32 || !(node->attributes.words.flags & 1) ||
                (node->attributes.words.flags & 0x800) || is_address_exposed_value(node) ||
                (!optimize_alias && !is_private_work_value(node))) continue;
            symbol = (TemporarySymbol *)node->right.symbol;
            if (symbol && symbol->symbol.kind != 19 && (void *)symbol != output_state &&
                ((symbol->attributes & 0x10000) || (symbol->attributes & 15) == 6)) continue;
            identifier = node->detail.node->info.words.identifier;
            if (!bitset_contains(block->flow_sets[WORK_MEMORY_LIVE_OUT], (int)identifier) &&
                bitset_contains(block->flow_sets[WORK_UNREAD_STORES], (int)identifier) && has_no_later_work_store(expression)) {
                node->opcode = 2;
                invalidate_work_tree(node);
            }
        }
    for (block = work; block; block = block->next)
        if (block->region && block->region->header == block) {
            table = block->region->references;
            for (i = 0; i < WORK_REFERENCE_BUCKETS; ++i) {
                head = table->buckets[i];
                link = &head;
                for (reference = head; reference; reference = reference->next) {
                    if (reference->node->attributes.words.flags & 8) *link = reference->next;
                    else link = &(*link)->next;
                }
                table->buckets[i] = head;
            }
        }
    for (block = work; block; block = block->next) cleanup_expressions(&block->body.expressions);
}

/* Original [0005d6f0,0005d800). Build each canonical memory value's
   conservative alias set, beginning with its own identifier. Values with
   the same hash bucket receive precise policy checks when alias analysis
   is enabled; broad direct/indirect categories supply the other cases. */
void build_work_alias_sets(void)
{
    WorkValue *value, *other;
    Node *node;
    BitSet *aliases;
    unsigned i;
    for (i = 0; i < WORK_VALUE_BUCKETS; ++i)
        for (value = work_value_buckets[i]; value; value = value->hash_next) {
            node = value->canonical;
            aliases = bitset_include(0, value->identifier);
            if (!(node->attributes.words.flags & 1)) {
                aliases = bitset_union(aliases, work_direct_values);
                if (optimize_alias) {
                    for (other = work_value_buckets[i]; other; other = other->hash_next)
                        if (work_effect_invalidates_value(node, other->canonical))
                            aliases = bitset_include(aliases, other->identifier);
                } else aliases = bitset_union(aliases, work_indirect_values);
            } else {
                if (node->attributes.words.flags & 2)
                    for (other = work_value_buckets[i]; other; other = other->hash_next)
                        if (work_effect_invalidates_value(node, other->canonical))
                            aliases = bitset_include(aliases, other->identifier);
                if (is_address_exposed_value(node) || (is_global_work_value(node, 1) && !optimize_alias))
                    aliases = bitset_union(aliases, work_indirect_values);
            }
            value->aliases = aliases;
        }
}

BitSet *work_unread_stores; /* 0007bd54: stores not yet read within this block */
BitSet *work_stored_values; /* 0007bd5c: block memory definitions */
BitSet *work_upward_reads;  /* 0007bd64: reads before a block definition */

/* Original [0005d5b0,0005d6f0). Collect upward-exposed reads and store
   definitions. A repeated store with no intervening read invalidates the
   previous store before recording its replacement. */
void __fastcall summarize_work_memory(WorkItem *work)
{
    Expression *expression;
    Node *node, *previous;
    TemporarySymbol *symbol;
    WorkValue *value;
    work_stored_values = 0;
    work_unread_stores = 0;
    work_upward_reads = 0;
    for (expression = work->body.expressions; expression; expression = expression->next) {
        node = expression->tree;
        collect_work_memory_reads(node);
        if (node->opcode != 0x32 || !(node->attributes.words.flags & 1) ||
            (node->attributes.words.flags & 0x800) || (work->flags & 1) ||
            is_address_exposed_value(node) || (!optimize_alias && !is_private_work_value(node))) continue;
        symbol = (TemporarySymbol *)node->right.symbol;
        if (symbol && symbol->symbol.kind != 19 && (void *)symbol != output_state &&
            ((symbol->attributes & 0x10000) || (symbol->attributes & 15) == 6)) continue;
        value = find_work_value(node->detail.node);
        if (!bitset_contains(work_unread_stores, value->identifier)) {
            work_unread_stores = bitset_include(work_unread_stores, value->identifier);
            work_stored_values = bitset_include(work_stored_values, value->identifier);
        } else {
            previous = value->last_store;
            previous->opcode = 2;
            invalidate_work_tree(previous);
        }
        value->last_store = node;
    }
    work->flow_sets[WORK_MEMORY_READS] = work_upward_reads;
    work->flow_sets[WORK_MEMORY_STORES] = work_stored_values;
    work->flow_sets[WORK_UNREAD_STORES] = work_unread_stores;
}

/* Original [0005d860,0005d970). Visit operands, then collect memory
   values read by loads and call-like effects. Reads retire pending stores;
   only reads preceding a definition become upward-exposed block uses. */
void __fastcall collect_work_memory_reads(Node *node)
{
    BitSet *reads;
    WorkValue *value;
    Node *operand;
    if ((node_shapes[node->opcode] & NODE_ARITY) == NODE_LEAF) return;
    collect_work_memory_reads(node->left.node);
    if (node_shapes[node->opcode] & NODE_HAS_SECOND)
        collect_work_memory_reads(node->second.node);
    if (node->opcode == 0x30) {
        value = find_work_value(node->detail.node);
        reads = assign_bitset(0, value->aliases);
    } else if (node->opcode == 0x3e || node->opcode == 0x40) {
        reads = node->opcode == 0x3e ? assign_bitset(0, work_global_values) : 0;
        for (operand = node->second.node; operand->opcode != 0x4c; operand = operand->second.node)
            if (operand->detail.node) {
                value = find_work_value(operand->detail.node);
                reads = bitset_union(reads, value->aliases);
            }
        if (!optimize_alias || !optimize_weak_inverse) {
            reads = bitset_union(reads, work_indirect_values);
            reads = bitset_union(reads, work_direct_values);
        }
    } else return;
    subtract_bitset(work_unread_stores, reads);
    work_upward_reads = bitset_union(work_upward_reads, subtract_bitset(reads, work_stored_values));
    release_bitset(reads);
}

/* Original [0005d800,0005d860). Find an already interned canonical
   memory value by symbol hash, type and address tree. Unlike insertion,
   this lookup does not compare the node's low address-attribute bits. */
WorkValue *__fastcall find_work_value(Node *node)
{
    SymbolHeader *symbol = node->right.symbol;
    unsigned hash = (symbol->kind == 19 ? symbol->field04 : symbol->identifier) & 63;
    WorkValue *value;
    for (value = work_value_buckets[hash]; value; value = value->hash_next)
        if (value->canonical->attributes.words.type == node->attributes.words.type &&
            trees_equal(node->left.node, value->canonical->left.node, 0)) return value;
    return 0;
}

/* Original [0005d4b0,0005d5b0). Solve live-in = uses | (live-out -
   definitions) backwards to a fixed point, keeping externally observable
   memory values live at the exit block. */
void __fastcall compute_work_memory_liveness(WorkItem *work)
{
    WorkItem *block;
    FlowLink *successor;
    BitSet *live_out;
    int changed;
    for (block = work; block; block = block->next) {
        if (block == global_exit_work && work_memory_values)
            block->flow_sets[WORK_MEMORY_LIVE_OUT] = assign_bitset(block->flow_sets[WORK_MEMORY_LIVE_OUT], work_memory_values);
        else clear_bitset(block->flow_sets[WORK_MEMORY_LIVE_OUT]);
        block->flow_sets[WORK_MEMORY_LIVE_IN] = ensure_bitset(block->flow_sets[WORK_MEMORY_LIVE_IN], 1);
        transfer_bitset(block->flow_sets[WORK_MEMORY_LIVE_IN], block->flow_sets[WORK_MEMORY_LIVE_OUT],
            block->flow_sets[WORK_MEMORY_STORES], block->flow_sets[WORK_MEMORY_READS]);
    }
    live_out = ensure_bitset(0, 1);
    do {
        changed = 0;
        for (block = global_exit_work->previous; block; block = block->previous) {
            clear_bitset(live_out);
            for (successor = block->successors; successor; successor = successor->next)
                bitset_union(live_out, successor->work->flow_sets[WORK_MEMORY_LIVE_IN]);
            if (!equal_bitsets(live_out, block->flow_sets[WORK_MEMORY_LIVE_OUT])) {
                block->flow_sets[WORK_MEMORY_LIVE_OUT] = assign_bitset(block->flow_sets[WORK_MEMORY_LIVE_OUT], live_out);
                changed = 1;
                transfer_bitset(block->flow_sets[WORK_MEMORY_LIVE_IN], block->flow_sets[WORK_MEMORY_LIVE_OUT],
                    block->flow_sets[WORK_MEMORY_STORES], block->flow_sets[WORK_MEMORY_READS]);
            }
        }
    } while (changed);
    release_bitset(live_out);
}

/* Original [0005d0a0,0005d0e0). Identifier lookup preserves the native
   signed-short remainder calculation and the independent identifier chain. */
NumberedExpression *__fastcall find_numbered_expression(Node *node)
{
    short identifier = node->info.words.identifier;
    NumberedExpression *expression = numbered_expressions_by_id[(int)identifier % NUMBERED_EXPRESSION_BUCKETS];
    for (; expression; expression = expression->identifier_next)
        if (expression->canonical->info.words.identifier == identifier) return expression;
    return 0;
}

/* Original [0005dad0,0005db10). Iterate the creation-order chain backwards.
   The -1 cursor starts a walk; null ends it. Restore the canonical tree's
   short value identity before handing it to a new optimizer pass. */
Node *__fastcall next_work_value_node(WorkValue **cursor)
{
    WorkValue *value;
    Node *node;
    if (!*cursor) return 0;
    if (*cursor == (WorkValue *)-1) {
        if (!last_work_value) return 0;
        *cursor = last_work_value;
    }
    value = *cursor;
    node = value->canonical;
    node->info.words.identifier = (short)value->identifier;
    *cursor = value->previous;
    return node;
}
