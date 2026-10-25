/* Invocation descriptor; native DLL adapter is in harness/dll-cases.c. */
#include <stdint.h>
#include "dll-cases.h"
#include <recon-tool.h>
#include <recon.h>
static const uint32_t order[] = { 0x1218e };
RECON_EXPORT const ReconCase case_descriptor = {
 RECON_ABI_VERSION, sizeof(ReconCase), "dll-lifecycle-and-exports",
 "Original attach, resolve exact two exports without invocation, detach.",
 order, RECON_COUNT(order), NULL, 0, RECON_IO, RECON_EXIT, 0,
 NULL, c1_lifecycle_invoke, NULL, NULL, 0
};
