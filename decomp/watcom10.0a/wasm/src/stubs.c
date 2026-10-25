/* Named pending App callees. No bodies or runtime effects are accepted.
 * The original file-local main helpers are externally named here so their
 * existing names remain visible in the MAP and native function registry. */
#include "wasm.h"
#include <recon.h>

void Fatal(unsigned message, ...) { (void)message; WASM_PENDING(); }
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

int OpenResFile(HANDLE_INFO *instance)
{ (void)instance; WASM_PENDING(); return 0; }
int FindResources(HANDLE_INFO *instance)
{ (void)instance; WASM_PENDING(); return 0; }
int InitResources(HANDLE_INFO *instance)
{ (void)instance; WASM_PENDING(); return 0; }

void AsmInit(int cpu, int fpu, int use32)
{ (void)cpu; (void)fpu; (void)use32; WASM_PENDING();  }
void add_constant(char *string)
{ (void)string; WASM_PENDING();  }
void do_envvar_cmdline(char *envvar, int level)
{ (void)envvar; (void)level; WASM_PENDING();  }
void parse_cmdline(int argc, char **argv)
{ (void)argc; (void)argv; WASM_PENDING();  }
int set_build_target(void)
{  WASM_PENDING(); return 0; }
void get_os_include(void)
{  WASM_PENDING();  }
void AddStringToIncludePath(char *path)
{ (void)path; WASM_PENDING();  }
int trademark(void)
{  WASM_PENDING(); return 0; }
void open_files(void)
{  WASM_PENDING();  }
void PushLineQueue(void)
{  WASM_PENDING();  }
void WriteObjModule(void) { WASM_PENDING(); }
void PrintStats(void) { WASM_PENDING(); }
void MsgFini(void) { WASM_PENDING(); }
void main_fini(void) { WASM_PENDING(); }
