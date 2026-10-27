/* Source/static/native units, not observed original CLI execution. */
#include <recon-tool.h>
#include <recon.h>
extern int final_control_unit( void );
extern const ReconTest final_control_tests[];
static const uint32_t order[]={0x1244c,0x123c8};
RECON_EXPORT const ReconCase case_descriptor={
 .abi_version=RECON_ABI_VERSION,.struct_size=sizeof(ReconCase),
 .name="0051-final-control-units",.description="Owned SYSTEM command source and message-disabled EOF/break state",
 .execution=RECON_DIRECT,.terminal=RECON_EXIT,.direct=final_control_unit,
 .first_entry_order=order,.first_entry_count=RECON_COUNT(order),
 .tests=final_control_tests,.test_count=3
};
