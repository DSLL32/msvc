/* One initial case only: bare question mark, NUL input, regular-file output.
 * New supported-path captures will be authored explicitly by the shared tool. */
#include <recon-tool.h>
#include <recon.h>
extern int main( int, char ** );

static int invoke( int argc, char **argv )
{
    return main( argc, argv );
}
static const uint32_t order[] = { 0x242e0, 0x16d3c, 0x243c8, 0x245ac, 0x33300, 0x24850, 0x348b8, 0x36880, 0x34908, 0x25090, 0x24e4c, 0x250cc, 0x348c8, 0x368b0, 0x33324, 0x33304, 0x36e84, 0x38bb0, 0x39260, 0x36c58, 0x38370, 0x38404 };
RECON_EXPORT const ReconCase case_descriptor = {
    .abi_version = RECON_ABI_VERSION, .struct_size = sizeof(ReconCase),
    .name = "help", .description = "WLINK bare ? help; reconstruction in progress",
    .first_entry_order = order, .first_entry_count = RECON_COUNT(order),
    .execution = RECON_IO, .terminal = RECON_FRONTIER,
    .frontier_rva = 0x38404, .io = invoke
};
