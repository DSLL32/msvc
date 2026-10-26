/* Narrow retained behavior checks; synthetic list contexts do not claim public reach. */
#define RECON_SHIM_IMPL
#include "register-sets.h"
#include "functions.h"
#include "symbol-storage.h"
#include "function-reader.h"
#include "small-nodes.h"
#include "flow.h"
#include "code-input.h"
#include "inlining.h"
#include "optimizer.h"
#include "optimizer-translate.h"
#include "optimizer-sets.h"
#include "expression-order.h"
#include "global-optimizer.h"
#include "registers.h"
#include "final-schedule.h"
#include "coff-output.h"
#include <recon-tool.h>
#include <recon.h>

typedef char CanonicalTemporaryCountOffset[offsetof(C2FunctionSymbol, temporary_count) == 100 ? 1 : -1];

static int plain_reuse(void)
{
    C2FunctionRegister records[2], *result;
    unsigned i, j;
    memset(records, 0xa5, sizeof(records));
    records[0].next_free = &records[1];
    records[1].next_free = NULL;
    records[0].number = 17;
    records[1].number = 0xfffffff0U;
    free_plain_register_nodes = records;
    for (i = 0; i < 2; ++i) {
        unsigned number = records[i].number;
        result = new_function_register(i ? 6 : 3);
        if (result != &records[i] || result->number != number ||
            free_plain_register_nodes != (i ? NULL : &records[1])) return 0;
        for (j = 0; j < 80; ++j) {
            unsigned char expected = j >= 28 && j < 32 ?
                (unsigned char)(number >> (8 * (j - 28))) : 0;
            if (((unsigned char *)result)[j] != expected) return 0;
        }
    }
    return 1;
}

static int set_reuse(void)
{
    C2RegisterSet records[2], *result, *slot0;
    memset(records, 0xa5, sizeof(records));
    records[0].links.next_free = &records[1];
    records[0].state = 0xa5012345U;
    slot0 = (C2RegisterSet *)0x13579bdfU;
    free_register_sets[0] = slot0; /* Auxiliary canary; no original slot0 default claim. */
    free_register_sets[1] = &records[0];
    result = new_register_set(1);
    return result == &records[0] && free_register_sets[1] == &records[1] &&
        free_register_sets[0] == slot0 && result->links.first == NULL &&
        result->pool == 1 && result->state == 0xa5000000U &&
        result->pending05[0] == 0xa5 && result->pending05[1] == 0xa5 &&
        result->pending05[2] == 0xa5;
}

static int original_real_results(void)
{
    C2Real result;
    unsigned char expected[36];
    int i;
    FILE *file = fopen("unit-real-results.bin", "rb");
    if (!file || fread(expected, 1, 36, file) != 36 || fclose(file)) return 0;
    for (i = 0; i < 3; ++i) {
        memset(&result, 0xa5, sizeof(result));
        c2_real_from_int(&i, &result);
        if (memcmp(&result, expected + 12 * i, 12)) return 0;
    }
    return 1;
}

static int register_set_existing_groups(void)
{
    C2RegisterSet set;
    C2RegisterSetNode nodes[2];
    memset(&set, 0, sizeof(set));
    memset(nodes, 0, sizeof(nodes));
    nodes[0].group = 0;
    nodes[0].next = &nodes[1];
    nodes[0].bits = 1;
    nodes[1].group = 0x80000000U;
    nodes[1].bits = 2;
    set.links.first = nodes;
    set.pool = 1;
    add_register_to_set(&set, 31);
    add_register_to_set(&set, 0x8000001fU);
    return set.links.first == nodes && nodes[0].next == &nodes[1] &&
        !nodes[1].next && nodes[0].bits == 0x80000001U &&
        nodes[1].bits == 0x80000002U;
}

static int register_set_node_reuse(void)
{
    C2RegisterSet set;
    C2RegisterSetNode nodes[2], saved, *result;
    memset(&set, 0xa5, sizeof(set));
    memset(nodes, 0xa5, sizeof(nodes));
    set.pool = 1;
    nodes[0].next = &nodes[1];
    nodes[1].next = NULL;
    saved = nodes[1];
    free_register_set_nodes[0] = (C2RegisterSetNode *)0x13579bdfU;
    last_free_register_set_nodes[0] = (C2RegisterSetNode *)0x2468ace0U;
    free_register_set_nodes[1] = &nodes[0];
    last_free_register_set_nodes[1] = &nodes[1];
    result = new_register_set_node(&set);
    return result == &nodes[0] && !result->group && !result->next && !result->bits &&
        free_register_set_nodes[1] == &nodes[1] &&
        last_free_register_set_nodes[1] == &nodes[1] &&
        free_register_set_nodes[0] == (C2RegisterSetNode *)0x13579bdfU &&
        last_free_register_set_nodes[0] == (C2RegisterSetNode *)0x2468ace0U &&
        !memcmp(&nodes[1], &saved, sizeof(saved));
}

static int register_set_clear(void)
{
    C2RegisterSet set;
    C2RegisterSetNode nodes[3];
    memset(&set, 0xa5, sizeof(set));
    memset(nodes, 0xa5, sizeof(nodes));
    set.links.first = &nodes[0];
    set.pool = 1;
    nodes[0].next = &nodes[1];
    nodes[1].next = NULL;
    nodes[2].next = NULL;
    free_register_set_nodes[1] = &nodes[2];
    last_free_register_set_nodes[1] = &nodes[2];
    clear_register_set(&set);
    if (set.links.first || free_register_set_nodes[1] != &nodes[2] ||
        last_free_register_set_nodes[1] != &nodes[1] ||
        nodes[2].next != &nodes[0] || nodes[0].next != &nodes[1] ||
        nodes[1].next || set.state != 0xa5a5a5a5U) return 0;
    set.pool = 12; /* Empty-set return does not read an unmodeled pool slot. */
    clear_register_set(&set);
    return !set.links.first && free_register_set_nodes[1] == &nodes[2] &&
        last_free_register_set_nodes[1] == &nodes[1];
}

static int clear_small_node_lists(void)
{
    unsigned i;
    for (i = 0; i < 49; ++i)
        free_small_nodes[i] = (C2SmallNode *)(0x1000U + 16 * i);
    clear_function_input_state();
    for (i = 0; i < 49; ++i)
        if (free_small_nodes[i]) return 0;
    return 1;
}

static int symbol_retention_flags(void)
{
    static const struct { unsigned extra, ordinary; int conditional, result; } rows[] = {
        {0, 0, 1, 0}, {0x40, 0, 1, 1}, {0x40, 0, 0, 0},
        {0xc0, 0, 0, 1}, {0x80, 0, 1, 0}, {0x240, 0, 1, 0},
        {0x40, 0x400, 1, 0}, {0x40, 0x1000, 1, 0},
        {0x40, 0x2000, 1, 1}, {0xc0, 0, -1, 1}
    };
    C2FunctionSymbol symbol, saved;
    unsigned i;
    for (i = 0; i < sizeof(rows) / sizeof(rows[0]); ++i) {
        memset(&symbol, 0xa5, sizeof(symbol));
        symbol.additional_flags = rows[i].extra;
        symbol.symbol.flags = rows[i].ordinary;
        saved = symbol;
        if (function_retains_symbols(&symbol, rows[i].conditional) != rows[i].result ||
            memcmp(&symbol, &saved, sizeof(symbol))) return 0;
    }
    return 1;
}

static int parameter_storage_layout(void)
{
    C2InputSymbol parameter;
    C2Symbol owner;
    memset(&parameter, 0, sizeof(parameter));
    memset(&owner, 0, sizeof(owner));
    parameter.symbol.kind = 1;
    parameter.symbol.value = 3;
    parameter.attributes = 0x30;
    owner.flags = 2;
    current_symbol = &owner;
    argument_stack_offset = 8;
    argument_layout_kind = 4;
    argument_alignment = 4;
    if (reserve_symbol_storage(&parameter.symbol) != 4 ||
        parameter.symbol.parameter_offset != 12 || argument_stack_offset != 16 ||
        parameter.symbol.value != 3 || parameter.symbol.flags != 0x800) return 0;
    /* An already reserved symbol is not allocated twice. */
    return !reserve_symbol_storage(&parameter.symbol) && argument_stack_offset == 16;
}

static int stack_slot_alignment(void)
{
    static const struct { unsigned position, bytes; int alignment; unsigned result; } rows[] = {
        {5, 1, 8, 5}, {5, 2, 8, 6}, {5, 4, 8, 8}, {5, 4, 1, 5},
        {5, 2, 1, 5}, {5, 2, 0, 0}, {5, 2, -1, 1}, {0xffffffffU, 4, 4, 0}
    };
    unsigned i;
    for (i = 0; i < sizeof(rows) / sizeof(rows[0]); ++i)
        if (align_stack_storage(rows[i].position, rows[i].bytes, rows[i].alignment) !=
            rows[i].result) return 0;
    return 1;
}

static int local_storage_sequence(void)
{
    local_stack_offset = 0;
    local_storage_alignment = 4;
    return allocate_local_storage(3, 4) == -4 &&
        allocate_local_storage(2, 2) == -6 &&
        allocate_local_storage(1, 0) == -7 &&
        allocate_local_storage(3, 0) == -12 && local_stack_offset == -12;
}

static int function_line_reset(void)
{
    char filename[] = "retained.c";
    input_last_line = 37;
    input_line_needs_record = 0;
    input_line_filename = filename;
    input_line_cache = (C2InputLineMap *)0x13579bdfU;
    return reset_function_line_input() == 37 && input_last_line == 37 &&
        input_line_needs_record == 1 && !input_line_cache && !input_line_filename;
}

static int small_node_reuse(void)
{
    union { unsigned alignment; unsigned char bytes[24]; } storage[2], saved;
    C2SmallNode *first = (C2SmallNode *)&storage[0];
    C2SmallNode *second = (C2SmallNode *)&storage[1];
    unsigned i;
    memset(storage, 0xa5, sizeof(storage));
    first->next = second;
    saved = storage[1];
    free_small_nodes[23] = (C2SmallNode *)0x13579bdfU;
    free_small_nodes[24] = first;
    if (allocate_small_node(24) != first || free_small_nodes[24] != second ||
        free_small_nodes[23] != (C2SmallNode *)0x13579bdfU ||
        memcmp(&storage[1], &saved, sizeof(saved))) return 0;
    for (i = 0; i < sizeof(storage[0]); ++i)
        if (storage[0].bytes[i] != (i == 8 ? 24 : 0)) return 0;
    return 1;
}

static int code_node_splice(void)
{
    C2Record nodes[5];
    memset(nodes, 0, sizeof(nodes));
    nodes[0].next = &nodes[1];
    nodes[1].previous = &nodes[0];
    nodes[1].next = &nodes[2];
    nodes[2].previous = &nodes[1];
    nodes[3].next = &nodes[4];
    nodes[4].previous = &nodes[3];
    link_code_nodes(&nodes[0], &nodes[3]);
    return nodes[0].next == &nodes[3] && nodes[3].previous == &nodes[0] &&
        nodes[3].next == &nodes[4] && nodes[4].previous == &nodes[3] &&
        nodes[4].next == &nodes[1] && nodes[1].previous == &nodes[4] &&
        nodes[1].next == &nodes[2] && nodes[2].previous == &nodes[1] && !nodes[2].next;
}

static int flow_pool_allocation(void)
{
    C2Pool canary;
    unsigned char *bytes;
    unsigned i;
    memset(&c2_pools[11], 0xa5, sizeof(C2Pool)); /* Own untouched backing only. */
    canary = c2_pools[11];
    bytes = c2_pool_allocate(12, 52);
    if (!bytes || !c2_pools[12].blocks || c2_pools[12].cursor != bytes + 52 ||
        c2_pools[12].remaining != 4088 - 52 ||
        memcmp(&c2_pools[11], &canary, sizeof(canary))) return 0;
    for (i = 0; i < 52; ++i)
        if (bytes[i]) return 0;
    return 1;
}

static int flow_block_reuse(void)
{
    C2FlowGraph flow;
    C2BasicBlock blocks[2], saved;
    C2BasicBlock *block;
    unsigned i;
    memset(&flow, 0xa5, sizeof(flow));
    initialize_flow_pool(&flow, 12, sizeof(C2BasicBlock), 20);
    for (i = 0; i < sizeof(flow.pending24); ++i)
        if (flow.pending24[i] != 0xa5) return 0;
    memset(blocks, 0xa5, sizeof(blocks));
    blocks[0].previous = &blocks[1];
    saved = blocks[1];
    flow.free_blocks = blocks;
    block = allocate_flow_block(&flow);
    if (block != blocks || flow.free_blocks != &blocks[1] ||
        memcmp(&blocks[1], &saved, sizeof(saved))) return 0;
    for (i = 0; i < sizeof(*block); ++i)
        if (((unsigned char *)block)[i]) return 0;
    return 1;
}

static int ended_token_input(void)
{
    C2Function function, saved_function;
    C2ILToken token, saved_token;
    memset(&function, 0xa5, sizeof(function));
    memset(&token, 0x5a, sizeof(token));
    saved_function = function;
    saved_token = token;
    input_end_of_module = 1;
    return read_input_token(&function, &token) == NULL &&
        !memcmp(&function, &saved_function, sizeof(function)) &&
        !memcmp(&token, &saved_token, sizeof(token));
}

/* Synthetic option updates also exercise nonzero aliases shared with CLI
 * switches, and the decoder's intentionally retained active flag. */
extern int c2_favor_speed, c2_alias_optimization, c2_float_precision;
extern int c2_frame_optimization, c2_global_optimization, c2_exception_optimization;
extern int c2_global_optimize, extra_function_passes, special_function_passes;
extern unsigned input_option_high_bit;
extern int input_no_global_optimization, input_global_optimization_active;
static int per_function_option_aliases(void)
{
    C2InputSymbol function;
    memset(&function, 0, sizeof(function));
    current_symbol = (C2Symbol *)&function;
    c2_exception_optimization = 0;
    set_function_input_options(0x83a40e00U);
    if (!input_option_high_bit || !c2_favor_speed || !c2_alias_optimization ||
        !c2_float_precision || !c2_frame_optimization || !special_function_passes ||
        !extra_function_passes || !c2_global_optimize || !c2_global_optimization ||
        input_no_global_optimization || input_global_optimization_active != 1) return 0;
    set_function_input_options(0);
    if (input_option_high_bit || c2_favor_speed || c2_alias_optimization ||
        c2_float_precision || c2_frame_optimization || special_function_passes ||
        extra_function_passes || c2_global_optimize || c2_global_optimization ||
        !input_no_global_optimization || input_global_optimization_active != 1) return 0;
    function.additional_flags = 0x100;
    c2_exception_optimization = 1;
    set_function_input_options(0x00200000U);
    return !c2_global_optimize && !input_global_optimization_active;
}

static int pragma_signed_fields(void)
{
    unsigned char bytes[] = {0x80, 0x0c, 0xfe, 0x80, 0x34, 0x12,
                             0x80, 0x00, 0x9b, 0x32, 0xe2};
    const long expected_values[] = {-500, 0x1234, -500000000L};
    ShimFILE input;
    FILE *saved = c2_current_il;
    C2ILToken token, expected;
    unsigned i;
    memset(&input, 0, sizeof(input));
    input._ptr = (char *)bytes;
    input._cnt = sizeof(bytes);
    c2_current_il = (FILE *)&input;
    for (i = 0; i < 3; ++i) {
        memset(&token, 0xa5, sizeof(token));
        token.value.scalar.control_short = i < 2 ? 32 : 31;
        expected = token;
        expected.value.integer = expected_values[i];
        read_control_fields(&token);
        if (memcmp(&token, &expected, sizeof(token))) {
            c2_current_il = saved;
            return 0;
        }
    }
    c2_current_il = saved;
    return input._cnt == 0 && input._ptr == (char *)bytes + sizeof(bytes);
}

static int control_record_reuse(void)
{
    unsigned char allocation[32], expected[32];
    C2Expression *record;
    C2InputSymbol symbol;
    C2Function function;
    memset(allocation, 0xa5, sizeof(allocation));
    *(C2SmallNode **)allocation = NULL;
    free_small_nodes[28] = (C2SmallNode *)allocation;
    record_line_delta = 0x1234;
    memset(expected, 0, 28);
    memset(expected + 28, 0xa5, 4);
    *(unsigned *)(expected + 4) = 0x182;
    expected[8] = 21;
    *(unsigned short *)(expected + 16) = 0x1234;
    record = new_control_expression(0x182);
    if (record != (C2Expression *)allocation || free_small_nodes[28] ||
        memcmp(allocation, expected, sizeof(allocation))) return 0;
    /* A symbol node reuses the same size bucket, clearing the control line. */
    memset(&symbol, 0, sizeof(symbol));
    symbol.symbol.kind = 3;
    input_value_type = 0x3004;
    *(C2SmallNode **)allocation = NULL;
    free_small_nodes[28] = (C2SmallNode *)allocation;
    memset(expected, 0, 28);
    *(unsigned *)(expected + 4) = 0x12a;
    expected[8] = 4;
    *(unsigned short *)(expected + 10) = 0x3004;
    *(C2Symbol **)(expected + 20) = &symbol.symbol;
    record = variable_input_expression(&symbol);
    if (record != (C2Expression *)allocation || free_small_nodes[28] ||
        memcmp(allocation, expected, sizeof(allocation))) return 0;
    release_small_node(record);
    if (free_small_nodes[28] != (C2SmallNode *)allocation ||
        memcmp(allocation, expected, sizeof(allocation))) return 0;
    memset(&function, 0, sizeof(function));
    record_line_delta = 0x5678;
    memset(expected, 0, 28);
    *(unsigned *)(expected + 4) = 0x17a;
    expected[8] = 22;
    *(unsigned short *)(expected + 16) = 0x5678;
    *(C2Function **)(expected + 20) = &function;
    record = new_function_entry(&function);
    return record == (C2Expression *)allocation && !free_small_nodes[28] &&
        !memcmp(allocation, expected, sizeof(allocation));
}

static int input_stack_reuse_and_link(void)
{
    C2InputStack entries[2], expected, free_tail, *stack = &entries[1];
    C2Expression sentinel, trees[2], prior;
    C2Record *last;
    memset(entries, 0xa5, sizeof(entries));
    memset(&sentinel, 0, sizeof(sentinel));
    memset(&prior, 0, sizeof(prior));
    memset(trees, 0, sizeof(trees));
    entries[0].next = NULL;
    entries[0].first = &sentinel;
    free_input_stacks = &entries[0];
    trees[0].next = &trees[1];
    trees[1].previous = &trees[0];
    memset(&expected, 0, sizeof(expected));
    expected.first = &sentinel;
    expected.last = &trees[1];
    expected.next = &entries[1];
    push_input_trees(&stack, trees);
    if (stack != entries || free_input_stacks ||
        memcmp(&entries[0], &expected, sizeof(expected)) ||
        sentinel.next != trees || trees[0].previous != &sentinel ||
        trees[0].next != &trees[1] || trees[1].previous != trees ||
        trees[1].next || entries[1].flags != 0xa5) return 0;
    if (prune_input_tree(NULL, 0) != NULL) return 0;
    last = append_input_trees(stack, (C2Record *)&prior);
    if (last != (C2Record *)&trees[1] || sentinel.next ||
        stack->last != &sentinel || stack->first != &sentinel ||
        prior.next != trees || trees[0].previous != &prior ||
        trees[0].next != &trees[1] || trees[1].previous != trees ||
        trees[1].next || entries[1].flags != 0xa5) return 0;
    free_input_stacks = &free_tail;
    if (pop_input_trees(&stack) != NULL) return 0;
    return stack == &entries[1] && free_input_stacks == &entries[0] &&
        entries[0].next == &free_tail && entries[0].first == &sentinel &&
        entries[0].last == &sentinel && !sentinel.next;
}

static int input_filename_identity(void)
{
    C2InputLineMap entries[2];
    char first[] = "one.c", second[] = "two.c", equal[] = "two.c";
    memset(entries, 0xa5, sizeof(entries));
    entries[0].previous = &entries[1];
    entries[1].previous = NULL;
    entries[0].filename = first;
    entries[1].filename = second;
    input_line_cache = entries;
    input_line_filename = NULL;
    if (set_input_filename(equal) || input_line_filename != second) return 0;
    if (set_input_filename(equal) || input_line_filename != second) return 0;
    if (set_input_filename("new.c") || !input_line_filename ||
        input_line_filename == first || input_line_filename == second ||
        strcmp(input_line_filename, "new.c")) return 0;
    return entries[0].first_line == 0xa5a5a5a5U && entries[1].source_line == 0xa5a5a5a5U;
}

