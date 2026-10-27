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
* Description:  Encountered i386 instruction selection tables.
*
****************************************************************************/


#include "block.h"
#include "pattern.h"
#include "tables.h"
#include "regchoices.h"
#include "stubs.h"
#include <recon.h>

/* 386table.c DoNop: any operands/result, no equality constraint,
 * preserve condition codes; V_NO, G_NO, RG_, FU_NO. The old pattern
 * has four-bit operand fields and a 16-bit op_type. */
#define PAT_ANY 0xfU
#define PAT_NEITHER_EQUAL 0x3000U
#define PAT_PRESERVE_CC 0x8000U
opcode_entry DoNop[]={
    { PAT_ANY|(PAT_ANY<<4)|(PAT_ANY<<8)|PAT_NEITHER_EQUAL|PAT_PRESERVE_CC,
      0,0,0,0 }
};
/* Parameter definition: unary arbitrary operand/result, no generated
 * instruction. Both original tables point to7ba98, bytesff3000000000. */
static opcode_entry Parm[]={
    {(ANY*OP1_MUL)|(ANY*RESULT_MUL)|NONE,0,0,0,0}
};
/* The encountered conversion selector7bab6 is the released R_DOCVT role;
 * its actual old reduction opcode is110 (newer reduction enums differ). */
enum { R_DOCVT=0x6e };
static opcode_entry Cvt[]={
    {(ANY*OP1_MUL)|(ANY*RESULT_MUL)|NONE,0,R_DOCVT,0,0}
};
/* Released Move4CC followed immediately by Move4: one selector sequence
 * whose first row can reduce zero assignment to XOR. The old equality row
 * has V_NO (not the later NVI(V_NO)); retain the actual1994 behavior. */
enum { V_OP1ZERO=9,V_SAME_LOCN=19,V_RESLOC=18,V_OP1LOC=7,
       G_MOVRC=33,G_MOVMC=27,G_MOVMA=32,G_MOVAM=31,
       G_RR1=18,G_RM1=19,G_MR1=20,G_UNKNOWN=103,
       R_MAKEXORRR=131,R_MOVOP1REG=138,FU_ALUX=2 };
static opcode_entry Move4CC[]={
    {_Un(C,R,NONE),V_OP1ZERO,R_MAKEXORRR,RG_DBL,0},
    {_UnPP(ANY,ANY,EQ_R1),0,0,0,0},
    {_UnPP(M,M,NONE),V_SAME_LOCN,0,0,0},
    {_UnPP(C,R,NONE),0,G_MOVRC,RG_DBL,FU_ALUX},
    {_UnPP(C,M,NONE),0,G_MOVMC,0,FU_ALUX},
    {_UnPP(R,M,NONE),V_RESLOC,G_MOVMA,RG_DBL_ACC,FU_ALUX},
    {_UnPP(M,R,NONE),V_OP1LOC,G_MOVAM,RG_DBL_ACC,FU_ALUX},
    {_UnPP(R,R,NONE),0,G_RR1,RG_DBL,FU_ALUX},
    {_UnPP(M,R,NONE),0,G_RM1,RG_DBL,FU_ALUX},
    {_UnPP(R,M,NONE),0,G_MR1,RG_DBL,FU_ALUX},
    {_Un(M,M,NONE),0,R_MOVOP1REG,RG_DBL,0},
    {_Un(R|M|U,R,NONE),0,G_UNKNOWN,RG_DBL,0},
    {_Un(R,ANY,NONE),0,G_UNKNOWN,RG_DBL,0},
    {_Un(ANY,ANY,NONE),0,G_UNKNOWN,RG_DBL_NEED,0}
};
/* Byte moves keep the old nonvolatile equality restriction (unlike
 * the encountered Move4 equality row, which has V_NO). */
static opcode_entry Move1[]={
    {_UnPP(ANY,ANY,EQ_R1),0x80,0,0,0},
    {_UnPP(M,M,NONE),V_SAME_LOCN,0,0,0},
    {_Un(C,R,NONE),V_OP1ZERO,R_MAKEXORRR,RG_BYTE,0},
    {_UnPP(C,R,NONE),0,G_MOVRC,RG_BYTE,FU_ALUX},
    {_UnPP(C,M,NONE),0,G_MOVMC,0,FU_ALUX},
    {_UnPP(R,M,NONE),V_RESLOC,G_MOVMA,RG_BYTE_ACC,FU_ALUX},
    {_UnPP(M,R,NONE),V_OP1LOC,G_MOVAM,RG_BYTE_ACC,FU_ALUX},
    {_UnPP(R,R,NONE),0,G_RR1,RG_BYTE,FU_ALUX},
    {_UnPP(M,R,NONE),0,G_RM1,RG_BYTE,FU_ALUX},
    {_UnPP(R,M,NONE),0,G_MR1,RG_BYTE,FU_ALUX},
    {_Un(M,M,NONE),0,R_MOVOP1REG,RG_BYTE,0},
    {_Un(ANY,R,NONE),0,G_UNKNOWN,RG_BYTE,0},
    {_Un(R|C,ANY,NONE),0,G_UNKNOWN,RG_BYTE,0},
    {_Un(ANY,ANY,NONE),0,G_UNKNOWN,RG_BYTE_NEED,0}
};
/* Integer comparison selection, released Cmp4's seventeen named rows.
 * V/G/R numbers below are the actual pre-64-bit 1994 enum values. */
enum { V_CONSTTEMP=53,V_CMPTRUE=28,V_CMPFALSE=29,V_OP2ZERO=17,V_AC_BETTER=39,
       R_TEMP2CONST=192,R_CMPTRUE=164,R_CMPFALSE=165,R_SWAPCMP=150,
       G_TEST=29,G_RR2=4,G_RM2=5,G_AC=2,G_RC=1,G_MC=3 };
