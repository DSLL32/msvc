/* Named pending App callees. No bodies or runtime effects are accepted.
 * The original file-local main helpers are externally named here so their
 * existing names remain visible in the MAP and native function registry. */
#include "breakout.h"
#include "direct.h"
#include <recon.h>

void Fatal(unsigned message, ...) { (void)message; WASM_PENDING(); }
void DiskError(void) { WASM_PENDING(); }
size_t FixGenPRef(physref *ref, char *buffer, int type) { WASM_PENDING(); return 0; }




char *put32(char *p, unsigned long value)
{ (void)p; (void)value; WASM_PENDING(); return NULL; }
int writeLidata(obj_rec *objr, pobj_state *state)
{ (void)objr; (void)state; WASM_PENDING(); return 0; }
int writeMisc32(obj_rec *objr, pobj_state *state)
{ (void)objr; (void)state; WASM_PENDING(); return 0; }
int writeComdat(obj_rec *objr, pobj_state *state)
{ (void)objr; (void)state; WASM_PENDING(); return 0; }
int writeLinsym(obj_rec *objr, pobj_state *state)
{ (void)objr; (void)state; WASM_PENDING(); return 0; }

void WresRecordError(int status) { (void)status; WASM_PENDING(); }


int WResReadExtraWResID(WResID *id, int handle)
{ (void)id; (void)handle; WASM_PENDING(); return 0; }
int WResReadFixedResRecord(WResResInfo *info, int handle)
{ (void)info; (void)handle; WASM_PENDING(); return 0; }
int readLangInfoList(int handle, WResResNode *resource)
{ (void)handle; (void)resource; WASM_PENDING(); return 0; }
int readMResDir(int handle, WResDir dir, int *duplicate_discarded)
{ (void)handle; (void)dir; (void)duplicate_discarded; WASM_PENDING(); return 0; }

void AsmError(int message) { (void)message; WASM_PENDING(); }






int InternalError(const char *file, unsigned line)
{ (void)file; (void)line; WASM_PENDING(); return 0; }
void MsgPrintf1(int resourceid, const char *arg)
{ (void)resourceid; (void)arg; WASM_PENDING(); }
void output(const unsigned char *text) { (void)text; WASM_PENDING(); }
int WResIDNameCmp(const WResIDName *first, const WResIDName *second)
{ (void)first; (void)second; WASM_PENDING(); return 0; }





























int comp_mem(int first, int second) { (void)first; (void)second; WASM_PENDING(); return -1; }

void jumpExtend(void) { WASM_PENDING(); }














void push_proc(dir_node *procedure) { (void)procedure; WASM_PENDING(); }




















unsigned GetGrpIdx(asm_sym *sym) { (void)sym; WASM_PENDING(); return 0; }
