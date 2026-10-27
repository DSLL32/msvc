/* Source/static/native units; no observed original execution claim. */
#include <recon-tool.h>
#include <recon.h>
extern int file_reopen_unit( void );
extern const ReconTest file_reopen_tests[];
static const uint32_t order[]={0x24fa8,0x24f88,0x24bf0,0x250cc,0x24e10};
RECON_EXPORT const ReconCase case_descriptor={
 .abi_version=RECON_ABI_VERSION,.struct_size=sizeof(ReconCase),
 .name="0046-file-reopen-units",.description="Owned existing binary files and genuine overlay reopen/create transitions",
 .execution=RECON_DIRECT,.terminal=RECON_EXIT,.direct=file_reopen_unit,
 .first_entry_order=order,.first_entry_count=RECON_COUNT(order),
 .tests=file_reopen_tests,.test_count=3
};