static opcode_entry Cmp4[]={
    {_Side(R,U),V_CONSTTEMP,R_TEMP2CONST,RG_DBL,0},
    {_Side(U,R),V_CONSTTEMP,R_TEMP2CONST,RG_DBL,0},
    {_Side(ANY,C),V_CMPTRUE,R_CMPTRUE,0,0},
    {_Side(ANY,C),V_CMPFALSE,R_CMPFALSE,0,0},
    {_SidCC(R,C),V_OP2ZERO,G_TEST,RG_DBL,FU_ALUX},
    {_SidCC(R,R),0,G_RR2,RG_DBL,FU_ALUX},
    {_SidCC(R,M),0,G_RM2,RG_DBL,FU_ALUX},
    {_SidCC(R,C),V_AC_BETTER,G_AC,RG_DBL_ACC,FU_ALUX},
    {_SidCC(R,C),0,G_RC,RG_DBL,FU_ALUX},
    {_SidCC(M,C),0,G_MC,0,FU_ALUX},
    {_Side(C,C),0,R_MOVOP1REG,RG_DBL,0},
    {_Side(C,ANY),0,R_SWAPCMP,RG_DBL,0},
    {_Side(M,R),0,R_SWAPCMP,RG_DBL,0},
    {_Side(M,M),0,R_MOVOP1REG,RG_DBL,0},
    {_Side(ANY,R|C),0,G_UNKNOWN,RG_DBL,0},
    {_Side(R|C,ANY),0,G_UNKNOWN,RG_DBL,0},
    {_Side(ANY,ANY),0,G_UNKNOWN,RG_DBL_NEED,0}
};
/* Released String side-effect instruction selector. Its address itself
 * distinguishes string addressing in IndexOkay; exactly one six-byte row. */
opcode_entry String[]={
    {ANY*OP1_MUL|ANY*OP2_MUL|ANY|NONE,0,63,0,1} /* G_REPOP, FU_ALU1 */
};
/* Encountered Add4: all 24 released semantic rows match the old 144-byte
 * table at7ad72, with the pre-64-bit verify/reduction/register enums below.
 * Both actual FPU dispatch lists select this same table. */
enum { V_OP2NEG=14,V_OP2LOW_W_ZERO=26,V_OP1SP=34,V_OP2ONE=15,
       V_OP2TWO_SIZE=45,V_LEA=36,V_SWAP_GOOD=37,
       R_MAKEMOVE=126,R_MAKESUB=162,R_CYPHIGH=108,R_SWAPOPS=151,
       R_LOADOP2=159,R_USEREGISTER=152,
       G_WORDR1=7,G_M1=9,G_MR2=6,G_LEA=21 };
static opcode_entry Add4[]={
    {_Bin(ANY,C,ANY,NONE),NVI(V_OP2ZERO),R_MAKEMOVE,RG_DBL,0},
    {_Bin(ANY,C,ANY,NONE),NVI(V_OP2NEG),R_MAKESUB,RG_DBL,0},
    {_Bin(M,C,M,EQ_R1),NVI(V_OP2LOW_W_ZERO),R_CYPHIGH,RG_,0},
    {_Bin(R,U,R,NONE),V_CONSTTEMP,R_TEMP2CONST,RG_DBL,0},
    {_BinSC(R,C,R,EQ_R1),V_OP1SP,G_RC,RG_DBL,FU_ALUX},
    {_BinSC(R,C,R,EQ_R1),V_OP2ONE,G_WORDR1,RG_DBL,FU_ALUX},
    {_BinSC(R,C,R,EQ_R1),V_OP2TWO_SIZE,G_WORDR1,RG_DBL,FU_ALUX},
    {_BinSC(R,C,R,EQ_R1),V_AC_BETTER,G_AC,RG_DBL_ACC,FU_ALUX},
    {_BinSC(R,C,R,EQ_R1),0,G_RC,RG_DBL,FU_ALUX},
    {_BinPP(R,C,R,NONE),V_LEA,G_LEA,RG_DBL,FU_ALUX},
    {_BinSC(M,C,M,EQ_R1),V_OP2ONE,G_M1,RG_,FU_ALUX},
    {_BinSC(M,C,M,EQ_R1),0,G_MC,RG_,FU_ALUX},
    {_BinSC(R,R,R,EQ_R1),0,G_RR2,RG_DBL,FU_ALUX},
    {_BinSC(R,M,R,EQ_R1),0,G_RM2,RG_DBL,FU_ALUX},
    {_BinSC(M,R,M,EQ_R1),0,G_MR2,RG_DBL,FU_ALUX},
    {_Bin(R,R,R,EQ_R2),0,R_SWAPOPS,RG_DBL,0},
    {_Bin(R,R,R,NONE),0,G_LEA,RG_DBL,FU_ALUX},
    {_Bin(R|M,R|M|C,R|M,EQ_R1),0,R_LOADOP2,RG_DBL,0},
    {_Bin(R|M|C,R|M,R|M,EQ_R2),0,R_SWAPOPS,RG_DBL,0},
    {_Bin(R,ANY,ANY,NONE),V_SWAP_GOOD,R_SWAPOPS,RG_DBL,0},
    {_Bin(R|M|C,R|M|C,R|M,NONE),0,R_USEREGISTER,RG_DBL,0},
    {_Bin(ANY,R|C,ANY,EQ_R1),0,G_UNKNOWN,RG_DBL,0},
    {_Bin(R|C,ANY,ANY,EQ_R2),0,G_UNKNOWN,RG_DBL,0},
    {_Bin(ANY,ANY,ANY,NONE),0,G_UNKNOWN,RG_DBL_NEED,0}
};
/* Encountered And4: eighteen named386table.c rows, actual old108B
 * contribution7b162..7b1ce. Both FPU dispatch lists select it. */
enum { V_OP2_FFFFFFFF=40,V_OP2LOW_W_FFFF=13,V_OP2HIGH_W_FFFF_REG=50,
       R_MOVOP2=135,R_CYPLOW=109 };
