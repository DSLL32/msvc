#include <recon-tool.h>
extern int main( int, char ** );
static int invoke( int argc, char **argv ) { return main( argc, argv ); }
static const uint32_t order[] = { 0x12a67, 0x11fdf, 0x14bfe, 0x18ac8, 0x19c40, 0x18b18, 0x14be4, 0x18ad8, 0x19c70, 0x1a244, 0x1ba00, 0x1c090, 0x1a018, 0x1b1c0, 0x1b254, 0x19f20, 0x1b180 };
RECON_EXPORT const ReconCase case_descriptor = {
    .abi_version = RECON_ABI_VERSION, .struct_size = sizeof(ReconCase),
    .name = "help", .description = "Fresh bare ? with explicit regular-file stdin containing 64 LF bytes",
    .first_entry_order = order, .first_entry_count = RECON_COUNT(order),
    .execution = RECON_IO, .terminal = RECON_FRONTIER, .frontier_rva = 0x1b180,
    .io = invoke
};