static int input_line_mapping(void)
{
    C2InputLineMap *first, *second;
    char name[] = "same.c", equal[] = "same.c";
    input_line_cache = input_first_line_map = NULL;
    input_line_filename = name;
    input_line_needs_record = 1;
    input_last_line = 7;
    if (resolve_input_source_line(0) || input_line_cache || input_last_line != 7 ||
        !input_line_needs_record) return 0;
    if (resolve_input_source_line(100) != 8) return 0;
    first = input_line_cache;
    if (first != input_first_line_map || first->previous || first->next ||
        first->first_line != 8 || first->last_line != 8 ||
        first->source_line != 100 || first->filename != name) return 0;
    if (resolve_input_source_line(103) != 11 || first->last_line != 11 ||
        input_last_line != 11 || input_line_cache != first) return 0;
    if (resolve_input_source_line(1103) != 1011 || input_line_cache != first) return 0;
    if (resolve_input_source_line(2104) != 1012) return 0;
    second = input_line_cache;
    if (second == first || first->next != second || second->previous != first ||
        second->source_line != 2104 || second->first_line != 1012 ||
        second->last_line != 1012 || input_first_line_map != first) return 0;
    input_line_filename = equal; /* Equal bytes are a distinct source identity. */
    if (resolve_input_source_line(2105) != 1013 || input_line_cache == second ||
        input_line_cache->filename != equal || input_line_cache->previous != second)
        return 0;
    input_line_needs_record = 1;
    if (resolve_input_source_line(2105) != 1014 || input_line_needs_record) return 0;
    return input_line_cache->first_line == 1014 && input_last_line == 1014;
}

static int input_register_cache(void)
{
    C2RegisterBlock block;
    C2InputSymbol symbol;
    C2FunctionRegister *reg;
    memset(&block, 0, sizeof(block));
    memset(&symbol, 0, sizeof(symbol));
    block.cursor = block.records;
    block.end = block.records + 32;
    block.records[0].number = 123;
    default_register_block = &block;
    symbol.symbol.kind = 3;
    reg = input_symbol_register(&symbol);
    if (reg != block.records || symbol.symbol.input_register != reg ||
        reg->symbol != &symbol.symbol || reg->kind != 11 || reg->type != 0x1004 ||
        reg->value_bytes != 4 || reg->canonical != reg || reg->number != 123)
        return 0;
    reg->pending18 = 0xa5a51234;
    return input_symbol_register(&symbol) == reg && block.cursor == block.records + 1 &&
        reg->pending18 == 0xa5a51234;
}

static int input_encoded_types(void)
{
    input_value_type = 0x3004;
    return adjust_input_return_size(0, 0x12345678U) == 0x12340000U &&
        adjust_input_return_size(1, 4) == 0x1004 &&
        adjust_input_return_size(2, 4) == 0x2004 &&
        adjust_input_return_size(3, 0x12345678U) == 0x12343004U &&
        adjust_input_return_size(4, 0) == 0x3004 &&
        adjust_input_return_size(11, 8) == 0x3004 &&
        adjust_input_return_size(5, 10) == 0x400a &&
        adjust_input_return_size(10, 8) == 0x4008 &&
        adjust_input_return_size(6, 4095) == 0x5fff &&
        adjust_input_return_size(6, 4096) == 0x5000 &&
        adjust_input_return_size(6, 0x80000000U) == 0x80005000U &&
        adjust_input_return_size(7, 0xffffffffU) == 0x1004;
}

static int typed_register_record(void)
{
    unsigned char allocation[32];
    C2FunctionRegister reg;
    C2Expression *tree = (C2Expression *)allocation;
    memset(allocation, 0xa5, sizeof(allocation));
    tree->next = NULL;
    free_small_nodes[28] = (C2SmallNode *)tree;
    if (new_typed_register_expression(0x120, 0x2004, &reg) != tree ||
        free_small_nodes[28] || tree->kind != 2 || tree->opcode != 0x120 ||
        tree->type != 0x2004 || tree->value.reg != &reg || tree->left ||
        tree->next || tree->previous || tree->flags || tree->location.attributes)
        return 0;
    if (allocation[28] != 0xa5 || allocation[29] != 0xa5 ||
        allocation[30] != 0xa5 || allocation[31] != 0xa5) return 0;
    release_small_node(tree);
    input_value_type = 0x3004;
    return new_register_expression(0x120, &reg) == tree && tree->kind == 3 &&
        tree->type == 0x3004 && tree->value.reg == &reg && !tree->left &&
        allocation[28] == 0xa5 && allocation[31] == 0xa5;
}

static int return_storage_conventions(void)
{
    unsigned convention, bytes;
    for (convention = 0; convention <= 24; ++convention) {
        if (convention & 3) {
            if (needs_input_return_storage(convention, 0x2004, 4) != 7) return 0;
            continue;
        }
        if (needs_input_return_storage(convention, 0x2004, 4) ||
            needs_input_return_storage(convention, 0x400a, 10) != (convention == 8))
            return 0;
        for (bytes = 0; bytes <= 10; ++bytes) {
            int expected = !bytes || bytes > 8 || (bytes & (bytes - 1));
            if (needs_input_return_storage(convention, 0x5000, bytes) != !!expected)
                return 0;
        }
    }
    return 1;
}

static int input_type_fields(void)
{
    unsigned char bytes[] = {0x42, 0x06, 0x80, 0x00, 0x10, 0x00, 0x00, 0x42};
    ShimFILE input;
    FILE *saved = c2_current_il;
    C2ILToken token, expected;
    memset(&input, 0, sizeof(input));
    input._ptr = (char *)bytes;
    input._cnt = sizeof(bytes);
    c2_current_il = (FILE *)&input;
    memset(&token, 0xa5, sizeof(token));
    token.opcode = 48;
    expected = token;
    expected.type = 0x2004;
    read_input_type(&token);
    if (memcmp(&token, &expected, sizeof(token))) return 0;
    expected.type = 0x5000;
    expected.bytes = 4096;
    read_input_type(&token);
    if (memcmp(&token, &expected, sizeof(token))) return 0;
    token.opcode = expected.opcode = 39;
    read_input_type(&token);
    c2_current_il = saved;
    return !memcmp(&token, &expected, sizeof(token)) && !input._cnt &&
        input._ptr == (char *)bytes + sizeof(bytes);
}

static int input_operand_pop(void)
{
    C2InputStack stack;
    C2Expression sentinel, operand, tail;
    memset(&stack, 0, sizeof(stack));
    memset(&sentinel, 0, sizeof(sentinel));
    memset(&operand, 0xa5, sizeof(operand));
    stack.first = &sentinel;
    stack.last = &operand;
    stack.flags = 0xa0;
    sentinel.next = &operand;
    operand.kind = 3;
    operand.previous = &sentinel;
    operand.next = &tail;
    return pop_input_expression(&stack) == &operand && stack.flags == 0x80 &&
        stack.last == &sentinel && stack.first == &sentinel && !sentinel.next &&
        !operand.previous && operand.next == &tail && operand.flags == 0xa5;
}

static int widest_address_view(void)
{
    C2FunctionRegister rows[2], before[2];
    memset(rows, 0, sizeof(rows));
    rows[0].canonical = rows;
    rows[0].variant_next = rows + 1;
    rows[0].kind = rows[1].kind = 3;
    rows[0].type = 0x5008;
    rows[1].type = 0x5010;
    rows[0].displacement = rows[1].displacement = 8;
    rows[0].value_bytes = 8;
    rows[1].value_bytes = 16;
    rows[1].canonical = rows;
    memcpy(before, rows, sizeof(rows));
    if (new_address_register(rows, 0x5000, 0, 0) != rows + 1 ||
        memcmp(rows, before, sizeof(rows))) return 0;
    rows[0].type = 0x3004;
    rows[1].type = 0x2004;
    rows[0].value_bytes = rows[1].value_bytes = 4;
    memcpy(before, rows, sizeof(rows));
    return new_address_register(rows, 0x2004, 0xa5a5a5a5U, 0) == rows &&
        !memcmp(rows, before, sizeof(rows));
}

static int integer_record_width(void)
{
    unsigned char allocation[36];
    C2Expression *tree = (C2Expression *)allocation;
    union { unsigned __int64 value; C2IntegerWords words; } integer;
    memset(allocation, 0xa5, sizeof(allocation));
    tree->next = NULL;
    free_small_nodes[32] = (C2SmallNode *)tree;
    if (new_integer_expression(0x1001, 0x80) != tree || tree->kind != 7 ||
        tree->opcode != 0x11d || tree->type != 0x1001 ||
        tree->integer.low != 0xffffff80U || tree->integer.high != 0xffffffffU)
        return 0;
    release_small_node(tree);
    if (new_integer_expression(0x2001, ~(unsigned __int64)0) != tree ||
        tree->integer.low != 0xff || tree->integer.high) return 0;
    release_small_node(tree);
    integer.words.low = 0x89abcdefU;
    integer.words.high = 0xfedcba98U;
    if (new_integer_expression(0x1008, integer.value) != tree ||
        tree->integer.low != integer.words.low || tree->integer.high != integer.words.high)
        return 0;
    release_small_node(tree);
    return new_integer_expression(0x1000, integer.value) == tree &&
        !tree->integer.low && !tree->integer.high &&
        allocation[32] == 0xa5 && allocation[35] == 0xa5;
}

static int binary_value_types(void)
{
    static const unsigned short cases[][3] = {
        {0x3004, 0x3008, 0x1004}, {0x3004, 0x4008, 0x3004},
        {0x1008, 0x3004, 0x3004}, {0x4004, 0x1008, 0x4004},
        {0x1008, 0x4004, 0x4004}, {0x4004, 0x4008, 0x4008},
        {0x4008, 0x4004, 0x4008}, {0x1008, 0x2004, 0x1008},
        {0x1004, 0x2008, 0x2008}, {0x1004, 0x2004, 0x2004},
        {0x2004, 0x1004, 0x2004}, {0x5004, 0x1004, 0x1004}
    };
    unsigned i;
    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i)
        if (common_input_value_type(cases[i][0], cases[i][1]) != cases[i][2])
            return 0;
    return 1;
}

static int folded_integer_width(void)
{
    unsigned char storage[36], expected[36];
    C2Expression *tree = (C2Expression *)storage;
    unsigned __int64 narrowed = ~(unsigned __int64)0x7f;
    unsigned short type = 0x1001;
    memset(storage, 0xa5, sizeof(storage));
    tree->type = 0x2008;
    tree->integer.value = 0x80;
    tree->location.expression.comparison = 0x12;
    memcpy(expected, storage, sizeof(storage));
    memcpy(expected + 10, &type, 2);
    memcpy(expected + 24, &narrowed, 8);
    expected[17] = 0x52;
    if (fold_input_constant(tree, type, NULL) != tree ||
        memcmp(storage, expected, sizeof(storage))) return 0;
    tree->integer.value = 0x7f;
    tree->location.expression.comparison = 0x12;
    memcpy(expected, storage, sizeof(storage));
    return fold_input_constant(tree, type, NULL) == tree &&
        !memcmp(storage, expected, sizeof(storage));
}

static int temporary_node_sizes(void)
{
    union { unsigned alignment; unsigned char bytes[40]; } storage;
    static const unsigned kinds[6] = {1, 11, 14, 16, 23, 25};
    unsigned kind, size, i, row;
    C2SmallNode *node = (C2SmallNode *)&storage;
    for (row = 0; row < 6; ++row) {
        kind = kinds[row];
        size = kind == 23 ? 24 : kind == 1 ? 28 : (kind == 16 || kind == 25) ? 36 : 32;
        memset(&storage, 0xa5, sizeof(storage));
        node->next = NULL;
        free_small_nodes[size] = node;
        if (allocate_small_node(kind) != node || free_small_nodes[size]) return 0;
        for (i = 0; i < sizeof(storage.bytes); ++i)
            if (storage.bytes[i] != (i < size ? (i == 8 ? kind : 0) : 0xa5))
                return 0;
    }
    return 1;
}

static int copied_operand_ownership(void)
{
    C2Expression source, *copy;
    union { unsigned alignment; unsigned char bytes[36]; } storage;
    unsigned char expected[36];
    unsigned kind, size;
    for (kind = 1; kind <= 7; kind += 6) {
        size = kind == 1 ? 28 : 32;
        memset(&source, 0xa5, sizeof(source));
        source.kind = kind;
        source.next = &source;
        memset(&storage, 0xa5, sizeof(storage));
        ((C2SmallNode *)&storage)->next = NULL;
        free_small_nodes[size] = (C2SmallNode *)&storage;
        memcpy(expected, storage.bytes, sizeof(expected));
        memcpy(expected, &source, size);
        memset(expected, 0, 4);
        expected[16] &= kind == 1 ? 0xcf : 0xef;
        if (kind == 1) expected[17] &= 0xef;
        copy = copy_input_expression(&source);
        if (copy != (C2Expression *)&storage ||
            memcmp(storage.bytes, expected, sizeof(expected)) ||
            source.next != &source || source.location.expression.properties != 0xa5 ||
            source.location.expression.comparison != 0xa5) return 0;
    }
    return 1;
}

extern int c2_exception_mode;
static int input_effect_flags(void)
{
    C2Expression tree;
    unsigned char flags;
    memset(&tree, 0, sizeof(tree));
    tree.kind = 2;
    tree.location.expression.properties = 0x10;
    if (input_has_side_effects(&tree)) return 0;
    tree.location.expression.properties |= 0x40;
    if (input_has_side_effects(&tree) != 1) return 0;
    tree.location.expression.properties = 0;
    tree.location.expression.comparison = 8;
    if (input_has_side_effects(&tree) != 1) return 0;
    tree.location.expression.comparison = 0x40;
    if (input_has_side_effects(&tree)) return 0;
    tree.kind = 19;
    tree.opcode = 0x175;
    c2_exception_mode = 0;
    if (input_has_side_effects(&tree) != 1) return 0;
    c2_exception_mode = 1;
    if (input_has_side_effects(&tree)) return 0;
    tree.kind = 17;
    tree.detail.intrinsic = 0;
    flags = intrinsics[0].flags;
    intrinsics[0].flags = 0;
    if (input_has_side_effects(&tree) != 1) return 0;
    intrinsics[0].flags = 0x20;
    if (input_has_side_effects(&tree)) return 0;
    intrinsics[0].flags = flags;
    return 1;
}

static int symbol_use_ownership(void)
{
    C2Expression symbol_record, use, *operand;
    union { unsigned alignment; unsigned char bytes[32]; } operand_storage;
    C2InputSymbol symbol;
    union { unsigned alignment; unsigned char bytes[24]; } storage;
    C2SymbolInputUse *entry = (C2SymbolInputUse *)&storage;
    unsigned i;
    memset(&symbol_record, 0xa5, sizeof(symbol_record));
    memset(&symbol, 0, sizeof(symbol));
    memset(&storage, 0xa5, sizeof(storage));
    entry->next = NULL;
    free_small_nodes[20] = (C2SmallNode *)entry;
    symbol_record.declaration = &symbol;
    symbol_record.tail.symbol_uses = (C2SymbolInputUse *)0x13579bdfU;
    symbol.storage.input_references = 0xffffffffU;
    bind_symbol_input_use(&symbol_record, &use);
    if (symbol_record.tail.symbol_uses != entry ||
        entry->next != (C2SymbolInputUse *)0x13579bdfU ||
        entry->expression != &use || entry->kind != 27 || entry->pending04 ||
        entry->pending10 || symbol.storage.input_references || free_small_nodes[20])
        return 0;
    for (i = 20; i < sizeof(storage.bytes); ++i)
        if (storage.bytes[i] != 0xa5) return 0;
    if (entry->pending09[0] || entry->pending09[1] || entry->pending09[2]) return 0;
    memset(&operand_storage, 0xa5, sizeof(operand_storage));
    ((C2SmallNode *)&operand_storage)->next = NULL;
    free_small_nodes[28] = (C2SmallNode *)&operand_storage;
    symbol.storage.input_references = 0;
    operand = resolve_symbol_input_operand(&symbol_record, -1);
    if (operand != (C2Expression *)&operand_storage || operand->kind != 4 ||
        operand->opcode != 0x12a || operand->value.symbol != &symbol.symbol ||
        symbol.storage.input_references != 0xffffffffU || free_small_nodes[28]) return 0;
    for (i = 28; i < sizeof(operand_storage.bytes); ++i)
        if (operand_storage.bytes[i] != 0xa5) return 0;
    return 1;
}

static int generic_pointer_types(void)
{
    C2Expression record, right, left, boundary, parameter, real;
    C2RegisterBlock block;
    C2FunctionRegister right_reg, left_reg;
    C2RegisterBlock *saved = first_function_register_block;
    int flags = function_input_flags;
    memset(&record, 0, sizeof(record));
    memset(&right, 0, sizeof(right));
    memset(&left, 0, sizeof(left));
    memset(&boundary, 0, sizeof(boundary));
    memset(&parameter, 0, sizeof(parameter));
    memset(&real, 0, sizeof(real));
    memset(&block, 0, sizeof(block));
    memset(&right_reg, 0, sizeof(right_reg));
    memset(&left_reg, 0, sizeof(left_reg));
    record.flags = 1;
    record.opcode = 0x151;
    record.type = right.type = left.type = parameter.type = 0x3004;
    right.kind = left.kind = 1;
    right.value.reg = &right_reg;
    left.value.reg = &left_reg;
    right_reg.type = left_reg.type = 0x3004;
    right_reg.flags = left_reg.flags = 0x40;
    left.location.expression.properties = 0x20;
    record.tail.right = &right;
    record.left = &left;
    record.next = &boundary;
    boundary.kind = 22;
    boundary.left = &parameter;
    boundary.next = &real;
    real.flags = 1;
    real.type = 0x4008;
    block.cursor = block.records + 3;
    block.records[0].type = block.records[1].type = 0x3004;
    block.records[1].kind = 0x10;
    block.records[2].type = 0x1004;
    first_function_register_block = &block;
    function_input_flags = 0;
    validate_function_code((C2Record *)&record);
    first_function_register_block = saved;
    if (function_input_flags != 1) return 0;
    function_input_flags = flags;
    return record.type == 0x2004 && right.type == 0x2004 && left.type == 0x1004 &&
        right_reg.type == 0x2004 && left_reg.type == 0x1004 &&
        right_reg.flags == 0x48 && left_reg.flags == 0x48 &&
        left.location.expression.properties == 0x20 && parameter.type == 0x1004 &&
        real.type == 0x4008 && block.records[0].type == 0x2004 &&
        block.records[1].type == 0x3004 && block.records[2].type == 0x1004;
}

extern int retain_function_input_pool;

static int pending_input_cleanup(void)
{
    C2InputStack first, second;
    C2PendingInput pending;
    C2Expression left, right;
    C2SmallNode *saved = free_small_nodes[32];
    int retain = retain_function_input_pool;
    memset(&left, 0, sizeof(left));
    memset(&right, 0, sizeof(right));
    left.kind = right.kind = 7;
    first.next = &second;
    second.next = NULL;
    first.first = &left;
    second.first = &right;
    free_input_stacks = &first;
    pending_input_records = &pending;
    retain_function_input_pool = 1;
    free_small_nodes[32] = NULL;
    finish_function_input();
    if (free_input_stacks || pending_input_records != &pending || free_small_nodes[32] != (C2SmallNode *)&right ||
        right.next != &left || left.next) return 0;
    free_small_nodes[32] = saved;
    retain_function_input_pool = retain;
    return 1;
}

static int pool_release_ownership(void)
{
    C2Pool pool;
    C2PoolBlock first, last, old;
    C2PoolBlock *saved = c2_free_blocks;
    C2PoolBlock *large = malloc(5008);
    if (!large) return 0;
    memset(&first, 0, sizeof(first));
    memset(&last, 0, sizeof(last));
    memset(&old, 0, sizeof(old));
    first.capacity = last.capacity = 4088;
    large->capacity = 5000;
    first.next = large;
    large->next = &last;
    last.next = NULL;
    pool.blocks = &first;
    pool.cursor = (unsigned char *)0x13579bdfU;
    pool.remaining = 177;
    c2_free_blocks = &old;
    release_input_pool(&pool);
    if (c2_free_blocks != &first || first.next != &last || last.next != &old ||
        pool.blocks != &first || pool.cursor != (unsigned char *)0x13579bdfU ||
        pool.remaining != 177 || first.capacity != 4088 || last.capacity != 4088)
        return 0;
    c2_free_blocks = saved;
    return 1;
}

