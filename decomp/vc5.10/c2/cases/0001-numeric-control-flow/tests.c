/* Narrow retained behavior checks; synthetic list contexts do not claim public reach. */
#define RECON_SHIM_IMPL
#include "register-sets.h"
#include "functions.h"
#include "symbol-storage.h"
#include "function-reader.h"
#include "small-nodes.h"
#include "flow.h"
#include "code-input.h"
#include <recon-tool.h>
#include <recon.h>

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
    C2PendingInput first, second;
    C2Expression left, right;
    C2SmallNode *saved = free_small_nodes[32];
    int retain = retain_function_input_pool;
    memset(&left, 0, sizeof(left));
    memset(&right, 0, sizeof(right));
    left.kind = right.kind = 7;
    first.next = &second;
    second.next = NULL;
    first.expression = &left;
    second.expression = &right;
    pending_input_records = &first;
    retain_function_input_pool = 1;
    free_small_nodes[32] = NULL;
    finish_function_input();
    if (pending_input_records || free_small_nodes[32] != (C2SmallNode *)&right ||
        right.next != &left || left.next) return 0;
    free_small_nodes[32] = saved;
    retain_function_input_pool = retain;
    return 1;
}

const ReconTest numeric_tests[] = {
    {"pending input cleanup releases each expression while retaining its pool", 1, pending_input_cleanup},
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
