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
* Description:  Code-generator tree initialization and arithmetic type rules.
*
****************************************************************************/


#include "codegen.h"
#include "stubs.h"
#include "procdef.h"
#include "cgtree.h"
#include "cfloat.h"
#include <recon.h>

/* treeconv.c's constant conversions before widened integer constants. */
struct tree_node *FoldCnvRnd(unsigned char op,struct tree_node *value,type_def *type)
{
    tree_node *node;
    struct cfloat *number,*old;
    if (value->class==18) { /* TN_CONS */
        if (value->tipe->refno==T_DEFAULT) number=CFCopy(value->u.name->c.value);
        else number=CnvCFToType(value->u.name->c.value,value->tipe);
        if (!(type->attr&1)) { /* Non-floating destination. */
            old=number;
            if (op==36) number=CFTrunc(number);
            else if (op==37) number=CFRound(number);
            else {
                /* The original leaves its result undefined for other ops. */
                STUB_TRAP();
            }
            CFFree(old);
            if (type->refno!=T_DEFAULT) {
                old=number;
                number=CnvCFToType(number,type);
                CFFree(old);
            }
        }
        node=TGConst(number,type);
        BurnTree(value);
    } else {
        TGDemote(value,type);
        node=NULL;
    }
    return node;
}

/* This older entry falls through to the separately called IntToCF entry;
 * it still uses a 32-bit integer rather than the released 64-bit helper. */
struct cfloat *CnvCFToType(struct cfloat *value,type_def *type)
{
    if (type->attr&1) return CFCopy(value);
    return IntToCF(CFGetIntegerValue(value),type);
}

/* The released successor is CFGetInteger64Value; this body returns 32 bits. */
long CFGetIntegerValue(struct cfloat *value)
{
    struct cfloat *truncated;
    unsigned long integer;
    int sign;
    truncated=CFTrunc(value);
    sign=CFTest(truncated);
    if (sign<0) CFNegate(truncated);
    integer=(unsigned long)CFCnvF32(truncated);
    if (sign<0) integer=0UL-integer;
    CFFree(truncated);
    return (long)integer;
}

struct cfloat *IntToCF(long value,type_def *type)
{
    if (type->attr&2) {
        switch (type->length) {
        case 1:return CFCnvI32F((signed char)value);
        case 2:return CFCnvI32F((short)value);
        case 4:case 6:return CFCnvI32F(value);
        }
    } else {
        switch (type->length) {
        case 1:return CFCnvU32F((unsigned char)value);
        case 2:return CFCnvU32F((unsigned short)value);
        case 4:case 6:return CFCnvU32F((unsigned long)value);
        }
    }
    Zoiks(78); /* Old conversion diagnostic, before 64-bit support. */
    return NULL;
}
