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


#include <math.h>
#include "codegen.h"
#include "cfloat.h"
#include <recon.h>

/* Earlier IEEE-host conversion uses the Watcom decimal runtime directly. */
double CFToF(struct cfloat *number)
{
    char buffer[20],*dest,*source;
    int count,i,exponent;
    double result;
    if (number->sign==0) return 0.0;
    count=number->len;
    if (count>17) count=17;
    dest=buffer;
    source=number->mant;
    for (i=0;i<count;++i) *dest++=*source++;
    *dest='\0';
    __ZBuf2F(buffer,&result);
    exponent=number->exp-count;
    if (exponent>308) {
        result=HUGE_VAL;
    } else {
        if (exponent< -325) return 0.0;
        if (exponent!=0) result=_Scale10V(result,exponent);
    }
    if (number->sign<0) result=-result;
    return result;
}
