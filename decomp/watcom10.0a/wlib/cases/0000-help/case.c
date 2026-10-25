#include <recon-tool.h>
extern int main( int, char ** );
static int invoke( int argc, char **argv ) { return main( argc, argv ); }
static const uint32_t order[] = { 0x12a67, 0x11fdf, 0x14bfe, 0x18ac8, 0x19c40 };
RECON_EXPORT const ReconCase case_descriptor = {
    .abi_version = RECON_ABI_VERSION, .struct_size = sizeof(ReconCase),
    .name = "help", .description = "Fresh bare ? with explicit regular-file stdin containing 64 LF bytes",
    .first_entry_order = order, .first_entry_count = RECON_COUNT(order),
    .execution = RECON_IO, .terminal = RECON_FRONTIER, .frontier_rva = 0x19c40,
    .io = invoke
};
