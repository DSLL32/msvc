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


/* Adapted from initial6b9cb443 specfuns.c/specfuns.dfn.
 * Actual1994 CF4d6de..4d744: five named entries and FileScope identity,
 * before the later global-namespace test and extra special functions. */
#include <string.h>
#include "symbol.h"
#include <recon.h>
typedef struct spec_func { const char *name; unsigned length; } SPEC_FUNC;
static SPEC_FUNC funs[] = {
    { "",0 }, { "main",4 }, { "WinMain",7 }, { "LibMain",7 },
    { "longjmp",7 }, { "_setjmp",7 }
};
unsigned char SpecialFunction(SYMBOL func)
{
    SYMBOL_NAME sn;
    const char *sname;
    SPEC_FUNC *spp;
    unsigned sname_len;
    unsigned char retn;
    sn=func->name;
    retn=0;
    if (sn!=0 && sn->containing==FileScope) {
        sname=sn->name;
        if (sname!=0) {
            sname_len=strlen(sname);
            for (spp=funs+5; spp!=funs; --spp) {
                if (spp->name[0]==sname[0] && sname_len==spp->length
                    && memcmp(spp->name,sname,sname_len)==0) break;
            }
            retn=spp-funs;
        }
    }
    return retn;
}
