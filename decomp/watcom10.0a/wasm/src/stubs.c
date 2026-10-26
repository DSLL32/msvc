/* Named pending App callees. No bodies or runtime effects are accepted.
 * The original file-local main helpers are externally named here so their
 * existing names remain visible in the MAP and native function registry. */
#include "breakout.h"
#include "direct.h"
#include <recon.h>

void Fatal(unsigned message, ...) { (void)message; WASM_PENDING(); }
void DiskError(void) { WASM_PENDING(); }
size_t FixGenPRef(physref *ref, char *buffer, int type) { WASM_PENDING(); return 0; }
void WompPrtMsg(unsigned message, ...) { WASM_PENDING(); }




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




int get_id_in_backquotes(struct asm_tok *buf, char **input, char **output)
{ (void)buf; (void)input; (void)output; WASM_PENDING(); return -1; }


int InternalError(const char *file, unsigned line)
{ (void)file; (void)line; WASM_PENDING(); return 0; }
char *set_processor_type(char *input)
{ (void)input; WASM_PENDING(); return NULL; }
void set_mem_type(int type) { (void)type; WASM_PENDING(); }
void set_some_kinda_name(int token, char *name)
{ (void)token; (void)name; WASM_PENDING(); }
FILE *open_file_in_include_path(char *name, char *fullpath)
{ (void)name; (void)fullpath; WASM_PENDING(); return NULL; }
void MsgPrintf1(int resourceid, const char *arg)
{ (void)resourceid; (void)arg; WASM_PENDING(); }
void output(const unsigned char *text) { (void)text; WASM_PENDING(); }
int WResIDNameCmp(const WResIDName *first, const WResIDName *second)
{ (void)first; (void)second; WASM_PENDING(); return 0; }









void write_flat_grp(void) { WASM_PENDING(); }



















int get_watcom_argument_string(char *buffer, unsigned char size, unsigned char *count)
{ (void)buffer; (void)size; (void)count; WASM_PENDING(); return -1; }

int comp_mem(int first, int second) { (void)first; (void)second; WASM_PENDING(); return -1; }
unsigned GetPrefixAssume(asm_sym *sym, unsigned def)
{ (void)sym; (void)def; WASM_PENDING(); return 0; }

void jumpExtend(void) { WASM_PENDING(); }
int AddFloatingPointEmulationFixup(const struct asm_ins *ins, unsigned char second)
{ (void)ins; (void)second; WASM_PENDING(); return -1; }



int conditional_error_directive(int index) { WASM_PENDING(); (void)index; return 0; }










void push_proc(dir_node *procedure) { (void)procedure; WASM_PENDING(); }
void AsmWarn(int level, int message, ...) { WASM_PENDING(); (void)level; (void)message; }
int AddAlias(int index) { WASM_PENDING(); (void)index; return 0; }











void fill_in_objfile_space(unsigned size) { (void)size; WASM_PENDING(); }







void double2tbyte(double value, char *bytes)
{ (void)value; (void)bytes; WASM_PENDING(); }
void OutSelect(unsigned char starts) { (void)starts; WASM_PENDING(); }


unsigned GetGrpIdx(asm_sym *sym) { (void)sym; WASM_PENDING(); return 0; }

int FixOverride(int index) { (void)index; WASM_PENDING(); return -1; }






void AsmErr(int message, ...) { (void)message; WASM_PENDING(); }

char *CMangler(asm_sym *symbol, char *buffer)
{ (void)symbol; (void)buffer; WASM_PENDING(); return NULL; }
