#define RECON_SHIM_IMPL
#include <stdlib.h>
#include <string.h>
#include <recon-tool.h>
#include <recon-fixture.h>
#include "case.h"

static int comparison_controls(void)
{
    const unsigned reserved[] = { 0xadf4, 0xba94, 0xc948, 0xc9b0 };
    unsigned char *oracle = device_pch_read(recon_case_assets(), "oracle.pch");
    unsigned char *actual = malloc(DEVICE_PCH_BYTES), *changed = malloc(DEVICE_PCH_BYTES);
    unsigned i;
    int ok = 0;
    if (!oracle || !actual || !changed) goto done;
    memcpy(actual, oracle, DEVICE_PCH_BYTES);
    for (i = 0; i < RECON_COUNT(reserved); ++i)
        memset(actual + 0x5aba + reserved[i], 0, 8);
    if (!device_pch_compare(actual, oracle, DEVICE_PCH_BYTES)) goto done;
    memcpy(changed, actual, DEVICE_PCH_BYTES);
    changed[0x5aba + 0x95c8] ^= 4;
    if (device_pch_compare(changed, oracle, DEVICE_PCH_BYTES)) goto done;
    memcpy(changed, actual, DEVICE_PCH_BYTES);
    memset(changed + 0x5aba + 0x9a44, 0, 4);
    if (device_pch_compare(changed, oracle, DEVICE_PCH_BYTES)) goto done;
    memcpy(changed, actual, DEVICE_PCH_BYTES);
    memcpy(changed + 0x5aba + 0xb49c, changed + 0x5aba + 0x9a44, 4);
    if (device_pch_compare(changed, oracle, DEVICE_PCH_BYTES)) goto done;
    memcpy(changed, actual, DEVICE_PCH_BYTES);
    changed[0x5aba + reserved[0]] = 1;
    if (device_pch_compare(changed, oracle, DEVICE_PCH_BYTES)) goto done;
    memcpy(changed, actual, DEVICE_PCH_BYTES);
    changed[0] ^= 1;
    if (device_pch_compare(changed, oracle, DEVICE_PCH_BYTES)) goto done;
    ok = 1;
done:
    free(changed); free(actual); free(oracle);
    return ok;
}
const ReconTest device_pch_tests[1] = {
    { "finite-device-sdk-pch-comparison", 1, comparison_controls }
};
