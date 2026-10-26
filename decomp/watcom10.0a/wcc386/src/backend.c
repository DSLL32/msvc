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
#include "procdef.h"
#include "memory.h"
#include "cgmemory.h"
#include "memory.h"
#include "objemit.h"
#include <recon.h>

unsigned Model,TargetModel;
unsigned char OptForSize;
unsigned short CGProcessorVersion;

/* intrface.c before BEInitCg became a separate public entry point. */
cg_init_info BEInit(unsigned switches,unsigned platform,unsigned optsize,unsigned short proc)
{
    cg_init_info info;
    BEMemInit();
    OptForSize=(unsigned char)optsize;
    if ((proc&15)<3) proc=(proc&~15)|3;
    proc&=~0x80;
    Model=switches;
    TargetModel=platform;
    CGProcessorVersion=proc;
    InitWeights(optsize);
    CGMemInit();
    InitSegDefs();
    InitDbgInfo();
    TypeInit();
    TInit();
    info.success=0;
    if (CGOpenf()) {
        info.version.is_large=1;
        info.version.target=1;
        info.version.revision=9;
    }
    return info;
}

/* The OSI build's BEMemInit contribution is exactly one RET. */
void BEMemInit(void)
{
}

unsigned char MemOut;
unsigned Chunks;

void CGMemInit(void)
{
    MemCoalesce();
    MemOut=MO_FATAL;
    Chunks=0;
    CalcMemLimit();
}

/* memmgt.c's tracked allocator calls the compiler-private allocator. The
 * initial MemLow check predates the later per-allocation tracking build. */
int InOptimizer;
void *CGAlloc(int size)
{
    void *chunk;
    MemLow();
    for (;;) {
        chunk=MemAlloc(size);
        if (chunk!=NULL) {
            ++Chunks;
            return chunk;
        }
        if (!MemCheck(size)) break;
    }
    if ((MemOut==MO_FATAL) | (InOptimizer!=0)) FatalError("Out of memory");
    else if (MemOut==MO_SUICIDE) Suicide();
    return NULL;
}

void BEDefSeg(unsigned short segment,unsigned char flags,char *name,unsigned alignment)
{
    DefSegment(segment,flags,name,alignment,0);
}

void BEStart(void)
{
    InitCG();
}

void CGFreeSize(void *block,int size)
{
    (void)size;
    --Chunks;
    MemFree(block);
}

/* 386 target hook is genuinely empty in the released i86segs.c as well. */
void InitSegment(void)
{
}

void BEDefType(cg_type type,unsigned alignment,unsigned long size)
{
    (void)alignment;
    TypeDefn(type,size);
}

unsigned short BESetSeg(unsigned short segment)
{
    return SetOP(segment);
}

/* intrface.c before the later pooled bck_info allocator. */
void *NewBackReturn;

void *BENewBack(void *symbol)
{
    struct bck_info *back;
    if (NewBackReturn==(void *)1) return NULL;
    if (NewBackReturn!=NULL) return NewBackReturn;
    back=CGAlloc(sizeof(*back));
    back->label=AskForLabel(symbol);
    back->imp=0;
    back->segment=symbol==NULL ? AskBackSeg():FESegID(symbol);
    if ((unsigned)back&1) Zoiks(67);
    return back;
}

struct code_lbl *AskForLabel(void *symbol)
{
    struct code_lbl *label;
    ++InOptimizer;
    label=DoAskForLabel(symbol);
    if (symbol!=NULL && (FEAttr(symbol)&0x100)) label->label.status|=0x100;
    --InOptimizer;
    return label;
}


struct code_lbl *DoAskForLabel(void *symbol)
{
    struct code_lbl *label;
    ++InOptimizer;
    label=CGAlloc(sizeof(*label));
    label->label.address=(unsigned long)-1;
    label->label.patch=NULL;
    label->label.status=0;
    label->alias=NULL;
    label->ins=NULL;
    label->refs=NULL;
    label->redirect=NULL;
    --InOptimizer;
    label->label.symbol=symbol;
    label->label.link=Handles;
    Handles=label;
    return label;
}

void DGLabel(void *handle)
{
    struct bck_info *back;
    back=handle;
    if ((unsigned)back&1) Zoiks(68);
    DGBlip();
    DataLabel(back->label);
    back->segment=AskOP();
}

