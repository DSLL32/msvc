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


/* Initial6b9cb443 bld/plusplus/c/mainproc.c; actual3f892..3f8a0. */
#include "symbol.h"
#include "pending.h"
#include "frontend.h"
#include <recon.h>
unsigned char SpecialFunction(SYMBOL sym);
int MainProcedure(SYMBOL sym) { return SpecialFunction(sym)==1; }

SCOPE SymScope(SYMBOL sym);
arg_list *SymFuncArgList(SYMBOL sym);
void LinkageSet(SYMBOL sym,char *language);
int LinkageIsC(SYMBOL sym);
void DefAddPrototype(SYMBOL sym) { (void)sym; STUB_TRAP(); }
void MainProcSetup(SYMBOL sym)
{
    if(SymScope(sym)!=FileScope) return;
    if(CompFlags.dump_prototypes) DefAddPrototype(sym);
    switch(SpecialFunction(sym)) {
    case 1:
        CompFlags.has_main=1;
        if(SymFuncArgList(sym)->num_args!=0) CompFlags.main_has_parms=1;
        LinkageSet(sym,"C");
        break;
    case 2:
        CompFlags.has_winmain=1;
        LinkageSet(sym,"C");
        break;
    case 3:
        CompFlags.has_libmain=1;
        LinkageSet(sym,"C");
        break;
    default:
        if(LinkageIsC(sym)) CompFlags.extern_C_defn_found=1;
        break;
    }
}