static opcode_entry And4[]={
    {_Bin(ANY,C,ANY,NONE),NVI(V_OP2ZERO),R_MOVOP2,RG_DBL,0},
    {_Bin(ANY,C,ANY,NONE),NVI(V_OP2_FFFFFFFF),R_MAKEMOVE,RG_DBL,0},
    {_Bin(M,C,M,EQ_R1),NVI(V_OP2LOW_W_FFFF),R_CYPHIGH,RG_,0},
    {_Bin(R|M|U,C,R|M|U,EQ_R1),NVI(V_OP2HIGH_W_FFFF_REG),R_CYPLOW,RG_DBL,0},
    {_BinSC(R,C,R,EQ_R1),V_AC_BETTER,G_AC,RG_DBL_ACC,FU_ALUX},
    {_BinSC(R,C,R,EQ_R1),0,G_RC,RG_DBL,FU_ALUX},
    {_BinSC(M,C,M,EQ_R1),0,G_MC,RG_,FU_ALUX},
    {_BinSC(R,R,R,EQ_R1),0,G_RR2,RG_DBL,FU_ALUX},
    {_BinSC(R,M,R,EQ_R1),0,G_RM2,RG_DBL,FU_ALUX},
    {_BinSC(M,R,M,EQ_R1),0,G_MR2,RG_DBL,FU_ALUX},
    {_Bin(C,R|M|U,ANY,NONE),0,R_SWAPOPS,RG_DBL,0},
    {_Bin(R|M,R|M|C,R|M,EQ_R1),0,R_LOADOP2,RG_DBL,0},
    {_Bin(R|M|C,R|M,R|M,EQ_R2),0,R_SWAPOPS,RG_DBL,0},
    {_Bin(R,ANY,ANY,NONE),V_SWAP_GOOD,R_SWAPOPS,RG_DBL,0},
    {_Bin(R|M|C,R|M|C,R|M,NONE),0,R_USEREGISTER,RG_DBL,0},
    {_Bin(ANY,R|C,ANY,EQ_R1),0,G_UNKNOWN,RG_DBL,0},
    {_Bin(R|C,ANY,ANY,EQ_R2),0,G_UNKNOWN,RG_DBL,0},
    {_Bin(ANY,ANY,ANY,NONE),0,G_UNKNOWN,RG_DBL_NEED,0}
};
/* OR and XOR share the released seventeen-row Or4 selector. Actual
 * old contribution7b030..7b096 matches both FPU dispatch lists. */
enum { V_OP2HIGH_W_ZERO_REG=51 };
static opcode_entry Or4[]={
    {_Bin(ANY,C,ANY,NONE),NVI(V_OP2ZERO),R_MAKEMOVE,RG_DBL,0},
    {_Bin(M,C,M,EQ_R1),NVI(V_OP2LOW_W_ZERO),R_CYPHIGH,RG_,0},
    {_Bin(R|M|U,C,R|M|U,EQ_R1),NVI(V_OP2HIGH_W_ZERO_REG),R_CYPLOW,RG_DBL,0},
    {_BinSC(R,C,R,EQ_R1),V_AC_BETTER,G_AC,RG_DBL_ACC,FU_ALUX},
    {_BinSC(R,C,R,EQ_R1),0,G_RC,RG_DBL,FU_ALUX},
    {_BinSC(M,C,M,EQ_R1),0,G_MC,RG_,FU_ALUX},
    {_BinSC(R,R,R,EQ_R1),0,G_RR2,RG_DBL,FU_ALUX},
    {_BinSC(R,M,R,EQ_R1),0,G_RM2,RG_DBL,FU_ALUX},
    {_BinSC(M,R,M,EQ_R1),0,G_MR2,RG_DBL,FU_ALUX},
    {_Bin(C,R|M|U,ANY,NONE),0,R_SWAPOPS,RG_DBL,0},
    {_Bin(R|M,R|M|C,R|M,EQ_R1),0,R_LOADOP2,RG_DBL,0},
    {_Bin(R|M|C,R|M,R|M,EQ_R2),0,R_SWAPOPS,RG_DBL,0},
    {_Bin(R,ANY,ANY,NONE),V_SWAP_GOOD,R_SWAPOPS,RG_DBL,0},
    {_Bin(R|M|C,R|M|C,R|M,NONE),0,R_USEREGISTER,RG_DBL,0},
    {_Bin(ANY,R|C,ANY,EQ_R1),0,G_UNKNOWN,RG_DBL,0},
    {_Bin(R|C,ANY,ANY,EQ_R2),0,G_UNKNOWN,RG_DBL,0},
    {_Bin(ANY,ANY,ANY,NONE),0,G_UNKNOWN,RG_DBL_NEED,0}
};
/* Encountered word AND selector7b0f6..7b162: the complete eighteen
 * source-named rows, using the original byte-high/low verifier values. */
