/* Value/reference optimization over global work blocks. */
#include "work-optimizer.h"
#include "options.h"
#include "tree-input.h"
#include "tree-rewrite.h"
extern int floating_frontend;
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
   trees. The native allocation is 24 bytes; the last two words are not
   initialized by this function and remain outside our encountered prefix. */
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
    record = arena_alloc(2, 24);
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
