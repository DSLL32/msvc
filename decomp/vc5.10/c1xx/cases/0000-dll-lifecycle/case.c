#include "cases.h"
#include <recon-tool.h>
#include <recon.h>

static const uint32_t order[] = { 0x2e52a };
RECON_EXPORT const ReconCase case_descriptor = {
    RECON_ABI_VERSION, sizeof(ReconCase), "dll-lifecycle-and-exports",
    "Original-backed attach, resolve Invoke/Abort without calls, detach; exact export names and ordinals are in binary_descriptor.",
    order, RECON_COUNT(order), NULL, 0,
    RECON_IO, RECON_EXIT, 0, NULL, c1xx_case_lifecycle, NULL, NULL, 0
};
