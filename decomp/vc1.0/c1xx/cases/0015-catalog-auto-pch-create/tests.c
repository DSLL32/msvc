#define RECON_SHIM_IMPL
#include <stdlib.h>
#include <string.h>
#include <recon-tool.h>
#include <recon-fixture.h>
#include "case.h"
#include "literal-workspace.h"

/* Only controls for the user-approved finite comparison, not new compiler
   invocations. Start with the raw oracle, zero its proven unwritten output,
   and bind the three address-only ELF workspace identities to this mapping. */
static int comparison_controls(void)
{
    static const unsigned positions[] = { 0x577d, 0x14779 };
    static const unsigned reserved[][2] = {
        { 0, 0xd324 }, { 0, 0xd334 }, { 0, 0xe228 },
        { 0, 0xe238 }, { 1, 0x153c }, { 1, 0x154c }
    };
    static const unsigned workspace[][3] = {
        { 0, 0xb63c, 9 }, { 1, 0xb68, 4 }, { 1, 0xb90, 16 }
    };
    unsigned char *oracle = catalog_pch_read(recon_case_assets(), "oracle.pch");
    unsigned char *actual = malloc(CATALOG_PCH_BYTES), *changed = malloc(CATALOG_PCH_BYTES);
    unsigned i;
    uint32_t pointer;
    int ok = 0;
    if (!oracle || !actual || !changed) goto done;
    memcpy(actual, oracle, CATALOG_PCH_BYTES);
    for (i = 0; i < RECON_COUNT(reserved); ++i)
        memset(actual + positions[reserved[i][0]] + reserved[i][1], 0, 8);
    for (i = 0; i < RECON_COUNT(workspace); ++i) {
        pointer = (uintptr_t)&literal_workspace + workspace[i][2];
        memcpy(actual + positions[workspace[i][0]] + workspace[i][1], &pointer, 4);
    }
    if (!catalog_pch_compare(actual, oracle, CATALOG_PCH_BYTES)) goto done;
    memcpy(changed, actual, CATALOG_PCH_BYTES);
    changed[positions[0] + 0x4020] ^= 4;
    if (catalog_pch_compare(changed, oracle, CATALOG_PCH_BYTES)) goto done;
    memcpy(changed, actual, CATALOG_PCH_BYTES);
    /* A distinct temporary role may not alias the first group's anchor. */
    memcpy(changed + positions[0] + 0xa7c0, changed + positions[0] + 0x9ca4, 4);
    if (catalog_pch_compare(changed, oracle, CATALOG_PCH_BYTES)) goto done;
    memcpy(changed, actual, CATALOG_PCH_BYTES);
    changed[positions[0] + 0xd324] = 1;
    if (catalog_pch_compare(changed, oracle, CATALOG_PCH_BYTES)) goto done;
    memcpy(changed, actual, CATALOG_PCH_BYTES);
    changed[positions[0] + 0xba46] ^= 1;
    if (catalog_pch_compare(changed, oracle, CATALOG_PCH_BYTES)) goto done;
    memcpy(changed, actual, CATALOG_PCH_BYTES);
    /* Only the approved three discarded WORDs and upper14bits may vary. */
    memset(changed + positions[0] + 0xb642, 0x5a, 2);
    memset(changed + positions[1] + 0xb6e, 0xa5, 2);
    memset(changed + positions[1] + 0xb96, 0xff, 2);
    changed[positions[0] + 0xba46] ^= 0xfc;
    changed[positions[0] + 0xba47] ^= 0xff;
    if (!catalog_pch_compare(changed, oracle, CATALOG_PCH_BYTES)) goto done;
    ok = 1;
done:
    free(changed);
    free(actual);
    free(oracle);
    return ok;
}

const ReconTest catalog_pch_tests[1] = {
    { "finite-catalog-pch-comparison", 1, comparison_controls }
};
