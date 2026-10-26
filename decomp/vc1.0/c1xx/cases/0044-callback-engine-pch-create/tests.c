#define RECON_SHIM_IMPL
#include <stdlib.h>
#include <string.h>
#include <recon-tool.h>
#include <recon-fixture.h>
#include "case.h"

static void put_word(unsigned char *p, uint32_t value)
{
    p[0] = value;
    p[1] = value >> 8;
    p[2] = value >> 16;
    p[3] = value >> 24;
}

static int finite_comparison_controls(void)
{
    unsigned char *oracle = callback_pch_read(recon_case_assets(), "oracle.pch");
    unsigned char *actual = malloc(CALLBACK_PCH_BYTES);
    unsigned char *changed = malloc(CALLBACK_PCH_BYTES);
    const uint32_t positions[] = { 21667, 83103 };
    unsigned i, j, first, flag = 0;
    int ok = 0;
    if (!oracle || !actual || !changed) goto done;
    memcpy(actual, oracle, CALLBACK_PCH_BYTES);
    /* Move every identified address together: unchanged relative identities
       must pass without relying on the original process's numeric addresses. */
    for (i = 0; i < RECON_COUNT(callback_pch_owners); ++i) {
        const CallbackPchOwner *r = &callback_pch_owners[i];
        for (j = 0; j < 32; ++j)
            if (r->fields & (1u << j)) {
                unsigned offset = positions[r->page] + r->offset + j * 4;
                put_word(actual + offset, callback_pch_word(oracle + offset) + 0x10000000);
            }
    }
    for (i = 0; i < RECON_COUNT(callback_pch_padding); ++i) {
        const CallbackPchPadding *r = &callback_pch_padding[i];
        memset(actual + r->offset, 0, r->bytes);
        actual[r->offset] = oracle[r->offset] & r->meaningful_mask;
        if (r->meaningful_mask) flag = r->offset;
    }
    if (!flag || !callback_pch_compare(actual, oracle, CALLBACK_PCH_BYTES)) goto done;
    first = positions[0] + 16416;
    memcpy(changed, actual, CALLBACK_PCH_BYTES);
    changed[first] ^= 4;
    if (callback_pch_compare(changed, oracle, CALLBACK_PCH_BYTES)) goto done;
    memcpy(changed, actual, CALLBACK_PCH_BYTES);
    memset(changed + first, 0, 4);
    if (callback_pch_compare(changed, oracle, CALLBACK_PCH_BYTES)) goto done;
    memcpy(changed, actual, CALLBACK_PCH_BYTES);
    changed[callback_pch_padding[0].offset] = 1;
    if (callback_pch_compare(changed, oracle, CALLBACK_PCH_BYTES)) goto done;
    memcpy(changed, actual, CALLBACK_PCH_BYTES);
    changed[flag] ^= 1;
    if (callback_pch_compare(changed, oracle, CALLBACK_PCH_BYTES)) goto done;
    memcpy(changed, actual, CALLBACK_PCH_BYTES);
    changed[flag] |= 0x80;
    if (callback_pch_compare(changed, oracle, CALLBACK_PCH_BYTES)) goto done;
    memcpy(changed, actual, CALLBACK_PCH_BYTES);
    changed[8] ^= 1;
    if (callback_pch_compare(changed, oracle, CALLBACK_PCH_BYTES)) goto done;
    if (callback_pch_compare(actual, oracle, CALLBACK_PCH_BYTES - 1)) goto done;
    ok = 1;
done:
    free(changed);
    free(actual);
    free(oracle);
    return ok;
}
const ReconTest callback_pch_tests[1] = {
    { "finite-callback-engine-pch-comparison", 1, finite_comparison_controls }
};
