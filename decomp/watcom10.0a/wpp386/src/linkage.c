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


/* Adapted from initial6b9cb443 bld/plusplus/c/linkage.c.
 * Actual1994 CF3f5f4..3f6de and3f748..3f7a0. The old initializer
 * relies on static NULL linkage heads and does not explicitly reset them.
 * Both trailing-byte records are packed to their actual nine-byte size. */
#include <string.h>
#include "compiler.h"
#include "carve.h"
#include "cpragma.h"
#include "symbol.h"
#include "linkage.h"
#include "pending.h"
#include <recon.h>
typedef struct linkage_stack *LINKAGE_STACK;
#pragma pack(1)
struct linkage_name { LINKAGE next; void *pragma; char name[1]; };
struct linkage_stack {
    LINKAGE_STACK prev;
    LINKAGE linkage;
    unsigned char block:1;
};
#pragma pack()
typedef char LinkageNameSize[(sizeof(struct linkage_name)==9)?1:-1];
typedef char LinkageStackSize[(sizeof(struct linkage_stack)==9)?1:-1];
static LINKAGE validLinkages;
static LINKAGE_STACK nestedLinkages, resetLinkages;
static carve_t carveSTACK;
LINKAGE CurrLinkage, CppLinkage, CLinkage;

LINKAGE findLinkage(char *name)
{
    LINKAGE p;
    for (p=validLinkages; p!=0; p=p->next) {
        if (strcmp(name,p->name)==0) return p;
    }
    return 0;
}
LINKAGE LinkageAdd(char *name, void *defn)
{
    LINKAGE p;
    unsigned len;
    p=findLinkage(name);
    if (p!=0) {
        p->pragma=defn;
        return p;
    }
    len=strlen(name);
    p=CPermAlloc(sizeof(*p)+len);
    memcpy(p->name,name,len+1);
    p->pragma=defn;
    p->next=validLinkages;
    validLinkages=p;
    return p;
}
LINKAGE checkLinkage(char *name)
{
    LINKAGE p;
    p=findLinkage(name);
    if (p!=0) return p;
    CErr2p(116,name);
    return CppLinkage;
}
void LinkagePush(char *name)
{
    LINKAGE next_linkage;
    LINKAGE_STACK top;
    next_linkage=checkLinkage(name);
    CurrLinkage=next_linkage;
    top=CarveAlloc(carveSTACK);
    top->prev=nestedLinkages;
    top->linkage=next_linkage;
    top->block=0;
    nestedLinkages=top;
}
void LinkageBlock(void)
{
    if (nestedLinkages!=0) nestedLinkages->block=1;
}
void linkageInit(INITFINI *defn)
{
    (void)defn;
    carveSTACK=CarveCreate(sizeof(struct linkage_stack),4);
    CppLinkage=LinkageAdd("C++",&DefaultInfo);
    CLinkage=LinkageAdd("C",&DefaultInfo);
    LinkagePush("C++");
    resetLinkages=nestedLinkages;
    LinkageBlock();
}

/* Actual1994 CF3f7ba..3f81d: static symbols use C++ linkage, with a
 * function type adjusted when necessary. The later SF_TEMPLATE_FN test
 * does not occur in this old body. Class ids7/13/2 are actual old values. */
void LinkageSet(SYMBOL sym,char *language)
{
    LINKAGE linkage;
    TYPE fn_type;
    switch (sym->id) {
    case 7: return;
    case 13: linkage=CppLinkage; break;
    case 2:
        fn_type=FunctionDeclarationType(sym->sym_type);
        if (fn_type!=0 && !(fn_type->flag&4))
            sym->sym_type=MakePlusPlusFunction(sym->sym_type);
        linkage=CppLinkage;
        break;
    default:
        linkage=CurrLinkage;
        if (language!=0) linkage=findLinkage(language);
    }
    if (linkage==CppLinkage) sym->flag|=0x80000000UL;
    else sym->flag&=~0x80000000UL;
}

int LinkageIsC(SYMBOL sym) { return (sym->flag&0x80000000UL)==0; }

void LinkagePop(void)
{ STUB_TRAP(); }

int LinkageIsCurr(SYMBOL sym)
{
    if (CurrLinkage==CppLinkage) return (sym->flag&0x80000000UL)!=0;
    return (sym->flag&0x80000000UL)==0;
}
void LinkagePushC(void) { STUB_TRAP(); }
void LinkagePushCpp(void) { STUB_TRAP(); }

LINKAGE LinkageMergePop(void) { STUB_TRAP(); return NULL; }
