/* Named pending App callees. No bodies or runtime effects are accepted.
 * The original file-local main helpers are externally named here so their
 * existing names remain visible in the MAP and native function registry. */
#include "asminit.h"
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

void WresRecordError(int status) { (void)status; WASM_PENDING(); }
void __FreeTypeList(WResDir dir) { (void)dir; WASM_PENDING(); }
int WResReadExtraWResID(WResID *id, int handle)
{ (void)id; (void)handle; WASM_PENDING(); return 0; }
int WResReadFixedResRecord(WResResInfo *info, int handle)
{ (void)info; (void)handle; WASM_PENDING(); return 0; }
int readLangInfoList(int handle, WResResNode *resource)
{ (void)handle; (void)resource; WASM_PENDING(); return 0; }
int readMResDir(int handle, WResDir dir, int *duplicate_discarded)
{ (void)handle; (void)dir; (void)duplicate_discarded; WASM_PENDING(); return 0; }

void AsmError(int message) { (void)message; WASM_PENDING(); }
int get_id(unsigned int *index, char **input, char **output)
{ (void)index; (void)input; (void)output; WASM_PENDING(); return -1; }
int get_inc_path(unsigned int *index, char **input, char **output)
{ (void)index; (void)input; (void)output; WASM_PENDING(); return -1; }
int get_float(struct asm_tok *buf, char **input, char **output)
{ (void)buf; (void)input; (void)output; WASM_PENDING(); return -1; }
int get_string(struct asm_tok *buf, char **input, char **output)
{ (void)buf; (void)input; (void)output; WASM_PENDING(); return -1; }

int get_id_in_backquotes(struct asm_tok *buf, char **input, char **output)
{ (void)buf; (void)input; (void)output; WASM_PENDING(); return -1; }
int get_special_symbol(struct asm_tok *buf, char **input, char **output)
{ (void)buf; (void)input; (void)output; WASM_PENDING(); return -1; }

int InternalError(const char *file, unsigned line)
{ (void)file; (void)line; WASM_PENDING(); return 0; }
void FreeInfo(dir_node *dir) { (void)dir; WASM_PENDING(); }
int ExpandSymbol(int index, unsigned char early_only)
{ (void)index; (void)early_only; WASM_PENDING(); return -1; }
int is_expr(int index) { (void)index; WASM_PENDING(); return 0; }
struct expr_list *evaluate(int *index, int end)
{ (void)index; (void)end; WASM_PENDING(); return NULL; }
int fix(struct expr_list *result, int start, int end)
{ (void)result; (void)start; (void)end; WASM_PENDING(); return 0; }
char *set_processor_type(char *input)
{ (void)input; WASM_PENDING(); return NULL; }
void set_mem_type(int type) { (void)type; WASM_PENDING(); }
void set_some_kinda_name(int token, char *name)
{ (void)token; (void)name; WASM_PENDING(); }
void get_fname(char *token, int type)
{ (void)token; (void)type; WASM_PENDING(); }
void InputQueueFile(char *filename) { (void)filename; WASM_PENDING(); }
void AsmFree(void *memory) { (void)memory; WASM_PENDING(); }
void MsgPrintf1(int resourceid, const char *arg)
{ (void)resourceid; (void)arg; WASM_PENDING(); }
void output(const unsigned char *text) { (void)text; WASM_PENDING(); }
int WResIDNameCmp(const WResIDName *first, const WResIDName *second)
{ (void)first; (void)second; WASM_PENDING(); return 0; }
int set_build_target(void)
{  WASM_PENDING(); return 0; }
void get_os_include(void)
{  WASM_PENDING();  }
void AddStringToIncludePath(char *path)
{ (void)path; WASM_PENDING();  }
void open_files(void)
{  WASM_PENDING();  }
void PushLineQueue(void)
{  WASM_PENDING();  }
void WriteObjModule(void) { WASM_PENDING(); }
void PrintStats(void) { WASM_PENDING(); }
void MsgFini(void) { WASM_PENDING(); }
void main_fini(void) { WASM_PENDING(); }
