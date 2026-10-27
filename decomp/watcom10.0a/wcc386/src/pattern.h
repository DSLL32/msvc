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


#ifndef WCC_PATTERN_H
#define WCC_PATTERN_H
#define C               0x01u /*  constant */
#define R               0x02u /*  register */
#define M               0x04u /*  memory */
#define U               0x08u /*  unknown whether reg or memory */
#define NEED_INDEX      0x10u
#define BASE_TEMP       0x20u

#define RESULT_MUL      1u
#define C_R     C
#define R_R     R
#define M_R     M
#define U_R     U
#define OP__MUL NEED_INDEX
#define OP1_MUL OP__MUL
#define C_1     (C * OP1_MUL)
#define R_1     (R * OP1_MUL)
#define M_1     (M * OP1_MUL)
#define U_1     (U * OP1_MUL)
#define OP2_MUL (OP1_MUL * OP__MUL)
#define C_2     (C * OP2_MUL)
#define R_2     (R * OP2_MUL)
#define M_2     (M * OP2_MUL)
#define U_2     (U * OP2_MUL)

#define NE_1    0x01u
#define NE_2    0x02u
#define CC_1    0x04u
#define CC_2    0x08u

#define OTHER_MUL       (OP2_MUL * OP__MUL)
#define NE_R1           (NE_1 * OTHER_MUL)
#define NE_R2           (NE_2 * OTHER_MUL)
#define ANY             (C | R | M | U)
#define NONE            (NE_R1 | NE_R2)
#define EQ_R1           (NONE - NE_R1)
#define EQ_R2           (NONE - NE_R2)
#define BOTH_EQ         0
#define NO_CC           0
#define SETS_SC         (CC_1 * OTHER_MUL)
#define PRESERVE        (CC_2 * OTHER_MUL)
#define SETS_CC         ((CC_1 + CC_2) * OTHER_MUL)
#define MASK_CC         (SETS_CC)

#define _Side(op1,op2) ((op1)*OP1_MUL|(op2)*OP2_MUL|ANY|NONE)
#define _SidCC(op1,op2) (_Side(op1,op2)|SETS_CC)
#define _Un(op,res,match) ((op)*OP1_MUL|(res)*RESULT_MUL|(match))
#define _UnPP(op,res,match) (_Un(op,res,match)|PRESERVE)
#define _Bin(op1,op2,res,match) ((op1)*OP1_MUL|(op2)*OP2_MUL|(res)*RESULT_MUL|(match))
#define _BinSC(op1,op2,res,match) (_Bin(op1,op2,res,match)|SETS_SC)
#define _BinPP(op1,op2,res,match) (_Bin(op1,op2,res,match)|PRESERVE)
#define NVI(verify) ((verify)|0x80)

/* Actual 1994 operand patterns occupy a WORD. */
typedef unsigned short operand_types;
#include <recon.h>
#endif
