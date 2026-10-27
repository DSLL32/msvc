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


/* Actual1994 out-of-line public getter precedes ExceptionsCheck. The
 * released source later reads CompFlags.excs_enabled directly. */
#include "frontend.h"
#include "types.h"
#include "ptree.h"
#include "pending.h"
#include "carve.h"
#include "typesig.h"
#include "ctexcept.h"
#include <recon.h>
int ExceptionsEnabled(void) { return CompFlags.excs_enabled; }
void ExceptionsCheck(void)
{
    if(!ExceptionsEnabled()) CErr1(625);
}

TYPE PointerTypeEquivalent(TYPE type);
TYPE TypedefModifierRemoveOnly(TYPE type);
TYPE TypedefModifierRemove(TYPE type);
TYPE ReferenceType(TYPE type);
TYPE StructType(TYPE type);
TYPE MakePointerTo(TYPE type);
TYPE MakeReferenceTo(TYPE type);
TYPE canonicalPtrType(TYPE type)
{
    TYPE test=PointerTypeEquivalent(type);
    if(!test) return TypedefModifierRemoveOnly(type);
    if(FunctionDeclarationType(test)) return MakePointerTo(TypedefModifierRemoveOnly(test));
    return MakePointerTo(TypedefModifierRemoveOnly(test->of));
}
TYPE TypeCanonicalThr(TYPE type)
{
    TYPE test;
    if(type) {
        test=ReferenceType(type);
        if(!test) type=canonicalPtrType(type);
        else type=MakeReferenceTo(canonicalPtrType(test));
    }
    return type;
}
/* The older categories end at PTR_SCALAR7.  In particular the actual
 * void-pointer test below examines test itself, unlike the later test->of. */
unsigned char ThrowCategory(TYPE type)
{
    TYPE cltype,test;
    if(!type) return 5;
    if(ReferenceType(type)) return 2;
    cltype=StructType(type);
    if(cltype) return cltype->u.c.info->last_vbase?4:0;
    test=PointerTypeEquivalent(type);
    if(!test) return 3;
    if(StructType(test->of)) return 1;
    return TypedefModifierRemove(test)->id==TYP_VOID?6:7;
}

void makeThrowCnvAccess(THROW_CNV_CTL *ctl,TYPE type,unsigned offset,
                       unsigned char access)
{
    int error=0;
    THROW_CNV *cnv=CarveAlloc(ctl->carver);
    cnv->offset=offset;
    cnv->sig=TypeSigFind(access,type,&ctl->err_locn,&error);
    if(error) ctl->flags|=1;
    RingAppend((void **)&ctl->hdr,cnv);
}
void makeThrowCnv(THROW_CNV_CTL *ctl,TYPE type,unsigned offset)
{
    /* The1994 frontend always asks for destructor/copy access here. */
    makeThrowCnvAccess(ctl,type,offset,20);
}
void makeThrowCnvNoAcc(THROW_CNV_CTL *ctl,TYPE type,unsigned offset)
{
    makeThrowCnvAccess(ctl,type,offset,0);
}
void throwClassCnvs(TYPE type,THROW_CNV_CTL *ctl)
{ (void)type; (void)ctl; STUB_TRAP(); }
void throwClassPtrCnvs(TYPE type,THROW_CNV_CTL *ctl)
{ (void)type; (void)ctl; STUB_TRAP(); }
TYPE TypePtrToVoid(void);
TYPE TypePointedAt(TYPE type,unsigned short *flags);
void makeCnvVoidStar(THROW_CNV_CTL *ctl)
{
    makeThrowCnvNoAcc(ctl,TypePtrToVoid(),0);
}
unsigned ThrowCnvInit(THROW_CNV_CTL *ctl,TYPE type)
{
    unsigned short flags;
    unsigned char category;
    ctl->carver=CarveCreate(sizeof(THROW_CNV),8);
    ctl->hdr=NULL;
    ctl->cur=NULL;
    type=TypeCanonicalThr(type);
    ctl->flags&=~1;
    category=ThrowCategory(type);
    if(category==2) makeThrowCnvNoAcc(ctl,type,0);
    else makeThrowCnv(ctl,type,0);
    switch(category) {
    case 7: makeCnvVoidStar(ctl); break;
    case 2:
        type=ReferenceType(type);
        if(StructType(type)) throwClassCnvs(type,ctl);
        else {
            type=TypePointedAt(type,&flags);
            if(type) {
                type=StructType(type);
                if(type) throwClassPtrCnvs(type,ctl);
                makeCnvVoidStar(ctl);
            }
        }
        break;
    case 0: case 4: throwClassCnvs(type,ctl); break;
    case 1: throwClassPtrCnvs(type,ctl); makeCnvVoidStar(ctl); break;
    }
    return RingCount(ctl->hdr);
}
PTREE PTreeExtractLocn(PTREE expr,CGIO_LOCATION *locn);
int throwCnvFront(TYPE type,PTREE expr)
{
    THROW_CNV_CTL ctl;
    PTreeExtractLocn(expr,&ctl.err_locn);
    ThrowCnvInit(&ctl,type);
    /* The old45B body does not call the later ThrowCnvFini. */
    return !(ctl.flags&1);
}
PTREE NodeUnary(unsigned char operation,PTREE expr);
PTREE NodeArg(PTREE expr);
PTREE ThrowTypeSig(TYPE type,PTREE expr)
{
    PTREE node=NULL;
    type=TypeCanonicalThr(type);
    if(throwCnvFront(type,expr)) {
        node=NodeUnary(117,NULL);
        node->type=type;
        node=NodeArg(node);
        node->type=type;
    }
    return node;
}

TYPE throwCnvTypeRetn(THROW_CNV_CTL *ctl,unsigned *offset,THROW_CNV *cnv)
{
    ctl->cur=cnv;
    *offset=cnv->offset;
    return cnv->sig->type;
}
TYPE ThrowCnvType(THROW_CNV_CTL *ctl,unsigned *offset)
{
    THROW_CNV *cnv=ctl->cur;
    if (!cnv) {
        cnv=ctl->hdr;
        if (!cnv) return NULL;
    } else if (cnv==ctl->hdr) return NULL;
    return throwCnvTypeRetn(ctl,offset,cnv->next);
}
void ThrowCnvFini(THROW_CNV_CTL *ctl)
{ CarveDestroy(ctl->carver); }
