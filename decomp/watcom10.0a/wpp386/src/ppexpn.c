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


/*1994 ppexpn.c uses recursive precedence levels and32-bit values,
 * unlike the later source's operand/operator stacks and64-bit conversion.
 * Only bit0 of the second DWORD is meaningful: unsigned arithmetic. */
#include <string.h>
#include "scan.h"
#include "macro.h"
#include "pending.h"
#include <recon.h>

typedef struct ppvalue {
    unsigned long value;
    unsigned is_unsigned;
} ppvalue;
void CConditional(ppvalue *value);
void CLogicalOr(ppvalue *value);
void MustRecog(int token);
long PpConstExpr(void)
{
    ppvalue value;
    CConditional(&value);
    return (long)value.value;
}
void CConditional(ppvalue *value)
{
    ppvalue yes, no;
    CLogicalOr(value);
    if (CurToken==3) {
        NextToken();
        CConditional(&yes);
        MustRecog(4);
        CConditional(&no);
        value->value=value->value ? yes.value : no.value;
        value->is_unsigned=(yes.is_unsigned|no.is_unsigned)&1;
    }
}


void CLogicalAnd(ppvalue *value);
void COr(ppvalue *value);
void CXOr(ppvalue *value);
void CAnd(ppvalue *value);
void CEquality(ppvalue *value);
void CRelational(ppvalue *value);
void CShift(ppvalue *value);
void CAdditive(ppvalue *value);
void CMultiplicative(ppvalue *value);
void CUnary(ppvalue *value);
long FoldSignedRShiftMax(long value);
void CLogicalOr(ppvalue *value)
{
    ppvalue right;
    CLogicalAnd(value);
    while (CurToken==0x15) {
        NextToken(); CLogicalAnd(&right);
        value->value=value->value!=0 || right.value!=0;
        value->is_unsigned=0;
    }
}
void CLogicalAnd(ppvalue *value)
{
    ppvalue right;
    COr(value);
    while (CurToken==0x1a) {
        NextToken(); COr(&right);
        value->value=value->value!=0 && right.value!=0;
        value->is_unsigned=0;
    }
}
void COr(ppvalue *value)
{
    ppvalue right;
    CXOr(value);
    while (CurToken==0x13) {
        NextToken(); CXOr(&right);
        value->value |= right.value;
        value->is_unsigned=(value->is_unsigned|right.is_unsigned)&1;
    }
}
void CXOr(ppvalue *value)
{
    ppvalue right;
    CAnd(value);
    while (CurToken==0x16) {
        NextToken(); CAnd(&right);
        value->value ^= right.value;
        value->is_unsigned=(value->is_unsigned|right.is_unsigned)&1;
    }
}
void CAnd(ppvalue *value)
{
    ppvalue right;
    CEquality(value);
    while (CurToken==0x18) {
        NextToken(); CEquality(&right);
        value->value &= right.value;
        value->is_unsigned=(value->is_unsigned|right.is_unsigned)&1;
    }
}
void CEquality(ppvalue *value)
{
    ppvalue right;
    int op;
    CRelational(value);
    while (CurToken==0x10 || CurToken==0x12) {
        op=CurToken; NextToken(); CRelational(&right);
        value->value=op==0x10 ? value->value==right.value : value->value!=right.value;
        value->is_unsigned=0;
    }
}
void CRelational(ppvalue *value)
{
    ppvalue right;
    int op, unsign;
    unsigned result;
    CShift(value);
    for (;;) {
        op=CurToken;
        if (op!=0x1f && op!=0x20 && op!=0x1b && op!=0x1c) return;
        NextToken(); CShift(&right);
        unsign=(value->is_unsigned|right.is_unsigned)&1;
        switch (op) {
        case 0x1f: result=unsign ? value->value<right.value : (long)value->value<(long)right.value; break;
        case 0x20: result=unsign ? value->value<=right.value : (long)value->value<=(long)right.value; break;
        case 0x1b: result=unsign ? value->value>right.value : (long)value->value>(long)right.value; break;
        default: result=unsign ? value->value>=right.value : (long)value->value>=(long)right.value; break;
        }
        value->value=result;
        value->is_unsigned=0;
    }
}
void CShift(ppvalue *value)
{
    ppvalue right;
    int op;
    CAdditive(value);
    while (CurToken==0x1d || CurToken==0x21) {
        op=CurToken; NextToken(); CAdditive(&right);
        if (op==0x1d) {
            if (right.value>=32) value->value=value->is_unsigned ? 0 : FoldSignedRShiftMax((long)value->value);
            else if (value->is_unsigned) value->value >>= right.value;
            else value->value=(long)value->value >> right.value;
        } else {
            if (right.value>=32) value->value=0;
            else value->value <<= right.value;
        }
    }
}
void CAdditive(ppvalue *value)
{
    ppvalue right;
    int op;
    CMultiplicative(value);
    while (CurToken==0x23 || CurToken==0x26) {
        op=CurToken; NextToken(); CMultiplicative(&right);
        if (op==0x23) {
            value->value += right.value;
            value->is_unsigned=(value->is_unsigned|right.is_unsigned)&1;
        } else {
            /* Actual old subtraction clears unsignedness, even for an
             * unsigned operand; retain this1994 behavior. */
            value->is_unsigned=0;
            value->value -= right.value;
        }
    }
}
void CMultiplicative(ppvalue *value)
{
    ppvalue right;
    int op, unsign;
    CUnary(value);
    while (CurToken==0x29 || CurToken==0x2b || CurToken==0x2d) {
        op=CurToken; NextToken(); CUnary(&right);
        unsign=(value->is_unsigned|right.is_unsigned)&1;
        if (op==0x29) value->value *= right.value;
        else if (!right.value) value->value=0;
        else if (op==0x2b) {
            if (unsign) value->value /= right.value;
            else value->value=(long)value->value/(long)right.value;
        } else {
            if (unsign) value->value %= right.value;
            else value->value=(long)value->value%(long)right.value;
        }
        value->is_unsigned=unsign;
    }
}
void COperand(ppvalue *value);
double SafeAtof(char *text);
void CUnary(ppvalue *value)
{
    int token;
    value->value=0;
    value->is_unsigned=0;
    token=CurToken;
    switch (token) {
    case 0x23: NextToken(); CUnary(value); break;
    case 0x26:
        NextToken(); CUnary(value); value->value=0-value->value; break;
    case 0x11:
        NextToken(); CUnary(value); value->value=!value->value;
        value->is_unsigned=0; break;
    case 0x0e:
        NextToken(); CUnary(value); value->value=~value->value; break;
    case 7:
        NextToken(); CConditional(value); MustRecog(8); break;
    case 0x36: COperand(value); break;
    case 0x37:
        switch (ConstType) {
        case 3: case 8: case 10:
            value->value=Constant; value->is_unsigned=1; break;
        case 11: case 12:
            value->value=(long)SafeAtof(Buffer); break;
        default: value->value=Constant; break;
        }
        NextToken();
        break;
    }
}
MEPTR MacroSearch(void);
int isMacroDefined(void)
{
    MEPTR entry=MacroSearch();
    int result=0;
    if (entry) { result=1; entry->macro_flags |= 8; }
    PPState=1;
    NextToken();
    return result;
}
void COperand(ppvalue *value)
{
    if (!strcmp("defined",Buffer)) {
        PPState=2;
        NextToken();
        if (CurToken==7) {
            NextToken();
            value->value=isMacroDefined();
            MustRecog(8);
        } else value->value=isMacroDefined();
    } else {
        NextToken();
        value->value=0;
    }
    value->is_unsigned=0;
}
