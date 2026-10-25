/* Initial '/?' invocation. No cached oracle is invented during migration.
 * Static source candidate only. A fresh pair must qualify main and this
 * first named callee frontier; there is still no cached output oracle. */
#include <recon-tool.h>
#include "wasm.h"
#include <recon.h>

static const uint32_t order[] = { 0xcb28, 0xcc0c, 0x1bc90, 0x1aaa8, 0x168c4, 0x161ec, 0x1aa80, 0x1bb50, 0xf6c0, 0xca94 };
RECON_EXPORT const ReconCase case_descriptor = {
    .abi_version = RECON_ABI_VERSION,
    .struct_size = sizeof(ReconCase),
    .name = "0001-help",
    .description = "Patched Watcom 10.0a WASM /? through the original binnt loader",
    .first_entry_order = order,
    .first_entry_count = RECON_COUNT(order),
    .execution = RECON_IO,
    .terminal = RECON_FRONTIER,
    .frontier_rva = 0xca94,
    .io = main,
};
