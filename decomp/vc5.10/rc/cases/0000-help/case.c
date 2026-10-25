#include <recon-tool.h>
#include "../../harness/provider.h"
static const uint32_t order[] = {0x11d2, 0x1100};
static const ReconTest tests[] = {
    {"uncaptured-export-flags-denied", 1, rc_provider_denies_flags},
    {"uncaptured-handler-denied", 1, rc_provider_denies_handler}
};
RECON_EXPORT const ReconCase case_descriptor = {
    .abi_version = RECON_ABI_VERSION, .struct_size = sizeof(ReconCase),
    .name = "help",
    .description = "Original RC help, exact four mutable diagnostics and imported dependency contract.",
    .first_entry_order = order, .first_entry_count = RECON_COUNT(order),
    .execution = RECON_IO, .terminal = RECON_EXIT,
    .setup = rc_provider_setup, .io = rc_provider_run,
    .tests = tests, .test_count = RECON_COUNT(tests)
};
