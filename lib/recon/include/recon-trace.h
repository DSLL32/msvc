#ifndef RECON_TRACE_H
#define RECON_TRACE_H
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Host-only tracing registration. Resolve application ELF symbols in the tool;
 * omit CRT/import/shim and case/test glue. Addresses and labels are copied. */
typedef struct ReconTraceEntry {
    const void *address;
    const char *name;
} ReconTraceEntry;
/* The caller owns fd. Configure starts disabled and initializes a fork-shared
 * first-entry bitmap. Invalid inputs fail with the legacy trace status126. */
void recon_trace_configure(const ReconTraceEntry *,size_t count,int fd);
void recon_trace_enable(int enabled);
/* Call only after all execution descendants have been reaped. Reset disables
 * tracing and clears registered seen bits; configure also releases old state.
 * Already-seen RVA seeds are omitted by the tool, not stored in this API. */
void recon_trace_reset(void);
#ifdef __cplusplus
}
#endif
#endif