void *AskForLblSym(struct code_lbl *label)
{
    void *symbol;
    ++InOptimizer;
    ValidLbl(label);
    symbol=label->label.symbol;
    --InOptimizer;
    return symbol;
}

/* optvlbl.c deliberately disables its validation body with #if 0. */
unsigned char ValidLbl(struct code_lbl *label)
{
    (void)label;
    return 1;
}

/* Object emission temporarily protects the optimizer's static state. */
void TellOptimizerByPassed(void)
{
    ++InOptimizer;
}

void TellAddress(struct code_lbl *label,unsigned long address)
{
    ++InOptimizer;
    ValidLbl(label);
    label->label.address=address;
    --InOptimizer;
}

array_control *AskLblPatch(struct code_lbl *label)
{
    array_control *patches;
    ++InOptimizer;
    ValidLbl(label);
    if (label->label.patch==NULL) label->label.patch=InitPatch();
    patches=label->label.patch;
    --InOptimizer;
    return patches;
}

void TellDonePatch(struct code_lbl *label)
{
    ++InOptimizer;
    ValidLbl(label);
    label->label.patch=NULL;
    --InOptimizer;
}

void TellByPassOver(void)
{
    --InOptimizer;
}

void DGUBytes(unsigned long size)
{
    DGBlip();
    IncLocation(size);
}

/* intrface.c: form the target's little-endian integer bytes explicitly. */
void DGInteger(unsigned long value,cg_type type)
{
    int length,i;
    unsigned char buffer[6];
    length=(int)TypeLength(type);
    for (i=0;i<length;++i) {
        buffer[i]=(unsigned char)value;
        value>>=8;
    }
    DGBytes(length,buffer);
}

void DGBytes(unsigned long length,unsigned char *data)
{
    DGBlip();
    DataBytes(length,data);
}

/* OSI-era zero-length PCSTART/PCEND markers were removed in later i86data.c. */
unsigned char GeneratingProfileCode;
void DataBytes(unsigned long length,unsigned char *data)
{
    if (length!=0) {
        TellOptimizerByPassed();
        SetUpObj(1);
        while (length!=0) {
            OutDataByte(*data++);
            --length;
        }
        TellByPassOver();
    } else if (Equal(data,(unsigned char *)"PCSTART",7)) {
        GeneratingProfileCode=1;
        EmptyQueue();
    } else if (Equal(data,(unsigned char *)"PCEND",5)) {
        GeneratingProfileCode=0;
    }
}

void DGBackPtr(struct bck_info *back,unsigned short segment,long offset,cg_type type)
{
    if ((unsigned)back&1) Zoiks(68);
    DGBlip();
    BackPtr(back,segment,offset,TypeAddress(type));
}

void BackPtr(struct bck_info *back,unsigned short segment,long offset,type_def *type)
{
    TellOptimizerByPassed();
    DoLblPtr(back->label,segment,type->length!=4 ? 3:1,offset);
    TellByPassOver();
}

void DoLblPtr(struct code_lbl *label,unsigned short segment,unsigned char fix_class,long offset)
{
    SetUpObj(1);
    TellKeepLabel(label);
    OutReloc(segment,fix_class,0);
    OutLblPatch(label,fix_class,offset);
}

void TellKeepLabel(struct code_lbl *label)
{
    ++InOptimizer;
    ValidLbl(label);
    --InOptimizer;
    label->label.status|=2;
}

void OutLblPatch(struct code_lbl *label,unsigned char fix_class,long offset)
{
    unsigned long value;
    value=AskAddress(label);
    if (value==0xffffffffUL) {
        OutPatch(label,5);
        value=0;
    }
    if (fix_class==2) {
        OutDataInt(0);
    } else {
        OutDataLong(value+offset);
        if (fix_class==3) OutDataInt(0);
    }
}

unsigned long AskAddress(struct code_lbl *label)
{
    ++InOptimizer;
    ValidLbl(label);
    --InOptimizer;
    return label->label.address;
}

void DGFloat(char *text,cg_type type)
{
    struct cfloat *value;
    union cg_float buffer;
    value=CFCnvSF(text,text+Length(text));
    CFCnvTarget(value,&buffer,TypeClass(TypeAddress(type)));
    CFFree(value);
    DGBytes(TypeLength(type),(unsigned char *)&buffer);
}

