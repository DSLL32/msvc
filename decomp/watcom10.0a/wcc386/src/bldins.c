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
* Description:  Backend block/control-flow construction.
*
****************************************************************************/


#include "address.h"
#include <recon.h>
static unsigned UnrollValue;

void BGGenCtrl(unsigned char op,bool_node *expr,struct code_lbl *label,unsigned char gen)
{
    switch (op) {
    case 86: /* O_LABEL */
        if (HaveCurrBlock) {
            GenBlock(2,1); /* JUMP */
            AddTarget(label,0);
            if (gen) Generate(0);
        }
        EnLink(label,0);
        HaveCurrBlock=1;
        CurrBlock->unroll_count=UnrollValue;
        break;
    case 81: /* O_GOTO */
        if (HaveCurrBlock) {
            GenBlock(2,1);
            AddTarget(label,0);
            if (gen) Generate(0);
        }
        HaveCurrBlock=0;
        break;
    case 85: /* O_INVOKE_LABEL */
        if (HaveCurrBlock) {
            GenBlock(0x40,1); /* CALL_LABEL */
            AddTarget(label,0);
            EnLink(AskForNewLabel(),1);
        }
        break;
    case 88: /* O_LABEL_RETURN */
        if (HaveCurrBlock) {
            AddIns(MakeNop());
            GenBlock(0x80,0);
            if (gen) Generate(0);
        }
        HaveCurrBlock=0;
        break;
    case 83: /* O_IF_TRUE */
        *expr->f=CurrBlock->label;
        *expr->t=label;
        BoolFree(expr);
        if (gen) Generate(0);
        break;
    case 84: /* O_IF_FALSE */
        *expr->t=CurrBlock->label;
        *expr->f=label;
        BoolFree(expr);
        if (gen) Generate(0);
        break;
    }
}

struct address_name *BGName(unsigned char kind,void *symbol,type_def *type)
{
    return MakeAddrName(kind,symbol,type);
}

/* bldins.c decimal constants retain the signed/unsigned conversion split. */
struct address_name *BGInteger(long value,type_def *type)
{
    struct cfloat *constant;
    if (type->attr&2) constant=CFCnvI32F(value);
    else constant=CFCnvU32F((unsigned long)value);
    return MakeConst(constant,type);
}

struct code_lbl *BGGetEntry(void)
{
    return CurrBlock->label;
}
