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
#include "cfloat.h"
#include "memory.h"
#include <recon.h>

struct cfloat *CFCnvSF(char *begin,char *end)
{
    struct cfloat *number;
    char saved=*end;
    if (saved!='\0') *end='\0';
    number=CFAlloc(end-begin);
    DoConvert(number,begin);
    if (saved!='\0') *end=saved;
    return number;
}

void DoConvert(struct cfloat *number,char *text)
{
    char *digits;
    int length,sign,exponent;
    while (*text==' ') ++text;
    sign=1;
    if (*text=='-') { sign=-1;++text; }
    else if (*text=='+') ++text;
    exponent=length=0;
    digits=number->mant;
    if (*text>='0' && *text<='9') {
        do {
            *digits++=*text++;
            ++length;++exponent;
        } while (*text>='0' && *text<='9');
        if (*text=='.') {
            ++text;
            while (*text>='0' && *text<='9') {
                *digits++=*text++;
                ++length;
            }
        }
    } else {
        if (*text!='.') return;
        ++text;
        if (*text<'0' || *text>'9') return;
        do {
            *digits++=*text++;
            ++length;
        } while (*text>='0' && *text<='9');
    }
    if (*text=='E' || *text=='e') {
        if (*++text=='-') exponent-=CFGetDec16(++text);
        else {
            if (*text=='+') ++text;
            exponent+=CFGetDec16(text);
        }
    }
    number->sign=(signed char)sign;
    number->len=length;
    number->exp=exponent;
    CFClean(number);
}

short CFGetDec16(char *text)
{
    unsigned short number=0;
    while (*text>='0' && *text<='9') number=number*10+*text++-'0';
    return (short)number;
}

/* The two public conversions share their CALL/restore/RET in the original. */
struct cfloat *CFCnvI32F(long value)
{
    return CFCnvLongToF(value,1);
}

struct cfloat *CFCnvU32F(unsigned long value)
{
    return CFCnvLongToF((long)value,0);
}

/* Both actual callers use the same minimum-I32 decimal object (79e28). */
static struct { int exp,len,alloc;signed char sign;char mant[11]; }
    MaxNegI32={10,10,0,-1,"2147483648"};

/* cfconv.c CFCnvLongToF, before 64-bit conversion support. */
struct cfloat *CFCnvLongToF(long value,unsigned char is_signed)
{
    struct cfloat *number;
    char mant[11],*digit;
    unsigned long dividend;
    signed char sign;
    int len;
    if (value==0) return CFAlloc(0);
    if (is_signed && (unsigned long)value==0x80000000UL) {
        number=CFCopy((struct cfloat *)&MaxNegI32);
        number->sign=-1;
        return number;
    }
    sign=1;
    dividend=(unsigned long)value;
    if (value<0 && is_signed) {
        dividend=0UL-dividend;
        sign=-1;
    }
    digit=mant+10;
    *digit='\0';
    len=0;
    while (dividend!=0) {
        *--digit=(char)(U32ModDiv(&dividend,10)+'0');
        ++len;
    }
    number=CFAlloc(len>5?10:5);
    Copy(digit,number->mant,len+1);
    number->exp=len;
    number->len=len;
    number->sign=sign;
    CFClean(number);
    return number;
}

/* cfloat/u32moddi.c: return remainder and replace the dividend by quotient. */
unsigned long U32ModDiv(unsigned long *value,unsigned long divisor)
{
    unsigned long remainder;
    remainder=*value%divisor;
    *value/=divisor;
    return remainder;
}

long CFCnvF32(struct cfloat *value)
{
    unsigned long data;
    int exponent;
    data=0;
    if (CFIsU32(value) || CFIsI32(value)) {
        data=CFGetDec32(value->mant);
        exponent=value->exp-value->len;
        while (exponent>0) { data*=10;--exponent; }
        while (exponent<0) { data=(unsigned long)((long)data/10);++exponent; }
        if (value->sign==-1) data=0UL-data;
    }
    return (long)data;
}

