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
* Description: Original-source attribution retained for the adapted entry.
*
****************************************************************************/

#ifndef ASMOPNDS_H
#define ASMOPNDS_H
#include <recon.h>
#define OP_NONE     0
#define OP_CL       0x00000001
#define OP_AL       0x00000002
#define OP_R8_GEN   0x00000004
#define OP_R8       ( OP_CL | OP_AL | OP_R8_GEN )
#define OP_AX       0x00000008
#define OP_DX       0x00000010
#define OP_R16_GEN  0x00000020
#define OP_R16      ( OP_AX | OP_DX | OP_R16_GEN )
#define OP_EAX      0x00000040
#define OP_R32_GEN  0x00000080
#define OP_R32      ( OP_EAX | OP_R32_GEN )
#define OP_R        ( OP_R8 | OP_R16 | OP_R32 )
#define OP_R1632    ( OP_R16 | OP_R32 )
#define OP_A        ( OP_AL | OP_AX | OP_EAX )
#define OP_MMX      ( OP_EAX | OP_DX | OP_CL )  //Kludge because of no bits
#define OP_I8       0x00000100
#define OP_I_1      0x00000200
#define OP_I_3      0x00000400
#define OP_I8_U     0x00000800
#define OP_I16      0x00001000
#define OP_I32      0x00002000
#define OP_J32      0x00004000
#define OP_J48      0x00008000
#define OP_I        ( OP_I8 | OP_I_1 | OP_I_3 | OP_I8_U | OP_I16 | OP_I32 )
#define OP_GE_8     ( OP_I8 | OP_I8_U | OP_I16 | OP_I32 )
#define OP_GE_16    ( OP_I16 | OP_I32 )
#define OP_GE_U8    ( OP_I8_U | OP_I16 | OP_I32 )
#define OP_M_B      0x00010000
#define OP_M_W      0x00020000
#define OP_M_DW     0x00040000
#define OP_M_QW     0x00080000
#define OP_M_TB     0x00100000
#define OP_M8       0x00200000
#define OP_M16      0x00400000
#define OP_M32      0x00800000
#define OP_M16_IND  ( OP_M8 | OP_M16 )          // indirect jmp - word ptr
#define OP_M32_IND  ( OP_M16 | OP_M32 )         // indirect jmp - dword ptr
#define OP_M48_IND  ( OP_M32 | OP_M8 )          // indirect jmp - fword ptr
#define OP_M        ( OP_M8 | OP_M16 | OP_M32 )
#define OP_M_ANY    ( OP_M_B | OP_M_W | OP_M_DW | OP_M_QW | OP_M_TB | OP_M )
#define OP_M8_R8    ( OP_M8 | OP_R8 )
#define OP_M16_R16    ( OP_M16 | OP_R16 )
#define OP_M32_R32    ( OP_M32 | OP_R32 )
#define OP_CR       0x01000000
#define OP_DR       0x02000000
#define OP_TR       0x04000000
#define OP_SPEC_REG ( OP_CR | OP_DR | OP_TR )
#define OP_SR2      0x08000000
#define OP_SR3      0x10000000
#define OP_SR       ( OP_SR2 | OP_SR3 )
#define OP_ST       0x20000000
#define OP_ST_REG   0x40000000
#define OP_STI    ( OP_ST | OP_ST_REG )
#define OP_SPECIAL 0x80000000UL
#define OP_LABEL            0x1
#define OP_REGISTER         0x2
#define OP_RES_ID           0x4
#define OP_DIRECTIVE        0x8
#define OP_DIRECT_EXPR      0x10
#define OP_ARITHOP          0x20
#define OP_PTR_MODIFIER     0x40
#define OP_UNARY_OPERATOR   0x80
#endif
