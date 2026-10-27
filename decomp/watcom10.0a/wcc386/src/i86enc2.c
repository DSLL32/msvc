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


#include "block.h"
#include "dbinfo.h"
#include "memory.h"
#include "encode.h"
#include "codegen.h"
#include "stubs.h"
#include <recon.h>

/* Earlier alignment policy: 586 procedures align to four, not later16. */
unsigned DepthAlign(unsigned depth)
{
    static unsigned char AlignArray[10]={0};
    unsigned cpu;
    if (AlignArray[0]==0 || depth==(unsigned)-1)
        far_memcpy((char *)AlignArray,FEAuxInfo(NULL,0x29),sizeof(AlignArray));
    if (OptForSize) return 1;
    cpu=CGProcessorVersion&15;
    if (cpu>=5) return depth==(unsigned)-1?4:1;
    if (cpu>=4) return depth==(unsigned)-1 || depth==(unsigned)-2?16:1;
    if (cpu>=3) return depth==(unsigned)-1 || depth==(unsigned)-2?4:1;
    if (depth==(unsigned)-1) return AlignArray[1];
    if (depth==0) depth=1;
    if (depth>=AlignArray[0]) depth=AlignArray[0]-1;
    return AlignArray[depth+1];
}

/* Older eleven-class table: there were no separate U8/I8/FL classes. */
static unsigned char Signed[]={0,3,0,3,0,3,0,0,1,1,0};
static unsigned char UCondTable[]={5,4,4,5,7,6,2,3};
static unsigned char SCondTable[]={5,4,4,5,15,14,12,13};
unsigned char CondCode(instruction *cond)
{
    unsigned kind;
    unsigned char opcode=cond->head.opcode;
    if (cond->type_class>XX || opcode<0x2e || opcode>0x35) STUB_TRAP();
    kind=(CGProcessorVersion&0x70)>=0x10?2:1;
    if (kind&Signed[cond->type_class]) return SCondTable[opcode-0x2e];
    return UCondTable[opcode-0x2e];
}

void Format(unsigned char kind);
void LayOpword(unsigned short value);
void LayRMRegOp(name *operand);
void LayModRM(name *operand);
void AddToTemp(unsigned char value);
void Finalize(void);
void ReFormat(unsigned char kind);
void Pushf(void) { STUB_TRAP(); }
void *FindAuxInfo(name *op,unsigned char request);
int OptInsSize(unsigned char kind,int destination);
unsigned char AskIfRTLabel(struct code_lbl *label);
void DoCall(struct code_lbl *label,unsigned char imported,unsigned char big,unsigned char pop)
{
    if (!big)
        CodeHandle(8|pop,(unsigned char)OptInsSize(8,1),label);
    else if (AskIfRTLabel(label) || imported ||
             AskCodeSeg()!=FESegID(AskForLblSym(label)))
        CodeHandle(0x18|pop,(unsigned char)OptInsSize(8,3),label);
    else CodeHandle(0x18|pop,(unsigned char)OptInsSize(8,2),label);
}
void CodeSequence(unsigned char *data,unsigned char length) { STUB_TRAP(); }
void CodeBytes(unsigned char *data,unsigned char length) { STUB_TRAP(); }
struct call_byte_seq { unsigned char length;unsigned char data[1]; };
/* i86enc2.c, before the later NEW_P5_PROFILING exceptions. */
void GenCall(instruction *ins)
{
    name *op;
    void *sym;
    unsigned char big,pop_bit;
    unsigned call_class;
    struct call_byte_seq *code;
    struct code_lbl *label;
    if (ins->flags.call_flags&0x10) Pushf();
    op=ins->operands[2];
    call_class=*(unsigned *)FindAuxInfo(op,2);
    code=FindAuxInfo(op,1);
    if (code!=NULL) {
        Finalize();
        if (code->length&0x80) CodeSequence(code->data,code->length&0x7f);
        else CodeBytes(code->data,code->length);
    } else if (call_class&2) { /* SUICIDAL */
        sym=op->v.symbol;
        label=((struct bck_info *)FEBack((SYM_HANDLE)(unsigned)sym))->label;
        if ((call_class&0x1000) && (FEAttr((SYM_HANDLE)(unsigned)sym)&8))
            CodeHandle(0x1c,(unsigned char)OptInsSize(12,3),label);
        else CodeHandle(12,(unsigned char)OptInsSize(12,1),label);
    } else {
        pop_bit=(ins->flags.call_flags&8)?0x40:0;
        big=(call_class&0x1000)!=0;
        sym=op->v.symbol;
        if (op->m.memory_type==CG_FE)
            DoCall(((struct bck_info *)FEBack((SYM_HANDLE)(unsigned)sym))->label,
                   (FEAttr((SYM_HANDLE)(unsigned)sym)&0x208)!=0,big,pop_bit);
        else DoCall(sym,1,big,pop_bit);
    }
}
void GenICall(instruction *ins)
{
    unsigned char entry=0;
    unsigned short opcode;
    if (ins->flags.call_flags&0x10) Pushf();
    if (ins->flags.call_flags&8) entry|=0x40;
    /* The 1994 body predates the NEW_P5_PROFILING exception. */
    entry|=(ins->flags.call_flags&0x80)?0x0d:9;
    if (ins->operands[2]->n.name_class==PT ||
        ins->operands[2]->n.name_class==CP) {
        entry|=0x10;
        opcode=0x08ff;
    } else {
        opcode=0x00ff;
    }
    ReFormat(entry);
    LayOpword(opcode);
    LayModRM(ins->operands[2]);
    Finalize();
}

void GenSetCC(instruction *cond)
{
    Format(2);
    LayOpword(0x90|CondCode(cond));
    if (cond->result->n.class==3) LayRMRegOp(cond->result);
    else LayModRM(cond->result);
    AddToTemp(0x0f);
    Finalize();
}

void GenReturn(long pop,unsigned char is_long,unsigned char is_interrupt)
{
    any_oc record;
    record.op.kind=14;
    record.op.reclen=sizeof(oc_ret);
    record.op.objlen=1;
    record.ret.ref=NULL; /* Insertion owns the original unspecified queue link. */
    record.ret.pops=(unsigned)pop;
    if (pop!=0) {
        record.op.kind|=0x40;
        record.op.objlen+=2;
    }
    if (is_long) record.op.kind|=0x10;
    if (is_interrupt) record.op.kind|=0x20;
    InputOC(&record);
}
