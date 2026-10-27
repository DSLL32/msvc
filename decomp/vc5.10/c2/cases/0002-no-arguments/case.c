#include "case-host.h"

extern const ReconTest noargs_tests[];

static const uint32_t order[] = {
    0x528b0,
    0x52a22,
    0x52acb,
    0x52bb7,
    0x52bd9,
    0x52e9b,
    0x52335,
    0x8b6ce,
    0x8b708,
    0x8b6e8,
    0x8b78c,
    0x8b7ba,
    0x8b6ae,
    0x8b688,
    0x8b367,
    0x8b3d0,
    0x8b566,
    0x8b57e,
    0x466a6,
    0x54b04,
};

RECON_EXPORT const ReconCase case_descriptor = {
    RECON_ABI_VERSION, sizeof(ReconCase), "no-arguments-missing-intermediate",
    "No arguments in an isolated cwd without in: genuine missing-intermediate diagnostic.",
    order, RECON_COUNT(order), NULL, 0,
    RECON_IO, RECON_EXIT, 0,
    setup, invoke, NULL, noargs_tests, 1
};
