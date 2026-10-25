/* Named pending App callees. No bodies or runtime effects are accepted.
 * The original file-local main helpers are externally named here so their
 * existing names remain visible in the MAP and native function registry. */
#include "wasm.h"
#include <recon.h>

void Fatal(unsigned message, ...) { (void)message; WASM_PENDING(); }
void PObjRegList(const pobj_list *list, size_t len)
{ (void)list; (void)len; WASM_PENDING(); }
int writeTheadr(obj_rec *objr, pobj_state *state)
{ (void)objr; (void)state; WASM_PENDING(); return 0; }
int writeComent(obj_rec *objr, pobj_state *state)
{ (void)objr; (void)state; WASM_PENDING(); return 0; }
int writeModend(obj_rec *objr, pobj_state *state)
{ (void)objr; (void)state; WASM_PENDING(); return 0; }
int writeMisc(obj_rec *objr, pobj_state *state)
{ (void)objr; (void)state; WASM_PENDING(); return 0; }
int writeSegdef(obj_rec *objr, pobj_state *state)
{ (void)objr; (void)state; WASM_PENDING(); return 0; }
int writeLedata(obj_rec *objr, pobj_state *state)
{ (void)objr; (void)state; WASM_PENDING(); return 0; }
int writeFixup(obj_rec *objr, pobj_state *state)
{ (void)objr; (void)state; WASM_PENDING(); return 0; }
int writePubdef(obj_rec *objr, pobj_state *state)
{ (void)objr; (void)state; WASM_PENDING(); return 0; }
int writeLinnum(obj_rec *objr, pobj_state *state)
{ (void)objr; (void)state; WASM_PENDING(); return 0; }
int writeLidata(obj_rec *objr, pobj_state *state)
{ (void)objr; (void)state; WASM_PENDING(); return 0; }
int writeMisc32(obj_rec *objr, pobj_state *state)
{ (void)objr; (void)state; WASM_PENDING(); return 0; }
int writeComdat(obj_rec *objr, pobj_state *state)
{ (void)objr; (void)state; WASM_PENDING(); return 0; }
int writeLinsym(obj_rec *objr, pobj_state *state)
{ (void)objr; (void)state; WASM_PENDING(); return 0; }

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
