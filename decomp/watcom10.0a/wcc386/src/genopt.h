#ifndef WCC_GENOPT_H
#define WCC_GENOPT_H
#include "block.h"
#include <recon.h>
extern unsigned char HaveLiveInfo;
void InitStackMap(void);
void FlushBlocks(unsigned char routine_done);
void FreeExtraSyms(name *last);
void BlowAwayFreeLists(void);
void GenPartialRoutine(unsigned char routine_done);
unsigned char BlkTooBig(void);
unsigned char FixReturns(void);
void FixEdges(void);
unsigned Renumber(void);
void BlockTrim(void);
void FindReferences(void);
unsigned char TailRecursion(void);
void NullConflicts(unsigned short usage);
unsigned char InsDead(void);
void FixMemRefs(void);
void PreOptimize(void);
void FixMemBases(void);
unsigned char MemCritical(void);
void Panic(unsigned char routine_done);
void MakeConflicts(void);
void SplitVars(void);
void AddCacheRegs(void);
void MakeLiveInfo(void);
void AxeDeadCode(void);
void FixIndex(void);
void FixSegments(void);
void FPRegAlloc(void);
unsigned char RegAlloc(unsigned char part);
void FPParms(void);
void PostOptimize(void);
void AssignTemps(void);
void FiniStackMap(void);
void FreeConflicts(void);
void SortBlocks(void);
void GenProlog(void);
void UnFixEdges(void);
void OptSegs(void);
void GenObject(void);
void GenEpilog(void);
void FreeProc(void);
void PropNullInfo(void);
#endif