static int inline_temporary_lifetimes(void)
{
    C2Function function;
    C2FlowGraph flow;
    C2BasicBlock block;
    C2Expression record, next, input, output, definition;
    C2FunctionRegister registers[2];
    int count = 99;
    memset(&function, 0, sizeof(function));
    memset(&flow, 0, sizeof(flow));
    memset(&block, 0, sizeof(block));
    memset(&record, 0, sizeof(record));
    memset(&next, 0, sizeof(next));
    memset(&input, 0, sizeof(input));
    memset(&output, 0, sizeof(output));
    memset(registers, 0, sizeof(registers));
    function.flow = &flow;
    flow.last = &block;
    block.records = (C2Record *)&record;
    record.flags = next.flags = 1;
    record.kind = next.kind = 11;
    record.next = &next;
    input.kind = output.kind = 1;
    input.value.reg = &registers[0];
    output.value.reg = &registers[1];
    registers[0].value = registers[1].value = &definition;
    record.left = &input;
    record.tail.right = &output;
    if (collect_inline_calls(&function, &count, 0) || count ||
        record.control_flags != 0xff || next.control_flags) return 0;
    input.kind = 3;
    registers[0].canonical = &registers[0];
    registers[0].variant_next = &registers[1];
    registers[0].view_flags = 0x80;
    registers[1].view_flags = 0x40;
    record.tail.right = NULL;
    record.opcode = 0x151;
    if (collect_inline_calls(&function, &count, 0) || count ||
        registers[0].view_flags != 0x80 || registers[1].view_flags != 0x40) return 0;
    record.opcode = 0x131;
    return !collect_inline_calls(&function, &count, 0) && !count &&
        registers[0].view_flags == 0x82 && registers[1].view_flags == 0x42;
}

static int input_pool_reset_writer(unsigned pool)
{
    C2Pool saved = c2_pools[pool];
    C2RegisterSet *head = free_register_sets[1];
    C2RegisterSet *head4 = free_register_sets[4];
    C2RegisterSetNode *nodes = free_register_set_nodes[1];
    C2RegisterSetNode *tail = last_free_register_set_nodes[1];
    c2_pools[pool].blocks = NULL;
    c2_pools[pool].cursor = (unsigned char *)0x12345678U;
    c2_pools[pool].remaining = 171;
    free_register_sets[pool] = (C2RegisterSet *)0x11111111U;
    last_free_register_sets[pool] = (C2RegisterSet *)0x22222222U;
    free_register_set_nodes[pool] = (C2RegisterSetNode *)0x33333333U;
    last_free_register_set_nodes[pool] = (C2RegisterSetNode *)0x44444444U;
    free_register_sets[11] = (C2RegisterSet *)0x55667788U;
    last_free_register_set_nodes[5] = (C2RegisterSetNode *)0x66778899U;
    reset_input_pool(pool);
    if (c2_pools[pool].blocks || c2_pools[pool].cursor || c2_pools[pool].remaining ||
        free_register_sets[pool] || last_free_register_sets[pool] ||
        free_register_set_nodes[pool] || last_free_register_set_nodes[pool]) return 0;
    c2_pools[pool] = saved;
    return free_register_sets[1] == head && free_register_sets[4] == head4 &&
        free_register_set_nodes[1] == nodes && last_free_register_set_nodes[1] == tail &&
        free_register_sets[11] == (C2RegisterSet *)0x55667788U &&
        last_free_register_set_nodes[5] == (C2RegisterSetNode *)0x66778899U;
}

static int input_pool_twelve_reset(void) { return input_pool_reset_writer(12); }
static int input_pool_six_reset(void) { return input_pool_reset_writer(6); }

static int symbol_boundary_full_width(void)
{
    C2Expression record;
    C2InputSymbol symbol;
    unsigned char expected[sizeof(symbol)];
    int saved = group_optimized_symbols;
    memset(&record, 0, sizeof(record));
    memset(&symbol, 0xa5, sizeof(symbol));
    record.declaration = &symbol;
    symbol.detail.variable.flags = 0;
    symbol.symbol.source_kind = 5;
    group_optimized_symbols = 0x12345678;
    memcpy(expected, &symbol, sizeof(symbol));
    if (symbol_range_boundary(&record) != 0x12345678 ||
        memcmp(expected, &symbol, sizeof(symbol))) return 0;
    symbol.symbol.source_kind = 12;
    if (symbol_range_boundary(&record) != 0x12345678) return 0;
    group_optimized_symbols = -1;
    symbol.symbol.source_kind = 7;
    if (symbol_range_boundary(&record) != -1) return 0;
    symbol.detail.variable.flags = 0x80;
    if (symbol_range_boundary(&record) != 1) return 0;
    symbol.detail.variable.flags = 0;
    symbol.symbol.source_kind = 0xff;
    if (symbol_range_boundary(&record)) return 0;
    group_optimized_symbols = 0;
    symbol.symbol.source_kind = 0;
    if (symbol_range_boundary(&record) != 1) return 0;
    group_optimized_symbols = saved;
    return 1;
}

static int block_scope_and_flags(void)
{
    C2Function function;
    C2FlowGraph flow;
    C2BasicBlock block;
    C2Record records[5], before[5];
    memset(&function, 0, sizeof(function));
    memset(&flow, 0, sizeof(flow));
    memset(&block, 0xa5, sizeof(block));
    memset(records, 0, sizeof(records));
    function.flow = &flow;
    flow.first = flow.last = &block;
    block.records = records;
    block.last_record = &records[4];
    block.flags = 0xa0020000;
    records[0].next = &records[1];
    records[1].next = &records[2];
    records[2].next = &records[3];
    records[3].next = &records[4];
    records[1].kind = records[2].kind = 19;
    records[1].opcode = 0x161;
    records[2].opcode = 0x163;
    records[3].kind = 16;
    records[3].opcode = 0x15b;
    memcpy(before, records, sizeof(records));
    partition_function_blocks(&function);
    return block.scope_depth == 1 && block.flags == 0xa1020000 &&
        !memcmp(records, before, sizeof(records)) &&
        block.pending08[0] == 0xa5 && block.pending24[95] == 0xa5;
}

static int flow_edge_reuse_ownership(void)
{
    C2FlowGraph flow;
    C2FlowEdge edge, next, outgoing, incoming, *result;
    C2BasicBlock source, destination;
    memset(&flow, 0, sizeof(flow));
    memset(&edge, 0xa5, sizeof(edge));
    memset(&source, 0xa5, sizeof(source));
    memset(&destination, 0xa5, sizeof(destination));
    flow.free_edges = &edge;
    edge.next_outgoing = &next;
    source.outgoing = &outgoing;
    destination.incoming = &incoming;
    result = new_flow_edge(&flow, &source, &destination);
    return result == &edge && flow.free_edges == &next &&
        edge.source == &source && edge.destination == &destination &&
        edge.next_outgoing == &outgoing && edge.next_incoming == &incoming &&
        source.outgoing == &edge && destination.incoming == &edge &&
        edge.flags == 0xa5a5a5a5 && source.pending10[7] == 0xa5 &&
        destination.pending10[0] == 0xa5;
}

extern int c2_alias_optimization, optimizer_context_mode;
static int optimizer_scope_and_fallback(void)
{
    C2Expression boundary[2], *records;
    unsigned count, i;
    int aliases = c2_alias_optimization, mode = optimizer_context_mode;
    memset(boundary, 0, sizeof(boundary));
    boundary[0].next = &boundary[1];
    boundary[0].kind = boundary[1].kind = 21;
    boundary[0].opcode = boundary[1].opcode = 0x17e;
    boundary[0].value.pointer = (void *)0x12345678U;
    if (count_optimizer_records((C2Record *)boundary) != 2) return 0;
    records = calloc(10001, sizeof(*records));
    if (!records) return 0;
    for (i = 0; i < 10001; ++i) {
        records[i].flags = 1;
        records[i].opcode = 0x151;
        records[i].next = i < 10000 ? &records[i + 1] : NULL;
    }
    c2_alias_optimization = 0x13579bdf;
    optimizer_context_mode = -1;
    count = count_optimizer_records((C2Record *)records);
    free(records);
    i = count == 1 && c2_alias_optimization == 0 && optimizer_context_mode == 1;
    c2_alias_optimization = aliases;
    optimizer_context_mode = mode;
    return i;
}

extern unsigned optimizer_register_count, optimizer_index_limit;
static int optimizer_alias_group_ownership(void)
{
    C2RegisterBlock block, *saved = first_function_register_block;
    C2InputSymbol symbol;
    C2FunctionRegister *reg;
    unsigned result;
    int valid;
    memset(&block, 0, sizeof(block));
    memset(&symbol, 0, sizeof(symbol));
    symbol.symbol.name = "auxiliary optimizer parameter";
    block.cursor = &block.records[2];
    reg = &block.records[0];
    reg->kind = 4;
    reg->canonical = reg;
    reg->symbol = &symbol.symbol;
    block.records[1].kind = 16;
    block.records[1].optimizer_index = 0x13579bdf;
    first_function_register_block = &block;
    reset_input_pool(6);
    optimizer_index_limit = 10;
    result = assign_optimizer_registers(0, 0);
    valid = result == 2 && optimizer_register_count == 2 && optimizer_index_limit == 11 &&
        symbol.detail.optimizer_number == 10 && reg->optimizer_index == 1 &&
        (reg->view_flags & 8) && reg->optimizer && reg->optimizer->number == 1 &&
        reg->optimizer->aliases && reg->optimizer->aliases->pool == 6 &&
        reg->optimizer->aliases->links.first &&
        reg->optimizer->aliases->links.first->group == 0 &&
        reg->optimizer->aliases->links.first->bits == 2 &&
        !reg->optimizer->secondary && block.records[1].optimizer_index == 0x13579bdf;
    first_function_register_block = saved;
    return valid;
}

static int optimizer_operand_numbers(void)
{
    C2Expression record, operands[2];
    C2FunctionRegister reg;
    C2OptimizerRegisterInfo info;
    unsigned result;
    memset(&record, 0, sizeof(record));
    memset(operands, 0xa5, sizeof(operands));
    memset(&reg, 0, sizeof(reg));
    memset(&info, 0, sizeof(info));
    reset_input_pool(6);
    info.aliases = new_register_set(6);
    reg.view_flags = 8;
    reg.optimizer = &info;
    record.flags = 1;
    record.tail.right = operands;
    operands[0].kind = 2;
    operands[0].value.reg = &reg;
    operands[0].next = &operands[1];
    operands[1].kind = 7;
    operands[1].next = NULL;
    result = number_optimizer_operands((C2Record *)&record, 6);
    return result == 7 && operands[0].optimizer_alias_number == 6 &&
        operands[1].optimizer_alias_number == 0 &&
        info.aliases->links.first && info.aliases->links.first->group == 0 &&
        info.aliases->links.first->bits == 0x40 &&
        operands[0].control_flags == 0xa5 && operands[1].control_flags == 0xa5;
}

static int optimizer_set_difference(void)
{
    C2RegisterSet destination, source;
    C2RegisterSetNode output[2], input[3], before[3];
    memset(&destination, 0xa5, sizeof(destination));
    memset(&source, 0x5a, sizeof(source));
    output[0].group = 0; output[0].bits = 0xf0000001U; output[0].next = &output[1];
    output[1].group = 64; output[1].bits = 0x80000003U; output[1].next = NULL;
    input[0].group = 31; input[0].bits = 0xf0000000U; input[0].next = &input[1];
    input[1].group = 32; input[1].bits = 0xffffffffU; input[1].next = &input[2];
    input[2].group = 64; input[2].bits = 0x80000000U; input[2].next = NULL;
    destination.links.first = output;
    source.links.first = input;
    memcpy(before, input, sizeof(input));
    subtract_optimizer_symbol_sets(&destination, &source);
    return destination.links.first == output && output[0].bits == 1 &&
        output[1].bits == 3 && output[0].next == &output[1] && !output[1].next &&
        destination.state == 0xa5a5a5a5U && source.state == 0x5a5a5a5aU &&
        !memcmp(input, before, sizeof(input));
}

static int optimizer_set_union(void)
{
    C2RegisterSet destination, source;
    C2RegisterSetNode existing, input[3], before[3], *node;
    memset(&destination, 0, sizeof(destination));
    memset(&source, 0, sizeof(source));
    reset_input_pool(6);
    destination.pool = 6;
    destination.state = 0x87654321;
    destination.links.first = &existing;
    existing.group = 64; existing.bits = 1; existing.next = NULL;
    input[0].group = 0; input[0].bits = 0x80000000U; input[0].next = &input[1];
    input[1].group = 64; input[1].bits = 2; input[1].next = &input[2];
    input[2].group = 96; input[2].bits = 0x12345678; input[2].next = NULL;
    source.links.first = input;
    memcpy(before, input, sizeof(input));
    merge_optimizer_symbol_sets(&destination, &source);
    node = destination.links.first;
    return node && node != &existing && node->group == 0 && node->bits == 0x80000000U &&
        node->next == &existing && existing.bits == 3 && existing.next &&
        existing.next->group == 96 && existing.next->bits == 0x12345678 &&
        !existing.next->next && destination.state == 0x87654321 &&
        !memcmp(input, before, sizeof(input));
}

static int optimizer_set_copy_ownership(void)
{
    C2RegisterSet source, *result;
    C2RegisterSetNode input[2], before[2], *node;
    memset(&source, 0, sizeof(source));
    reset_input_pool(6);
    source.pool = 6;
    source.state = 0xdeadbeef;
    input[0].group = 0; input[0].bits = 0x12345678; input[0].next = &input[1];
    input[1].group = 64; input[1].bits = 0x80000000U; input[1].next = NULL;
    source.links.first = input;
    memcpy(before, input, sizeof(input));
    result = copy_optimizer_symbol_set(&source);
    node = result->links.first;
    return result != &source && result->pool == 6 && result->state == 0 &&
        node && node != input && node->group == 0 && node->bits == 0x12345678 &&
        node->next && node->next != &input[1] && node->next->group == 64 &&
        node->next->bits == 0x80000000U && !node->next->next &&
        source.state == 0xdeadbeef && !memcmp(input, before, sizeof(input));
}

static int optimizer_set_assignment_reuse(void)
{
    C2RegisterSet source, destination;
    C2RegisterSetNode input[2], before[2], existing;
    memset(&source, 0, sizeof(source));
    memset(&destination, 0, sizeof(destination));
    reset_input_pool(6);
    source.pool = destination.pool = 6;
    destination.state = 0xdeadbeef;
    input[0].group = 0; input[0].bits = 0x12345678; input[0].next = &input[1];
    input[1].group = 96; input[1].bits = 0x80000000U; input[1].next = NULL;
    existing.group = 64; existing.bits = 7; existing.next = NULL;
    source.links.first = input;
    destination.links.first = &existing;
    memcpy(before, input, sizeof(input));
    copy_optimizer_symbol_set_into(&destination, &source);
    return destination.links.first == &existing && existing.group == 0 &&
        existing.bits == 0x12345678 && existing.next && existing.next != &input[1] &&
        existing.next->group == 96 && existing.next->bits == 0x80000000U &&
        !existing.next->next && destination.state == 0xdeadbeef &&
        !memcmp(input, before, sizeof(input));
}

static int optimizer_set_release_queues(void)
{
    C2RegisterSet existing, live;
    C2RegisterSetNode previous, nodes[2];
    memset(&existing, 0, sizeof(existing));
    memset(&live, 0, sizeof(live));
    memset(&previous, 0, sizeof(previous));
    reset_input_pool(6);
    existing.pool = live.pool = 6;
    live.state = 0xab123456;
    nodes[0].group = 0; nodes[0].bits = 0x13579bdf; nodes[0].next = &nodes[1];
    nodes[1].group = 64; nodes[1].bits = 0x80000000U; nodes[1].next = NULL;
    live.links.first = nodes;
    free_register_sets[6] = last_free_register_sets[6] = &existing;
    free_register_set_nodes[6] = last_free_register_set_nodes[6] = &previous;
    release_optimizer_symbol_set(&live);
    release_optimizer_symbol_set(NULL);
    return free_register_sets[6] == &existing && last_free_register_sets[6] == &live &&
        existing.links.next_free == &live && !live.links.next_free &&
        live.state == 0xab000000 && free_register_set_nodes[6] == &previous &&
        previous.next == nodes && last_free_register_set_nodes[6] == &nodes[1] &&
        nodes[0].next == &nodes[1] && !nodes[1].next &&
        nodes[0].bits == 0x13579bdf && nodes[1].bits == 0x80000000U;
}

static int optimizer_scope_parameter_identity(void)
{
    C2Function function;
    C2FunctionSymbol symbol;
    C2BasicBlock block;
    C2Expression before, after, *begin, *end;
    C2FunctionRegister original, selected, inherited, *parameter;
    C2Symbol local;
    C2DebugLink symbols;
    C2OptimizerUse old_use, *use;
    unsigned result;
    extern C2FunctionRegister *optimizer_parameter_register;
    memset(&function, 0, sizeof(function));
    memset(&symbol, 0, sizeof(symbol));
    memset(&block, 0, sizeof(block));
    memset(&before, 0, sizeof(before));
    memset(&after, 0, sizeof(after));
    memset(&original, 0, sizeof(original));
    memset(&selected, 0, sizeof(selected));
    memset(&inherited, 0, sizeof(inherited));
    memset(&local, 0, sizeof(local));
    function.symbol = &symbol;
    function.source_line_base = 100;
    current_function = &function;
    reset_input_pool(4);
    reset_input_pool(6);
    original.optimizer_uses = &old_use;
    old_use.next = NULL;
    old_use.reg = &inherited;
    selected.view_flags = 2;
    local.input_register = &selected;
    symbols.next = NULL;
    symbols.symbol = &local;
    optimizer_parameter_register = &original;
    begin = new_control_expression(0x17e);
    end = new_control_expression(0x17f);
    begin->value.pointer = end->value.pointer = &symbols;
    begin->location.line = 17;
    before.next = begin;
    begin->previous = &before;
    begin->next = end;
    end->previous = begin;
    end->next = &after;
    after.previous = end;
    block.records = (C2Record *)begin;
    block.last_record = (C2Record *)&after;
    result = translate_optimizer_input(&block, 2, 0, 0);
    parameter = optimizer_parameter_register;
    use = parameter->optimizer_uses;
    return result == 4 && parameter != &original && parameter->kind == 12 &&
        parameter->previous_parameter == &original && use && use != &old_use &&
        use->reg == &inherited && use->next && use->next->reg == &selected &&
        !use->next->next && old_use.reg == &inherited && !old_use.next &&
        before.next == &after && after.previous == &before &&
        record_line_delta == 17 && record_source_line == 117 &&
        !block.alias_in && !block.alias_uses && !block.alias_kills && !block.alias_out;
}

static int optimizer_full_width_membership(void)
{
    C2RegisterSet set;
    C2RegisterSetNode nodes[2], before[2];
    memset(&set, 0, sizeof(set));
    nodes[0].group = 0;
    nodes[0].bits = 0x80000001U;
    nodes[0].next = &nodes[1];
    nodes[1].group = 0xffffffe0U;
    nodes[1].bits = 0x80000000U;
    nodes[1].next = NULL;
    set.links.first = nodes;
    memcpy(before, nodes, sizeof(nodes));
    return optimizer_set_contains(&set, 0) == 1 &&
        optimizer_set_contains(&set, 31) == 0x80000000U &&
        optimizer_set_contains(&set, 32) == 0 &&
        optimizer_set_contains(&set, 0xffffffffU) == 0x80000000U &&
        !memcmp(nodes, before, sizeof(nodes));
}

static int optimizer_iterator_continuation(void)
{
    C2RegisterSet set;
    C2RegisterSetNode nodes[2], before[2];
    memset(&set, 0, sizeof(set));
    nodes[0].group = 0;
    nodes[0].bits = 0x80000001U;
    nodes[0].next = &nodes[1];
    nodes[1].group = 96;
    nodes[1].bits = 2;
    nodes[1].next = NULL;
    set.links.first = nodes;
    memcpy(before, nodes, sizeof(nodes));
    if (next_optimizer_symbol(&set) != 0 || next_optimizer_symbol(NULL) != 31 ||
        next_optimizer_symbol(NULL) != 97 || next_optimizer_symbol(NULL) != ~0U ||
        memcmp(nodes, before, sizeof(nodes))) return 0;
    nodes[0].bits = 0;
    return next_optimizer_symbol(&set) == ~0U && next_optimizer_symbol(NULL) == 97;
}

static int optimizer_use_pointer_identity(void)
{
    C2FunctionRegister first, equal, ignored;
    C2OptimizerUse existing, *result;
    memset(&first, 0, sizeof(first));
    memset(&equal, 0, sizeof(equal));
    memset(&ignored, 0, sizeof(ignored));
    existing.reg = &first;
    existing.next = NULL;
    ignored.kind = 10;
    if (add_optimizer_use(&existing, &first, 4) != &existing ||
        add_optimizer_use(&existing, &ignored, 4) != &existing) return 0;
    ignored.kind = 11;
    if (add_optimizer_use(&existing, &ignored, 4) != &existing) return 0;
    reset_input_pool(4);
    result = add_optimizer_use(&existing, &equal, 4);
    return result && result != &existing && result->reg == &equal &&
        result->next == &existing && existing.reg == &first && !existing.next;
}

