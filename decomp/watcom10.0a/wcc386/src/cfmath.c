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
* Description:  Decimal constant-float conversion.
*
****************************************************************************/


#include "codegen.h"
#include "memory.h"
#include "cfloat.h"
#include "compiler.h"
#include "stubs.h"
#include <recon.h>

void CFClean(struct cfloat *number)
{
    int head_index,tail_index;
    char *head;
    tail_index=number->len-1;
    while (number->mant[tail_index]=='0') {
        if (tail_index==0) {
            number->exp=1;
            number->sign=0;
            number->len=1;
            number->mant[1]='\0';
            return;
        }
        --tail_index;
    }
    head_index=0;
    head=number->mant;
    while (*head=='0') { ++head;++head_index; }
    if (head_index>0) {
        tail_index-=head_index;
        Copy(head,number->mant,tail_index+1);
        number->exp-=head_index;
    }
    /* The earlier body does not impose the later CF_MAX_PREC limit. */
    number->len=tail_index+1;
    number->mant[tail_index+1]='\0';
}

int CFOrder(struct cfloat *left,struct cfloat *right)
{
    int index,difference;
    if (left->exp>right->exp) return 1;
    if (left->exp<right->exp) return -1;
    for (index=0;index<left->len;++index) {
        if (index>=right->len) return 1;
        difference=CFAccess(left,index)-CFAccess(right,index);
        if (difference<0) return -1;
        if (difference>0) return 1;
    }
    if (index<right->len) return -1;
    return 0;
}

void CFNegate(struct cfloat *value)
{
    value->sign=-value->sign;
}

int CFTest(struct cfloat *value)
{
    return value->sign;
}

int CFCompare(struct cfloat *left,struct cfloat *right)
{
    if (left->sign<right->sign) return -1;
    if (left->sign>right->sign) return 1;
    return left->sign*CFOrder(left,right);
}

int CFAccess(struct cfloat *value,int index)
{
    return (unsigned char)value->mant[index]-'0';
}

struct cfloat *CFDiv(struct cfloat *left,struct cfloat *right) { STUB_TRAP();return NULL; }
struct cfloat *CFInverse(struct cfloat *value) { STUB_TRAP();return NULL; }