enum { V_OP2LOW_B_FF=11,V_OP2HIGH_B_FF=23 };
static opcode_entry And2[]={
    {_Bin(ANY,C,ANY,NONE),NVI(V_OP2ZERO),R_MOVOP2,RG_WORD,0},
    {_Bin(ANY,C,ANY,NONE),NVI(V_OP2LOW_W_FFFF),R_MAKEMOVE,RG_WORD,0},
    {_Bin(R|M|U,C,R|M|U,EQ_R1),NVI(V_OP2LOW_B_FF),R_CYPHIGH,RG_TWOBYTE,0},
    {_Bin(R|M|U,C,R|M|U,EQ_R1),NVI(V_OP2HIGH_B_FF),R_CYPLOW,RG_TWOBYTE,0},
    {_BinSC(R,C,R,EQ_R1),0,G_AC,RG_WORD_ACC,FU_ALUX},
    {_BinSC(R,C,R,EQ_R1),0,G_RC,RG_WORD,FU_ALUX},
    {_BinSC(M,C,M,EQ_R1),0,G_MC,RG_,FU_ALUX},
    {_BinSC(R,R,R,EQ_R1),0,G_RR2,RG_WORD,FU_ALUX},
    {_BinSC(R,M,R,EQ_R1),0,G_RM2,RG_WORD,FU_ALUX},
    {_BinSC(M,R,M,EQ_R1),0,G_MR2,RG_WORD,FU_ALUX},
    {_Bin(C,R|M|U,ANY,NONE),0,R_SWAPOPS,RG_WORD,0},
    {_Bin(R|M,R|M|C,R|M,EQ_R1),0,R_LOADOP2,RG_WORD,0},
    {_Bin(R|M|C,R|M,R|M,EQ_R2),0,R_SWAPOPS,RG_WORD,0},
    {_Bin(R,ANY,ANY,NONE),V_SWAP_GOOD,R_SWAPOPS,RG_WORD,0},
    {_Bin(R|M|C,R|M|C,R|M,NONE),0,R_USEREGISTER,RG_WORD,0},
    {_Bin(ANY,R|C,ANY,EQ_R1),0,G_UNKNOWN,RG_WORD,0},
    {_Bin(R|C,ANY,ANY,EQ_R2),0,G_UNKNOWN,RG_WORD,0},
    {_Bin(ANY,ANY,ANY,NONE),0,G_UNKNOWN,RG_WORD_NEED,0}
};
/* Byte AND, the complete sixteen source-named rows at7b096..7b0f6. */
static opcode_entry And1[]={
    {_Bin(ANY,C,ANY,NONE),NVI(V_OP2ZERO),R_MOVOP2,RG_BYTE,0},
    {_Bin(ANY,C,ANY,NONE),NVI(V_OP2LOW_B_FF),R_MAKEMOVE,RG_BYTE,0},
    {_BinSC(R,C,R,EQ_R1),0,G_AC,RG_BYTE_ACC,FU_ALUX},
    {_BinSC(R,C,R,EQ_R1),0,G_RC,RG_BYTE,FU_ALUX},
    {_BinSC(M,C,M,EQ_R1),0,G_MC,RG_,FU_ALUX},
    {_BinSC(R,R,R,EQ_R1),0,G_RR2,RG_BYTE,FU_ALUX},
    {_BinSC(R,M,R,EQ_R1),0,G_RM2,RG_BYTE,FU_ALUX},
    {_BinSC(M,R,M,EQ_R1),0,G_MR2,RG_BYTE,FU_ALUX},
    {_Bin(C,R|M|U,ANY,NONE),0,R_SWAPOPS,RG_BYTE,0},
    {_Bin(R|M,R|M|C,R|M,EQ_R1),0,R_LOADOP2,RG_BYTE,0},
    {_Bin(R|M|C,R|M,R|M,EQ_R2),0,R_SWAPOPS,RG_BYTE,0},
    {_Bin(R,ANY,ANY,NONE),V_SWAP_GOOD,R_SWAPOPS,RG_BYTE,0},
    {_Bin(R|M|C,R|M|C,R|M,NONE),0,R_USEREGISTER,RG_BYTE,0},
    {_Bin(ANY,R|C,ANY,EQ_R1),0,G_UNKNOWN,RG_BYTE,0},
    {_Bin(R|C,ANY,ANY,EQ_R2),0,G_UNKNOWN,RG_BYTE,0},
    {_Bin(ANY,ANY,ANY,NONE),0,G_UNKNOWN,RG_BYTE_NEED,0}
};
/* Structure move selector: encountered full five-row30B MoveXX
 * contribution7b88e..7b8ac; memory forcing remains genuine reductions. */
enum { R_FORCEOP1MEM=115,R_FORCERESMEM=117,R_MAKESTRMOVE=128 };
static opcode_entry MoveXX[]={
    {_Un(M|U,M|U,EQ_R1),NVI(0),0,RG_,0},
    {_UnPP(M,M,NONE),V_SAME_LOCN,0,RG_,0},
    {_Un(U,ANY,NONE),0,R_FORCEOP1MEM,RG_,0},
    {_Un(ANY,U,NONE),0,R_FORCERESMEM,RG_,0},
    {_Un(ANY,ANY,NONE),0,R_MAKESTRMOVE,RG_,0}
};
/* Encountered near-address load, complete eight-row48B contribution
 *7b8be..7b8ee from386table.c LoadA. */
enum { V_OFFSETZERO=5,V_OP1ADDR=6,R_MOVEINDEX=132,R_RESREG=144,
       G_MADDR=28,G_RADDR=34,G_LOADADDR=64 };
static opcode_entry LoadA[]={
    {_Un(M,ANY,NONE),V_OFFSETZERO,R_MOVEINDEX,RG_DBL,0},
    {_UnPP(M,M,NONE),V_OP1ADDR,G_MADDR,RG_,FU_ALUX},
    {_UnPP(M,R,NONE),V_OP1ADDR,G_RADDR,RG_DBL,FU_ALUX},
    {_UnPP(M,R,NONE),0,G_LOADADDR,RG_DBL,FU_ALUX},
    {_Un(M,ANY,NONE),V_OP1ADDR,G_UNKNOWN,RG_DBL,0},
    {_Un(M,M,NONE),0,R_RESREG,RG_DBL,0},
    {_Un(M,ANY,NONE),0,G_UNKNOWN,RG_DBL_NEED,0},
    {_Un(ANY,ANY,NONE),0,R_FORCEOP1MEM,RG_DBL,0}
};
/* CheckCC explicitly selects the old word-move sequence at7b792,
 * even in this386 build. Include its leading CC selector as the actual
 * MOV2 dispatch7b78c and preserve all23 source-named rows through7b816. */
enum { G_SR=26,G_RS=25,G_MS1=23,G_SM1=24,G_SEG_SEG=86,
       V_SIZE=32,V_WORDREG_AVAIL=35,R_SEG_SEG=176,R_MOVRESMEM=180 };