static int optimizer_dense_header_reuse(void)
{
    C2RegisterSet first, second, source, *result;
    C2Pool unknown[3];
    unsigned old_words[2] = {0x13579bdfU, 0x80000000U};
    unsigned accounting[4];
    unsigned incoming[2] = {0xfffffffeU, 0x80000000U};
    memset(&first, 0xa5, sizeof(first));
    memset(&second, 0, sizeof(second));
    memset(&source, 0x6b, sizeof(source));
    first.links.next_free = &second;
    first.state = 0xab123456;
    free_register_sets[8] = &first;
    last_free_register_sets[8] = &second;
    free_register_set_nodes[8] = (C2RegisterSetNode *)0x12345678U;
    last_free_register_set_nodes[8] = (C2RegisterSetNode *)0x23456789U;
    memset(&c2_pools[9], 0x6c, sizeof(unknown));
    memcpy(unknown, &c2_pools[9], sizeof(unknown));
    memcpy(accounting, c2_node_bytes, sizeof(accounting));
    result = new_optimizer_dense_set(8, 33);
    if (result != &first || result->pool != 8 || result->state != 0xab000021U ||
        result->pending05[0] != 0xa5 || result->pending05[1] != 0xa5 ||
        result->pending05[2] != 0xa5 || !result->links.words ||
        result->links.words[0] || result->links.words[1] ||
        free_register_sets[8] != &second || last_free_register_sets[8] != &second ||
        memcmp(unknown, &c2_pools[9], sizeof(unknown)) ||
        memcmp(accounting, c2_node_bytes, sizeof(accounting))) return 0;
    add_optimizer_dense_member(result, 0);
    add_optimizer_dense_member(result, 31);
    add_optimizer_dense_member(result, 32);
    if (result->links.words[0] != 0x80000001U || result->links.words[1] != 1 ||
        optimizer_dense_contains(result, 0) != 1 || optimizer_dense_contains(result, 31) != 1 ||
        optimizer_dense_contains(result, 32) != 1 || optimizer_dense_contains(result, 1)) return 0;
    source.links.words = incoming;
    source.state = 0xcd000021U;
    merge_optimizer_dense_sets(result, &source);
    if (result->links.words[0] != 0xffffffffU || result->links.words[1] != 0x80000001U ||
        result->state != 0xab000021U || source.state != 0xcd000021U ||
        incoming[0] != 0xfffffffeU || incoming[1] != 0x80000000U) return 0;
    incoming[0] = 0xffffffffU;
    incoming[1] = 0x80000001U;
    if (!optimizer_dense_sets_equal(result, &source)) return 0;
    source.state = 0xcd000022U;
    if (optimizer_dense_sets_equal(result, &source)) return 0;
    source.state = 0xcd000021U;
    incoming[1] ^= 0x80000000U;
    if (optimizer_dense_sets_equal(result, &source)) return 0;
    source.links.words = NULL;
    second.links.words = NULL;
    source.state = 0xcd000000U;
    second.state = 0xab000000U;
    if (!optimizer_dense_sets_equal(&source, &second)) return 0;
    result->links.words[0] = 0x12345678U;
    result->links.words[1] = 0x80000000U;
    fill_optimizer_dense_set(result, 33);
    if (result->links.words[0] != 0xffffffffU || result->links.words[1] != 0x80000001U ||
        result->state != 0xab000021U) return 0;
    fill_optimizer_dense_set(result, 32);
    if (result->links.words[0] != 0xffffffffU || result->links.words[1] != 0x80000001U ||
        result->state != 0xab000021U) return 0;
    source.links.words = incoming;
    source.state = 0xcd000021U;
    incoming[0] = 0x80000001U;
    incoming[1] = 0x80000000U;
    intersect_optimizer_dense_sets(result, &source);
    if (result->links.words[0] != 0x80000001U || result->links.words[1] != 0x80000000U ||
        incoming[0] != 0x80000001U || incoming[1] != 0x80000000U ||
        result->state != 0xab000021U || source.state != 0xcd000021U) return 0;
    incoming[0] = 0x2468ace0U;
    incoming[1] = 0x40000001U;
    copy_optimizer_dense_set(result, &source);
    if (result->links.words == incoming || result->links.words[0] != 0x2468ace0U ||
        result->links.words[1] != 0x40000001U || incoming[0] != 0x2468ace0U ||
        incoming[1] != 0x40000001U || result->state != 0xab000021U ||
        source.state != 0xcd000021U) return 0;
    first.links.words = old_words;
    release_optimizer_set_header(&first);
    return free_register_sets[8] == &second && second.links.next_free == &first &&
        last_free_register_sets[8] == &first && !first.links.next_free &&
        first.state == 0xab000000U && old_words[0] == 0x13579bdfU &&
        old_words[1] == 0x80000000U &&
        free_register_set_nodes[8] == (C2RegisterSetNode *)0x12345678U &&
        last_free_register_set_nodes[8] == (C2RegisterSetNode *)0x23456789U;
}

void __fastcall mark_reachable_flow_blocks(C2FlowGraph *flow);
void __fastcall number_function_blocks(C2Function *function);
void __fastcall compute_block_dominators(C2Function *function);
void __fastcall compute_block_predecessors(C2Function *function);

static int reachable_flow_postorder(void)
{
    C2Function function;
    C2FlowGraph flow;
    C2BasicBlock blocks[5];
    C2FlowEdge edges[5], before[5];
    unsigned i;
    memset(&flow, 0, sizeof(flow));
    memset(blocks, 0, sizeof(blocks));
    memset(edges, 0, sizeof(edges));
    flow.last = blocks;
    for (i = 0; i < 5; ++i) {
        blocks[i].previous = i < 4 ? &blocks[i + 1] : NULL;
        blocks[i].flags = 0x80000001U | (i << 4);
    }
    edges[0].source = &blocks[0];
    edges[0].destination = &blocks[1];
    edges[0].next_outgoing = &edges[1];
    edges[1].source = &blocks[0];
    edges[1].destination = &blocks[2];
    edges[2].source = &blocks[1];
    edges[2].destination = &blocks[3];
    edges[3].source = &blocks[2];
    edges[3].destination = &blocks[3];
    edges[4].source = &blocks[3];
    edges[4].destination = &blocks[0];
    edges[2].next_incoming = &edges[3];
    blocks[0].incoming = &edges[4];
    blocks[1].incoming = &edges[0];
    blocks[2].incoming = &edges[1];
    blocks[3].incoming = &edges[2];
    blocks[0].outgoing = &edges[0];
    blocks[1].outgoing = &edges[2];
    blocks[2].outgoing = &edges[3];
    blocks[3].outgoing = &edges[4];
    memcpy(before, edges, sizeof(edges));
    mark_reachable_flow_blocks(&flow);
    if (flow.traversal_first != &blocks[3] || flow.traversal_last != &blocks[0] ||
        blocks[3].traversal_previous || blocks[3].traversal_next != &blocks[1] ||
        blocks[1].traversal_previous != &blocks[3] || blocks[1].traversal_next != &blocks[2] ||
        blocks[2].traversal_previous != &blocks[1] || blocks[2].traversal_next != &blocks[0] ||
        blocks[0].traversal_previous != &blocks[2] || blocks[0].traversal_next ||
        memcmp(edges, before, sizeof(edges))) return 0;
    for (i = 0; i < 5; ++i)
        if (blocks[i].flags != (0x80000000U | (i << 4) | (i < 4))) return 0;
    blocks[3].previous = NULL;
    memset(&function, 0, sizeof(function));
    function.flow = &flow;
    reset_input_pool(4);
    reset_input_pool(12);
    number_function_blocks(&function);
    compute_block_dominators(&function);
    compute_block_predecessors(&function);
    if (flow_block_count != 4 || blocks[0].dominators->links.words[0] != 1 ||
        blocks[1].dominators->links.words[0] != 3 || blocks[2].dominators->links.words[0] != 5 ||
        blocks[3].dominators->links.words[0] != 9) return 0;
    for (i = 0; i < 4; ++i)
        if (blocks[i].predecessors->links.words[0] != 15) return 0;
    return !memcmp(edges, before, sizeof(edges));
}

static int small_node_list_recycling(void)
{
    C2SmallNode *first, *second, *third;
    clear_function_input_state();
    first = allocate_small_node(24);
    second = allocate_small_node(21);
    third = allocate_small_node(24);
    first->next = second;
    second->next = third;
    third->next = NULL;
    release_small_node_list(first);
    if (free_small_nodes[24] != third || third->next != first || first->next ||
        free_small_nodes[28] != second || second->next) return 0;
    release_small_node_list(NULL);
    return free_small_nodes[24] == third && free_small_nodes[28] == second;
}

static int input_use_owner_identity(void)
{
    C2InputSymbol symbol;
    C2Expression use, owners[3], before[3];
    C2SymbolInputUse *nodes[3];
    unsigned i;
    memset(&symbol, 0, sizeof(symbol));
    memset(&use, 0, sizeof(use));
    memset(owners, 0x6b, sizeof(owners));
    memcpy(before, owners, sizeof(owners));
    clear_function_input_state();
    for (i = 0; i < 3; ++i) {
        nodes[i] = (C2SymbolInputUse *)allocate_small_node(27);
        nodes[i]->expression = &owners[i];
    }
    nodes[0]->next = nodes[1];
    nodes[1]->next = nodes[2];
    nodes[2]->next = NULL;
    use.declaration = &symbol;
    use.tail.symbol_uses = nodes[0];
    symbol.input_operand = &use;
    symbol.storage.input_references = 0x80000002U;
    unlink_input_use(symbol.input_uses, &owners[0]);
    if (use.tail.symbol_uses != nodes[1] || symbol.storage.input_references != 0x80000001U)
        return 0;
    unlink_input_use(symbol.input_uses, &owners[2]);
    return symbol.input_operand == &use && use.tail.symbol_uses == nodes[1] && !nodes[1]->next &&
        symbol.storage.input_references == 0x80000000U && free_small_nodes[20] == (C2SmallNode *)nodes[2] &&
        nodes[2]->next == nodes[0] && !nodes[0]->next && !memcmp(owners, before, sizeof(owners));
}

static int flow_recycling_lists(void)
{
    C2FlowGraph flow;
    C2BasicBlock blocks[4];
    C2FlowEdge edges[4];
    memset(&flow, 0, sizeof(flow));
    memset(blocks, 0, sizeof(blocks));
    memset(edges, 0, sizeof(edges));
    edges[0].source = edges[1].source = &blocks[0];
    edges[0].destination = edges[1].destination = &blocks[1];
    edges[0].next_outgoing = edges[0].next_incoming = &edges[1];
    blocks[0].outgoing = blocks[1].incoming = &edges[0];
    flow.free_edges = &edges[2];
    edges[1].flags = 0x87654321U;
    release_flow_edge(&flow, &edges[1]);
    if (edges[0].next_outgoing || edges[0].next_incoming ||
        flow.free_edges != &edges[1] || edges[1].next_outgoing != &edges[2] ||
        edges[1].flags != 0x87654321U || edges[1].source != &blocks[0] ||
        edges[1].destination != &blocks[1]) return 0;
    release_flow_edge(&flow, &edges[0]);
    if (blocks[0].outgoing || blocks[1].incoming ||
        flow.free_edges != &edges[0] || edges[0].next_outgoing != &edges[1]) return 0;
    flow.first = &blocks[0];
    flow.last = &blocks[2];
    flow.free_blocks = &blocks[3];
    blocks[0].next = &blocks[1];
    blocks[1].previous = &blocks[0];
    blocks[1].next = &blocks[2];
    blocks[2].previous = &blocks[1];
    blocks[1].flags = 0x12345678;
    recycle_flow_block(&flow, &blocks[1]);
    if (blocks[0].next != &blocks[2] || blocks[2].previous != &blocks[0] ||
        flow.free_blocks != &blocks[1] || blocks[1].previous != &blocks[3] ||
        blocks[1].flags != 0x12345678) return 0;
    recycle_flow_block(&flow, &blocks[0]);
    if (flow.first != &blocks[2] || blocks[2].previous) return 0;
    recycle_flow_block(&flow, &blocks[2]);
    return !flow.first && !flow.last && flow.free_blocks == &blocks[2] &&
        blocks[2].previous == &blocks[0] && blocks[0].previous == &blocks[1];
}

static int expression_dispatch_owner_branch(void)
{
    C2Function function;
    C2FlowGraph flow;
    C2BasicBlock block;
    C2Record head, end;
    C2Expression statement, result;
    C2FunctionRegister reg;
    memset(&function, 0, sizeof(function));
    memset(&flow, 0, sizeof(flow));
    memset(&block, 0, sizeof(block));
    memset(&head, 0, sizeof(head));
    memset(&end, 0, sizeof(end));
    memset(&statement, 0, sizeof(statement));
    memset(&result, 0, sizeof(result));
    memset(&reg, 0, sizeof(reg));
    function.flow = &flow;
    flow.first = flow.last = &block;
    block.records = &head;
    block.last_record = &end;
    head.next = (C2Record *)&statement;
    statement.next = (C2Expression *)&end;
    statement.kind = 11;
    statement.opcode = 0x147;
    statement.tail.right = &result;
    statement.left = &result;
    result.kind = 1;
    result.value.reg = &reg;
    reg.kind = 3;
    reg.value = &statement;
    /* This definition must be skipped; entering it reaches a pending child. */
    simplify_function_expressions(&function, 1);
    if (reg.value != &statement || statement.control_flags) return 0;
    /* A NULL owner admits the statement, whose empty argument list returns. */
    reg.value = NULL;
    statement.left = NULL;
    simplify_function_expressions(&function, 2);
    return !reg.value && statement.tail.right == &result && !statement.control_flags;
}

void __fastcall prune_detached_operands(C2Expression *expression);
static int detached_operand_roles(void)
{
    C2Expression expression, arguments[4], results[2];
    unsigned i;
    memset(&expression, 0, sizeof(expression));
    memset(arguments, 0, sizeof(arguments));
    memset(results, 0, sizeof(results));
    for (i = 0; i != 3; ++i) arguments[i].next = &arguments[i + 1];
    arguments[0].kind = 4;
    arguments[1].kind = 1;
    arguments[2].kind = 6;
    arguments[3].kind = 3;
    arguments[1].location.expression.properties = 0x30;
    arguments[2].location.expression.properties = 0x60;
    arguments[2].location.expression.comparison = 0x81;
    results[0].kind = 5;
    results[0].next = &results[1];
    results[0].location.expression.comparison = 0x41;
    results[1].kind = 2;
    results[1].location.expression.comparison = 0x82;
    expression.left = &arguments[0];
    expression.tail.right = &results[0];
    prune_detached_operands(&expression);
    return expression.left == &arguments[0] && arguments[0].next == &arguments[3] &&
        !arguments[1].next && !arguments[2].next && !arguments[3].next &&
        arguments[1].location.expression.properties == 0x30 &&
        arguments[2].location.expression.properties == 0x60 &&
        arguments[2].location.expression.comparison == 0xa1 &&
        results[0].location.expression.comparison == 0x61 &&
        results[1].location.expression.comparison == 0x82 &&
        results[0].next == &results[1] && expression.tail.right == &results[0];
}

extern int expression_rewrite_changed;
void __fastcall apply_integer_mask(C2Expression *expression, unsigned __int64 mask);
static int integer_mask_representatives(void)
{
    C2Expression literal, owner, saved;
    memset(&literal, 0xa5, sizeof(literal));
    memset(&owner, 0, sizeof(owner));
    literal.kind = 7;
    literal.opcode = 0x11d;
    literal.type = 0x2004;
    literal.integer.value = 0x12345678;
    saved = literal;
    owner.opcode = 0x136;
    owner.left = &literal;
    expression_rewrite_changed = 0;
    apply_integer_mask(&owner, 0xff);
    if (literal.integer.value != 0x78 || !expression_rewrite_changed ||
        owner.left != &literal || owner.opcode != 0x136) return 0;
    saved.integer.value = 0x78;
    if (memcmp(&literal, &saved, sizeof(literal))) return 0;
    literal.type = 0x1004;
    literal.integer.value = (unsigned __int64)(__int64)-128;
    saved = literal;
    expression_rewrite_changed = 0;
    apply_integer_mask(&literal, 0xff);
    return !memcmp(&literal, &saved, sizeof(literal)) && !expression_rewrite_changed;
}

static int expression_order_stability(void)
{
    C2Expression nodes[7], before[7], *head;
    static const unsigned keys[7] = {4, 0xffffffffU, 4, 0, 0x80000000U, 1, 4};
    static const unsigned sequence[7] = {1, 4, 0, 2, 6, 5, 3};
    unsigned i;
    if (sort_expression_inputs(NULL, compare_expression_keys)) return 0;
    memset(nodes, 0xa5, sizeof(nodes));
    for (i = 0; i < 7; ++i) {
        nodes[i].next = i == 6 ? NULL : &nodes[i + 1];
        nodes[i].order_key = keys[i];
    }
    memcpy(before, nodes, sizeof(nodes));
    head = sort_expression_inputs(nodes, compare_expression_keys);
    for (i = 0; i < 7; ++i) {
        if (head != &nodes[sequence[i]]) return 0;
        head = head->next;
    }
    if (head) return 0;
    for (i = 0; i < 7; ++i) {
        if (memcmp((unsigned char *)&nodes[i] + sizeof(nodes[i].next),
                   (unsigned char *)&before[i] + sizeof(before[i].next),
                   sizeof(nodes[i]) - sizeof(nodes[i].next))) return 0;
    }
    return sort_expression_inputs(&nodes[3], compare_expression_keys) == &nodes[3] &&
           !nodes[3].next;
}

static int plain_register_release_variants(void)
{
    C2FunctionRegister records[3], before[3], old_head;
    unsigned i;
    memset(records, 0xa5, sizeof(records));
    memset(&old_head, 0x5a, sizeof(old_head));
    for (i = 0; i < 3; ++i) {
        records[i].kind = 3;
        records[i].variant_next = i == 2 ? NULL : &records[i + 1];
    }
    memcpy(before, records, sizeof(records));
    free_plain_register_nodes = &old_head;
    release_function_register(records);
    if (free_plain_register_nodes != &records[2]) return 0;
    for (i = 0; i < 3; ++i) {
        before[i].kind = 0x10;
        before[i].next_free = &old_head;
        if (memcmp(&records[i], &before[i], sizeof(records[i]))) return 0;
    }
    return 1;
}

void initialize_factoring_storage(void);
void __fastcall record_factor_operand(C2Expression *expression);
int __fastcall compare_factor_benefit(C2Expression *left, C2Expression *right);
extern unsigned factoring_opcode, factoring_marker;

static int factor_count_weight_order(void)
{
    C2Expression values[2], before[2];
    reset_input_pool(4);
    initialize_factoring_storage();
    memset(values, 0, sizeof(values));
    values[0].kind = values[1].kind = 11;
    values[0].opcode = 0x130;
    values[1].opcode = 0x131;
    values[0].order_key = 1;
    values[1].order_key = 0x00ff0001;
    memcpy(before, values, sizeof(values));
    factoring_opcode = 0x143;
    factoring_marker = 0x137;
    record_factor_operand(&values[0]);
    record_factor_operand(&values[0]);
    record_factor_operand(&values[0]);
    record_factor_operand(&values[1]);
    record_factor_operand(&values[1]);
    if (compare_factor_benefit(&values[0], &values[1]) != -1 ||
        compare_factor_benefit(&values[1], &values[0]) != 1) return 0;
    record_factor_operand(&values[1]);
    return !compare_factor_benefit(&values[0], &values[1]) &&
           !memcmp(values, before, sizeof(values));
}

static int register_mark_flow_owner(void)
{
    C2Function function;
    C2FlowGraph flow;
    C2BasicBlock last, first;
    C2Expression last_record, first_record;
    C2FunctionRegister registers[2], before[2];
    memset(&function, 0, sizeof(function));
    memset(&flow, 0, sizeof(flow));
    memset(&last, 0, sizeof(last));
    memset(&first, 0, sizeof(first));
    memset(&last_record, 0, sizeof(last_record));
    memset(&first_record, 0, sizeof(first_record));
    memset(registers, 0, sizeof(registers));
    last_record.kind = first_record.kind = 2;
    last_record.value.reg = &registers[0];
    first_record.value.reg = &registers[1];
    registers[0].flags = 0x80;
    registers[1].flags = 0x40;
    memcpy(before, registers, sizeof(registers));
    function.flow = &flow;
    flow.last = &last;
    flow.first = &first;
    last.records = (C2Record *)&last_record;
    first.records = (C2Record *)&first_record;
    first_function_register_block = NULL;
    if (release_unreferenced_temporaries(&function)) return 0;
    before[0].flags |= 2;
    return !memcmp(registers, before, sizeof(registers));
}

C2Expression *__fastcall fold_integer_operation(C2Expression *expression,
    unsigned short type, C2Expression *left, C2Expression *right);
