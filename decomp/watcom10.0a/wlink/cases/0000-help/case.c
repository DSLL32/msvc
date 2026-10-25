/* One initial case only: bare question mark, NUL input, regular-file output.
 * New supported-path captures will be authored explicitly by the shared tool. */
#include <recon-tool.h>
#include <recon.h>
extern int main( int, char ** );

static int invoke( int argc, char **argv )
{
    return main( argc, argv );
}
static const uint32_t order[] = { 0x242e0, 0x16d3c, 0x243c8, 0x245ac, 0x33300, 0x24850, 0x348b8, 0x36880, 0x34908, 0x25090 };
RECON_EXPORT const ReconCase case_descriptor = {
    .abi_version = RECON_ABI_VERSION, .struct_size = sizeof(ReconCase),
    .name = "help", .description = "WLINK bare ? help; reconstruction in progress",
    .first_entry_order = order, .first_entry_count = RECON_COUNT(order),
    .execution = RECON_IO, .terminal = RECON_FRONTIER,
    .frontier_rva = 0x25090, .io = invoke
};
