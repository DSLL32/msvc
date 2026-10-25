/* Narrow retained behavior checks; synthetic list contexts do not claim public reach. */
#define RECON_SHIM_IMPL
#include "register-sets.h"
#include "functions.h"
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

const ReconTest numeric_tests[] = {
    {"plain register reuse preserves number", 1, plain_reuse},
    {"set reuse preserves unrelated bytes", 1, set_reuse},
    {"original 0/1/2 extended-real results", 1, original_real_results},
    {"existing unsigned register-set groups", 1, register_set_existing_groups},
    {"register-set node reuse clears only the popped node", 1, register_set_node_reuse},
    {"clearing a register set appends its nodes", 1, register_set_clear},
    {"function entry clears all small-node free lists", 1, clear_small_node_lists},
    {"local-symbol retention flag combinations", 1, symbol_retention_flags}
};
