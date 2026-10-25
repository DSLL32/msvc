/* Initial '/?' invocation. No cached oracle is invented during migration.
 * Static source candidate only. A fresh pair must qualify main and this
 * first named callee frontier; there is still no cached output oracle. */
#include <recon-tool.h>
#include "wasm.h"
#include <recon.h>

static const uint32_t order[] = { 0xcb28, 0xcc0c };
RECON_EXPORT const ReconCase case_descriptor = {
    .abi_version = RECON_ABI_VERSION,
    .struct_size = sizeof(ReconCase),
    .name = "0001-help",
    .description = "Patched Watcom 10.0a WASM /? through the original binnt loader",
    .first_entry_order = order,
    .first_entry_count = RECON_COUNT(order),
    .execution = RECON_IO,
    .terminal = RECON_FRONTIER,
    .frontier_rva = 0xcc0c,
    .io = main,
};
