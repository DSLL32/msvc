/* Named pending App callees. No bodies or runtime effects are accepted.
 * The original file-local main helpers are externally named here so their
 * existing names remain visible in the MAP and native function registry. */
#include "wasm.h"
#include <recon.h>

void *AsmAlloc(size_t size)
{
    (void)size; WASM_PENDING(); return NULL;
}
void Fatal(unsigned message, ...) { (void)message; WASM_PENDING(); }
void newBlk(cv_t *cv) { (void)cv; WASM_PENDING(); }
void GenMSOmfInit(void) { WASM_PENDING(); }
void do_init_stuff(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    WASM_PENDING();
}
void WriteObjModule(void) { WASM_PENDING(); }
void PrintStats(void) { WASM_PENDING(); }
void MsgFini(void) { WASM_PENDING(); }
void main_fini(void) { WASM_PENDING(); }
