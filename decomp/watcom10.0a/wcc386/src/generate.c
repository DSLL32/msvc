/****************************************************************************
*
*                            Open Watcom Project
*
*    Portions Copyright (c) 1983-2002 Sybase, Inc. All Rights Reserved.
*
*  ========================================================================
*
*    This file contains Original Code and/or Modifications of Original
*    Code as defined in and that are subject to the Sybase Open Watcom
*    Public License version 1.0 (the 'License'). You may not use this file
*    except in compliance with the License. BY USING THIS FILE YOU AGREE TO
*    ALL TERMS AND CONDITIONS OF THE LICENSE. A copy of the License is
*    provided with the Original Code and Modifications, and is also
*    available at www.sybase.com/developer/opensource.
*
*    The Original Code and all software distributed under the License are
*    distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
*    EXPRESS OR IMPLIED, AND SYBASE AND ALL CONTRIBUTORS HEREBY DISCLAIM
*    ALL SUCH WARRANTIES, INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF
*    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR
*    NON-INFRINGEMENT. Please see the License for the specific language
*    governing rights and limitations under the License.
*
*  ========================================================================
*
* Description:  Basic type initialization and type-node allocation.
*
****************************************************************************/


/* cgen2.c's emission-state initializer. In the earlier target the next
 * imported segment starts at -1 directly, before the later SegData field. */
#include "codegen.h"
#include "cgmemory.h"
#include "genopt.h"
#include "dbinfo.h"
#include "stubs.h"
#include <recon.h>

struct proc_def;
struct block;
struct proc_def *CurrProc;
struct block *CurrBlock,*BlockList,*HeadBlock;
unsigned InsId;
unsigned char BlockByBlock,ClassPointer;

void InitCG(void)
{
    InOptimizer=0;
    InsId=0;
    CurrProc=NULL;
    CurrBlock=NULL;
    BlockList=NULL;
    HeadBlock=NULL;
    BlockByBlock=0;
    InitFP();
    InitRegTbl();
    ScoreInit();
    InitQueue();
    InitMakeAddr();
    RegTreeInit();
    InitIns();
    InitConflict();
    InitRT();
    InitNames();
    /* The earlier cfloat allocator is initialized here, without the later
     * callback table consumed during BEInit. */
    CFInit();
    ObjInit();
    ClassPointer=TypeClass(TypePtr);
    InitSegment();
}

unsigned char HaveLiveInfo;
/* Old generate.c: no later dominator pass or final
 * FlushQueue call. The one physical empty target-memory hook is called
 * twice; do not invent distinct callable identities from later names. */
void Generate(unsigned char routine_done)
{
    if (BGInInline()) return;
    HaveLiveInfo=0;
    if (Model&0x80000000U) { /* NO_OPTIMIZATION */
        if (!BlockByBlock) {
            InitStackMap();
            BlockByBlock=1;
        }
        DGBlip(); /* Same physical no-argument progress hook as LNBlip. */
        FlushBlocks(0);
        FreeExtraSyms(LastTemp);
        if (MemLow()) BlowAwayFreeLists();
        return;
    }
    if (BlockByBlock) {
        if (MemLow() || routine_done) GenPartialRoutine(routine_done);
        else BlkTooBig();
        return;
    }
    if (MemLow()) {
        InitStackMap();
        GenPartialRoutine(routine_done);
        BlowAwayFreeLists();
        return;
    }
    if (!routine_done) {
        BlkTooBig();
        return;
    }
    FixReturns();
    FixEdges();
    Renumber();
    BlockTrim();
    FindReferences();
    TailRecursion();
    NullConflicts(2); /* USE_IN_ANOTHER_BLOCK */
    InsDead();
    FixMemRefs();
    FindReferences();
    PreOptimize();
    FixMemBases();
    if (MemCritical()) {
        Panic(0);
        return;
    }
    MakeConflicts();
    if (Model&0x400000) SplitVars(); /* LOOP_OPTIMIZATION */
    AddCacheRegs();
    PropNullInfo(); /* Older placement, after conflict construction. */
    MakeLiveInfo();
    HaveLiveInfo=1;
    AxeDeadCode();
    FixIndex();
    FixSegments();
    FPRegAlloc();
    if (!RegAlloc(0)) {
        Panic(1);
        return;
    }
    FPParms();
    FixMemBases();
    PostOptimize();
    InitStackMap();
    AssignTemps();
    FiniStackMap();
    FreeConflicts();
    /* Actual 1994 order; later released Generate moved UnFixEdges later. */
    UnFixEdges();
    SortBlocks();
    GenProlog();
    OptSegs();
    GenObject();
    if (!(CurrProc->prolog_state&2)) GenEpilog();
    FreeProc();
}