opcode_entry Move2CC[]={
    {_Un(C,R,NONE),V_OP1ZERO,R_MAKEXORRR,RG_WORD,0},
    {_UnPP(ANY,ANY,EQ_R1),NVI(0),0,RG_,0},
    {_UnPP(M,M,NONE),V_SAME_LOCN,0,RG_,0},
    {_UnPP(C,R,NONE),0,G_MOVRC,RG_WORD,FU_ALUX},
    {_UnPP(C,M,NONE),0,G_MOVMC,RG_,FU_ALUX},
    {_UnPP(R,M,NONE),V_RESLOC,G_MOVMA,RG_WORD_ACC,FU_ALUX},
    {_UnPP(M,R,NONE),V_OP1LOC,G_MOVAM,RG_WORD_ACC,FU_ALUX},
    {_UnPP(R,R,NONE),0,G_RR1,RG_WORD,FU_ALUX},
    {_UnPP(M,R,NONE),0,G_RM1,RG_WORD,FU_ALUX},
    {_UnPP(R,M,NONE),0,G_MR1,RG_WORD,FU_ALUX},
    {_UnPP(R,R,NONE),0,G_SR,RG_WORD_SEG,FU_ALUX},
    {_UnPP(R,R,NONE),0,G_RS,RG_SEG_WORD,FU_ALUX},
    {_UnPP(R,M,NONE),0,G_MS1,RG_SEG_WORD,FU_ALUX},
    {_UnPP(M,R,NONE),0,G_SM1,RG_WORD_SEG,FU_ALUX},
    {_UnPP(R,R,NONE),V_SIZE,G_SEG_SEG,RG_SEG_SEG,FU_ALUX},
    {_UnPP(R,R,NONE),V_WORDREG_AVAIL,R_SEG_SEG,RG_SEG_SEG,0},
    {_UnPP(R,R,NONE),0,G_SEG_SEG,RG_SEG_SEG,FU_ALUX},
    {_Un(C,R,NONE),0,R_MOVOP1REG,RG_WORD_SEG,0},
    {_Un(M,M,NONE),0,R_MOVOP1REG,RG_WORD,0},
    {_Un(R|M|U,R,NONE),0,G_UNKNOWN,RG_ANYWORD,0},
    {_Un(R,ANY,NONE),0,G_UNKNOWN,RG_ANYWORD,0},
    {_Un(ANY,ANY,NONE),0,G_UNKNOWN,RG_ANYWORD_NEED,0},
    /* Older terminal constant fallback, absent from later released table. */
    {_Un(C,ANY,NONE),0,R_MOVRESMEM,RG_DBL,0}
};
/* Encountered word OR/XOR, all17 source-named rows7afca..7b030. */
enum { V_OP2LOW_B_ZERO=12,V_OP2HIGH_B_ZERO=10 };
static opcode_entry Or2[]={
    {_Bin(ANY,C,ANY,NONE),NVI(V_OP2ZERO),R_MAKEMOVE,RG_WORD,0},
    {_Bin(R|M|U,C,R|M|U,EQ_R1),NVI(V_OP2LOW_B_ZERO),R_CYPHIGH,RG_TWOBYTE,0},
    {_Bin(R|M|U,C,R|M|U,EQ_R1),NVI(V_OP2HIGH_B_ZERO),R_CYPLOW,RG_TWOBYTE,0},
    {_BinSC(R,C,R,EQ_R1),0,G_AC,RG_WORD_ACC,FU_ALUX},
    {_BinSC(R,C,R,EQ_R1),0,G_RC,RG_WORD,FU_ALUX},
    {_BinSC(M,C,M,EQ_R1),0,G_MC,RG_,FU_ALUX},
    {_BinSC(R,R,R,EQ_R1),0,G_RR2,RG_WORD,FU_ALUX},
    {_BinSC(R,M,R,EQ_R1),0,G_RM2,RG_WORD,FU_ALUX},
    {_BinSC(M,R,M,EQ_R1),0,G_MR2,RG_WORD,FU_ALUX},
    {_Bin(C,R|M|U,ANY,NONE),0,R_SWAPOPS,RG_WORD,0},
    {_Bin(R|M,R|M|C,R|M,EQ_R1),0,R_LOADOP2,RG_WORD,0},
    {_Bin(R|M|C,R|M,R|M,EQ_R2),0,R_SWAPOPS,RG_WORD,0},
    {_Bin(R,ANY,ANY,NONE),V_SWAP_GOOD,R_SWAPOPS,RG_WORD,0},
    {_Bin(R|M|C,R|M|C,R|M,NONE),0,R_USEREGISTER,RG_WORD,0},
    {_Bin(ANY,R|C,ANY,EQ_R1),0,G_UNKNOWN,RG_WORD,0},
    {_Bin(R|C,ANY,ANY,EQ_R2),0,G_UNKNOWN,RG_WORD,0},
    {_Bin(ANY,ANY,ANY,NONE),0,G_UNKNOWN,RG_WORD_NEED,0}
};
/* Byte OR/XOR, complete15-row90B source table7af70..7afca. */
static opcode_entry Or1[]={
    {_Bin(ANY,C,ANY,NONE),NVI(V_OP2ZERO),R_MAKEMOVE,RG_BYTE,0},
    {_BinSC(R,C,R,EQ_R1),0,G_AC,RG_BYTE_ACC,FU_ALUX},
    {_BinSC(R,C,R,EQ_R1),0,G_RC,RG_BYTE,FU_ALUX},
    {_BinSC(M,C,M,EQ_R1),0,G_MC,RG_,FU_ALUX},
    {_BinSC(R,R,R,EQ_R1),0,G_RR2,RG_BYTE,FU_ALUX},
    {_BinSC(R,M,R,EQ_R1),0,G_RM2,RG_BYTE,FU_ALUX},
    {_BinSC(M,R,M,EQ_R1),0,G_MR2,RG_BYTE,FU_ALUX},
    {_Bin(C,R|M|U,ANY,NONE),0,R_SWAPOPS,RG_BYTE,0},
    {_Bin(R|M,R|M|C,R|M,EQ_R1),0,R_LOADOP2,RG_BYTE,0},
    {_Bin(R|M|C,R|M,R|M,EQ_R2),0,R_SWAPOPS,RG_BYTE,0},
    {_Bin(R,ANY,ANY,NONE),V_SWAP_GOOD,R_SWAPOPS,RG_BYTE,0},
    {_Bin(R|M|C,R|M|C,R|M,NONE),0,R_USEREGISTER,RG_BYTE,0},
    {_Bin(R|C,ANY,ANY,EQ_R2),0,G_UNKNOWN,RG_BYTE,0},
    {_Bin(ANY,R|C,ANY,EQ_R1),0,G_UNKNOWN,RG_BYTE,0},
    {_Bin(ANY,ANY,ANY,NONE),0,G_UNKNOWN,RG_BYTE_NEED,0}
};
/* Encountered DWORD shift selector: all seventeen semantic rows from
 * 386table.c Shft4 match the old102B contribution7b48c..7b4f2.
 * Both actual FPU dispatch arrays select the same object. */
