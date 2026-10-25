#include "case-host.h"

static const uint32_t order[] = {
    0x528b0,
    0x52a22,
    0x52acb,
    0x52bb7,
    0x52bd9,
    0x8b7f8,
    0x52cfe,
    0x8b367,
    0x8b6e8,
    0x8b708,
    0x8b78c,
    0x8b7ba,
    0x8b3d0,
    0x8b6ce,
    0x8b6ae,
    0x8b566,
    0x8b57e,
    0x466a6,
    0x54b04,
};

RECON_EXPORT const ReconCase case_descriptor = {
    RECON_ABI_VERSION, sizeof(ReconCase), "unsupported-help-diagnostic",
    "Complete original unsupported /? diagnostic, exactly 51 stderr bytes and exit 1.",
    order, RECON_COUNT(order), NULL, 0,
    RECON_IO, RECON_EXIT, 0x0,
    setup, invoke, NULL, NULL, 0
};
