/* Named pending App callees. No bodies or runtime effects are accepted.
 * The original file-local main helpers are externally named here so their
 * existing names remain visible in the MAP and native function registry. */
#include "wasm.h"
#include <recon.h>

carve_t CarveCreate(size_t elm_size, size_t blk_size)
{
    (void)elm_size; (void)blk_size; WASM_PENDING(); return NULL;
}
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