enum { V_LSHONE=41,R_CHANGESHIFT=104,R_ADDRR=181,R_OP1RESTEMP=143,
       R_OP2CL=141,G_R1SHIFT=13,G_1SHIFT=12,G_RNSHIFT=15,
       G_NSHIFT=14,G_RCLSHIFT=17,G_CLSHIFT=16 };
static opcode_entry Shft4[]={
    {_Bin(ANY,C,ANY,NONE),V_OP2NEG,R_CHANGESHIFT,RG_DBL_SHIFT,0},
    {_Bin(ANY,C,ANY,NONE),NVI(V_OP2ZERO),R_MAKEMOVE,RG_DBL,0},
    {_Bin(R,C,R,EQ_R1),V_LSHONE,R_ADDRR,RG_DBL,0},
    {_BinPP(R,C,R,NONE),V_LEA,G_LEA,RG_DBL,FU_ALUX},
    {_Bin(R,C,R,EQ_R1),V_OP2ONE,G_R1SHIFT,RG_DBL,1},
    {_Bin(M,C,M,EQ_R1),V_OP2ONE,G_1SHIFT,RG_,1},
    {_Bin(R,C,R,EQ_R1),0,G_RNSHIFT,RG_DBL,1},
    {_Bin(M,C,M,EQ_R1),0,G_NSHIFT,RG_,1},
    {_Bin(R,R,R,EQ_R1),0,G_RCLSHIFT,RG_DBL_SHIFT,1},
    {_Bin(M,R,M,EQ_R1),0,G_CLSHIFT,RG_DBL_SHIFT,1},
    {_Bin(R|M|C,C,R|M,NONE),0,R_OP1RESTEMP,RG_DBL_SHIFT,0},
    {_Bin(R|M|C,R,R|M,NONE),0,R_OP1RESTEMP,RG_DBL_SHIFT,0},
    {_Bin(ANY,C,ANY,EQ_R1),0,G_UNKNOWN,RG_DBL,0},
    {_Bin(ANY,C,ANY,NONE),0,G_UNKNOWN,RG_DBL_NEED,0},
    {_Bin(ANY,R,ANY,EQ_R1),0,G_UNKNOWN,RG_DBL_SHIFT,0},
    {_Bin(ANY,R,ANY,NONE),0,G_UNKNOWN,RG_DBL_SHIFT_NEED,0},
    {_Bin(ANY,ANY,ANY,NONE),0,R_OP2CL,RG_,0}
};
/* DWORD multiply: fifteen released semantic rows, exactly the old
 *90B contribution7b26a..7b2c4; common to both FPU selections. */
enum { V_LEA_GOOD=47,R_MOVOP1RES=139,G_186RMUL=83,G_186MUL=82,
       G_0FRR2=90,G_0FRM2=91,FU_IMUL=3 };
static opcode_entry Mul4[]={
    {_BinPP(R,C,R,NONE),V_LEA,G_LEA,RG_DBL,FU_ALUX},
    {_Bin(R,C,R,NONE),0,G_186RMUL,RG_DBL,FU_IMUL},
    {_Bin(M,C,R,NONE),V_LEA_GOOD,R_MOVOP1RES,RG_DBL,0},
    {_Bin(M,C,R,NONE),0,G_186MUL,RG_DBL,FU_IMUL},
    {_Bin(U,C,R,NONE),0,G_UNKNOWN,RG_DBL,0},
    {_Bin(ANY,C,M,NONE),0,R_RESREG,RG_DBL,0},
    {_Bin(ANY,C,ANY,NONE),0,G_UNKNOWN,RG_DBL_NEED,0},
    {_Bin(C,ANY,ANY,NONE),0,R_SWAPOPS,RG_DBL,0},
    {_Bin(R,R,R,EQ_R1),0,G_0FRR2,RG_DBL,FU_IMUL},
    {_Bin(R,M,R,EQ_R1),0,G_0FRM2,RG_DBL,FU_IMUL},
    {_Bin(C,R|M|U,ANY,NONE),0,R_SWAPOPS,RG_DBL,0},
    {_Bin(R|M|C,R,R,EQ_R2),0,R_SWAPOPS,RG_DBL,0},
    {_Bin(R,ANY,ANY,NONE),V_SWAP_GOOD,R_SWAPOPS,RG_DBL,0},
    {_Bin(R|M|C,R|M|C,R|M,NONE),0,R_USEREGISTER,RG_DBL,0},
    {_Bin(ANY,ANY,ANY,NONE),0,G_UNKNOWN,RG_DBL_NEED,0}
};
/* Byte compare/test share the released TestOrCmp1 selector, exactly
 * seventeen rows/102B at7b4f2..7b558 in both FPU dispatch lists. */
static opcode_entry TestOrCmp1[]={
    {_Side(R,U),V_CONSTTEMP,R_TEMP2CONST,RG_BYTE,0},
    {_Side(U,R),V_CONSTTEMP,R_TEMP2CONST,RG_BYTE,0},
    {_Side(ANY,C),V_CMPTRUE,R_CMPTRUE,RG_,0},
    {_Side(ANY,C),V_CMPFALSE,R_CMPFALSE,RG_,0},
    {_SidCC(R,R),0,G_RR2,RG_BYTE,FU_ALUX},
    {_SidCC(R,M),0,G_RM2,RG_BYTE,FU_ALUX},
    {_SidCC(R,C),V_OP2ZERO,G_TEST,RG_BYTE,FU_ALUX},
    {_SidCC(R,C),0,G_AC,RG_BYTE_ACC,FU_ALUX},
    {_SidCC(R,C),0,G_RC,RG_BYTE,FU_ALUX},
    {_SidCC(M,C),0,G_MC,RG_,FU_ALUX},
    {_Side(C,C),0,R_MOVOP1REG,RG_BYTE,0},
    {_Side(C,ANY),0,R_SWAPCMP,RG_BYTE,0},
    {_Side(M,R),0,R_SWAPCMP,RG_BYTE,0},
    {_Side(M,M),0,R_MOVOP1REG,RG_BYTE,0},
    {_Side(ANY,R|C),0,G_UNKNOWN,RG_BYTE,0},
    {_Side(R|C,ANY),0,G_UNKNOWN,RG_BYTE,0},
    {_Side(ANY,ANY),0,G_UNKNOWN,RG_BYTE_NEED,0}
};
/* DWORD bit-test selector: eighteen semantic rows, old108B
 * contribution7b5c4..7b630, selected by both actual FPU lists. */
