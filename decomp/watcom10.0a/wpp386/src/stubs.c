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
#include "symbol.h"
#include "ptree.h"
#include "fnbody.h"
#include <recon.h>

int myatoi(char *text) { (void)text; STUB_TRAP(); return 0; }
void myitoa(int value, char *text) { (void)value; (void)text; STUB_TRAP(); }
char *CopyStr(char *source, char *dest) { (void)source; (void)dest; STUB_TRAP(); return 0; }
unsigned Length(char *text) { (void)text; STUB_TRAP(); return 0; }

void *alloc_from_cleanup(unsigned size) { (void)size; STUB_TRAP(); return 0; }
void cmemFini(INITFINI *entry) { (void)entry; STUB_TRAP(); }
unsigned CErr1(unsigned code) { (void)code; STUB_TRAP(); return 0; }
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

void preprocFini0(INITFINI *entry) { (void)entry; STUB_TRAP(); }

void pragmaFini(INITFINI *entry) { (void)entry; STUB_TRAP(); }

void gstackFini(INITFINI *entry) { (void)entry; STUB_TRAP(); }
void conPoolsFini(INITFINI *entry) { (void)entry; STUB_TRAP(); }
void cdoptFini(INITFINI *entry) { (void)entry; STUB_TRAP(); }
void extrefFini(INITFINI *entry) { (void)entry; STUB_TRAP(); }
void typeSigFini(INITFINI *entry) { (void)entry; STUB_TRAP(); }
void ptreeFini(INITFINI *entry) { (void)entry; STUB_TRAP(); }
void parseFini(INITFINI *entry) { (void)entry; STUB_TRAP(); }
void hashFini(INITFINI *entry) { (void)entry; STUB_TRAP(); }
void formatTypeFini(INITFINI *entry) { (void)entry; STUB_TRAP(); }
void functionFini(INITFINI *entry) { (void)entry; STUB_TRAP(); }
void linkageFini(INITFINI *entry) { (void)entry; STUB_TRAP(); }
void scopeFini(INITFINI *entry) { (void)entry; STUB_TRAP(); }
void rewriteFini(INITFINI *entry) { (void)entry; STUB_TRAP(); }
void templateFini(INITFINI *entry) { (void)entry; STUB_TRAP(); }
void fnovFini(INITFINI *entry) { (void)entry; STUB_TRAP(); }
void dataInitFini(INITFINI *entry) { (void)entry; STUB_TRAP(); }
void nodePromoFini(INITFINI *entry) { (void)entry; STUB_TRAP(); }
int makeExitStatus(int fatal) { (void)fatal; STUB_TRAP(); return 0; }


void PpOpen(void) { STUB_TRAP(); }

void PrtChar(int ch) { (void)ch; STUB_TRAP(); }

void PpParse(void) { STUB_TRAP(); }
void CgFrontFini(void) { STUB_TRAP(); }
void cgioBuffReleaseMemory(void) { STUB_TRAP(); }
int macroCompare(MEPTR first, MEPTR second) { (void)first; (void)second; STUB_TRAP(); return 0; }
unsigned CErr2p(unsigned code, const char *text) { (void)code; (void)text; STUB_TRAP(); return 0; }

void cleanupOpenFiles(void) { STUB_TRAP(); }








unsigned lookupMagicKeyword(char *name) { (void)name; STUB_TRAP(); return 0; }




void parserSuicideHandler(void) { STUB_TRAP(); }



unsigned PragRegName(char *name) { (void)name; STUB_TRAP(); return 0; }
void CFatal(const char *message) { (void)message; STUB_TRAP(); }


/* Named pending callees of the reached old default-argument helper body. */
PTREE IcEmitExpr(PTREE expr);

void RtnGenCallBackRunTimeTableThunk(SYMBOL parm) { (void)parm; STUB_TRAP(); }
void RtnGenCallBackArrayDtor(TYPE parm) { (void)parm; STUB_TRAP(); }
void RtnGenCallBackCdoptAccOpEq(TYPE parm) { (void)parm; STUB_TRAP(); }
int genDefaultCtor(TYPE type) { (void)type; STUB_TRAP(); return 0; }
int genDefaultAssign(TYPE type) { (void)type; STUB_TRAP(); return 0; }
void ModuleInitConnect(void) { STUB_TRAP(); }
