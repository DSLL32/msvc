/* Narrow retained behavior checks; synthetic list contexts do not claim public reach. */
#define RECON_SHIM_IMPL
#include "register-sets.h"
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

const ReconTest numeric_tests[] = {
    {"plain register reuse preserves number", 1, plain_reuse},
    {"set reuse preserves unrelated bytes", 1, set_reuse},
    {"original 0/1/2 extended-real results", 1, original_real_results}
};