static int integer_fold_zero_divisor(void)
{
    C2Expression operation, operands[2], before[2];
    unsigned opcode, mode;
    memset(&operation, 0xa7, sizeof(operation));
    memset(operands, 0x5c, sizeof(operands));
    operands[0].kind = operands[1].kind = 7;
    operands[0].integer.low = 0x89abcdefU;
    operands[0].integer.high = 0xfedcba98U;
    operands[1].integer.value = 0;
    memcpy(before, operands, sizeof(operands));
    for (opcode = 0x149; opcode <= 0x14a; ++opcode)
        for (mode = 1; mode <= 2; ++mode) {
            operation.opcode = opcode;
            if (fold_integer_operation(&operation, (unsigned short)((mode << 12) | 8),
                    &operands[0], &operands[1])) return 0;
            if (memcmp(before, operands, sizeof(operands))) return 0;
        }
    return 1;
}

static int global_initial_set_owner(void)
{
    C2Function function;
    C2FlowGraph flow;
    C2BasicBlock block;
    C2RegisterSet initial;

    memset(&function, 0, sizeof(function));
    memset(&flow, 0, sizeof(flow));
    memset(&block, 0, sizeof(block));
    memset(&initial, 0, sizeof(initial));
    function.flow = &flow;
    flow.last = &block;
    first_function_register_block = NULL;
    secondary_register_set = &initial;
    next_function_register_number = 0x12345678;
    reset_input_pool(13);
    initialize_global_registers(&function);
    return global_initial_registers == &initial &&
        next_function_register_number == 0x12345678;
}

static int global_register_old_bit_width(void)
{
    C2RegisterSet set;
    C2RegisterSetNode node;

    memset(&set, 0, sizeof(set));
    memset(&node, 0, sizeof(node));
    set.pool = 1;
    set.state = 0x55aa1100U;
    set.links.first = &node;
    node.bits = 0x40000001U;
    if (test_and_add_global_register(&set, 31) != 0) return 0;
    if (test_and_add_global_register(&set, 31) != 0x80000000U) return 0;
    return node.bits == 0xc0000001U && node.group == 0 && !node.next &&
        set.pool == 1 && set.state == 0x55aa1100U && set.links.first == &node;
}

static int final_register_scratch_bounds(void)
{
    C2RegisterBlock block, expected;
    C2RegisterBlock *saved = first_function_register_block;
    C2Function function;
    C2FlowGraph flow;

    memset(&block, 0xa5, sizeof(block));
    memset(&function, 0, sizeof(function));
    memset(&flow, 0, sizeof(flow));
    block.next = NULL;
    block.cursor = block.records + 2;
    block.end = block.records + 32;
    block.records[0].kind = 3;
    block.records[1].kind = 16;
    expected = block;
    memset(&expected.records[0].pending2c, 0, 36);
    function.flow = &flow;
    first_function_register_block = &block;
    simplify_final_blocks(&function);
    first_function_register_block = saved;
    return !memcmp(&block, &expected, sizeof(block));
}

static int final_zero_bits_variant_bounds(void)
{
    C2FunctionRegister rows[4], expected[4];
    C2Expression record, result;
    unsigned i;

    memset(rows, 0xa5, sizeof(rows));
    memset(&record, 0, sizeof(record));
    memset(&result, 0, sizeof(result));
    for (i = 0; i != 4; ++i) {
        rows[i].variant_next = i == 3 ? NULL : &rows[i + 1];
        rows[i].displacement = 0;
        rows[i].value_bytes = 4;
    }
    rows[0].canonical = rows;
    rows[1].value_bytes = 2;
    rows[2].displacement = 1;
    rows[3].value_bytes = 8;
    memcpy(expected, rows, sizeof(rows));
    expected[0].known_zero_bits = 16;
    expected[1].known_zero_bits = 0;
    result.type = 0x2004;
    result.value.reg = rows;
    record.tail.right = &result;
    propagate_final_zero_bits(&record, 16);
    return !memcmp(rows, expected, sizeof(rows));
}

static int final_required_bits_shared_default(void)
{
    C2Expression record, output, before_record, before_output;
    C2FunctionRegister reg, before_reg;
    memset(&record, 0xa5, sizeof(record));
    memset(&output, 0x5a, sizeof(output));
    memset(&reg, 0x3c, sizeof(reg));
    record.tail.right = &output;
    output.value.reg = &reg;
    reg.unused_high_bits = 0x91;
    record.opcode = 0x131;
    before_record = record;
    before_output = output;
    before_reg = reg;
    if (required_final_input_bits(&record) != 0x91) return 0;
    if (memcmp(&record, &before_record, sizeof(record)) ||
        memcmp(&output, &before_output, sizeof(output)) ||
        memcmp(&reg, &before_reg, sizeof(reg))) return 0;
    record.opcode = 0x145;
    if (required_final_input_bits(&record)) return 0;
    record.opcode = 0;
    return required_final_input_bits(&record) == 0;
}

static int final_multiplier_hash_identity(void)
{
    C2FinalMultiplyRecipe *one, *negative, *collision;
    int remaining;
    initialize_final_arithmetic();
    one = find_final_multiplier_recipe(1);
    negative = find_final_multiplier_recipe(-1);
    remaining = c2_pools[1].remaining;
    collision = find_final_multiplier_recipe(512);
    if (!one || !negative || !collision || one == negative ||
        collision == one || collision == negative) return 0;
    if (c2_pools[1].remaining != remaining - 20) return 0;
    remaining = c2_pools[1].remaining;
    if (find_final_multiplier_recipe(1) != one ||
        find_final_multiplier_recipe(-1) != negative ||
        find_final_multiplier_recipe(512) != collision) return 0;
    return c2_pools[1].remaining == remaining;
}

static int final_address_constant_wrap(void)
{
    C2Expression before, after;
    C2Expression *record = (C2Expression *)allocate_small_node(11);
    C2Expression *definition = (C2Expression *)allocate_small_node(11);
    memset(&before, 0, sizeof(before));
    memset(&after, 0, sizeof(after));
    record->type = definition->type = 0x1004;
    record->opcode = 0x131;
    definition->opcode = 0x131;
    definition->left = new_integer_expression(0x1004, 0x7fffffff);
    definition->previous = &before;
    definition->next = &after;
    before.next = after.previous = definition;
    expand_final_operand(record, definition, NULL, 1, NULL);
    return record->opcode == 1 && record->type == 0x1004 &&
        record->left && record->left->kind == 7 && !record->left->next &&
        record->left->integer.value == 0xffffffff80000000ULL &&
        before.next == &after && after.previous == &before;
}

C2Expression *__fastcall final_record_uses_machine_register(C2Expression *record,
    C2MachineRegister *reg);

static int fixed_register_overlap_boundaries(void)
{
    C2Expression record, operands[2];
    C2FunctionRegister views[2];
    C2MachineRegister *fixed = &machine_registers[1];

    if (fixed->canonical != (C2FunctionRegister *)fixed ||
        fixed->number != 1 || fixed->bit_width != 32 || fixed->bit_displacement) return 0;
    memset(&record, 0, sizeof(record));
    memset(operands, 0, sizeof(operands));
    memset(views, 0, sizeof(views));
    record.left = &operands[0];
    operands[0].kind = operands[1].kind = 1;
    operands[0].next = &operands[1];
    operands[0].original_register = &views[0];
    operands[1].original_register = &views[1];
    views[0].canonical = views[1].canonical = fixed->canonical;
    views[0].displacement = 32;
    views[0].value_bytes = 1;
    views[1].displacement = 31;
    views[1].value_bytes = 1;
    if (final_record_uses_machine_register(&record, fixed) != &operands[1]) return 0;
    views[1].displacement = -1;
    if (final_record_uses_machine_register(&record, fixed)) return 0;
    views[1].value_bytes = 2;
    if (final_record_uses_machine_register(&record, fixed) != &operands[1]) return 0;
    views[1].canonical = &views[1];
    if (final_record_uses_machine_register(&record, fixed)) return 0;
    operands[0].original_register = (C2FunctionRegister *)fixed;
    return final_record_uses_machine_register(&record, fixed) == &operands[0];
}

C2Expression *__fastcall find_final_register_output(C2Expression *record,
    C2FunctionRegister *reg);

static int fixed_output_byte_alias(void)
{
    C2Expression record, operand;
    C2FunctionRegister adjacent;
    C2FunctionRegister *root = (C2FunctionRegister *)&machine_registers[2];
    C2FunctionRegister *byte = (C2FunctionRegister *)&machine_registers[18];

    if (root->canonical != root || root->value_bytes != 32 || root->displacement ||
        byte->canonical != root || byte->value_bytes != 8 || byte->displacement) return 0;
    memset(&record, 0, sizeof(record));
    memset(&operand, 0, sizeof(operand));
    memset(&adjacent, 0, sizeof(adjacent));
    record.tail.right = &operand;
    operand.kind = 1;
    operand.original_register = byte;
    if (find_final_register_output(&record, root) != &operand ||
        find_final_register_output(&record, byte) != &operand ||
        find_final_register_output(&record, (C2FunctionRegister *)&machine_registers[1])) return 0;
    adjacent.canonical = root;
    adjacent.value_bytes = 8;
    adjacent.displacement = 8;
    operand.original_register = &adjacent;
    return !find_final_register_output(&record, byte) &&
        find_final_register_output(&record, root) == &operand;
}

static int machine_sentinel_identity(void)
{
    unsigned char *end = (unsigned char *)(machine_registers + C2_MACHINE_REGISTER_COUNT);

    return end == (unsigned char *)&machine_register_storage.sentinel &&
        &machine_register_storage.sentinel.kind == end + 4 &&
        ((C2FunctionRegister *)end)->kind == 2;
}

static int symbol_iterator_chain_and_end(void)
{
    C2ScopeHash scope, empty;
    C2Symbol first, second, last;
    memset(&scope, 0, sizeof(scope));
    memset(&empty, 0, sizeof(empty));
    memset(&first, 0, sizeof(first));
    memset(&second, 0, sizeof(second));
    memset(&last, 0, sizeof(last));
    scope.buckets[0] = &first;
    first.hash_next = &second;
    scope.buckets[1023] = &last;
    if (next_scope_symbol(&scope, NULL) != &first ||
        next_scope_symbol(&scope, &first) != &second ||
        next_scope_symbol(&scope, &second) != &last ||
        next_scope_symbol(&scope, &last) != NULL) return 0;
    if (next_scope_symbol(&empty, NULL) != NULL) return 0;
    return next_scope_symbol(&scope, NULL) == &first;
}

extern void __fastcall clear_frame_symbol_state(C2Symbol *symbol);
static int frame_symbol_flag_scope(void)
{
    C2InputSymbol symbol, expected;
    unsigned i;
    for (i = 0; i < 3; ++i) {
        memset(&symbol, 0xa5, sizeof(symbol));
        symbol.symbol.kind = i == 2 ? 4 : 1;
        symbol.attributes = i == 1 ? 0x20 : 0x10;
        symbol.symbol.flags = ~0U;
        expected = symbol;
        if (i == 0) expected.symbol.flags &= ~0x800U;
        clear_frame_symbol_state(&symbol.symbol);
        if (memcmp(&symbol, &expected, sizeof(symbol))) return 0;
    }
    return 1;
}

extern C2FunctionRegister *frame_register_chain, *frame_extra_slot;
extern C2Symbol *frame_reserved_symbol;
extern int frame_register_count, c2_exception_mode;
extern void __fastcall register_frame_variable(C2FunctionRegister *reg, int references);
static int frame_register_weight_order(void)
{
    C2FunctionRegister a, b, c;
    memset(&a, 0, sizeof(a)); memset(&b, 0, sizeof(b)); memset(&c, 0, sizeof(c));
    a.value_bytes = c.value_bytes = 4;
    b.value_bytes = 8;
    frame_register_chain = NULL;
    frame_register_count = 0;
    frame_reserved_symbol = NULL;
    frame_extra_slot = NULL;
    c2_exception_mode = 0;
    register_frame_variable(&a, 1);
    register_frame_variable(&b, 2);
    register_frame_variable(&c, 3);
    if (frame_register_chain != &a || a.frame_next != &c ||
        c.frame_previous != &a || c.frame_next != &b || b.frame_previous != &c) return 0;
    register_frame_variable(&c, 1);
    return frame_register_count == 3 && frame_register_chain == &c &&
        c.frame_uses == 4 && c.frame_previous == NULL && c.frame_next == &a &&
        a.frame_previous == &c && a.frame_next == &b && b.frame_previous == &a &&
        b.frame_next == NULL;
}

extern C2BasicBlock *__fastcall find_register_save_region(C2BasicBlock *,
    C2BasicBlock *, C2BasicBlock **);
static int register_save_region_boundaries(void)
{
    C2BasicBlock f, a, b, l, outside, *end;
    C2FlowEdge incoming, fa, fb, al, bl;
    memset(&f, 0, sizeof(f)); memset(&a, 0, sizeof(a));
    memset(&b, 0, sizeof(b)); memset(&l, 0, sizeof(l));
    memset(&outside, 0, sizeof(outside));
    memset(&incoming, 0, sizeof(incoming)); memset(&fa, 0, sizeof(fa));
    memset(&fb, 0, sizeof(fb)); memset(&al, 0, sizeof(al)); memset(&bl, 0, sizeof(bl));
    f.previous = &a; a.previous = &b; b.previous = &l;
    a.next = &f; b.next = &a; l.next = &b;
    f.number84 = 1; a.number84 = 2; b.number84 = 3; l.number84 = 4;
    f.incoming = &incoming;
    f.outgoing = &fa; fa.next_outgoing = &fb;
    fa.source = fb.source = &f; fa.destination = &a; fb.destination = &b;
    b.incoming = &fb; b.outgoing = &bl;
    l.incoming = &al; al.next_incoming = &bl;
    al.source = &a; al.destination = &l; bl.source = &b; bl.destination = &l;
    end = &outside;
    if (find_register_save_region(&f, &l, &end) != &b || end != &a) return 0;
    al.source = &outside;
    end = &outside;
    return find_register_save_region(&f, &l, &end) == NULL && end == &outside;
}

extern void __fastcall number_final_peephole_records(C2Function *function);
static int final_peephole_numbering_scope(void)
{
    C2Function function;
    C2FlowGraph flow;
    C2BasicBlock block;
    C2Expression records[5], expected[5];
    int i;
    memset(&function, 0, sizeof(function)); memset(&flow, 0, sizeof(flow));
    memset(&block, 0, sizeof(block)); memset(records, 0xa5, sizeof(records));
    function.flow = &flow; flow.last = flow.first = &block;
    block.records = (C2Record *)&records[0]; block.last_record = (C2Record *)&records[3];
    for (i = 0; i < 4; ++i) { records[i].next = &records[i + 1]; records[i].flags = 0; records[i].kind = 22; }
    records[1].kind = 25;
    records[2].flags = 1;
    memcpy(expected, records, sizeof(records));
    expected[1].peephole_number = 1; expected[2].peephole_number = 2;
    number_final_peephole_records(&function);
    return !memcmp(records, expected, sizeof(records));
}

/* This auxiliary context checks the newly reached canonical-root domains;
 * it does not claim execution of the pending peephole input transformer. */
static int flags_and_sentinel_roots(void)
{
    C2Expression record, operands[3];
    C2FunctionRegister view;
    C2FunctionRegister *flags = (C2FunctionRegister *)&machine_registers[66];
    C2FunctionRegister *end = (C2FunctionRegister *)(machine_registers + C2_MACHINE_REGISTER_COUNT);

    if (machine_registers[66].definition || machine_registers[5].canonical != (C2FunctionRegister *)&machine_registers[5] ||
        machine_register_storage.sentinel.canonical != end ||
        (unsigned char *)&machine_register_storage.sentinel.canonical != (unsigned char *)end + 8)
        return 0;
    memset(&record, 0, sizeof(record));
    memset(operands, 0, sizeof(operands));
    memset(&view, 0, sizeof(view));
    record.left = record.tail.right = operands;
    operands[0].kind = operands[1].kind = operands[2].kind = 1;
    operands[0].original_register = end;
    operands[1].original_register = (C2FunctionRegister *)&machine_registers[5];
    operands[2].original_register = &view;
    operands[0].next = &operands[1];
    operands[1].next = &operands[2];
    view.canonical = flags;
    view.value_bytes = 1;
    view.displacement = 4;
    if (final_record_uses_machine_register(&record, &machine_registers[66]) ||
        find_final_register_output(&record, flags)) return 0;
    view.displacement = 3;
    if (final_record_uses_machine_register(&record, &machine_registers[66]) != &operands[2] ||
        find_final_register_output(&record, flags) != &operands[2]) return 0;
    operands[2].original_register = flags;
    return final_record_uses_machine_register(&record, &machine_registers[66]) == &operands[2] &&
        find_final_register_output(&record, flags) == &operands[2];
}

C2FunctionRegister *__fastcall add_final_cached_value(C2FunctionRegister *, C2FunctionRegister *, C2Expression *);
C2Expression *__fastcall select_final_cached_value(C2Expression *, C2FunctionRegister **);
static int final_cache_equivalence_identity(void)
{
    C2FunctionRegister regs[3], *head, *out;
    C2Expression values[3], record, input, output;
    unsigned i;
    memset(regs, 0xa5, sizeof(regs));
    memset(values, 0xa5, sizeof(values));
    head = add_final_cached_value(NULL, &regs[0], &values[0]);
    if (head != &regs[0] || head->cache_group_next || head->cache_equivalent_next ||
        head->cache_values != &values[0] || values[0].cached_next) return 0;
    regs[0].cache_equivalent_next = &regs[1];
    regs[1].cache_equivalent_next = NULL;
    regs[1].cache_values = &values[0];
    if (add_final_cached_value(head, &regs[1], &values[1]) != head ||
        regs[0].cache_values != &values[1] || regs[1].cache_values != &values[1] ||
        values[1].cached_next != &values[0] || values[0].cached_next) return 0;
    head = add_final_cached_value(head, &regs[2], &values[2]);
    if (head != &regs[2] || head->cache_group_next != &regs[0] ||
        head->cache_equivalent_next || head->cache_values != &values[2] ||
        values[2].cached_next || regs[0].cache_values != &values[1]) return 0;
    for (i = 0; i < sizeof(regs); ++i)
        if (i % sizeof(regs[0]) < 44 || i % sizeof(regs[0]) >= 56)
            if (((unsigned char *)regs)[i] != 0xa5) return 0;
    memset(&record, 0, sizeof(record));
    memset(&input, 0, sizeof(input));
    memset(&output, 0, sizeof(output));
    record.opcode = 1;
    record.left = &input;
    record.tail.right = &output;
    output.kind = 1;
    output.original_register = &regs[2];
    out = &regs[1];
    input.kind = 2;
    if (select_final_cached_value(&record, &out) || out != &regs[1]) return 0;
    input.kind = 7;
    if (select_final_cached_value(&record, &out) != &input || out != &regs[2]) return 0;
    input.kind = 3;
    return select_final_cached_value(&record, &out) == &input && out == &regs[2];
}

static int schedule_resource_initialization_scope(void)
{
    C2Function function;
    C2Symbol symbol;
    C2ScheduleWeights *weights[4];
    C2ScheduleOpcodeCost *costs[4];
    C2ScheduleSelector selectors[4];
    unsigned i, j;
    memset(&function, 0, sizeof(function));
    memset(&symbol, 0, sizeof(symbol));
    function.flags = 0x18;
    symbol.flags = 4;
    current_symbol = &symbol;
    schedule_final_mode = 0;
    for (i = 0; i < 4; ++i) {
        memset(schedule_resources, 0xa5, sizeof(schedule_resources));
        c2_cpu_generation = i + 3;
        initialize_schedule_function(&function);
        if (schedule_cpu_index != i || schedule_function_flag8 != 1 ||
            schedule_function_flag16 != 1 || schedule_aggregate_return != 1 ||
            schedule_parameter_4921f8 ||
            schedule_issue_width != (i == 0 ? 2 : i == 1 ? 1 : i == 2 ? 2 : 3) ||
            schedule_resource_count != (i == 1 ? 2 : 4)) return 0;
        weights[i] = schedule_weights;
        costs[i] = schedule_opcode_costs;
        selectors[i] = schedule_select_node;
        if (schedule_weights->fixed_resource_weight != (i == 3 ? 23 : -1) ||
            schedule_opcode_costs->result_delay) return 0;
        for (j = 0; j < (unsigned)schedule_resource_count; ++j)
            if (schedule_resources[j].capacity != (i == 3 ? 3 : 1) ||
                schedule_resources[j].spacing != (i == 3 ? (j == 1 ? 3 : j == 2 ? 2 : 1) :
                    (i != 1 && j == 2 ? 3 : 1)) ||
                schedule_resources[j].issued || schedule_resources[j].occupied ||
                schedule_resources[j].pending06 != 0xa5a5) return 0;
        if (i == 1)
            for (j = 2 * sizeof(schedule_resources[0]); j < sizeof(schedule_resources); ++j)
                if (((unsigned char *)schedule_resources)[j] != 0xa5) return 0;
    }
    if (weights[1] != weights[0] + 1 || weights[3] != weights[0] + 3 ||
        costs[0] == costs[1] || costs[1] == costs[2] || costs[2] != costs[3] ||
        selectors[0] != selectors[2] || selectors[0] == selectors[1] ||
        selectors[1] == selectors[3]) return 0;
    symbol.flags = 8;
    initialize_schedule_function(&function);
    return !schedule_aggregate_return;
}