static opcode_entry Test4[]={
    {_Side(M,C),NVI(V_OP2LOW_W_ZERO),R_CYPHIGH,RG_,0},
    {_Side(R|M|U,C),NVI(V_OP2HIGH_W_ZERO_REG),R_CYPLOW,RG_DBL,0},
    {_Side(R,U),V_CONSTTEMP,R_TEMP2CONST,RG_DBL,0},
    {_Side(U,R),V_CONSTTEMP,R_TEMP2CONST,RG_DBL,0},
    {_Side(ANY,C),V_CMPTRUE,R_CMPTRUE,RG_,0},
    {_Side(ANY,C),V_CMPFALSE,R_CMPFALSE,RG_,0},
    {_SidCC(R,R),0,G_RR2,RG_DBL,FU_ALUX},
    {_SidCC(R,M),0,G_RM2,RG_DBL,FU_ALUX},
    {_SidCC(R,C),V_AC_BETTER,G_AC,RG_DBL_ACC,FU_ALUX},
    {_SidCC(R,C),0,G_RC,RG_DBL,FU_ALUX},
    {_SidCC(M,C),0,G_MC,RG_,FU_ALUX},
    {_Side(C,C),0,R_MOVOP1REG,RG_DBL,0},
    {_Side(C,ANY),0,R_SWAPCMP,RG_DBL,0},
    {_Side(M,R),0,R_SWAPCMP,RG_DBL,0},
    {_Side(M,M),0,R_MOVOP1REG,RG_DBL,0},
    {_Side(ANY,R|C),0,G_UNKNOWN,RG_DBL,0},
    {_Side(R|C,ANY),0,G_UNKNOWN,RG_DBL,0},
    {_Side(ANY,ANY),0,G_UNKNOWN,RG_DBL_NEED,0}
};
/* Aggregate push: unknown-to-memory then R_DOLONGPUSH,7ba6e..7ba7a. */
static opcode_entry PushXX[]={
    {_Un(U,ANY,NONE),0,115,RG_,0}, /* R_FORCEOP1MEM */
    {_Un(M,ANY,NONE),0,111,RG_,0} /* R_DOLONGPUSH */
};
/* 386table.c Push4, four actual rows at7ba44..7ba5c. */
static opcode_entry Push4[]={
    {_UnPP(R,ANY,NONE),0,7,RG_DBL,FU_ALUX}, /* G_WORDR1 */
    {_UnPP(M,ANY,NONE),0,9,RG_,1}, /* G_M1, FU_ALU1 */
    {_UnPP(C,ANY,NONE),0,35,RG_,FU_ALUX}, /* G_C1 */
    {_Un(ANY,ANY,NONE),0,G_UNKNOWN,RG_DBL,0}
};
/* Direct call: actual Call row at7ba7a; G_CALL74 and FU_CALL10. */
static opcode_entry Call[]={
    {_Bin(ANY,ANY,ANY,NONE),0,74,RG_,10}
};
/* Indirect call selection is the single source CallI row,6B at7ba80. */
static opcode_entry CallI[]={
    {_Bin(ANY,ANY,ANY,NONE),0,76,RG_,10} /* G_ICALL, FU_CALL */
};
/* Word bit-test selector: eighteen source Test2 rows, exact108B
 * contribution7b558..7b5c4 in both actual dispatch arrays. */
static opcode_entry Test2[]={
    {_Side(R|M|U,C),NVI(V_OP2LOW_B_ZERO),R_CYPHIGH,RG_TWOBYTE,0},
    {_Side(R|M|U,C),NVI(V_OP2HIGH_B_ZERO),R_CYPLOW,RG_TWOBYTE,0},
    {_Side(R,U),V_CONSTTEMP,R_TEMP2CONST,RG_WORD,0},
    {_Side(U,R),V_CONSTTEMP,R_TEMP2CONST,RG_WORD,0},
    {_Side(ANY,C),V_CMPTRUE,R_CMPTRUE,RG_,0},
    {_Side(ANY,C),V_CMPFALSE,R_CMPFALSE,RG_,0},
    {_SidCC(R,R),0,G_RR2,RG_WORD,FU_ALUX},
    {_SidCC(R,M),0,G_RM2,RG_WORD,FU_ALUX},
    {_SidCC(R,C),0,G_AC,RG_WORD_ACC,FU_ALUX},
    {_SidCC(R,C),0,G_RC,RG_WORD,FU_ALUX},
    {_SidCC(M,C),0,G_MC,RG_,FU_ALUX},
    {_Side(C,C),0,R_MOVOP1REG,RG_WORD,0},
    {_Side(C,ANY),0,R_SWAPCMP,RG_WORD,0},
    {_Side(M,R),0,R_SWAPCMP,RG_WORD,0},
    {_Side(M,M),0,R_MOVOP1REG,RG_WORD,0},
    {_Side(ANY,R|C),0,G_UNKNOWN,RG_WORD,0},
    {_Side(R|C,ANY),0,G_UNKNOWN,RG_WORD,0},
    {_Side(ANY,ANY),0,G_UNKNOWN,RG_WORD_NEED,0}
};
/* i87table.c seven-row binary selector, encountered by AssignARegister.
 * Actual 1994 contribution7c350..7c37a; other FPU/software tables remain
 * unsupported until encountered. Old reduction and register indices retained. */
