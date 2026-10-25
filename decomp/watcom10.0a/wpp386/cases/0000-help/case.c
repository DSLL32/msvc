/* One initial public help case. No fabricated oracle or trace is supplied.
 * The first real App entry is the allocator used by CRT stream initialization.
 */
#include <recon-tool.h>
#include "startup.h"
#include <recon.h>

/* Candidate's next expected frontier; no fresh acceptance claimed yet. */
static const uint32_t first_entry_order[] = { 0x81ba3, 0x81e41, 0xa1599, 0x81ab5, 0x81add };

RECON_EXPORT const ReconCase case_descriptor = {
    .abi_version=RECON_ABI_VERSION, .struct_size=sizeof(ReconCase),
    .name="help", .description="Public WPP386 ?; private allocator initialization frontier",
    .first_entry_order=first_entry_order, .first_entry_count=RECON_COUNT(first_entry_order),
    .execution=RECON_IO, .terminal=RECON_FRONTIER, .frontier_rva=0x81add,
    .io=wpp_native_startup
};