unsigned char CFIsU32(struct cfloat *value)
{
    static struct { int exp,len,alloc;signed char sign;char mant[11]; }
        MaxU32={10,10,0,1,"4294967295"};
    return CFIsType(value,(struct cfloat *)&MaxU32);
}

unsigned char CFIsType(struct cfloat *value,struct cfloat *limit)
{
    int order;
    if (value->exp<=0 || value->exp>limit->exp) return 0;
    if (value->sign==-1 && limit->sign==1) return 0;
    if (value->exp<value->len) return 0;
    order=CFOrder(value,limit);
    if (order==1) return 0;
    if (order==0 && value->sign==1 && limit->sign==-1) return 0;
    return 1;
}

long CFGetDec32(char *text)
{
    unsigned long number;
    number=0;
    while (*text>='0' && *text<='9') {
        number=number*10+(unsigned char)*text-'0';
        ++text;
    }
    return (long)number;
}

unsigned char CFIsI16(struct cfloat *value)
{
    static struct { int exp,len,alloc;signed char sign;char mant[6]; }
        MaxNegI16={5,5,0,-1,"32768"};
    return CFIsType(value,(struct cfloat *)&MaxNegI16);
}

short CFCnvF16(struct cfloat *value)
{
    unsigned short data;
    short exponent;
    data=0;
    if (CFIsI16(value) || CFIsU16(value)) {
        data=(unsigned short)CFGetDec16(value->mant);
        exponent=(short)(value->exp-value->len);
        while (exponent>0) { data=(unsigned short)(data*10U);--exponent; }
        while (exponent<0) { data=(unsigned short)((short)data/10);++exponent; }
        if (value->sign==-1) data=(unsigned short)(0U-data);
    }
    return (short)data;
}

struct cfloat *CFTrunc(struct cfloat *value)
{
    struct cfloat *number;
    int length;
    length=value->exp;
    if (length<=0) return CFAlloc(1);
    number=CFCopy(value);
    if (length<number->len) {
        number->len=length;
        number->mant[length]='\0';
    }
    return number;
}

struct cfloat *CFCopy(struct cfloat *value)
{
    struct cfloat *number;
    number=CFAlloc(value->len);
    /* This older body copies sizeof(cfloat)+len+1, three bytes more than
     * the released offsetof(mant)+len+1 form. The allocator owns the tail. */
    Copy((char *)value,(char *)number,sizeof(*number)+value->len+1);
    return number;
}

unsigned char CFIsI32(struct cfloat *value)
{
    return CFIsType(value,(struct cfloat *)&MaxNegI32);
}

unsigned char CFIsU16(struct cfloat *value)
{
    static struct { int exp,len,alloc;signed char sign;char mant[6]; }
        MaxU16={5,5,0,1,"65535"};
    return CFIsType(value,(struct cfloat *)&MaxU16);
}

unsigned char CFIsU8(struct cfloat *value);
unsigned char CFIsI8(struct cfloat *value);
/* Earlier CFIsSize accepts either signed or unsigned representation, before
 * the later independent CFUnSignedSize routine. The scalar sizes end at4. */
unsigned char CFIsSize(struct cfloat *value,unsigned size)
{
    switch (size) {
    case 1: return CFIsU8(value) || CFIsI8(value);
    case 2: return CFIsU16(value) || CFIsI16(value);
    case 4: return CFIsU32(value) || CFIsI32(value);
    }
    return 0;
}

unsigned char CFIs32(struct cfloat *value)
{
    return CFIsI32(value) || CFIsU32(value);
}

unsigned char CFIsU8(struct cfloat *value)
{
    static struct { int exp,len,alloc;signed char sign;char mant[4]; }
        MaxU8={3,3,0,1,"255"};
    return CFIsType(value,(struct cfloat *)&MaxU8);
}
