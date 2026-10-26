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

/* Initial plusplus/c/utility.c at6b9cb443, matched against the actual
 *1994 vector-copy and compiler-owned allocation routines. */
#include <string.h>
#include "compiler.h"
#include "cmdln-options.h"
#include "utility.h"
#include "pending.h"
#include <recon.h>

char *stvcpy_after(char *target, const char **source, unsigned size)
{
    const char *p;
    for (p=*source; size>0; --size) *target++=*p++;
    *target++='\0';
    *source=p;
    return target;
}

char *stvcpy(char *target, const char *source, unsigned size)
{
    return stvcpy_after(target,&source,size)-1;
}

char *vctsave(const char *vector, unsigned size)
{
    char *result;
    result=(char *)CMemAlloc(size+1);
    stvcpy(result,vector,size);
    return result;
}

char *stpcpy_after(char *target, const char **source)
{
    const char *p;
    for (p=*source; (*target++=*p++)!=0; ) ;
    *source=p;
    return target;
}

char *stpcpy(char *target, const char *source)
{
    return stpcpy_after(target,&source)-1;
}

char *strsave(const char *name)
{
    return vctsave(name,strlen(name));
}

/* Initial utility.c and actual1994 CF5ab9b..5abb6 agree. */
int strpref(const char *prefix,const char *str)
{
    while (*prefix) {
        if (*prefix!=*str) return 0;
        ++prefix;
        ++str;
    }
    return 1;
}

char *CStrSave(const char *name)
{
    unsigned length=strlen(name)+1;
    char *copy=(char *)CPermAlloc(length);
    memcpy(copy,name,length);
    return copy;
}

double SafeAtof(char *text) { (void)text; STUB_TRAP(); return 0; }
