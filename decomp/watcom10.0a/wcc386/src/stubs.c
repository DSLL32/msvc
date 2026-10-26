#include "memory.h"
#include "compiler.h"
#include "cmemory.h"
#include "types.h"
#include "macros.h"
#include "symbols.h"
#include "quads.h"
#include "command.h"
#include "stubs.h"
#include <recon.h>
int myatoi(char *text) { (void)text; STUB_TRAP(); return 0; }
void myitoa(int value,char *text) { (void)value; (void)text; STUB_TRAP(); }
char *CopyStr(char *src,char *dst) { (void)src; (void)dst; STUB_TRAP(); return 0; }
unsigned Length(char *text) { (void)text; STUB_TRAP(); return 0; }
void Ccoalesce(MCB *block) { (void)block; STUB_TRAP(); }
void PurgeMemory(void) { STUB_TRAP(); }

void PrintStats(void) { STUB_TRAP(); }
void EmitAbort(void) { STUB_TRAP(); }
void PragmaFini(void) { STUB_TRAP(); }
void FreeFNames(void) { STUB_TRAP(); }
void CErr1(int code) { (void)code; STUB_TRAP(); }
void DelErrFile(void) { STUB_TRAP(); }
void OpenErrFile(void) { STUB_TRAP(); }
void MergeInclude(void) { STUB_TRAP(); }
void EmitInit(void) { STUB_TRAP(); }
void PrtChar(int character) { (void)character; STUB_TRAP(); }
int OpenSrcFile(char *name,int flag) { (void)name; (void)flag; STUB_TRAP(); return 0; }
void Parse(void) { STUB_TRAP(); }

void PageOutQuads(void) { STUB_TRAP(); }
void PageOutLeafs(void) { STUB_TRAP(); }
void PageOutSyms(void) { STUB_TRAP(); }
void CSuicide(void) { STUB_TRAP(); }




int MacroCompare(MEPTR first,MEPTR second) { (void)first; (void)second; STUB_TRAP(); return 0; }
void CErr2p(int code,char *text) { (void)code; (void)text; STUB_TRAP(); }




void OpenPageFile(void) { STUB_TRAP(); }


SEGADDR_T AccessSegment(struct seg_info *segment) { (void)segment; STUB_TRAP(); return 0; }
void PageRead(char *buffer,unsigned count) { (void)buffer; (void)count; STUB_TRAP(); }
void PageWrite(char *buffer,unsigned count) { (void)buffer; (void)count; STUB_TRAP(); }
void PageSeek(unsigned long offset) { (void)offset; STUB_TRAP(); }


int GetNextChar(void) { STUB_TRAP(); return 0; }


void LeafGet(LEAFPTR leaf,unsigned index) { (void)leaf; (void)index; STUB_TRAP(); }
void GenCOptions(char **cmdline) { (void)cmdline; STUB_TRAP(); }
void MakePgmName(char *name) { (void)name; STUB_TRAP(); }
int OpenPgmFile(void) { STUB_TRAP(); return 0; }
void MakeTmpName(char *name) { (void)name; STUB_TRAP(); }