static int schedule_dependency_disjoint_roots(void)
{
    C2ScheduleRegisterUse use;
    C2ScheduleNode written, current;
    memset(&written, 0xa5, sizeof(written));
    memset(&current, 0x5a, sizeof(current));
    use.reg = (C2FunctionRegister *)&machine_registers[3];
    use.node = &written;
    use.next = NULL;
    schedule_register_writes = &use;
    add_schedule_register_dependencies((C2FunctionRegister *)&machine_registers[4], &current, 1);
    return schedule_register_writes == &use && use.reg == (C2FunctionRegister *)&machine_registers[3] &&
        use.node == &written && !use.next && written.flags3a == 0xa5 && current.flags3a == 0x5a;
}
static int schedule_operand_cost_widths(void)
{
    C2ScheduleNode node;
    C2Expression record, inputs[2], output;
    memset(&node, 0, sizeof(node));
    memset(&record, 0, sizeof(record));
    memset(inputs, 0, sizeof(inputs));
    memset(&output, 0, sizeof(output));
    node.record = &record;
    record.left = inputs;
    inputs[0].next = &inputs[1];
    record.tail.right = &output;
    record.opcode = 0x25;
    inputs[1].kind = 7;
    inputs[1].integer.low = 1;
    if (schedule_node_operand_cost(&node)) return 0;
    inputs[1].integer.high = 1;
    if (schedule_node_operand_cost(&node) != 7) return 0;
    inputs[1].kind = 1;
    node.properties = 0x40;
    if (schedule_node_operand_cost(&node) != 8) return 0;
    record.opcode = 0xcc;
    if (schedule_node_operand_cost(&node) != 5) return 0;
    record.opcode = 0x21;
    node.properties = 0x20;
    if (schedule_node_operand_cost(&node) != 1) return 0;
    node.properties = 0x40;
    if (schedule_node_operand_cost(&node) != 2) return 0;
    record.opcode = 0x1f;
    output.type = 0x1004;
    if (schedule_node_operand_cost(&node) != 24) return 0;
    output.type = 0x1001;
    return schedule_node_operand_cost(&node) == 0 && node.properties == 0x40 &&
        inputs[1].integer.high == 1;
}

extern C2ScheduleGraph *active_schedule_graph;
extern C2ScheduleEdgeHook schedule_output_mode;
extern unsigned schedule_edge_count;
C2ScheduleDependency *__fastcall new_schedule_dependency(C2ScheduleNode *, C2ScheduleNode *, unsigned);
static int schedule_edge_reuse_owned_bytes(void)
{
    C2ScheduleGraph graph;
    C2ScheduleNode source, destination;
    C2ScheduleDependency edge, old_outgoing, old_incoming, *result;
    memset(&graph, 0, sizeof(graph));
    memset(&source, 0xa5, sizeof(source));
    memset(&destination, 0x5a, sizeof(destination));
    memset(&edge, 0x6b, sizeof(edge));
    edge.next_outgoing = NULL;
    edge.properties = 0xf7;
    graph.flow.free_edges = (C2FlowEdge *)&edge;
    source.outgoing = &old_outgoing;
    destination.incoming = &old_incoming;
    source.outgoing_count = 0xffff;
    destination.incoming_count = 0x8000;
    active_schedule_graph = &graph;
    schedule_output_mode = NULL;
    schedule_edge_count = 0xfffffff0U;
    result = new_schedule_dependency(&source, &destination, 0x80000001U);
    return result == &edge && !graph.flow.free_edges &&
        source.outgoing == &edge && destination.incoming == &edge &&
        edge.source == &source && edge.destination == &destination &&
        edge.next_outgoing == &old_outgoing && edge.next_incoming == &old_incoming &&
        edge.flags == 0x80000001U && !edge.delay && edge.properties == 0xe0 &&
        edge.pending17 == 0x6b && !source.outgoing_count &&
        destination.incoming_count == 0x8001 && source.incoming_count == 0xa5a5 &&
        destination.outgoing_count == 0x5a5a && schedule_edge_count == 0xfffffff1U;
}

unsigned __fastcall schedule_dependency_operand_penalty(C2ScheduleDependency *, C2FunctionRegister *);
static int schedule_address_penalty_ownership(void)
{
    C2ScheduleNode first, second;
    C2ScheduleDependency edge;
    C2Expression source, destination, input, output, base, index;
    C2FunctionRegister used, other;
    C2Symbol symbol;
    memset(&first, 0, sizeof(first));
    memset(&second, 0, sizeof(second));
    memset(&edge, 0, sizeof(edge));
    memset(&source, 0, sizeof(source));
    memset(&destination, 0, sizeof(destination));
    memset(&input, 0, sizeof(input));
    memset(&output, 0, sizeof(output));
    memset(&base, 0, sizeof(base));
    memset(&index, 0, sizeof(index));
    memset(&symbol, 0, sizeof(symbol));
    first.record = &source;
    second.record = &destination;
    edge.source = &first;
    edge.destination = &second;
    destination.left = &input;
    destination.tail.right = &output;
    input.kind = 5;
    input.aggregate_value = &base;
    input.aggregate_second = &index;
    base.original_register = &used;
    index.original_register = &other;
    output.kind = 6;
    output.aggregate_value = &index;
    if (schedule_dependency_operand_penalty(&edge, &used) != 1) return 0;
    base.original_register = &other;
    if (schedule_dependency_operand_penalty(&edge, &used)) return 0;
    input.kind = 1;
    index.original_register = &used;
    if (schedule_dependency_operand_penalty(&edge, &used) != 1) return 0;
    destination.left = destination.tail.right = NULL;
    source.opcode = 1;
    source.kind = 13;
    destination.opcode = 0xba;
    symbol.flags = 4;
    current_symbol = &symbol;
    if (schedule_dependency_operand_penalty(&edge, (C2FunctionRegister *)&machine_registers[5]) != 2) return 0;
    source.opcode = 0xb9;
    return !schedule_dependency_operand_penalty(&edge, (C2FunctionRegister *)&machine_registers[5]);
}

extern C2ScheduleRegisterUse *free_schedule_register_uses;
extern C2ScheduleMemory *free_schedule_memory_uses;
void __fastcall clear_schedule_register_uses(void);
void __fastcall release_schedule_memory_uses(C2ScheduleMemory *, C2ScheduleMemory *);
static int schedule_use_recycling_scope(void)
{
    C2ScheduleRegisterUse reads[2], writes[2], old;
    C2ScheduleMemory memory[2], old_memory;
    memset(reads, 0xa5, sizeof(reads));
    memset(writes, 0x5a, sizeof(writes));
    memset(memory, 0x6b, sizeof(memory));
    reads[0].next = &reads[1]; reads[1].next = NULL;
    writes[0].next = &writes[1]; writes[1].next = NULL;
    schedule_register_reads = reads;
    schedule_register_writes = writes;
    free_schedule_register_uses = &old;
    clear_schedule_register_uses();
    if (schedule_register_reads || schedule_register_writes ||
        free_schedule_register_uses != &writes[1] || writes[1].next != &writes[0] ||
        writes[0].next != &reads[1] || reads[1].next != &reads[0] || reads[0].next != &old ||
        (unsigned)reads[0].reg != 0xa5a5a5a5U || (unsigned)writes[1].node != 0x5a5a5a5aU) return 0;
    free_schedule_memory_uses = &old_memory;
    memory[0].next = &memory[1];
    release_schedule_memory_uses(memory, &memory[1]);
    if (free_schedule_memory_uses != memory || memory[0].next != &memory[1] ||
        memory[1].next != &old_memory || (unsigned)memory[1].operand != 0x6b6b6b6bU) return 0;
    release_schedule_memory_uses(NULL, (C2ScheduleMemory *)1);
    return free_schedule_memory_uses == memory;
}

unsigned __fastcall scale_schedule_priority(unsigned, int);
static int schedule_priority_full_width(void)
{
    return scale_schedule_priority(0x80000003U, -1) == 0x40000001U &&
        scale_schedule_priority(0x80000003U, 1) == 6 &&
        scale_schedule_priority(0x80000003U, 0) == 0x80000003U &&
        scale_schedule_priority(0x80000003U, 32) == 0x80000003U &&
        scale_schedule_priority(0x80000003U, -33) == 0x40000001U &&
        scale_schedule_priority(0x80000003U, (int)0x80000000U) == 0x80000003U;
}

extern C2ScheduleNode *schedule_ready_head, *schedule_ready_tail;
extern unsigned schedule_ready_count;
void __fastcall enqueue_schedule_node(C2ScheduleNode *);
void __fastcall rebuild_schedule_ready_queue(void);
static int schedule_ready_order_and_count(void)
{
    C2ScheduleNode nodes[4];
    unsigned i;
    memset(nodes, 0xa5, sizeof(nodes));
    schedule_ready_head = schedule_ready_tail = NULL;
    schedule_ready_count = 0xffffffffU;
    for (i = 0; i < 4; ++i) {
        nodes[i].current_priority = i ? 3 : 0x80000000U;
        nodes[i].number = i == 0 ? 20 : i == 2 ? 1 : 2;
        enqueue_schedule_node(&nodes[i]);
    }
    if (schedule_ready_count != 3 || schedule_ready_head != nodes || schedule_ready_tail != &nodes[3] ||
        nodes[0].ready_next != &nodes[2] || nodes[2].ready_next != &nodes[1] ||
        nodes[1].ready_next != &nodes[3] || nodes[3].ready_next ||
        nodes[3].ready_previous != &nodes[1] || nodes[1].ready_previous != &nodes[2] ||
        nodes[2].ready_previous != nodes || nodes[0].ready_previous) return 0;
    for (i = 0; i < 4; ++i)
        if (nodes[i].pending18 != 0xa5a5a5a5U || nodes[i].cost != 0xa5) return 0;
    schedule_ready_count = 0x80000002U;
    nodes[0].current_priority = 0;
    rebuild_schedule_ready_queue();
    return schedule_ready_count == 0x80000002U && schedule_ready_head == nodes && nodes[0].ready_next == &nodes[2];
}

extern int generate_prologue, generate_frame;
void __fastcall adjust_final_stack_operands(C2Expression *operands, int depth);
/* Distinct flags exercise the inhibited branch without entering its pending
 * adjustment helper. Synthetic contexts do not establish public CLI reach. */
static int stack_operand_mode_owners(void)
{
    C2Expression operand;
    C2FunctionRegister reg;
    memset(&operand, 0xa5, sizeof(operand));
    memset(&reg, 0, sizeof(reg));
    operand.opcode = 0x120;
    operand.next = NULL;
    operand.value.reg = &reg;
    reg.kind = 5;
    generate_frame = 1; generate_prologue = 0;
    adjust_final_stack_operands(&operand, 99);
    if (operand.stack_adjustment != 0 || operand.opcode != 0x120 ||
        operand.value.reg != &reg || operand.kind != 0xa5) return 0;
    operand.stack_adjustment = 0x12345678;
    operand.opcode = 0x124;
    operand.tail.aggregate_register = &reg;
    reg.kind = 3;
    generate_frame = 0; generate_prologue = 1;
    adjust_final_stack_operands(&operand, -123);
    return operand.stack_adjustment == 0 && operand.opcode == 0x124 &&
        operand.tail.aggregate_register == &reg && operand.kind == 0xa5;
}

void __fastcall normalize_output_conditions(C2Function *function);
void finalize_register_displacements(void);
static int output_conditions_and_displacements(void)
{
    C2Function function;
    C2FlowGraph flow;
    C2BasicBlock block;
    C2Expression records[3];
    C2RegisterBlock registers;
    C2FunctionRegister variant;
    C2Symbol symbol;
    memset(&function, 0, sizeof(function));
    memset(&flow, 0, sizeof(flow));
    memset(&block, 0, sizeof(block));
    memset(records, 0xa5, sizeof(records));
    function.flow = &flow; flow.last = &block;
    block.records = (C2Record *)records;
    records[0].next = &records[1]; records[1].next = &records[2]; records[2].next = NULL;
    records[0].kind = records[1].kind = 16; records[2].kind = 14;
    records[0].detail.data = &symbol; records[0].condition_code = 1;
    records[1].detail.data = NULL; records[1].opcode = 0x15c; records[1].condition_code = 16;
    records[2].condition_code = 255;
    normalize_output_conditions(&function);
    if (records[0].condition_code != 4 || records[1].condition_code != 11 ||
        records[2].condition_code != 255 || records[0].pending0b != 0xa5 ||
        records[1].pending0b != 0xa5 || records[0].detail.data != &symbol) return 0;
    memset(&registers, 0, sizeof(registers));
    memset(&variant, 0xa5, sizeof(variant));
    memset(&symbol, 0, sizeof(symbol));
    registers.cursor = registers.records + 2;
    registers.records[0].kind = 3;
    registers.records[0].canonical = &registers.records[0];
    registers.records[0].view_flags = 0x20;
    registers.records[0].frame_displacement = -16;
    registers.records[0].displacement = (long)0x80000008U;
    registers.records[0].variant_next = &variant;
    variant.variant_next = NULL; variant.displacement = 32;
    registers.records[1].kind = 4;
    registers.records[1].canonical = &registers.records[1];
    registers.records[1].symbol = &symbol; symbol.parameter_offset = 12;
    registers.records[1].displacement = -4;
    first_function_register_block = &registers;
    finalize_register_displacements();
    return (unsigned)registers.records[0].displacement == 0x7ffffff8U &&
        registers.records[1].displacement == 8 && variant.displacement == 16 &&
        variant.kind == 0xa5 && variant.number == 0xa5a5a5a5U &&
        registers.records[0].frame_displacement == -16 && symbol.parameter_offset == 12;
}

int __fastcall encode_final_record(C2Expression *record, unsigned char *encoding,
    unsigned char *marks, void *relocations);
static int push_encoding_byte_scope(void)
{
    C2Expression record, input;
    unsigned char bytes[20];
    unsigned rows[4] = {1,4,6,7};
    unsigned encodings[4] = {0x50,0x53,0x55,0x56};
    unsigned i, j;
    memset(&record, 0, sizeof(record));
    memset(&input, 0, sizeof(input));
    record.opcode = 13; record.kind = 11; record.type = 0x1004;
    record.left = &input;
    input.opcode = 0x11f; input.kind = 1;
    for (i = 0; i < 4; ++i) {
        memset(bytes, 0xa5, sizeof(bytes));
        input.original_register = (C2FunctionRegister *)&machine_registers[rows[i]];
        if (encode_final_record(&record, bytes, NULL, NULL) != 1 || bytes[0] != encodings[i]) return 0;
        for (j = 1; j < sizeof(bytes); ++j) if (bytes[j] != 0xa5) return 0;
        if (record.opcode != 13 || record.type != 0x1004 || input.opcode != 0x11f) return 0;
    }
    return 1;
}

static int integer_arithmetic_encoding_recipes(void)
{
    C2Expression record, input, literal, before_input, before_literal;
    unsigned char bytes[20];
    unsigned i, j, count;
    static const struct {
        unsigned opcode, width, immediate, count;
        unsigned char bytes[3];
    } cases[] = {
        {0x21,4,0,1,{0x40}},       /* INC EAX */
        {0x21,1,0,2,{0xfe,0xc0}},  /* INC AL */
        {0xdf,4,0,1,{0x99}},       /* CDQ */
        {0x20,4,0,2,{0xf7,0xf8}},  /* IDIV EAX */
        {0x2d,4,5,3,{0x83,0xc0,5}},/* ADD EAX,5 with signed-byte immediate */
        {0x32,4,5,3,{0x83,0xe8,5}},/* SUB EAX,5 */
        {0x29,4,3,3,{0xc1,0xf8,3}} /* SAR EAX,3 */
    };
    memset(&record, 0, sizeof(record));
    memset(&input, 0, sizeof(input));
    memset(&literal, 0, sizeof(literal));
    record.kind = 11; record.left = &input;
    input.opcode = 0x11f; input.kind = 1;
    input.original_register = (C2FunctionRegister *)&machine_registers[1];
    input.next = &literal;
    literal.opcode = 0x11d; literal.kind = 7;
    for (i = 0; i < sizeof(cases)/sizeof(cases[0]); ++i) {
        memset(bytes, 0xa5, sizeof(bytes));
        record.opcode = cases[i].opcode; record.type = 0x1000 | cases[i].width;
        input.type = record.type; literal.integer.value = cases[i].immediate;
        before_input = input; before_literal = literal;
        count = encode_final_record(&record, bytes, NULL, NULL);
        if (count != cases[i].count || memcmp(bytes, cases[i].bytes, count)) return 0;
        for (j = count; j < sizeof(bytes); ++j) if (bytes[j] != 0xa5) return 0;
        if (memcmp(&input, &before_input, sizeof(input)) ||
            memcmp(&literal, &before_literal, sizeof(literal))) return 0;
    }
    return 1;
}

extern unsigned char *encoding_cursor, *encoding_origin, *encoding_marks;
extern void *encoding_relocations;
void __fastcall emit_encoding_integer(unsigned value, int bytes);
int __fastcall encode_immediate_operand(C2Expression *operand, int bytes, int allow_byte);
static int encoding_integer_width_and_relaxation(void)
{
    unsigned char bytes[16];
    C2Expression operand;
    int width, result;
    unsigned i;
    static const int values[] = {-129,-128,-1,0,127,128};
    memset(&operand, 0, sizeof(operand));
    operand.opcode = 0x11d;
    for (width = 0; width <= 4; ++width) {
        memset(bytes, 0xa5, sizeof(bytes));
        encoding_cursor = bytes + 1;
        emit_encoding_integer(0x89abcdefU, width);
        if (encoding_cursor != bytes + 1 + width) return 0;
        for (i = 0; i < sizeof(bytes); ++i) {
            unsigned char expected = i >= 1 && i <= (unsigned)width && width != 3 ?
                (unsigned char)(0x89abcdefU >> ((i - 1) * 8)) : 0xa5;
            if (bytes[i] != expected) return 0;
        }
    }
    for (i = 0; i < sizeof(values)/sizeof(values[0]); ++i) {
        unsigned j;
        memset(bytes, 0xa5, sizeof(bytes));
        encoding_cursor = encoding_origin = bytes;
        encoding_marks = NULL; encoding_relocations = NULL;
        operand.integer.low = values[i];
        result = encode_immediate_operand(&operand, 4, 1);
        width = values[i] >= -128 && values[i] <= 127 ? 1 : 4;
        if (result != (width == 1 ? 2 : 0) || encoding_cursor != bytes + width) return 0;
        for (j = 0; j < sizeof(bytes); ++j) {
            unsigned char expected = j < (unsigned)width ?
                (unsigned char)((unsigned)values[i] >> (j * 8)) : 0xa5;
            if (bytes[j] != expected) return 0;
        }
    }
    memset(bytes, 0xa5, sizeof(bytes));
    encoding_cursor = encoding_origin = bytes;
    operand.integer.low = 0x89abcdefU;
    if (encode_immediate_operand(&operand, 6, 0) || encoding_cursor != bytes + 6) return 0;
    return bytes[0] == 0 && bytes[1] == 0 && bytes[2] == 0xef && bytes[3] == 0xcd &&
        bytes[4] == 0xab && bytes[5] == 0x89 && bytes[6] == 0xa5 && bytes[15] == 0xa5;
}

