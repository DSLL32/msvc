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


/* Initial6b9cb443 cgsegid.c, actual1994 function selector2937f..293d2. */
#include "compiler.h"
#include "symbol.h"
#include "types.h"
#include "cmdline.h"
#include "pending.h"
#include "frontend.h"
#include <recon.h>
int SymIsInitialized(SYMBOL sym);
unsigned short SegmentForDefinedFunc(SYMBOL sym);
unsigned short SegmentImport(void);
unsigned short SegmentDefaultCode(void);
TYPE TypeGetActualFlags(TYPE type,unsigned short *flags);
unsigned short CgSegIdFunction(SYMBOL sym)
{
    unsigned short id,flags;
    id=sym->segid;
    if (id==0) {
        if (SymIsInitialized(sym)) id=SegmentForDefinedFunc(sym);
        else {
            TypeGetActualFlags(sym->sym_type,&flags);
            if (flags&8) id=SegmentImport();
            else if (IsBigCode()) id=SegmentImport();
            else id=SegmentDefaultCode();
        }
        sym->segid=id;
    }
    return id;
}

int SymIsAutomatic(SYMBOL sym);
int SymIsFunction(SYMBOL sym);
int SymIsStaticDataMember(SYMBOL sym);
void SegmentMarkUsed(unsigned short id);
unsigned short cgSegIdBased(SYMBOL sym,unsigned short flags)
{ (void)sym; (void)flags; STUB_TRAP(); return 0; }
unsigned CgMemorySize(TYPE type);
int SymRequiresCtoring(SYMBOL sym);
int SymRequiresDtoring(SYMBOL sym);
int SymIsExtern(SYMBOL sym);
unsigned SegmentAlignment(SYMBOL sym);
unsigned short SegmentAddSym(SYMBOL sym,unsigned short id,unsigned size,unsigned align);
unsigned cgSegIdAlign(SYMBOL sym,unsigned short flags)
{
    return (flags&32)?16:SegmentAlignment(sym);
}
int cgSegIdConst(SYMBOL sym,unsigned short flags,unsigned char control)
{
    if(!(flags&1) || (control&2)) return 0;
    if(SymRequiresCtoring(sym)) return 0;
    if(SymRequiresDtoring(sym)) return 0;
    return 1;
}
unsigned short cgSegIdNearVariable(SYMBOL sym,unsigned short flags,unsigned size,unsigned char control)
{
    unsigned align=cgSegIdAlign(sym,flags);
    unsigned short id;
    if(SymIsExtern(sym)) id=4;
    else if(control&1) id=5;
    else if(cgSegIdConst(sym,flags,control)) id=3;
    else id=4;
    return SegmentAddSym(sym,id,size,align);
}
unsigned short cgSegIdFarVariable(SYMBOL sym,unsigned short flags,unsigned size,unsigned char control)
{ (void)sym; (void)flags; (void)size; (void)control; STUB_TRAP(); return 0; }
unsigned short cgSegIdHugeVariable(SYMBOL sym,unsigned short flags,unsigned size,unsigned char control)
{ (void)sym; (void)flags; (void)size; (void)control; STUB_TRAP(); return 0; }
unsigned short cgSegIdVariable(SYMBOL sym,unsigned short flags,unsigned char control)
{
    unsigned size=CgMemorySize(sym->sym_type);
    int assume_near;
    if(flags&4) return cgSegIdNearVariable(sym,flags,size,control);
    if(flags&32) return cgSegIdHugeVariable(sym,flags,size,control);
    if(flags&8) return cgSegIdFarVariable(sym,flags,size,control);
    assume_near=1;
    if(IsBigData()) {
        if(size==0 || size>DataThreshold) assume_near=0;
        else if(CompFlags.zc_switch_used && cgSegIdConst(sym,flags,control))
            assume_near=0;
    }
    if(assume_near) return cgSegIdNearVariable(sym,flags,size,control);
    return cgSegIdFarVariable(sym,flags,size,control);
}
unsigned short cgSegIdAuto(SYMBOL sym)
{
    unsigned short id=sym->segid;
    if(id==0) sym->segid=id=12;
    return id;
}
unsigned short CgSegIdData(SYMBOL sym,unsigned char control)
{
    unsigned short id,flags;
    if(SymIsAutomatic(sym)) return cgSegIdAuto(sym);
    if(SymIsFunction(sym)) return CgSegIdFunction(sym);
    id=sym->segid;
    if(id==0) {
        TypeGetActualFlags(sym->sym_type,&flags);
        if(flags&0x700) id=cgSegIdBased(sym,flags);
        else if(SymIsStaticDataMember(sym)) id=cgSegIdVariable(sym,flags,control);
        else id=cgSegIdVariable(sym,flags,control);
        sym->segid=id;
    }
    SegmentMarkUsed(id);
    return id;
}
unsigned short CgSegId(SYMBOL sym) { return CgSegIdData(sym,0); }
