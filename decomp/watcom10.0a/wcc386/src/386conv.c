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
* Description:  Select i386 scalar conversion implementations.
*
****************************************************************************/


#include "block.h"
#include "genopt.h"
#include "pattern.h"
#include "stubs.h"
#include "regchoices.h"
#include <recon.h>

enum { CONV_OK=10, LAST_CONV_TABLE=23, BEG_RTNS=24, CONV_FPOK=106,
       R_MAKECALL=0x7a };
/* Source CRtn at7a82a: conversion through a software runtime routine. */
static opcode_entry CRtn[]={
    {(ANY*OP1_MUL)|(ANY*RESULT_MUL)|NONE,0,R_MAKECALL,0,0}
};
/* Encountered zero-extension U1 to U4 selector, source Z1to4.
 * The complete five-row object at7a7ca uses old vergen/reduction IDs. */
enum { V_GOOD_CLR=46,R_CLRHIGH_R=186,G_MOVZX=89,G_CVT_UNKNOWN=103,
       R_CVT_MOVRESREG=140 };
static opcode_entry Z1to4[]={
    {_Un(R|M,R,NONE),V_GOOD_CLR,R_CLRHIGH_R,RG_BYTE_4BYTE,0},
    {_Un(R|M,R,NONE),0,G_MOVZX,RG_BYTE_DBL,1},
    {_Un(U|C,R,NONE),0,G_CVT_UNKNOWN,RG_BYTE_DBL,0},
    {_Un(ANY,M,NONE),0,R_CVT_MOVRESREG,RG_BYTE_DBL,0},
    {_Un(ANY,ANY,NONE),0,G_CVT_UNKNOWN,RG_BYTE_DBL_NEED_DBL,0}
};
/* Newly encountered U2-to-U4 conversion: seven source Z2to4 rows,
 * exactly42B at7a7e8..7a812 in the actual old conversion table. */
enum { V_OP1RELOC=38,R_CVT_MOVOP1TEMP=134 };
static opcode_entry Z2to4[]={
    {_Un(ANY,R,NONE),0,R_CLRHIGH_R,RG_CWD,0},
    {_Un(R|M,R,NONE),V_GOOD_CLR,R_CLRHIGH_R,RG_WORD_DBL,0},
    {_Un(C,ANY,NONE),V_OP1RELOC,R_CVT_MOVOP1TEMP,RG_,0},
    {_Un(R|M,R,NONE),0,G_MOVZX,RG_WORD_DBL,1},
    {_Un(U|C,R,NONE),0,G_CVT_UNKNOWN,RG_WORD_DBL,0},
    {_Un(ANY,M,NONE),0,R_CVT_MOVRESREG,RG_WORD_DBL,0},
    {_Un(ANY,ANY,NONE),0,G_CVT_UNKNOWN,RG_WORD_DBL_NEED_DBL,0}
};
/* Earlier Z4to8 is a single register-pair reduction row at7a812..7a818,
 * before the later generic ANY-result/empty-register-set spelling. */
static opcode_entry Z4to8[]={
    {_Un(ANY,R,NONE),0,178,RG_8,0} /* R_CLRHIGH_D */
};
extern int RoutineNum;
unsigned char AskHow(unsigned char from,unsigned char to);
void MoveSegOp(instruction *ins,instruction *new_ins,int duplicate);
instruction *rDOCVT(instruction *ins)
{
    name *src,*dst,*temp;
    instruction *new_ins;
    unsigned char how;
    src=ins->operands[0];
    dst=ins->result;
    if (src->n.name_class!=XX && ins->base_type_class==XX)
        ins->base_type_class=src->n.name_class;
    ins->head.state=0;
    /* Actual old constant shortcut excludes XX; the later U8/I8 types
     * do not exist in this target. DX:AX preserves the U2 division case. */
    if (src->n.class==0 && src->c.const_type==0 && ins->type_class!=XX &&
        (dst->n.class!=3 || dst->r.reg!=0xc3)) how=CONV_OK;
    else how=AskHow(ins->base_type_class,ins->type_class);
    if (how<CONV_OK) {
        temp=AllocTemp(how);
        new_ins=MakeUnary(ins->head.opcode,src,temp,how);
        new_ins->base_type_class=ins->base_type_class;
        new_ins->type_class=how;
        ins->base_type_class=how;
        ins->table=NULL;
        ins->operands[0]=temp;
        MoveSegOp(ins,new_ins,0);
        PrefixIns(ins,new_ins);
    } else if (how==CONV_OK) {
        new_ins=MakeMove(src,dst,ins->type_class);
        DupSeg(ins,new_ins);
        ReplIns(ins,new_ins);
    } else if (how==CONV_FPOK) {
        ins->head.opcode=0x26;
        ins->table=NULL;
        ins->u.gen_table=NULL;
        new_ins=ins;
        if (ins->type_class!=FS && ins->type_class!=FD)
            ins->type_class=ins->base_type_class;
    } else if (how<=LAST_CONV_TABLE) {
        /* Other dedicated conversion selectors remain unavailable in
         * both builds until their semantic table is encountered. */
        if (how==19) ins->table=Z1to4; /* Z1TO4 */
        else if (how==20) ins->table=Z2to4; /* Z2TO4 */
        else if (how==21) ins->table=Z4to8; /* Z4TO8 */
        else STUB_TRAP();
        new_ins=ins;
    } else {
        ins->table=CRtn;
        RoutineNum=how-BEG_RTNS;
        new_ins=ins;
    }
    return new_ins;
}
enum {
    C2TO1=11,
    C4TO1=12,
    CU4=4,
    CONV_BAD=109,
    CI4=5,
    Z1TO2=18,
    S1TO2=14,
    C4TO2=13,
    Z1TO4=19,
    S1TO4=15,
    Z2TO4=20,
    S2TO4=16,
    CHP_PT=23,
    C_S_U=31,
    C_D_U=35,
    C_S_4=29,
    C_D_4=33,
    EXT_PT=22,
    C_U4_S=24,
    C_I4_S=25,
    C_D_S=37,
    C_U4_D=26,
    C_I4_D=27,
    C_S_D=28,
    S4TO8=17,Z4TO8=21
};
/* Rows are destination type, columns source: U1,I1,U2,I2,U4,I4,CP,PT,FS,FD.
 * Exact old100B CvtTable source projection; no I8/U8/FL universe. */