extern C2InputLineMap *diagnostic_line_cache;
static int diagnostic_location_bidirectional_cache(void)
{
    C2InputLineMap entries[3], original[3];
    char *file;
    memset(entries, 0, sizeof(entries));
    entries[0].next = &entries[1];
    entries[0].first_line = 1; entries[0].last_line = 3;
    entries[0].source_line = 100; entries[0].filename = "first.c";
    entries[1].next = &entries[2]; entries[1].previous = &entries[0];
    entries[1].first_line = 4; entries[1].last_line = 8;
    entries[1].source_line = 20; entries[1].filename = "second.c";
    entries[2].previous = &entries[1];
    entries[2].first_line = 9; entries[2].last_line = 12;
    entries[2].source_line = 0xfffffff0U; entries[2].filename = "third.c";
    memcpy(original, entries, sizeof(entries));
    input_first_line_map = entries; input_line_cache = &entries[2];
    input_last_line = 12; diagnostic_line_cache = &entries[1];
    if (reset_function_line_input() != 12 || diagnostic_line_cache ||
        input_line_cache != &entries[2] || input_first_line_map != entries) return 0;
    if (resolve_diagnostic_location(8, &file) != 24 || file != entries[1].filename ||
        diagnostic_line_cache != &entries[1]) return 0;
    if (resolve_diagnostic_location(12, &file) != 0xfffffff3U || file != entries[2].filename) return 0;
    if (resolve_diagnostic_location(1, &file) != 100 || file != entries[0].filename) return 0;
    if (resolve_diagnostic_location(0, &file) || file != NULL || diagnostic_line_cache != entries) return 0;
    file = entries[2].filename;
    if (resolve_diagnostic_location(13, &file) || file != NULL || diagnostic_line_cache != entries) return 0;
    return !memcmp(original, entries, sizeof(entries));
}

extern int coff_reuse_index;
extern C2OutputSection *coff_reuse_sections[8];
static int associative_section_reuse_scope(void)
{
    C2OutputSection section, expected;
    unsigned i;
    static const unsigned char selections[] = {2,1,3,4,5,6,7};
    for (i = 0; i < sizeof(selections); ++i)
        if (coff_selection_code((unsigned char)(i + 1)) != selections[i]) return 0;
    memset(&section, 0xa5, sizeof(section));
    section.index = 0x8123;
    expected = section;
    expected.characteristics = 0x42001040U;
    expected.name = "reuse-test";
    expected.flags |= 8;
    expected.selection = 5;
    expected.association = 0xbeef;
    coff_reuse_index = 0; coff_reuse_sections[0] = &section;
    if (add_associative_coff_section(expected.name, expected.characteristics, 5, 0xbeef, 1, 1) != 0x8123 ||
        coff_reuse_index != 1 || memcmp(&section, &expected, sizeof(section))) return 0;
    expected.characteristics = 0x80000020U;
    expected.selection = 6;
    expected.association = 0x1234;
    coff_reuse_index = 0;
    if (add_associative_coff_section(expected.name, expected.characteristics, 6, 0x1234, 0, 1) != 0x8123 ||
        coff_reuse_index != 1 || memcmp(&section, &expected, sizeof(section))) return 0;
    return 1;
}

extern char decorated_name[2059];
extern int c2_identifier_limit;
char *__fastcall coff_symbol_name(C2Symbol *symbol);
static int coff_name_shared_buffer_and_scope(void)
{
    C2InputSymbol symbol, before;
    char *result;
    unsigned i;
    memset(&symbol, 0xa5, sizeof(symbol));
    symbol.symbol.kind = 1; symbol.symbol.source_kind = 0;
    symbol.symbol.serial = 42; symbol.symbol.name = "abcde"; symbol.attributes = 0;
    before = symbol;
    memset(decorated_name, 0xa5, sizeof(decorated_name));
    c2_identifier_limit = 3;
    result = coff_symbol_name(&symbol.symbol);
    if (result != decorated_name || strcmp(result, "abc") || decorated_name[4] != 'e' ||
        decorated_name[5] != 0 || memcmp(&symbol, &before, sizeof(symbol))) return 0;
    for (i = 6; i < sizeof(decorated_name); ++i) if ((unsigned char)decorated_name[i] != 0xa5) return 0;
    c2_identifier_limit = 2058; c2_il_extended_header = 0;
    symbol.symbol.name = NULL;
    if (strcmp(coff_symbol_name(&symbol.symbol), "$T42")) return 0;
    symbol.symbol.source_kind = '$';
    if (strcmp(coff_symbol_name(&symbol.symbol), "$S42")) return 0;
    symbol.symbol.source_kind = '%';
    if (strcmp(coff_symbol_name(&symbol.symbol), "$SG42")) return 0;
    symbol.symbol.kind = 4;
    if (strcmp(coff_symbol_name(&symbol.symbol), "$E42")) return 0;
    symbol.symbol.kind = 1; symbol.symbol.source_kind = 0; symbol.symbol.name = "local";
    symbol.attributes = 0x10; symbol.storage.local.lifetime = -1;
    if (strcmp(coff_symbol_name(&symbol.symbol), "local$")) return 0;
    symbol.storage.local.lifetime = 3;
    if (strcmp(coff_symbol_name(&symbol.symbol), "local$42")) return 0;
    symbol.symbol.kind = 3; symbol.symbol.source_kind = 1;
    if (strcmp(coff_symbol_name(&symbol.symbol), "$L42")) return 0;
    symbol.symbol.name = "?named@@"; symbol.symbol.kind = 9;
    return coff_symbol_name(&symbol.symbol) == decorated_name && !strcmp(decorated_name, "?named@@");
}

extern int deferred_compilation;
void __fastcall set_coff_symbol_name(IMAGE_SYMBOL *symbol, char *name, int copy);
static int coff_symbol_name_byte_scope(void)
{
    IMAGE_SYMBOL symbol, expected;
    C2OutputList list, saved;
    C2OutputChunk chunk;
    unsigned i, n;
    char *names[] = {"", "a", "1234567", "12345678"};
    char *long_name = "function-name-over-eight";
    for (n = 0; n < 4; ++n) {
        memset(&symbol, 0xa5, sizeof(symbol)); expected = symbol;
        memset(expected.N.ShortName, 0, 8);
        memcpy(expected.N.ShortName, names[n], strlen(names[n]));
        set_coff_symbol_name(&symbol, names[n], 0);
        if (memcmp(&symbol, &expected, sizeof(symbol))) return 0;
    }
    memset(&list, 0, sizeof(list)); memset(&saved, 0xa5, sizeof(saved));
    memset(&chunk, 0xa5, sizeof(chunk));
    list.buffered = 1; list.completed_bytes = 10; list.used_bytes = 3; list.last = &chunk;
    coff_output_list = &list; coff_current_list = &saved; deferred_compilation = 0;
    memset(&symbol, 0xa5, sizeof(symbol)); expected = symbol;
    expected.N.Name.Short = 0; expected.N.Name.Long = 17;
    set_coff_symbol_name(&symbol, long_name, 0);
    if (memcmp(&symbol, &expected, sizeof(symbol)) || coff_current_list != &saved ||
        list.used_bytes != 3 + strlen(long_name) + 1 || list.write_count != 1 ||
        strcmp((char *)chunk.data + 3, long_name)) return 0;
    for (i = 0; i < 3; ++i) if (chunk.data[i] != 0xa5) return 0;
    for (i = list.used_bytes; i < sizeof(chunk.data); ++i) if (chunk.data[i] != 0xa5) return 0;
    return list.completed_bytes == 10 && list.last == &chunk;
}

struct C2CoffIndexTarget { unsigned char pending00[4]; unsigned index; };
struct C2CoffIndexFixup {
    unsigned serial;
    struct C2CoffIndexTarget *target;
    struct C2CoffIndexFixup *next;
};
extern struct C2CoffIndexFixup *coff_index_fixups[32];
void __fastcall remember_function_coff_symbol(unsigned serial, unsigned index);
static int function_coff_fixup_collision_scope(void)
{
    struct C2CoffIndexFixup nodes[5];
    struct C2CoffIndexTarget targets[5];
    unsigned i;
    memset(targets, 0xa5, sizeof(targets));
    for (i = 0; i < 5; ++i) {
        nodes[i].serial = i & 1 ? 0x3f : 0x8000001fU;
        nodes[i].target = &targets[i]; nodes[i].next = i == 4 ? NULL : &nodes[i + 1];
    }
    coff_index_fixups[31] = nodes;
    remember_function_coff_symbol(0x8000001fU, 0xf1234567U);
    if (coff_index_fixups[31] != &nodes[1] || nodes[1].next != &nodes[3] || nodes[3].next) return 0;
    for (i = 0; i < 5; ++i) {
        unsigned j;
        if (targets[i].index != (i & 1 ? 0xa5a5a5a5U : 0xf1234567U) || nodes[i].target != &targets[i]) return 0;
        for (j = 0; j < 4; ++j) if (targets[i].pending00[j] != 0xa5) return 0;
    }
    return 1;
}

/* Synthetic callback substitutions exercise only the emitter's owned data
 * and ordering. They do not qualify any pending debug callback body. */
void __fastcall finish_function_code_offsets(C2Function *function);
extern int c2_constant_strings, c2_zero_alignment, c2_global_optimize;
extern int generate_frame, generate_prologue;
static unsigned emission_event_count, emission_events[8], emission_size;
static unsigned char emission_bytes[32];
static C2Section *emission_section;
static C2Symbol *emission_symbol;
static int emission_owner_error;
static void __fastcall test_emission_section(C2Section *section)
{
    if (section != emission_section) emission_owner_error = 1;
    emission_events[emission_event_count++] = 1;
}
static void __fastcall test_emission_entry(C2Symbol *symbol)
{
    if (symbol != emission_symbol) emission_owner_error = 1;
    emission_events[emission_event_count++] = 2;
}
static void __fastcall test_emission_exit(C2Symbol *symbol)
{
    if (symbol != emission_symbol) emission_owner_error = 1;
    emission_events[emission_event_count++] = 3;
}
static void __fastcall test_emission_bytes(const void *bytes, unsigned count)
{
    emission_events[emission_event_count++] = 0x100 + count;
    if (count > sizeof(emission_bytes) - emission_size) { emission_owner_error = 1; return; }
    memcpy(emission_bytes + emission_size, bytes, count);
    emission_size += count;
}
static int final_emission_offsets_and_callbacks(void)
{
    C2Function function;
    C2FunctionSymbol symbol;
    C2Section section;
    C2FlowGraph flow;
    C2BasicBlock block;
    C2Expression entry, instruction, operand;
    unsigned expected[] = {1, 0x102, 2, 0x101, 1, 3, 0x10f};
    /* Alias binding happens at link time; force actual address loads rather
     * than the C compiler folding distinct source declarations unequal. */
    void * volatile identities[5];
    unsigned i;
    memset(&function, 0, sizeof(function)); memset(&symbol, 0, sizeof(symbol));
    memset(&section, 0, sizeof(section)); memset(&flow, 0, sizeof(flow));
    memset(&block, 0, sizeof(block)); memset(&entry, 0, sizeof(entry));
    memset(&instruction, 0, sizeof(instruction)); memset(&operand, 0, sizeof(operand));
    function.symbol = &symbol; function.flow = &flow; flow.last = &block;
    block.records = (C2Record *)&entry; entry.kind = 22; entry.next = &instruction;
    instruction.kind = 11; instruction.type = 0x1004; instruction.opcode = 13;
    instruction.left = &operand; operand.kind = 1; operand.opcode = 0x11f;
    operand.original_register = (C2FunctionRegister *)&machine_registers[1];
    symbol.symbol.parameter_offset = 100; symbol.symbol.association.section_serial = 41;
    section.symbol.serial = 41; section.symbol.parameter_offset = 98;
    global_scope.buckets[41] = &section.symbol;
    emission_section = &section; emission_symbol = &symbol.symbol;
    emission_event_count = emission_size = emission_owner_error = 0;
    memset(emission_bytes, 0xa5, sizeof(emission_bytes));
    output_slot_497ce4.target = test_emission_section;
    output_slot_497dbc.target = test_emission_entry;
    output_slot_497de0.target = test_emission_exit;
    output_slot_497dd4.target = test_emission_bytes;
    c2_constant_strings = c2_zero_alignment = c2_global_optimize = 1;
    generate_frame = 37; generate_prologue = 91;
    finish_function_code_offsets(&function);
    if (emission_owner_error || emission_event_count != 7 || emission_size != 18 ||
        memcmp(emission_events, expected, sizeof(expected)) ||
        section.symbol.parameter_offset != 116 || symbol.symbol.parameter_offset != 100 ||
        generate_frame != 37 || generate_prologue != 91) return 0;
    if (emission_bytes[0] || emission_bytes[1] || emission_bytes[2] != 0x50) return 0;
    for (i = 3; i < 18; ++i) if (emission_bytes[i] != 0x90) return 0;
    for (i = 18; i < sizeof(emission_bytes); ++i) if (emission_bytes[i] != 0xa5) return 0;
    identities[0] = (void *)c2_set_argv;
    identities[1] = (void *)output_noop_section; identities[2] = (void *)output_noop_bytes;
    identities[3] = (void *)output_noop_filename; identities[4] = (void *)output_noop_line;
    return identities[0] == identities[1] && identities[0] == identities[2] &&
        identities[0] == identities[3] && identities[0] == identities[4];
}
extern int c2_line_numbers;
extern unsigned coff_function_source_line, coff_function_line_count;
extern unsigned coff_line_restart, coff_last_source_line;
static int coff_line_callback_word_scope(void)
{
    C2FunctionSymbol symbol;
    memset(&symbol, 0xa5, sizeof(symbol));
    c2_codeview_types = c2_line_numbers = 0;
    coff_function_source_line = 0xfedc1234;
    coff_function_line_count = 0xabcdffff;
    output_callback_4346cf(&symbol.symbol);
    if (coff_function_line_count != 0xabcd0000) return 0;
    coff_line_restart = 0x12345678; coff_last_source_line = 0xfedcba98;
    output_callback_435633(&symbol.symbol);
    return coff_line_restart == 1 && coff_function_line_count == 0xabcd0000 &&
        coff_last_source_line == 0xfedcba98 && symbol.code_bytes == 0xa5a5a5a5U;
}

extern int c2_default_library, c2_il_eight_byte_target, function_input_flags;
void initialize_function_output(void);
static int function_output_enabled_owner(void)
{
    C2Symbol symbol;
    memset(&symbol, 0, sizeof(symbol)); symbol.name = "_main";
    current_symbol = &symbol;
    output_enabled = 1; c2_default_library = 0;
    c2_il_eight_byte_target = function_input_flags = 0;
    /* The old duplicate interpretation reaches the genuine pending runtime
     * lookup here. The actual output-enabled owner suppresses that call. */
    initialize_function_output();
    return output_enabled == 1 && !c2_default_library && current_symbol == &symbol;
}
void __fastcall write_fpo_record(C2Symbol *symbol, unsigned offset, unsigned code_bytes,
    unsigned local_words, unsigned parameter_words, unsigned char prologue,
    unsigned char saved_registers, int seh, int frame, int use_bp);
extern C2Section *function_fpo_section;
extern char *c2_output_name;
static void __fastcall test_fpo_relocation(unsigned offset, C2Symbol *symbol)
{
    if (offset != (unsigned)-16 || symbol != emission_symbol) emission_owner_error = 1;
    emission_events[emission_event_count++] = 4;
}
static int fpo_record_layout_and_owners(void)
{
    C2Section section;
    C2Symbol symbol, before;
    unsigned char expected[16] = {0x44,0x33,0x22,0x11,0x78,0x56,0x34,0x12,
        0x11,0x00,0xff,0x80,0x98,0xba,0xf3,0xdb};
    unsigned i;
    memset(&section, 0xa5, sizeof(section)); memset(&symbol, 0xa5, sizeof(symbol));
    before = symbol; section.symbol.parameter_offset = 0xfffffff8U;
    emission_section = function_fpo_section = &section; emission_symbol = &symbol;
    emission_event_count = emission_size = emission_owner_error = 0;
    memset(emission_bytes, 0xa5, sizeof(emission_bytes));
    output_slot_497ce4.target = test_emission_section;
    output_slot_497db0.target = test_emission_bytes;
    output_slot_497db4.target = test_fpo_relocation;
    c2_output_name = "numeric.obj"; output_enabled = 1;
    write_fpo_record(&symbol, 0x11223344, 0x12345678, 0x80ff0011,
        0xfedcba98, 0xf3, 3, 1, 1, 1);
    if (emission_owner_error || emission_event_count != 3 || emission_size != 16 ||
        emission_events[0] != 1 || emission_events[1] != 0x110 || emission_events[2] != 4 ||
        memcmp(emission_bytes, expected, 16) || section.symbol.parameter_offset != 8 ||
        memcmp(&symbol, &before, sizeof(symbol))) return 0;
    for (i = 16; i < sizeof(emission_bytes); ++i) if (emission_bytes[i] != 0xa5) return 0;
    c2_output_name = NULL;
    write_fpo_record(&symbol, 1, 2, 3, 4, 5, 6, 1, 1, 1);
    c2_output_name = "numeric.obj"; output_enabled = 0;
    write_fpo_record(&symbol, 1, 2, 3, 4, 5, 6, 1, 1, 1);
    return emission_event_count == 3 && emission_size == 16 && section.symbol.parameter_offset == 8;
}

void __fastcall write_coff_relocation(unsigned serial, unsigned offset,
    unsigned symbol_index, unsigned short type);
static int coff_relocation_last_row_and_reuse(void)
{
    C2OutputSection section;
    C2CoffSectionData data, expected_data;
    C2OutputBlock508 block, free_block, next_free, expected_block, expected_free;
    memset(&section, 0xa5, sizeof(section)); memset(&data, 0xa5, sizeof(data));
    memset(&block, 0xa5, sizeof(block)); memset(&free_block, 0x6b, sizeof(free_block));
    memset(&next_free, 0x3c, sizeof(next_free));
    section.data = &data; section.base_offset = 0x1000;
    data.relocations = &block; data.relocation_count = 0xffffffffU;
    block.count = 49; block.last = &block;
    free_block.last = &next_free;
    expected_data = data; expected_data.relocations = &free_block; expected_data.relocation_count = 0;
    expected_block = block; expected_block.count = 50; expected_block.last = &free_block;
    expected_block.entries[49].VirtualAddress = 16;
    expected_block.entries[49].SymbolTableIndex = 0x88776655;
    expected_block.entries[49].Type = 7;
    expected_free = free_block; expected_free.count = 0; expected_free.last = &block;
    coff_current_section = &section; coff_free_blocks_508 = &free_block;
    write_coff_relocation(0x123, 0x1010, 0x88776655, 7);
    return coff_free_blocks_508 == &next_free &&
        !memcmp(&data, &expected_data, sizeof(data)) &&
        !memcmp(&block, &expected_block, sizeof(block)) &&
        !memcmp(&free_block, &expected_free, sizeof(free_block)) &&
        section.base_offset == 0x1000 && next_free.count == 0x3c3c3c3c;
}
extern unsigned frame_allocated_offset, function_output_reset6c;
extern C2Symbol *frame_reserved_symbol;
extern C2FunctionRegister *frame_extra_slot;
extern int prologue_has_reserved_storage;
void finish_function_output(void);
static int function_output_reset_exact_owners(void)
{
    C2Symbol symbol;
    C2FunctionRegister reg;
    argument_stack_offset = -23; frame_allocated_offset = 0xfedcba98;
    frame_reserved_symbol = &symbol; frame_extra_slot = &reg;
    prologue_has_reserved_storage = 0x12345678; function_output_reset6c = 0x9abcdef0;
    generate_frame = 37; generate_prologue = 91; c2_default_library = 43;
    finish_function_output();
    return argument_stack_offset == 8 && !frame_allocated_offset &&
        !frame_reserved_symbol && !frame_extra_slot && !prologue_has_reserved_storage &&
        !function_output_reset6c && generate_frame == 37 && generate_prologue == 91 &&
        c2_default_library == 43;
}

static int final_cleanup_reset_slots(void)
{
    unsigned pools[3] = {1,5,8};
    C2RegisterSet heads[15], tails[15];
    C2RegisterSetNode node_heads[15], node_tails[15];
    unsigned i, j, pool;
    for (i = 0; i < 3; ++i) {
        pool = pools[i];
        for (j = 0; j < 15; ++j) {
            free_register_sets[j] = &heads[j]; last_free_register_sets[j] = &tails[j];
            free_register_set_nodes[j] = &node_heads[j]; last_free_register_set_nodes[j] = &node_tails[j];
        }
        c2_pools[pool].blocks = NULL;
        c2_pools[pool].cursor = (unsigned char *)&heads[0];
        c2_pools[pool].remaining = 17;
        reset_input_pool(pool);
        if (c2_pools[pool].blocks || c2_pools[pool].cursor || c2_pools[pool].remaining) return 0;
        for (j = 0; j < 15; ++j) {
            if (free_register_sets[j] != (j == pool ? NULL : &heads[j]) ||
                last_free_register_sets[j] != (j == pool ? NULL : &tails[j]) ||
                free_register_set_nodes[j] != (j == pool ? NULL : &node_heads[j]) ||
                last_free_register_set_nodes[j] != (j == pool ? NULL : &node_tails[j])) return 0;
        }
    }
    return 1;
}
void clear_symbol_input_registers(void);
static int final_symbol_register_cleanup(void)
{
    C2RegisterBlock blocks[2];
    C2Symbol symbols[3], expected[3];
    memset(blocks, 0xa5, sizeof(blocks)); memset(symbols, 0xa5, sizeof(symbols));
    blocks[0].next = &blocks[1]; blocks[1].next = NULL;
    blocks[0].cursor = blocks[0].records + 3; blocks[1].cursor = blocks[1].records + 1;
    blocks[0].records[0].symbol = &symbols[0]; blocks[0].records[0].kind = 3;
    blocks[0].records[1].symbol = &symbols[1]; blocks[0].records[1].kind = 0x10;
    blocks[0].records[2].symbol = NULL; blocks[0].records[2].kind = 4;
    blocks[1].records[0].symbol = &symbols[2]; blocks[1].records[0].kind = 4;
    memcpy(expected, symbols, sizeof(symbols));
    expected[0].input_register = expected[2].input_register = NULL;
    first_function_register_block = &blocks[0];
    clear_symbol_input_registers();
    return !memcmp(symbols, expected, sizeof(symbols)) &&
        blocks[0].records[0].symbol == &symbols[0] && blocks[0].records[1].kind == 0x10 &&
        blocks[1].records[0].number == 0xa5a5a5a5U;
}

