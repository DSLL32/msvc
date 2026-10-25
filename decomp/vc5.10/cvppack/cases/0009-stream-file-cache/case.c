#include <recon-tool.h>
extern int cvpack_cache_direct(void);
static const unsigned int entries[] = {0x4620,0x10a0,0x1f10,0x1010,0x1190,0x1310,0x1380,0x1470,0x1400,0x20a0,0x1070,0x1510,0x1810,0x1f30,0x2040,0x1aa0,0x1de0,0x1980,0x19b0,0x1cc0,0x1cf0,0x1b20,0x1bf0,0x1a80,0x1680,0x1710,0x1ea0,0x1000,0x1910,0x1790,0x1fc0,0x11b0,0x1240};
RECON_EXPORT const ReconCase case_descriptor = {
 .abi_version=RECON_ABI_VERSION,.struct_size=sizeof(ReconCase),
 .name="stream-file-cache",.description="Internal file API: genuine initialized unmapped owners, page-crossing copy/readback, resize and cache reuse/eviction; no ordinary CLI reach claim",
 .first_entry_order=entries,.first_entry_count=sizeof(entries)/sizeof(entries[0]),
 .execution=RECON_DIRECT,.terminal=RECON_EXIT,.direct=cvpack_cache_direct
};
