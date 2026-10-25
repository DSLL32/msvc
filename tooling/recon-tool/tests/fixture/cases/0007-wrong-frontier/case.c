#include "recon-tool.h"
extern int fixture_main(int, char **);
static const uint32_t order[] = {0x1000, 0x1400};
const ReconCase case_descriptor = {.abi_version = RECON_ABI_VERSION,
                                   .struct_size = sizeof(ReconCase),
                                   .name = "wrong-frontier",
                                   .description = "trace ends B but INT3 belongs to A",
                                   .first_entry_order = order,
                                   .first_entry_count = 2,
                                   .execution = RECON_IO,
                                   .terminal = RECON_FRONTIER,
                                   .frontier_rva = 0x1400,
                                   .io = fixture_main};