static int arithmetic_token_no_payload(void)
{
    C2ILToken token, expected;
    unsigned opcode;
    for (opcode = 2; opcode <= 4; ++opcode) {
        memset(&token, 0xa5, sizeof(token)); expected = token; expected.opcode = opcode;
        if (decode_input_token(opcode, NULL, &token) != &token ||
            memcmp(&token, &expected, sizeof(token))) return 0;
    }
    return 1;
}

extern int expression_rewrite_changed;
int __fastcall simplify_unary_constant(C2Expression *expression, C2Expression *context, int mode);
C2Expression *__fastcall factor_unary_operands(C2Expression *expression, C2Expression *context, unsigned opcode);
static int unary_constant_modes_and_widths(void)
{
    C2Expression node, expected, parent, first, second;
    unsigned short types[8] = {0x1004,0x1004,0x1004,0x1004,0x2004,0x2004,0x1004,0x2004};
    int modes[8] = {0,1,1,2,3,0,1,-1};
    unsigned __int64 values[8] = {~0ULL,~0ULL,0xffffffff80000000ULL,1,1,0xffffffffULL,1,0xffffffffULL};
    unsigned __int64 results[8] = {~0ULL,1,0xffffffff80000000ULL,~0ULL,0xffffffffULL,1,1,0xffffffffULL};
    int changed[8] = {0,1,0,1,1,1,0,0};
    unsigned i;
    for (i = 0; i < 8; ++i) {
        memset(&node, 0xa5, sizeof(node)); node.kind = 7; node.opcode = 0x11d;
        node.type = types[i]; node.integer.value = values[i]; expected = node;
        expected.integer.value = results[i]; expression_rewrite_changed = 77;
        if (simplify_unary_constant(&node, NULL, modes[i]) != changed[i] ||
            expression_rewrite_changed != (changed[i] ? 1 : 77) ||
            memcmp(&node, &expected, sizeof(node))) return 0;
    }
    memset(&parent, 0, sizeof(parent)); memset(&first, 0, sizeof(first)); memset(&second, 0, sizeof(second));
    parent.kind = 11; parent.opcode = 0x143; parent.type = 0x1004; parent.left = &first;
    first.kind = second.kind = 7; first.opcode = second.opcode = 0x11d;
    first.type = second.type = 0x1004; first.integer.value = 1; second.integer.value = ~0ULL;
    first.next = &second;
    if (!simplify_unary_constant(&parent, NULL, 1) || first.integer.value != 1 ||
        second.integer.value != 1 || parent.left != &first || first.next != &second) return 0;
    first.opcode = 0x137; second.opcode = 0x141;
    expression_rewrite_changed = 77;
    return factor_unary_operands(&parent, NULL, 0x137) == &parent &&
        expression_rewrite_changed == 77 && first.next == &second && parent.left == &first;
}

unsigned __int64 __fastcall expression_shift_mask(C2Expression *expression);
int __fastcall retain_full_shift_mask(C2Expression *operand, C2Expression *expression);
static int shift_mask_width_count_and_rewrite(void)
{
    C2Expression expression, first, count, before, saved_count;
    C2Expression parent, number, expected_expression, expected_parent, expected_number;
    unsigned counts[8] = {0,1,31,32,63,64,255,256};
    unsigned __int64 right[8] = {0xffffffffULL,0x7fffffffULL,1,0,0,0,0,0xffffffffULL};
    unsigned __int64 left[8] = {~0ULL,~1ULL,0xffffffff80000000ULL,0,0,0,0,~0ULL};
    unsigned i;
    memset(&expression, 0xa5, sizeof(expression)); memset(&first, 0xa5, sizeof(first));
    memset(&count, 0xa5, sizeof(count)); expression.left = &first; first.next = &count;
    for (i = 0; i < 8; ++i) {
        count.integer.value = 0x1234567800000000ULL | counts[i]; saved_count = count;
        expression.opcode = 0x145; expression.type = 0x1004; before = expression;
        if (expression_shift_mask(&expression) != right[i] ||
            memcmp(&expression, &before, sizeof(expression)) || memcmp(&count, &saved_count, sizeof(count))) return 0;
        expression.opcode = 0x14b; before = expression;
        if (expression_shift_mask(&expression) != left[i] ||
            memcmp(&expression, &before, sizeof(expression))) return 0;
    }
    for (i = 0; i < 2; ++i) {
        memset(&parent, 0xa5, sizeof(parent)); memset(&number, 0xa5, sizeof(number));
        expression.opcode = 0x145; expression.type = 0x2004; expression.next = &number;
        count.kind = 7; count.type = 0x1004; count.integer.value = i ? 5 : 2;
        parent.opcode = 0x14b; parent.type = 0x2004; parent.left = &expression;
        number.kind = 7; number.integer.value = i ? 2 : 4; number.next = NULL;
        expected_expression = expression; expected_expression.opcode = 0x146;
        saved_count = count; saved_count.type = 0x2004; saved_count.integer.value = i ? 0x07ffffff : 0x3fffffff;
        expected_parent = parent; if (i) expected_parent.opcode = 0x145;
        expected_number = number; expected_number.integer.value = i ? 3 : 2;
        expression_rewrite_changed = 77;
        if (!retain_full_shift_mask(&expression, &parent) || expression_rewrite_changed != 1 ||
            memcmp(&expression, &expected_expression, sizeof(expression)) ||
            memcmp(&count, &saved_count, sizeof(count)) ||
            memcmp(&parent, &expected_parent, sizeof(parent)) ||
            memcmp(&number, &expected_number, sizeof(number))) return 0;
    }
    return 1;
}

static int shift_range_outputs_and_counts(void)
{
    C2Expression operand, mask_record, byte_operand, constant, shift_record, first, count;
    C2Expression *reported, saved_operand, saved_mask, saved_byte, saved_constant;
    C2Expression saved_shift, saved_first, saved_count;
    C2FunctionRegister mask_reg, shift_reg;
    unsigned i;
    int bytes, result;
    static const struct { unsigned mask, shift, kind; int result, bytes, written; } rows[] = {
        {0xff00, 0, 2, 1, 1, 1}, {0xff00, 8, 2, 1, 2, 1},
        {0xff00, 8, 1, 0, 2, 1}, {0xff00, 32, 2, 1, 5, 1},
        {0xff00, 24, 2, 0, -17, 0}, {0xff00, 7, 2, 0, -17, 0},
        {0xff, 0, 2, -1, 0, 1}, {0xff, 8, 2, -1, 1, 1},
        {0xff, 16, 1, 0, 2, 1}, {0x7f, 8, 2, 0, -17, 0}
    };
    for (i = 0; i < sizeof(rows) / sizeof(rows[0]); ++i) {
        memset(&operand, 0xa5, sizeof(operand)); memset(&mask_record, 0xa5, sizeof(mask_record));
        memset(&byte_operand, 0xa5, sizeof(byte_operand)); memset(&constant, 0xa5, sizeof(constant));
        memset(&shift_record, 0xa5, sizeof(shift_record)); memset(&first, 0xa5, sizeof(first));
        memset(&count, 0xa5, sizeof(count));
        memset(&mask_reg, 0, sizeof(mask_reg)); memset(&shift_reg, 0, sizeof(shift_reg));
        operand.kind = 2; operand.type = 0x2004; operand.value.reg = &mask_reg;
        mask_reg.value = &mask_record;
        mask_record.opcode = 0x146; mask_record.type = 0x2004; mask_record.left = &byte_operand;
        byte_operand.kind = 2; byte_operand.type = 0x2004; byte_operand.next = &constant;
        byte_operand.value.reg = &shift_reg; shift_reg.value = &shift_record;
        constant.kind = 7; constant.integer.low = rows[i].mask;
        shift_record.opcode = 0x14b; shift_record.left = &first;
        first.kind = rows[i].kind; first.next = &count;
        count.kind = 7; count.integer.low = rows[i].shift;
        saved_operand = operand; saved_mask = mask_record; saved_byte = byte_operand;
        saved_constant = constant; saved_shift = shift_record; saved_first = first; saved_count = count;
        bytes = -17; reported = &operand;
        result = get_final_shift_range(&operand, &bytes, &reported);
        if (result != rows[i].result || bytes != rows[i].bytes ||
            reported != (rows[i].written ? &shift_record : &operand) ||
            memcmp(&operand, &saved_operand, sizeof(operand)) ||
            memcmp(&mask_record, &saved_mask, sizeof(mask_record)) ||
            memcmp(&byte_operand, &saved_byte, sizeof(byte_operand)) ||
            memcmp(&constant, &saved_constant, sizeof(constant)) ||
            memcmp(&shift_record, &saved_shift, sizeof(shift_record)) ||
            memcmp(&first, &saved_first, sizeof(first)) || memcmp(&count, &saved_count, sizeof(count))) return 0;
    }
    return 1;
}

const ReconTest numeric_tests[] = {
    {"shift ranges preserve signed results, machine shift counts and pointer outputs", 1, shift_range_outputs_and_counts},
    {"shift masks preserve low-byte counts, signed widths and exact rewritten owners", 1, shift_mask_width_count_and_rewrite},
    {"unary constant modes preserve signed widths, staged order and untouched fields", 1, unary_constant_modes_and_widths},
    {"arithmetic input token forms preserve every untouched payload byte", 1, arithmetic_token_no_payload},
    {"final cleanup resets only the reached pool1,5,8 pointer slots", 1, final_cleanup_reset_slots},
    {"final register cleanup clears only linked nonempty symbols outside kind16", 1, final_symbol_register_cleanup},
    {"COFF relocations preserve the final row and recycled circular block payload", 1, coff_relocation_last_row_and_reuse},
    {"function output reset writes only its six actual owners", 1, function_output_reset_exact_owners},
    {"function output tests its actual enabled owner independently of library suppression", 1, function_output_enabled_owner},
    {"FPO records preserve SDK byte layout, callback identities and wrapped section offset", 1, fpo_record_layout_and_owners},
    {"final emission preserves callback order, padding and section offsets", 1, final_emission_offsets_and_callbacks},
    {"inactive COFF line callbacks preserve upper count bits and symbol payload", 1, coff_line_callback_word_scope},
    {"COFF name writes preserve symbol payload and restore the active list", 1, coff_symbol_name_byte_scope},
    {"function COFF fixups unlink only matching serials and preserve target prefixes", 1, function_coff_fixup_collision_scope},
    {"COFF names retain shared-buffer identity, signed scope and truncation writes", 1, coff_name_shared_buffer_and_scope},
    {"associative COFF sections preserve reused fields and exact selection codes", 1, associative_section_reuse_scope},
    {"diagnostic locations walk both cache directions without changing source maps", 1, diagnostic_location_bidirectional_cache},
    {"integer encoding preserves width, byte relaxation and untouched tails", 1, encoding_integer_width_and_relaxation},
    {"register PUSH encoding writes only its actual opcode byte", 1, push_encoding_byte_scope},
    {"integer arithmetic recipes preserve x86 bytes and untouched input/output tails", 1, integer_arithmetic_encoding_recipes},
    {"output encoding preserves byte scope and finalized displacement owners", 1, output_conditions_and_displacements},
    {"stack operand adjustment distinguishes prologue and frame selectors", 1, stack_operand_mode_owners},
    {"ready queues preserve unsigned priority ties and signed count bounds", 1, schedule_ready_order_and_count},
    {"schedule priority scaling preserves unsigned shifts and machine counts", 1, schedule_priority_full_width},
    {"schedule use recycling preserves payload and distinct list ownership", 1, schedule_use_recycling_scope},
    {"schedule penalties follow the actual address operand owners", 1, schedule_address_penalty_ownership},
    {"schedule edge reuse preserves the tail and full-width flags", 1, schedule_edge_reuse_owned_bytes},
    {"schedule dependencies skip disjoint canonical registers", 1, schedule_dependency_disjoint_roots},
    {"schedule operand costs retain high words and width boundaries", 1, schedule_operand_cost_widths},
    {"schedule initialization preserves unused resource rows and padding", 1, schedule_resource_initialization_scope},
    {"final cache preserves equivalence ownership and failed selector output", 1, final_cache_equivalence_identity},
    {"flags overlap distinguishes stack and typed sentinel canonical roots", 1, flags_and_sentinel_roots},
    {"final peephole numbering includes inactive labels and preserves untouched records", 1, final_peephole_numbering_scope},
    {"register save region rejects external predecessors and preserves failure output", 1, register_save_region_boundaries},
    {"frame register list preserves width groups and reference ordering", 1, frame_register_weight_order},
    {"frame symbol reset preserves unrelated bytes and other storage classes", 1, frame_symbol_flag_scope},
    {"symbol iterator retains hash chains and the final bucket", 1, symbol_iterator_chain_and_end},
    {"machine sentinel prefix preserves table-end identity", 1, machine_sentinel_identity},
    {"machine output views preserve byte aliases and adjacent ranges", 1, fixed_output_byte_alias},
    {"fixed register overlap distinguishes adjacent ranges and canonical identities", 1, fixed_register_overlap_boundaries},
    {"final address folding preserves DWORD wrap and unlinks consumed definitions", 1, final_address_constant_wrap},
    {"multiply recipe collisions retain distinct signed keys and allocation identity", 1, final_multiplier_hash_identity},
    {"required final bits retain unsigned state and shared default identity", 1, final_required_bits_shared_default},
    {"zero-bit propagation preserves displaced and wider register variants", 1, final_zero_bits_variant_bounds},
    {"final register scratch reset preserves sentinel and unused records", 1, final_register_scratch_bounds},
    {"set insertion returns the full previous high-bit mask", 1, global_register_old_bit_width},
    {"global initialization captures the existing register-set owner", 1, global_initial_set_owner},
    {"integer zero-divisor rejection preserves both full operand records", 1, integer_fold_zero_divisor},
    {"register marking selects flow offset zero with distinct block owners", 1, register_mark_flow_owner},
    {"factor buckets retain duplicate counts and asymmetric weight comparison", 1, factor_count_weight_order},
    {"plain register release preserves the original variant free-head behavior", 1, plain_register_release_variants},
    {"expression sorting is stable across unsigned keys and preserves payloads", 1, expression_order_stability},
    {"integer mask propagation retains signed representatives and unrelated bytes", 1, integer_mask_representatives},
    {"detached operand pruning preserves list and flag-byte roles", 1, detached_operand_roles},
    {"expression dispatcher distinguishes NULL and nonNULL definition owners", 1, expression_dispatch_owner_branch},
    {"flow recycling preserves both edge chains and block endpoints", 1, flow_recycling_lists},
    {"input-use unlink preserves the owner alias and full-width count", 1, input_use_owner_identity},
    {"small-node list release saves successors before recycling", 1, small_node_list_recycling},
    {"flow traversal preserves cycles, postorder and unreachable flags", 1, reachable_flow_postorder},
    {"dense header reuse preserves flags, payload ownership and unmodeled pool gaps", 1, optimizer_dense_header_reuse},
    {"optimizer iterator retains zero-group continuation and input ownership", 1, optimizer_iterator_continuation},
    {"optimizer use deduplication preserves pointer identity and existing tail", 1, optimizer_use_pointer_identity},
    {"optimizer membership preserves full-width masks and unsigned groups", 1, optimizer_full_width_membership},
    {"optimizer scope parameters preserve copied uses and scalar numbering", 1, optimizer_scope_parameter_identity},
    {"optimizer set release preserves payloads and appends both free queues", 1, optimizer_set_release_queues},
    {"optimizer set assignment reuses owned nodes without copying flags", 1, optimizer_set_assignment_reuse},
    {"optimizer set copies own their nodes and preserve source flags", 1, optimizer_set_copy_ownership},
    {"optimizer set union inserts sorted groups and retains both owners", 1, optimizer_set_union},
    {"optimizer set subtraction preserves unsigned masks and source ownership", 1, optimizer_set_difference},
    {"optimizer operands retain integer alias numbers and group membership", 1, optimizer_operand_numbers},
    {"optimizer alias groups retain scalar indices and pool6 ownership", 1, optimizer_alias_group_ownership},
    {"optimizer counts real scope entries and disables alias work for large functions", 1, optimizer_scope_and_fallback},
    {"pool6 reset clears only its encountered cells and preserves unknown gaps", 1, input_pool_six_reset},
    {"flow-edge reuse preserves flags and both endpoint-list owners", 1, flow_edge_reuse_ownership},
    {"flow partition tracks scope depth and only adds its owned block flag", 1, block_scope_and_flags},
    {"symbol range mode preserves full DWORD results and never writes its input", 1, symbol_boundary_full_width},
    {"pool12 reset clears only its encountered cells and preserves unknown gaps", 1, input_pool_twelve_reset},
    {"inline collection tracks temporary lifetimes and address variants", 1, inline_temporary_lifetimes},
    {"pool release unlinks oversized blocks and retains the small-block chain", 1, pool_release_ownership},
    {"input cleanup drains stack sentinels while preserving the distinct pending list", 1, pending_input_cleanup},
    {"generic pointer conversion respects shared operands and register ownership", 1, generic_pointer_types},
    {"plain register reuse preserves number", 1, plain_reuse},
    {"set reuse preserves unrelated bytes", 1, set_reuse},
    {"original 0/1/2 extended-real results", 1, original_real_results},
    {"existing unsigned register-set groups", 1, register_set_existing_groups},
    {"register-set node reuse clears only the popped node", 1, register_set_node_reuse},
    {"clearing a register set appends its nodes", 1, register_set_clear},
    {"function entry clears all small-node free lists", 1, clear_small_node_lists},
    {"local-symbol retention flag combinations", 1, symbol_retention_flags},
    {"parameter storage padding and idempotence", 1, parameter_storage_layout},
    {"byte and word stack-slot alignment", 1, stack_slot_alignment},
    {"successive local stack allocations", 1, local_storage_sequence},
    {"function line reset retains the cumulative line", 1, function_line_reset},
    {"small-node reuse clears exactly its owned size", 1, small_node_reuse},
    {"code-node insertion preserves both chains", 1, code_node_splice},
    {"pool12 zeroed allocation leaves unknown backing alone", 1, flow_pool_allocation},
    {"flow-block reuse clears only the popped allocation", 1, flow_block_reuse},
    {"ended module does not consume or modify another token", 1, ended_token_input},
    {"function option stores share CLI globals and retain active state", 1, per_function_option_aliases},
    {"pragma short/long sign extension preserves unrelated token fields", 1, pragma_signed_fields},
    {"control record clears its exact 28-byte allocation", 1, control_record_reuse},
    {"input stack reuse and drain preserve complete tree chain", 1, input_stack_reuse_and_link},
    {"filename selection retains cached string identity", 1, input_filename_identity},
    {"source-line mapping preserves spans and filename identity", 1, input_line_mapping},
    {"symbol register cache preserves owner and allocation identity", 1, input_register_cache},
    {"input type encoding retains defined width and signed size boundary", 1, input_encoded_types},
    {"typed register expression clears exact size and keeps register identity", 1, typed_register_record},
    {"defined return conventions retain aggregate and power-of-two rules", 1, return_storage_conventions},
    {"input type decoding retains untouched token fields", 1, input_type_fields},
    {"operand pop clears only owned stack links and flag", 1, input_operand_pop},
    {"address views reuse compatible widths and widest aggregates without mutation", 1, widest_address_view},
    {"integer records preserve signed width, full payload and allocation canary", 1, integer_record_width},
    {"binary type selection preserves pointer, real and unsigned precedence", 1, binary_value_types},
    {"constant narrowing marks only changed values and preserves record tails", 1, folded_integer_width},
    {"temporary and binary node sizes preserve allocation canaries", 1, temporary_node_sizes},
    {"operand copying preserves payloads, originals and allocation canaries", 1, copied_operand_ownership},
    {"side-effect flags retain volatile, exception and intrinsic distinctions", 1, input_effect_flags},
    {"symbol uses preserve list ownership, full-width count and allocation tail", 1, symbol_use_ownership}
};