enum { V_OP1ONE=8,R_FORCEOP1CMEM=114,R_FORCEOP2CMEM=116 };
static opcode_entry Bin87[]={
    {_BinPP(C,R|M|U,ANY,NONE),V_OP1ONE,G_UNKNOWN,RG_8087,0},
    {_BinPP(C,R|M|U,ANY,NONE),V_OP1ZERO,G_UNKNOWN,RG_8087,0},
    {_BinPP(C,ANY,ANY,NONE),0,R_FORCEOP1CMEM,RG_8087,0},
    {_BinPP(ANY,C,ANY,NONE),V_OP2ONE,G_UNKNOWN,RG_8087,0},
    {_BinPP(ANY,C,ANY,NONE),V_OP2ZERO,G_UNKNOWN,RG_8087,0},
    {_BinPP(ANY,C,ANY,NONE),0,R_FORCEOP2CMEM,RG_8087,0},
    {_BinPP(ANY,ANY,ANY,NONE),0,G_UNKNOWN,RG_8087,0}
};
/* Encountered Move87D from i87table.c, twenty typed rows120B at7c20c.
 * The reduction numbers are the exact older table values, not later enums. */
enum { V_SAME_TYPE=42,R_SPLITMOVE_FP=147,R_MOVOP1MEM_FP=137,R_MOVRESMEM_FP=179 };
static opcode_entry Move87D[]={
    {_UnPP(R|M|U,R|M|U,EQ_R1),0,0,RG_,0},
    {_UnPP(M,M,NONE),V_SAME_LOCN,0,RG_,0},
    {_UnPP(M,M,NONE),V_SAME_TYPE,R_SPLITMOVE_FP,RG_,0},
    {_UnPP(M|C,R,NONE),0,R_SPLITMOVE_FP,RG_8,0},
    {_UnPP(R,M,NONE),0,R_SPLITMOVE_FP,RG_8,0},
    {_UnPP(C,M,NONE),V_SAME_TYPE,R_SPLITMOVE_FP,RG_8,0},
    {_UnPP(U,M,NONE),0,G_UNKNOWN,RG_8087_NEED,0},
    {_UnPP(R,M,NONE),0,G_UNKNOWN,RG_ST0_STI,0},
    {_UnPP(M|C,U,NONE),0,G_UNKNOWN,RG_8087_NEED,0},
    {_UnPP(M,R,NONE),0,G_UNKNOWN,RG_STI_ST0,0},
    {_UnPP(R,ANY,NONE),0,R_MOVOP1MEM_FP,RG_8_8087,0},
    {_UnPP(ANY,R,NONE),0,R_MOVRESMEM_FP,RG_8087_8,0},
    {_UnPP(C,ANY,NONE),V_OP1ZERO,G_UNKNOWN,RG_8087,0},
    {_UnPP(C,ANY,NONE),V_OP1ONE,G_UNKNOWN,RG_8087,0},
    {_UnPP(C,ANY,NONE),0,R_FORCEOP1CMEM,RG_8087,0},
    {_UnPP(M,R,NONE),0,G_UNKNOWN,RG_8087,0},
    {_UnPP(R,M,NONE),0,G_UNKNOWN,RG_8087,0},
    {_UnPP(R|M,ANY,NONE),0,G_UNKNOWN,RG_8087_NEED,0},
    {_UnPP(R,R,NONE),0,G_UNKNOWN,RG_ST0_STI,0},
    {_UnPP(ANY,ANY,NONE),0,G_UNKNOWN,RG_8087_NEED,0}
};
/* 386table.c unsigned/signed DWORD remainder selection, all five rows
 * at7b3a2..7b3c0, identical in the two actual FPU dispatch lists. */
enum { R_MOVOP2TEMP_MOD=136,R_DIVREGISTER_MOD=154 };
static opcode_entry Mod4[]={
    {_Bin(ANY,C,ANY,NONE),0,R_MOVOP2TEMP_MOD,RG_DBL_MOD,0},
    {_Bin(R,R,R,NONE),0,10,RG_DBL_MOD,4}, /* G_R2, FU_IDIV */
    {_Bin(R,M,R,NONE),0,11,RG_DBL_MOD,4}, /* G_M2, FU_IDIV */
    {_Bin(R,U,R,NONE),0,G_UNKNOWN,RG_DBL_MOD,0},
    {_Bin(ANY,ANY,ANY,NONE),0,R_DIVREGISTER_MOD,RG_DBL_MOD,0}
};
/* i87table.c Push87D: actual OSI rows at7c344, selected by PSHFD
 * when an x87 is enabled. The old R_SPLITUNARY ordinal is149. */
static opcode_entry Push87D[]={
    {_Un(R|M|C,ANY,NONE),0,149,RG_8,0},
    {_Un(ANY,ANY,NONE),0,G_UNKNOWN,RG_8087,0}
};
struct opcode_entry *OpcodeTable(unsigned char index)
{
    /* This encountered selector is the first differing FPU dispatch. */
    if ((index==RTN8C || index==RTN8) &&
        (CGProcessorVersion&0x70)>=0x10) return Bin87;
    if (index==MOVFD && (CGProcessorVersion&0x70)>=0x10) return Move87D;
    if (index==PSHFD && (CGProcessorVersion&0x70)>=0x10) return Push87D;
    /* The other encountered entries are identical in both FPU choices. */
    if (index==ADD4) return Add4;
    if (index==SHFT4) return Shft4;
    if (index==MUL4) return Mul4;
    if (index==MOD4) return Mod4;
    if (index==CMP1 || index==TEST1) return TestOrCmp1;
    if (index==TEST4) return Test4;
    if (index==TEST2) return Test2;
    if (index==PUSHX) return PushXX;
    if (index==PUSH4) return Push4;
    if (index==CALL) return Call;
    if (index==CALLI) return CallI;
    if (index==AND4) return And4;
    if (index==AND2) return And2;
    if (index==AND1) return And1;
    if (index==OR4) return Or4;
    if (index==OR2) return Or2;
    if (index==OR1) return Or1;
    if (index==DPARM) return Parm;
    if (index==CVT) return Cvt;
    if (index==MOV4) return Move4CC;
    if (index==MOVX) return MoveXX;
    if (index==LA) return LoadA;
    if (index==MOV1) return Move1;
    if (index==MOV2) return Move2CC;
    if (index==CMP4) return Cmp4;
    if (index!=DONOTHING) STUB_TRAP();
    return DoNop;
}
