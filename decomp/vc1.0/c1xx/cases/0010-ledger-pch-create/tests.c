#define RECON_SHIM_IMPL
#include <stdlib.h>
#include <string.h>
#include <recon-tool.h>
#include <recon-fixture.h>
#include "case.h"

/* These are comparison-policy controls, not additional compiler cases. */
static int pointer_comparison_controls(void)
{
    unsigned char *oracle = ledger_pch_oracle(recon_case_assets());
    unsigned char *changed = malloc(LEDGER_PCH_BYTES);
    int ok = 0;
    if (!oracle || !changed) goto done;
    if (!ledger_pch_compare(oracle, oracle, LEDGER_PCH_BYTES)) goto done;
    memcpy(changed, oracle, LEDGER_PCH_BYTES);
    /* A masked arena pointer must still identify its exact target. */
    changed[LEDGER_PAGE_OFFSET + 0x9198] ^= 4;
    if (ledger_pch_compare(changed, oracle, LEDGER_PCH_BYTES)) goto done;
    memcpy(changed, oracle, LEDGER_PCH_BYTES);
    /* Two different heap roles cannot become aliases. */
    memcpy(changed + LEDGER_PAGE_OFFSET + 0xb2fc,
        changed + LEDGER_PAGE_OFFSET + 0xa4b4, 4);
    if (ledger_pch_compare(changed, oracle, LEDGER_PCH_BYTES)) goto done;
    memcpy(changed, oracle, LEDGER_PCH_BYTES);
    changed[LEDGER_PAGE_OFFSET + 0xa226] = 1;
    if (ledger_pch_compare(changed, oracle, LEDGER_PCH_BYTES)) goto done;
    memcpy(changed, oracle, LEDGER_PCH_BYTES);
    changed[LEDGER_PAGE_OFFSET + 0xa254 + 3] &= ~2;
    if (ledger_pch_compare(changed, oracle, LEDGER_PCH_BYTES)) goto done;
    ok = 1;
done:
    free(changed);
    free(oracle);
    return ok;
}

const ReconTest ledger_pch_tests[1] = {
    { "finite-pch-pointer-comparison", 1, pointer_comparison_controls }
};
