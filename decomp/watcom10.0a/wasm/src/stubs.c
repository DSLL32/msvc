/* Named pending App callees. No bodies or runtime effects are accepted.
 * The original file-local main helpers are externally named here so their
 * existing names remain visible in the MAP and native function registry. */
#include "breakout.h"
#include "direct.h"
#include <recon.h>

void DiskError(void) { WASM_PENDING(); }

int InternalError(const char *file, unsigned line)
{ (void)file; (void)line; WASM_PENDING(); return 0; }
