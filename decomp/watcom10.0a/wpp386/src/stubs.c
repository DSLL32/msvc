/* Pending application entries. */
#include "memory.h"
#include "compiler.h"
#include "frontend.h"
#include "initstate.h"
#include "carve.h"
#include "cgio.h"
#include "macro.h"
#include "vstk.h"
#include "types.h"
#include "cpragma.h"
#include "pending.h"
#include <recon.h>

int myatoi(char *text) { (void)text; STUB_TRAP(); return 0; }
void myitoa(int value, char *text) { (void)value; (void)text; STUB_TRAP(); }
char *CopyStr(char *source, char *dest) { (void)source; (void)dest; STUB_TRAP(); return 0; }
unsigned Length(char *text) { (void)text; STUB_TRAP(); return 0; }

void ExitPointReleaseRtn(EXIT_POINT *point) { (void)point; STUB_TRAP(); }
void *alloc_from_cleanup(unsigned size) { (void)size; STUB_TRAP(); return 0; }
void cmemFini(INITFINI *entry) { (void)entry; STUB_TRAP(); }
void CErr1(unsigned code) { (void)code; STUB_TRAP(); }
void CSuicide(void) { STUB_TRAP(); }
void cppNamesFini(INITFINI *entry) { (void)entry; STUB_TRAP(); }
void hfileFini(INITFINI *entry) { (void)entry; STUB_TRAP(); }
void fnameFini(INITFINI *entry) { (void)entry; STUB_TRAP(); }
void ioSuppFini(INITFINI *entry) { (void)entry; STUB_TRAP(); }
void reserveRelease(void) { STUB_TRAP(); }
void errFileFini(INITFINI *entry) { (void)entry; STUB_TRAP(); }

