#include <recon-tool.h>
extern int rcdll_native_invoke(int, char **);
static const uint32_t order[] = {0x8b29,0x8b46,0x7443,0x84c4,0x109b2,0x10c4c,0x8bcd,0x2c64,0x2c98};
RECON_EXPORT const ReconCase case_descriptor = {
 .abi_version=RECON_ABI_VERSION, .struct_size=sizeof(ReconCase),
 .name="0000-help", .description="Cached original RC export /help; real attach, mutable diagnostic callback, detach.",
 .first_entry_order=order, .first_entry_count=RECON_COUNT(order),
 .execution=RECON_IO, .terminal=RECON_EXIT,
 .io=rcdll_native_invoke
};
