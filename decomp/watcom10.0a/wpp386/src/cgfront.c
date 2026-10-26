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
* Description:  WHEN YOU FIGURE OUT WHAT THIS FILE DOES, PLEASE
*               DESCRIBE IT HERE!
*
****************************************************************************/


/* Adapted initial6b9cb443 cgfront.c. Actual1994 CF2735e..273e3
 * uses check_syntax for the module prefix and keeps the old ten-counter
 * vector/packed pending-data cell, before the later emitDataCGFILE state.
 * TextSegName's initial empty object is not freed by this old body. */
#include <string.h>
#include "compiler.h"
#include "cmdline.h"
#include "frontend.h"
#include "utility.h"
#include "cgio.h"
#include "fnbody.h"
#include "symbol.h"
#include "pending.h"
#include <recon.h>
char *stpcpy(char *dest,const char *source);
void SegmentInit(char *name);
void clearCounters(unsigned *vector,unsigned count);
static unsigned cgfront_counters[10];
static CGFILE *codeCGFILE,*dataCGFILE;
static void *pending_data;
static unsigned char cgfront_flags;
void cgfrontInit(INITFINI *defn)
{
    char seg_name[32];
    (void)defn;
    if (*TextSegName==0) {
        if (CompFlags.check_syntax) stpcpy(stpcpy(seg_name,ModuleName),"_TEXT");
        else stpcpy(seg_name,"_TEXT");
        TextSegName=strsave(seg_name);
    }
    SegmentInit(TextSegName);
    clearCounters(cgfront_counters,10);
    codeCGFILE=0;
    dataCGFILE=CgioCreateFile(0);
    pending_data=0;
    cgfront_flags&=~4;
}
/* Descriptive recovered name for actual App utility11c27. Adjacent
 *11c36 updates current/high-water counter pairs; no source name survived. */
void clearCounters(unsigned *vector,unsigned count)
{ memset(vector,0,count*sizeof(*vector)); }

void CgFrontModInitInit(void)
{
    ModuleInitInit();
    codeCGFILE=0;
}

static unsigned char ActiveDestructorMethod;
CGFILE_GEN *getGenData(void);
void emitSourcePosn(CGIO_LOCATION *position);
void cgEmitIns(CGINTER *instruction);
void emitPendingInitializers(void);
void emitSetType(void);
void emitInitialization(CGFILE_GEN *gen) { (void)gen; STUB_TRAP(); }
void cgEmitCodeUint(int opcode,unsigned value)
{
    CGINTER instruction;
    instruction.opcode=opcode;
    instruction.value.uvalue=value;
    cgEmitIns(&instruction);
}
void cgEmit(CGINTER *instruction)
{
    CGFILE_GEN *gen;
    emitPendingInitializers();
    gen=getGenData();
    if (!(cgfront_flags&4)) emitSourcePosn(&gen->source_location);
    emitSetType();
    switch (instruction->opcode) {
    case 0x5c:
        gen->curr_type=0;
        break;
    case 0x3a:
        gen->curr_type=0;
        /* fall through */
    case 0x3d:
        clearCounters(gen->counters,10);
        gen->cs_label=0;
        gen->cs_allocated=0;
        break;
    }
    cgEmitIns(instruction);
}
CGFILE_GEN *getGenData(void);
void emitSourcePosn(CGIO_LOCATION *position)
{
    CGFILE_GEN *gen;
    gen=getGenData();
    if (position && memcmp(position,&gen->emit_line_no,sizeof(*position))
        && position->src_file) {
        cgEmitCodeUint(0x5a,(unsigned)position->src_file);
        cgEmitCodeUint(0x59,position->line);
        gen->emit_line_no=*position;
    }
}
static unsigned icValue;
void setICValue(unsigned value) { icValue=value; }
void addICValue(unsigned value) { icValue+=value; }
void CgFrontCode(int opcode);
void CgFrontCodePtr(int opcode,void *value)
{
    setICValue((unsigned)value);
    CgFrontCode(opcode);
}
void CgFrontCodeInt(int opcode,int value)
{
    addICValue((unsigned)value);
    CgFrontCode(opcode);
}
void CgFrontCode(int opcode)
{
    CGINTER instruction;
    instruction.opcode=opcode;
    instruction.value.uvalue=icValue;
    icValue=0;
    cgEmit(&instruction);
}
int SymIsDtor(SYMBOL sym);
CGFILE_INS CgioBuffLastWrite(void);
CGFILE *getStatFile(void)
{ return codeCGFILE?codeCGFILE:dataCGFILE; }
CGFILE_GEN *getGenData(void)
{ return getStatFile()->gen; }
CGFILE_INS CgFrontFuncOpen(SYMBOL sym,CGIO_LOCATION *position)
{
    CGFILE_INS registration;
    codeCGFILE=CgioCreateFile(sym);
    if (position) codeCGFILE->defined=*position;
    emitSourcePosn(position);
    CgFrontCodePtr(0x3a,sym);
    CgFrontCodePtr(0x3b,CurrScope);
    ActiveDestructorMethod=DestructorMethod;
    if (!SymIsDtor(sym)) CgFrontCodeInt(0x3e,ActiveDestructorMethod);
    CgFrontCode(3);
    registration=CgioBuffLastWrite();
    getGenData()->emit_line_no.src_file=0;
    emitSourcePosn(position);
    return registration;
}

