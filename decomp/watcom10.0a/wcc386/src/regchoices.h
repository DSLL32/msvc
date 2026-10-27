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
* Description:  386 register-choice list identities and operand constraints.
*
****************************************************************************/


#ifndef WCC_REGCHOICES_H
#define WCC_REGCHOICES_H
#include "procdef.h"
#include <recon.h>

enum reg_set_index {
    RL_,
    RL_AL,
    RL_AH,
    RL_CL,
    RL_BYTE,
    RL_AX,
    RL_CX,
    RL_DX,
    RL_WORD,
    RL_TWOBYTE,
    RL_SEG,
    RL_ANYWORD,
    RL_EAX,
    RL_EDX,
    RL_DX_AX,
    RL_DOUBLE,
    RL_ABCD,
    RL_ECX,
    RL_DX_EAX,
    RL_LONG_INDEX,
    RL_FAR_POINTER,
    RL_EDX_EAX,
    RL_FPPARM2,
    RL_8,
    RL_ST0,
    RL_STI,
    RL_NUMBER_OF_SETS
};
#define RL_WS RL_ANYWORD

enum reg_index {
    RG_,
    RG_4CRTN,
    RG_8CRTN,
    RG_8087,
    RG_8087_NEED,
    RG_8087_8,
    RG_8087_ZAP_ACC,
    RG_8087_DBL,
    RG_8_8087,
    RG_8_NEED_DBL,
    RG_DBL_8087,
    RG_DBL_BYTE,
    RG_ST0_STI,
    RG_STI_ST0,
    RG_BYTE,
    RG_TWOBYTE,
    RG_WORD,
    RG_ANYWORD,
    RG_DBL,
    RG_8,
    RG_BYTE_ACC,
    RG_WORD_ACC,
    RG_DBL_ACC,
    RG_BYTE_DIV,
    RG_WORD_DIV,
    RG_DBL_DIV,
    RG_CBW,
    RG_CBD,
    RG_CBDE,
    RG_CWD,
    RG_CWDE,
    RG_CDQ,
    RG_BYTE_WORD,
    RG_BYTE_WORD_NEED_WORD,
    RG_BYTE_WORD_NEED_BYTE,
    RG_BYTE_DBL,
    RG_BYTE_DBL_NEED_DBL,
    RG_WORD_DBL,
    RG_WORD_DBL_NEED_DBL,
    RG_WORD_DBL_NEED_WORD,
    RG_BYTE_MOD,
    RG_WORD_MOD,
    RG_DBL_MOD,
    RG_BYTE_MUL,
    RG_WORD_MUL,
    RG_DBL_MUL,
    RG_BYTE_NEED,
    RG_WORD_NEED,
    RG_DBL_NEED,
    RG_BYTE_SHIFT,
    RG_WORD_SHIFT,
    RG_DBL_SHIFT,
    RG_BYTE_SHIFT_NEED,
    RG_WORD_SHIFT_NEED,
    RG_DBL_SHIFT_NEED,
    RG_ANYWORD_NEED,
    RG_LEA,
    RG_SEG_WORD,
    RG_WORD_SEG,
    RG_SEG_SEG,
    RG_FAR_POINTER,
    RG_WORD_OP,
    RG_BYTE_4BYTE,
    RG_BYTE_2BYTE,
    RG_NUMBER_OF_REGS
};
/* Actual 1994 records use five byte-sized list indexes. */
typedef struct op_regs { unsigned char left,right,result,zap,need; } op_regs;
typedef char old_op_regs_size[(sizeof(op_regs)==5)?1:-1];
extern op_regs RegList[];
extern hw_reg_set *RegSets[];
#endif