/* Pending callbacks referenced by the encountered driver tables. */
void cgioFini(INITFINI *entry) { (void)entry; STUB_TRAP(); }
void rtnGenFini(INITFINI *entry) { (void)entry; STUB_TRAP(); }
void srcFileFini(INITFINI *entry) { (void)entry; STUB_TRAP(); }
void typesFini(INITFINI *entry) { (void)entry; STUB_TRAP(); }
void typeCompareFini(INITFINI *entry) { (void)entry; STUB_TRAP(); }
void preprocInit0(INITFINI *entry) { (void)entry; STUB_TRAP(); }
void preprocFini0(INITFINI *entry) { (void)entry; STUB_TRAP(); }
void preprocInit1(INITFINI *entry) { (void)entry; STUB_TRAP(); }
void preprocFini1(INITFINI *entry) { (void)entry; STUB_TRAP(); }
void preprocInit2(INITFINI *entry) { (void)entry; STUB_TRAP(); }
void InitFiniStub(INITFINI *entry) { (void)entry; STUB_TRAP(); }
void analysisInit0(INITFINI *entry) { (void)entry; STUB_TRAP(); }
void analysisFini0(INITFINI *entry) { (void)entry; STUB_TRAP(); }
void analysisInit1(INITFINI *entry) { (void)entry; STUB_TRAP(); }
void analysisFini1(INITFINI *entry) { (void)entry; STUB_TRAP(); }
void analysisInit2(INITFINI *entry) { (void)entry; STUB_TRAP(); }
void analysisFini2(INITFINI *entry) { (void)entry; STUB_TRAP(); }
void objectInit0(INITFINI *entry) { (void)entry; STUB_TRAP(); }
void objectFini0(INITFINI *entry) { (void)entry; STUB_TRAP(); }
void objectInit1(INITFINI *entry) { (void)entry; STUB_TRAP(); }
void objectFini1(INITFINI *entry) { (void)entry; STUB_TRAP(); }
void objectInit2(INITFINI *entry) { (void)entry; STUB_TRAP(); }
void objectFini2(INITFINI *entry) { (void)entry; STUB_TRAP(); }
void objectInit3(INITFINI *entry) { (void)entry; STUB_TRAP(); }
void objectFini3(INITFINI *entry) { (void)entry; STUB_TRAP(); }
void objectInit4(INITFINI *entry) { (void)entry; STUB_TRAP(); }
void objectFini4(INITFINI *entry) { (void)entry; STUB_TRAP(); }
void objectInit5(INITFINI *entry) { (void)entry; STUB_TRAP(); }
void objectFini5(INITFINI *entry) { (void)entry; STUB_TRAP(); }
void objectInit6(INITFINI *entry) { (void)entry; STUB_TRAP(); }
void objectFini6(INITFINI *entry) { (void)entry; STUB_TRAP(); }
void objectInit7(INITFINI *entry) { (void)entry; STUB_TRAP(); }
void objectFini7(INITFINI *entry) { (void)entry; STUB_TRAP(); }
void objectInit8(INITFINI *entry) { (void)entry; STUB_TRAP(); }
void objectFini8(INITFINI *entry) { (void)entry; STUB_TRAP(); }
void objectInit9(INITFINI *entry) { (void)entry; STUB_TRAP(); }
void objectFini9(INITFINI *entry) { (void)entry; STUB_TRAP(); }
void objectInit10(INITFINI *entry) { (void)entry; STUB_TRAP(); }
void objectFini10(INITFINI *entry) { (void)entry; STUB_TRAP(); }
void objectInit11(INITFINI *entry) { (void)entry; STUB_TRAP(); }
void objectFini11(INITFINI *entry) { (void)entry; STUB_TRAP(); }
void objectInit12(INITFINI *entry) { (void)entry; STUB_TRAP(); }
void objectInit13(INITFINI *entry) { (void)entry; STUB_TRAP(); }
void objectFini13(INITFINI *entry) { (void)entry; STUB_TRAP(); }
void objectInit14(INITFINI *entry) { (void)entry; STUB_TRAP(); }
void objectFini14(INITFINI *entry) { (void)entry; STUB_TRAP(); }
void objectInit15(INITFINI *entry) { (void)entry; STUB_TRAP(); }
void objectFini15(INITFINI *entry) { (void)entry; STUB_TRAP(); }
void objectInit16(INITFINI *entry) { (void)entry; STUB_TRAP(); }
void objectFini16(INITFINI *entry) { (void)entry; STUB_TRAP(); }
void objectInit17(INITFINI *entry) { (void)entry; STUB_TRAP(); }
void objectFini17(INITFINI *entry) { (void)entry; STUB_TRAP(); }
void statsPrint(INITFINI *entry) { (void)entry; STUB_TRAP(); }
void macroFini(INITFINI *entry) { (void)entry; STUB_TRAP(); }
int makeExitStatus(int fatal) { (void)fatal; STUB_TRAP(); return 0; }
void PTypeCheckInit(void) { STUB_TRAP(); }
void ErrFileOpen(void) { STUB_TRAP(); }
void PpOpen(void) { STUB_TRAP(); }
void OpenPgmFile(void) { STUB_TRAP(); }
void PrtChar(int ch) { (void)ch; STUB_TRAP(); }
int OpenSrcFile(char *filename, int is_lib) { (void)filename; (void)is_lib; STUB_TRAP(); return 0; }
void PpParse(void) { STUB_TRAP(); }
void CgFrontModInitInit(void) { STUB_TRAP(); }
void ParseDecls(void) { STUB_TRAP(); }
void RtnGenerate(void) { STUB_TRAP(); }
void ModuleInitFini(void) { STUB_TRAP(); }
void ScopeEndFileScope(void) { STUB_TRAP(); }
void ScopeCreatePCHDebugSym(void) { STUB_TRAP(); }
void CgBackEnd(void) { STUB_TRAP(); }
void CgFrontFini(void) { STUB_TRAP(); }
void cgioBuffReleaseMemory(void) { STUB_TRAP(); }
int macroCompare(MEPTR first, MEPTR second) { (void)first; (void)second; STUB_TRAP(); return 0; }
unsigned CErr2p(unsigned code, const char *text) { (void)code; (void)text; STUB_TRAP(); return 0; }

void cleanupOpenFiles(void) { STUB_TRAP(); }





void typeFree(TYPE type) { (void)type; STUB_TRAP(); }
void *RingPromote(void **head, void *item, void *previous) { (void)head; (void)item; (void)previous; STUB_TRAP(); return 0; }



unsigned lookupMagicKeyword(char *name) { (void)name; STUB_TRAP(); return 0; }


void GenCOptions(char **argv) { (void)argv; STUB_TRAP(); }
void MakePgmName(char *filename) { (void)filename; STUB_TRAP(); }