void emitPendingInitializers(void)
{
    CGFILE_GEN *gen;
    if (!(cgfront_flags&4)) {
        gen=getGenData();
        if ((gen->flags&1) || (gen->flags&2)) emitInitialization(gen);
    }
}
void emitSetType(void)
{
    CGFILE_GEN *gen;
    gen=getGenData();
    if (gen->curr_type && gen->curr_type!=gen->emit_type) {
        gen->emit_type=gen->curr_type;
        cgEmitCodeUint(0x19,(unsigned)gen->curr_type);
    }
}
void CgioWriteIC(CGFILE *file,const void *data,unsigned size);
void cgWriteIC(CGFILE *file,CGINTER *instruction)
{ CgioWriteIC(file,instruction,sizeof(*instruction)); }
static CGINTER ins_init_beg={0x5b,{0}};
static CGINTER ins_init_done={0x5c,{0}};
unsigned short CgSegIdData(SYMBOL sym,unsigned char control);
void DgSymbol(SYMBOL sym);
void CgFrontFuncInitFlags(SYMBOL sym)
{
    if(sym) {
        ins_init_beg.value.pvalue=sym;
        ins_init_done.value.pvalue=sym;
        CgSegIdData(sym,1);
        cgWriteIC(codeCGFILE,&ins_init_beg);
        cgfront_flags|=8;
        DgSymbol(sym);
        cgWriteIC(codeCGFILE,&ins_init_done);
        cgfront_flags&=~8;
    }
}
void cgEmitIns(CGINTER *instruction)
{
    CGFILE_GEN *gen;
    gen=getGenData();
    if (gen->flags&8) {
        ins_init_beg.value.pvalue=gen->init_sym;
        cgWriteIC(codeCGFILE,&ins_init_beg);
        gen->flags&=~0x18U;
        gen->flags|=0x10;
    }
    cgWriteIC(codeCGFILE,instruction);
}



/* Recovered names: unlike the later combined CgFrontSwitchFile, these old
 * entries separately save the current function symbol and restore its file. */
CGFILE *CgioLocateFile(SYMBOL sym);
SYMBOL CgFrontSaveFile(void)
{
    return codeCGFILE ? codeCGFILE->symbol : NULL;
}
void CgFrontRestoreFile(SYMBOL sym)
{
    if(sym) codeCGFILE=CgioLocateFile(sym);
}

/* Old expression boundary, recovered descriptive name. The1994 body resets
 * the current generation record's counter8, before any semantic traversal. */
