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
* Description:  Map the 1994 opcode/type pairs to named generation tables.
*
****************************************************************************/


#include "tables.h"
#include <recon.h>
/* Source386optab.c projected to the actual old76-opcode/11-type universe:
 * U8/I8/FL columns and P5DIV/later SET opcodes are absent; the old ordering
 * places MOV and careful LA before LA. All836 emitted selector bytes match
 * the encountered original OpTable7a938..7ac7c. Old unused slots are genuine
 * NO rows, matching released OP_SLACK, rather than unknown zero defaults. */
table_def OpTable[76][11]={
    {NO,NO,NO,NO,NO,NO,NO,BAD,NO,NO,NO}, /* OP_NOP */
    {ADD1,ADD1,ADD2,ADD2,ADD4,ADD4,ADDCP,BAD,RTN4FC,RTN8C,BAD}, /* OP_ADD */
    {BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD}, /* OP_EXT_ADD */
    {SUB1,SUB1,SUB2,SUB2,SUB4,SUB4,SUBCP,BAD,RTN4F,RTN8,BAD}, /* OP_SUB */
    {BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD}, /* OP_EXT_SUB */
    {MUL1,MUL1,MUL2,MUL2,MUL4,MUL4,BAD,BAD,RTN4FC,RTN8C,BAD}, /* OP_MUL */
    {BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD}, /* OP_EXT_MUL */
    {DIV1,DIV1,DIV2,DIV2,DIV4,DIV4,BAD,BAD,RTN4F,RTN8,BAD}, /* OP_DIV */
    {MOD1,MOD1,MOD2,MOD2,MOD4,MOD4,BAD,BAD,RTN4F,RTN8,BAD}, /* OP_MOD */
    {AND1,AND1,AND2,AND2,AND4,AND4,BAD,BAD,BAD,BAD,BAD}, /* OP_AND */
    {OR1,OR1,OR2,OR2,OR4,OR4,BAD,BAD,BAD,BAD,BAD}, /* OP_OR */
    {OR1,OR1,OR2,OR2,OR4,OR4,BAD,BAD,BAD,BAD,BAD}, /* OP_XOR */
    {SHFT1,SHFT1,SHFT2,SHFT2,SHFT4,SHFT4,BAD,BAD,BAD,BAD,BAD}, /* OP_RSHIFT */
    {SHFT1,SHFT1,SHFT2,SHFT2,SHFT4,SHFT4,BAD,BAD,BAD,BAD,BAD}, /* OP_LSHIFT */
    {BAD,BAD,BAD,BAD,BAD,RTN4,BAD,BAD,BFUNS,BFUND,BAD}, /* OP_POW */
    {BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,BFUNS,BFUND,BAD}, /* OP_ATAN2 */
    {BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,BFUNS,BFUND,BAD}, /* OP_FMOD */
    {NEG1,NEG1,NEG2,NEG2,NEG4,NEG4,BAD,BAD,NEGF,NEGF,BAD}, /* OP_NEGATE */
    {NOT1,NOT1,NOT2,NOT2,NOT4,NOT4,BAD,BAD,BAD,BAD,BAD}, /* OP_COMPLEMENT */
    {BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,UFUNS,UFUND,BAD}, /* OP_LOG */
    {BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,UFUNS,UFUND,BAD}, /* OP_COS */
    {BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,UFUNS,UFUND,BAD}, /* OP_SIN */
    {BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,UFUNS,UFUND,BAD}, /* OP_TAN */
    {BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,UFUNS,UFUND,BAD}, /* OP_SQRT */
    {BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,UFUNS,UFUND,BAD}, /* OP_FABS */
    {BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,UFUNS,UFUND,BAD}, /* OP_ACOS */
    {BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,UFUNS,UFUND,BAD}, /* OP_ASIN */
    {BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,UFUNS,UFUND,BAD}, /* OP_ATAN */
    {BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,UFUNS,UFUND,BAD}, /* OP_COSH */
    {BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,UFUNS,UFUND,BAD}, /* OP_EXP */
    {BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,UFUNS,UFUND,BAD}, /* OP_LOG10 */
    {BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,UFUNS,UFUND,BAD}, /* OP_SINH */
    {BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,UFUNS,UFUND,BAD}, /* OP_TANH */
    {NO,NO,NO,NO,RTN4,NO,NO,NO,NO,NO,NO}, /* OP_PTR_TO_NATIVE */
    {NO,NO,NO,NO,RTN4,NO,NO,NO,NO,NO,NO}, /* OP_PTR_TO_FORIEGN */
    {NO,NO,NO,NO,NO,NO,NO,NO,NO,NO,NO}, /* OP_SLACK_19 */
    {CVT,CVT,CVT,CVT,CVT,CVT,CVT,BAD,CVT,CVT,CVT}, /* OP_CONVERT */
    {CVT,CVT,CVT,CVT,CVT,CVT,BAD,BAD,CVT,CVT,BAD}, /* OP_ROUND */
    {MOV1,MOV1,MOV2,MOV2,MOV4,MOV4,MOVCP,BAD,MOVFS,MOVFD,MOVX}, /* OP_MOV */
    {BAD,BAD,BAD,BAD,LA,LA,LACP,BAD,BAD,BAD,BAD}, /* old careful address operation */
    {BAD,BAD,BAD,BAD,LA,LA,LACP,BAD,BAD,BAD,BAD}, /* OP_LA */
    {CALLI,CALLI,CALLI,CALLI,CALLI,CALLI,CALLI,BAD,CALLI,CALLI,CALLI}, /* OP_CALL_INDIRECT */
    {PUSH1,PUSH1,PUSH2,PUSH2,PUSH4,PUSH4,PSHCP,BAD,PSHFS,PSHFD,PUSHX}, /* OP_PUSH */
    {BAD,BAD,POP2,POP2,POP4,POP4,BAD,BAD,BAD,BAD,BAD}, /* OP_POP */
    {DPARM,DPARM,DPARM,DPARM,DPARM,DPARM,DPARM,BAD,DPARM,DPARM,DPARM}, /* OP_PARM_DEF */
    {SJUMP,SJUMP,SJUMP,SJUMP,SJUMP,SJUMP,SJUMP,BAD,BAD,BAD,BAD}, /* OP_SELECT */
    {TEST1,TEST1,TEST2,TEST2,TEST4,TEST4,BAD,BAD,BAD,BAD,BAD}, /* OP_BIT_TEST_TRUE */
    {TEST1,TEST1,TEST2,TEST2,TEST4,TEST4,BAD,BAD,BAD,BAD,BAD}, /* OP_BIT_TEST_FALSE */
    {CMP1,CMP1,CMP2,CMP2,CMP4,CMP4,CMPCP,BAD,CMPF,CMPD,BAD}, /* OP_CMP_EQUAL */
    {CMP1,CMP1,CMP2,CMP2,CMP4,CMP4,CMPCP,BAD,CMPF,CMPD,BAD}, /* OP_CMP_NOT_EQUAL */
    {CMP1,CMP1,CMP2,CMP2,CMP4,CMP4,CMPCP,BAD,CMPF,CMPD,BAD}, /* OP_CMP_GREATER */
    {CMP1,CMP1,CMP2,CMP2,CMP4,CMP4,CMPCP,BAD,CMPF,CMPD,BAD}, /* OP_CMP_LESS_EQUAL */
    {CMP1,CMP1,CMP2,CMP2,CMP4,CMP4,CMPCP,BAD,CMPF,CMPD,BAD}, /* OP_CMP_LESS */
    {CMP1,CMP1,CMP2,CMP2,CMP4,CMP4,CMPCP,BAD,CMPF,CMPD,BAD}, /* OP_CMP_GREATER_EQUAL */
    {CALL,CALL,CALL,CALL,CALL,CALL,CALL,BAD,CALL,CALL,CALL}, /* OP_CALL */
    {NO,NO,NO,NO,NO,NO,NO,NO,NO,NO,NO}, /* old unused opcode0x37 */
    {NO,NO,NO,NO,NO,NO,NO,NO,NO,NO,NO}, /* old unused opcode0x38 */
    {NO,NO,NO,NO,NO,NO,NO,NO,NO,NO,NO}, /* old unused opcode0x39 */
    {NO,NO,NO,NO,NO,NO,NO,NO,NO,NO,NO}, /* old unused opcode0x3a */
    {NO,NO,NO,NO,NO,NO,NO,NO,NO,NO,NO}, /* old unused opcode0x3b */
    {NO,NO,NO,NO,NO,NO,NO,NO,NO,NO,NO}, /* old unused opcode0x3c */
    {NO,NO,NO,NO,NO,NO,NO,NO,NO,NO,NO}, /* old unused opcode0x3d */
    {NO,NO,NO,NO,NO,NO,NO,NO,NO,NO,NO}, /* old unused opcode0x3e */
    {NO,NO,NO,NO,NO,NO,NO,NO,NO,NO,NO}, /* old unused opcode0x3f */
    {NO,NO,NO,NO,NO,NO,NO,NO,NO,NO,NO}, /* old unused opcode0x40 */
    {NO,NO,NO,NO,NO,NO,NO,NO,NO,NO,NO}, /* old unused opcode0x41 */
    {NO,NO,NO,NO,NO,NO,NO,NO,NO,NO,NO}, /* old unused opcode0x42 */
    {NO,NO,NO,NO,NO,NO,NO,NO,NO,NO,NO}, /* old unused opcode0x43 */
    {NO,NO,NO,NO,NO,NO,NO,NO,NO,NO,NO}, /* old unused opcode0x44 */
    {NO,NO,NO,NO,NO,NO,NO,NO,NO,NO,NO}, /* old unused opcode0x45 */
    {NO,NO,NO,NO,NO,NO,NO,NO,NO,NO,NO}, /* old unused opcode0x46 */
    {NO,NO,NO,NO,NO,NO,NO,NO,NO,NO,NO}, /* old unused opcode0x47 */
    {NO,NO,NO,NO,NO,NO,NO,NO,NO,NO,NO}, /* old unused opcode0x48 */
    {NO,NO,NO,NO,NO,NO,NO,NO,NO,NO,NO}, /* old unused opcode0x49 */
    {NO,NO,NO,NO,NO,NO,NO,NO,NO,NO,NO}, /* old unused opcode0x4a */
    {NO,NO,NO,NO,NO,NO,NO,NO,NO,NO,NO}, /* OP_BLOCK */
};
