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
* Description:  Code-generator string utilities.
*
****************************************************************************/


#include <string.h>
#include "memory.h"
#include <recon.h>
#if defined(__WATCOMC__)
/* CypLength/CypCopy were compiler intrinsics in the original source. */
#pragma intrinsic(strlen,memcpy,memcmp)
#endif

char *Copy(char *source,char *destination,unsigned length)
{
    while (length--) *destination++=*source++;
    return destination;
}

/* utils.c CypEqual: the 1994 intrinsic returns all bits set for equality.
 * Selective intrinsic preserves its no-helper graph in the period build. */
unsigned char Equal(unsigned char *source,unsigned char *destination,int length)
{
    return memcmp(source,destination,length)==0?0xff:0;
}

char *CopyStr(char *source,char *destination)
{
    unsigned length;
    length=strlen(source);
    memcpy(destination,source,length+1);
    return destination+length;
}

unsigned Length(char *text)
{
    return strlen(text);
}

