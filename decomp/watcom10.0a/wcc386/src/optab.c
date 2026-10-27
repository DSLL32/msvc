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
* Description:  Choose generation tables for typed backend instructions.
*
****************************************************************************/


#include "block.h"
#include "tables.h"
#include "stubs.h"
#include <recon.h>

opcode_entry *OpcodeTable(unsigned char index);
opcode_entry *CodeTable(instruction *ins)
{
    unsigned idx;
    if (ins->head.opcode>=76 || ins->type_class>=11) STUB_TRAP();
    idx=OpTable[ins->head.opcode][ins->type_class];
    if (idx==BAD) Zoiks(52);
    return OpcodeTable(idx);
}

unsigned char DoesSomething(instruction *ins)
{
    if (ins->head.opcode==0x4b) return 1;
    if (ins->u.gen_table==NULL) return 1;
    return ins->u.gen_table->generate!=0;
}
