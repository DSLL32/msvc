/* Real nested environment options reject the later-only /fr spelling. */
#include <recon-tool.h>
#include "wasm.h"
#include <recon.h>
/* Static-empty shared interface entry; no independent entry observation. */
static const uint32_t shared_empty_seen[] = { 0xf6f4 };
static const uint32_t order[] = { 0xcb28, 0xcc0c, 0x1bc90, 0x1aaa8, 0x168c4, 0x161ec, 0x1aa80, 0x1bb50, 0xf6c0, 0xca94, 0x166a4, 0x1e708, 0x205d0, 0x1e758, 0x16688, 0x1e718, 0x20600, 0x20bd4, 0x21f70, 0x22660, 0x209a8, 0x21730, 0x217c4, 0x208b0, 0x216f0, 0x21670, 0x215c0, 0x20708, 0x21630, 0x6e68, 0xba18, 0xb98c, 0xb91c, 0xb8d0, 0xd184, 0xc970, 0x19a1c, 0x79a8, 0x71c0, 0x19ae4, 0x8b04, 0x883c, 0x87f0, 0xfa3c, 0x8a64, 0xdd50, 0x19e30, 0x19e70, 0x15f58, 0x15884, 0x15e08, 0xd0e8, 0xdcf4, 0xd7bc, 0xccb0, 0x16774, 0xcb74, 0x16710, 0x1e668, 0x204b0, 0x20548, 0x204c0, 0x21570, 0x204e4, 0x20508, 0x205b0, 0x1e520 };
RECON_EXPORT const ReconCase case_descriptor = {
    .abi_version = RECON_ABI_VERSION,
    .struct_size = sizeof(ReconCase),
    .name = "0010-option-contract",
    .description = "Nested environment options and unsupported-option diagnostic",
    .first_entry_order = order,
    .first_entry_count = RECON_COUNT(order),
    .execution = RECON_IO,
    .terminal = RECON_EXIT,
    .first_entry_seen = shared_empty_seen,
    .first_entry_seen_count = RECON_COUNT(shared_empty_seen),
    .io = main,
};