static unsigned char CvtTable[10][10]={
    {CONV_OK,CONV_OK,C2TO1,C2TO1,C4TO1,C4TO1,CU4,CONV_BAD,CU4,CU4},
    {CONV_OK,CONV_OK,C2TO1,C2TO1,C4TO1,C4TO1,CU4,CONV_BAD,CI4,CI4},
    {Z1TO2,S1TO2,CONV_OK,CONV_OK,C4TO2,C4TO2,CU4,CONV_BAD,CU4,CU4},
    {Z1TO2,S1TO2,CONV_OK,CONV_OK,C4TO2,C4TO2,CU4,CONV_BAD,CI4,CI4},
    {Z1TO4,S1TO4,Z2TO4,S2TO4,CONV_OK,CONV_OK,CHP_PT,CONV_BAD,C_S_U,C_D_U},
    {Z1TO4,S1TO4,Z2TO4,S2TO4,CONV_OK,CONV_OK,CU4,CONV_BAD,C_S_4,C_D_4},
    {CU4,CI4,CU4,CI4,EXT_PT,CU4,CONV_OK,CONV_BAD,CONV_BAD,CONV_BAD},
    {CONV_BAD,CONV_BAD,CONV_BAD,CONV_BAD,CONV_BAD,CONV_BAD,CONV_BAD,CONV_BAD,CONV_BAD,CONV_BAD},
    {CU4,CI4,CU4,CI4,C_U4_S,C_I4_S,CONV_BAD,CONV_BAD,CONV_OK,C_D_S},
    {CU4,CI4,CU4,CI4,C_U4_D,C_I4_D,CONV_BAD,CONV_BAD,C_S_D,CONV_OK},
};
/* Rows are destination type, columns source: U1,I1,U2,I2,U4,I4,CP,PT,FS,FD.
 * Exact old100B FPCvtTable source projection; no I8/U8/FL universe. */
static unsigned char FPCvtTable[10][10]={
    {CONV_OK,CONV_OK,C2TO1,C2TO1,C4TO1,C4TO1,CU4,CONV_BAD,CU4,CU4},
    {CONV_OK,CONV_OK,C2TO1,C2TO1,C4TO1,C4TO1,CU4,CONV_BAD,CI4,CI4},
    {Z1TO2,S1TO2,CONV_OK,CONV_OK,C4TO2,C4TO2,CU4,CONV_BAD,CU4,CU4},
    {Z1TO2,S1TO2,CONV_OK,CONV_OK,C4TO2,C4TO2,CU4,CONV_BAD,CI4,CI4},
    {Z1TO4,S1TO4,Z2TO4,S2TO4,CONV_OK,CONV_OK,CHP_PT,CONV_BAD,CONV_FPOK,CONV_FPOK},
    {Z1TO4,S1TO4,Z2TO4,S2TO4,CONV_OK,CONV_OK,CU4,CONV_BAD,CONV_FPOK,CONV_FPOK},
    {CU4,CI4,CU4,CI4,EXT_PT,CU4,CONV_OK,CONV_BAD,CONV_BAD,CONV_BAD},
    {CONV_BAD,CONV_BAD,CONV_BAD,CONV_BAD,CONV_BAD,CONV_BAD,CONV_BAD,CONV_BAD,CONV_BAD,CONV_BAD},
    {CU4,CI4,CU4,CONV_FPOK,CONV_FPOK,CONV_FPOK,CONV_BAD,CONV_BAD,CONV_FPOK,CONV_FPOK},
    {CU4,CI4,CU4,CONV_FPOK,CONV_FPOK,CONV_FPOK,CONV_BAD,CONV_BAD,CONV_FPOK,CONV_FPOK},
};
unsigned char AskHow(unsigned char from,unsigned char to)
{
    /* Older target special-cases the XX double-word division operand,
     * instead of rejecting XX as the later released implementation does. */
    if (to==XX) {
        if (from==I4) return S4TO8;
        if (from==U4) return Z4TO8;
        return CONV_BAD;
    }
    /* An unknown source type has no matrix row: keep that undefined domain
     * trapped in both builds rather than reading into the next object. */
    if (from>=XX || to>=XX) STUB_TRAP();
    if ((CGProcessorVersion&0x70)>=0x10) return FPCvtTable[to][from];
    return CvtTable[to][from];
}

