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
#include "stubs.h"
#include <recon.h>

/* 386table.c DoNop: any operands/result, no equality constraint,
 * preserve condition codes; V_NO, G_NO, RG_, FU_NO. The old pattern
 * has four-bit operand fields and a 16-bit op_type. */
#define PAT_ANY 0xfU
#define PAT_NEITHER_EQUAL 0x3000U
#define PAT_PRESERVE_CC 0x8000U
static opcode_entry DoNop[]={
    { PAT_ANY|(PAT_ANY<<4)|(PAT_ANY<<8)|PAT_NEITHER_EQUAL|PAT_PRESERVE_CC,
      0,0,0,0 }
};
struct opcode_entry *OpcodeTable(unsigned char index)
{
    /* Both original FPU/non-FPU lists use this same DoNop object.
     * Other table data has not yet been reconstructed. */
    if (index!=69) STUB_TRAP(); /* DONOTHING */
    return DoNop;
}