void BEFiniBack(void *handle)
{
    struct bck_info *back=handle;
    if (!((unsigned)back&1)) TellNoSymbol(back->label);
}

/* opttell.c before kill records were limited to CODELABEL symbols. */
void TellNoSymbol(struct code_lbl *label)
{
    ++InOptimizer;
    if (label!=NULL) {
        ValidLbl(label);
        label->label.symbol=NULL;
        GenKillLabel(label);
    }
    --InOptimizer;
}

/* encode.c's OC_LDONE record is emitted in the code segment. */
void GenKillLabel(struct code_lbl *label)
{
    unsigned short old;
    old=SetOP(AskCodeSeg());
    CodeHandle(0x11,0,label);
    SetOP(old);
}

/* This target frees each back-info record directly, predating BckInfoHead. */
void BEFreeBack(void *handle)
{
    if (!((unsigned)handle&1)) CGFreeSize(handle,sizeof(struct bck_info));
}

void CGProcDecl(void *symbol,cg_type type)
{
    if (BGInInline()) BGProcInline(symbol,TypeAddress(type));
    else {
        TellObjNewProc(symbol);
        BGProcDecl(symbol,TypeAddress(type));
    }
}

struct code_lbl *AskForSymLabel(void *symbol,unsigned char kind)
{
    switch (kind) {
    case CG_FE:
        return ((struct bck_info *)FEBack((SYM_HANDLE)(unsigned)symbol))->label;
    case CG_LBL:case CG_CLB:
        return symbol;
    case CG_BACK:
        return ((struct bck_info *)symbol)->label;
    case CG_TBL:
        return ((tbl_control *)symbol)->label;
    case CG_VTB:
        return ((tbl_control *)symbol)->value_label;
    default:
        Zoiks(78);
        return NULL;
    }
}

void CGParmDecl(void *symbol,cg_type type)
{
    if (BGInInline()) BGParmInline(symbol,TypeAddress(type));
    else BGParmDecl(symbol,TypeAddress(type));
}

/* intrface.c temporarily routes FEBack/BENewBack to the temporary name. */
name *LkAddBack(void *symbol,name *back)
{
    void *result;
    NewBackReturn=back==NULL ? (void *)1:(void *)((unsigned)back|1);
    result=FEBack((SYM_HANDLE)(unsigned)symbol);
    NewBackReturn=NULL;
    return (name *)((unsigned)result&~1U);
}

void *CGTemp(cg_type type)
{
    return BGGlobalTemp(TypeAddress(type));
}

void *BENewLabel(void)
{
    return AskForNewLabel();
}

/* optask.c: unused code labels may die without an LDONE queue record. */
struct code_lbl *AskForNewLabel(void)
{
    struct code_lbl *label;
    ++InOptimizer;
    label=DoAskForLabel(NULL);
    --InOptimizer;
    label->label.status=5; /* CODELABEL | DYINGLABEL */
    return label;
}

/* intrface.c before the later NO_OPTIMIZATION bypass; direct label allocation
 * is present in this build rather than a second BENewLabel interface call. */
void *CGLastParm(void)
{
    struct code_lbl *top;
    top=NULL;
    if (!BGInInline()) {
        top=AskForNewLabel();
        BGGenCtrl(86,NULL,top,1); /* O_LABEL */
    }
    return top;
}

void CGControl(unsigned char op,void *condition,void *label)
{
    TGControl(op,condition,label);
}

/* intrface.c predates the later dllimport-specific CGFEName branch. */
void *CGFEName(void *symbol,cg_type type)
{
    return TGLeaf(BGName(CG_FE,symbol,TypeAddress(type)));
}

void *CGUnary(unsigned char op,void *value,cg_type type)
{
    return TGUnary(op,value,TypeAddress(type));
}

void *CGInteger(long value,cg_type type)
{
    return TGLeaf(BGInteger(value,TypeAddress(type)));
}

void *CGBinary(unsigned char op,void *left,void *right,cg_type type)
{
    if (op==92) left=TGTrash(left); /* O_COMMA */
    return TGBinary(op,left,right,TypeAddress(type));
}

void *CGCompare(unsigned char op,void *left,void *right,cg_type type)
{
    return TGCompare(op,left,right,TypeAddress(type));
}