void PreOptimize(void)
{
    unsigned char change;
    if (!(Model&0x80000000U)) {
        MakeMovAddrConsts();
        PushPostOps();
        DeadTemps();
        InsDead();
        MakeFlowGraph();
        BlockTrim();
        CommonSex((Model&0x400000U)!=0);
        SetOnCondition();
        BlockTrim();
        AddANop();
        if (Model&0x400000U) {
            change=0;
            if (TransLoops(0)) change=1;
            if (LoopInvariant()) change=1;
            if (change) {
                CommonSex(1);
                InsDead();
                CommonInvariant();
            }
            if (IndVars()) {
                CommonSex(0);
                InsDead();
                change=1;
            }
            BlockTrim();
            if (TransLoops(1)) {
                BlockTrim();
                CommonSex(0);
                change=1;
            }
            /* This older pass has no later ReConstFold call. */
            LoopEnregister();
            if (change) BlockTrim();
        }
        MulToShiftAdd();
        KillMovAddrConsts();
    } else {
        MakeFlowGraph();
        AddANop();
    }
    FindReferences();
}

/* generate.c and cse.c each had a file-local NextBlock. Distinguish the
 * linear callback's linker name while retaining its separate real vertex. */
block *LinearNextBlock(block *blk,block *unused);
void DeadInstructions(void);
void BuildIndex(void);
void MergeIndex(void);
unsigned char LdStAlloc(void);
void Score(void);
unsigned char LoopRegInvariant(void);
void Conditions(void);
void FPExpand(void);
void Schedule(void);
void LiveInfoUpdate(void);
void LdStCompress(void);
void FPOptimize(void);
void PostOptimize(void)
{
    if (!(Model&0x80000000U)) {
        /* Unlike the later released build, the 1994 compiler runs both
         * post-register-allocation peephole passes. */
        PeepOpt(HeadBlock,LinearNextBlock,NULL,1);
        DeadInstructions();
        BuildIndex();
        DeadInstructions();
    }
    MergeIndex();
    if (!(Model&0x80000000U)) {
        LdStAlloc();
        Score();
        if (!BlockByBlock) LoopRegInvariant();
        Conditions();
    }
    FPExpand();
    if (!(Model&0x80000000U)) {
        DeadInstructions();
        if (Model&0x200000U) {
            HaveLiveInfo=0;
            Schedule();
            LiveInfoUpdate();
            HaveLiveInfo=1;
        }
        LdStCompress();
        PeepOpt(HeadBlock,LinearNextBlock,NULL,1);
    }
    FPOptimize();
}
block *LinearNextBlock(block *blk,block *unused)
{
    return blk->next_block;
}








void AddANop(void)
{
    AddAnIns(BlockList,MakeNop());
}

void FiniQueue(void);
void InitSegment(void);
void ObjFini(void);
void TellFreeAllLabels(void);
void ScoreFini(void);
/* The1994 body predates abortCG; FiniSegment folds to the same empty
 * physical entry as InitSegment, represented by that existing void hook. */
void FiniCG(void)
{
    FiniQueue();
    InitSegment();
    ObjFini();
    TellFreeAllLabels();
    BlowAwayFreeLists();
    ScoreFini();
}