void CgFrontExprBegin(void)
{
    if(codeCGFILE) codeCGFILE->gen->counters[8]=0;
}

/*1994 only records the requested source location here; actual IC emission
 * occurs later in cgEmit/emitSourcePosn, unlike the released2002 wrapper. */
void CgFrontDbgLine(CGIO_LOCATION *position)
{
    getGenData()->source_location=*position;
}

void CgSetType(TYPE type);
void CgFrontSymbol(SYMBOL sym)
{
    getGenData()->emit_type=sym->sym_type;
    CgSetType(sym->sym_type);
    CgFrontCodePtr(21,sym);
}
void CgSetTypeExact(TYPE type)
{
    getGenData()->curr_type=type;
}
unsigned char ObjModelArgument(TYPE type);
TYPE MakeReferenceTo(TYPE type);
void CgSetType(TYPE type)
{
    if(ObjModelArgument(type)==2) type=MakeReferenceTo(type);
    CgSetTypeExact(type);
}


void CgioBuffZap(CGFILE_INS location,const void *data,unsigned length);
void CgFrontZapUint(CGFILE_INS location,int opcode,unsigned value)
{
    CGINTER ins;
    ins.opcode=opcode;
    ins.value.uvalue=value;
    CgioBuffZap(location,&ins,sizeof(ins));
}

void LabelReturn(void);
void CgFrontReturnSymbol(SYMBOL sym)
{
    LabelReturn();
    setICValue((unsigned)sym);
    CgFrontCode(39);
}

TYPE TypedefModifierRemove(TYPE type);
void CgioCloseOutputFile(CGFILE *file);
void updateHighWaterCounters(void)
{
    unsigned i,value;
    if(codeCGFILE) {
        for(i=0;i<10;++i) {
            value=codeCGFILE->gen->emitted_counters[i];
            if(value>cgfront_counters[i]) cgfront_counters[i]=value;
        }
    }
}
void CgFrontFuncClose(SYMBOL func)
{
    TYPE type=TypedefModifierRemove(func->sym_type)->of;
    TYPE stripped=TypedefModifierRemove(type);
    if(stripped->id!=TYP_VOID) {
        if(stripped->id==TYP_CLASS) type=MakeReferenceTo(type);
        CgSetType(type);
    }
    CgFrontCode(61);
    updateHighWaterCounters();
    CgioCloseOutputFile(codeCGFILE);
    codeCGFILE=NULL;
}

void cgEmitData(CGINTER *instruction)
{
    CGFILE *file=(cgfront_flags&8)?codeCGFILE:dataCGFILE;
    if(cgfront_flags&1) {
        cgWriteIC(file,&ins_init_beg);
        cgfront_flags&=~3;
        cgfront_flags|=2;
    }
    cgWriteIC(file,instruction);
}
void CgFrontData(int opcode)
{
    CGINTER instruction;
    instruction.opcode=opcode;
    instruction.value.uvalue=icValue;
    cgEmitData(&instruction);
    icValue=0;
}

/* Descriptive names for the two1994 pending control-sequence flushers.
 * Later cgfront.c emits these counters at different public boundaries. */
void flushCsAllocations(void)
{
    CGFILE_GEN *gen=getGenData();
    if(gen->cs_allocated>0) {
        cgEmitCodeUint(4,gen->cs_allocated);
        gen->cs_allocated=0;
    }
}
void flushCsLabels(void)
{
    CGFILE_GEN *gen=getGenData();
    if(gen->cs_label>0) {
        flushCsAllocations();
        cgEmitCodeUint(6,gen->cs_label);
        gen->cs_label=0;
    }
}
CGFILE *CgFrontSwitchFile(SYMBOL sym)
{
    CGFILE *old=codeCGFILE;
    if(old==NULL || old->symbol!=sym) {
        emitPendingInitializers();
        flushCsLabels();
        codeCGFILE=CgioLocateFile(sym);
    }
    return old;
}

void CgFrontLabdefGoto(unsigned id) { (void)id; STUB_TRAP(); }
