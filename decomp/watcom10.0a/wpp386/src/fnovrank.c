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


/* Adapted initial6b9cb443 fnovload.c. Actual1994 diagnostics use
 * two global lists and a global candidate count, before FNOV_DIAG objects.
 * FnovFreeDiag's second list call falls through into FnovListFree in the
 * original; both logical entries and their disjoint owners are retained. */
/* The1994 ranking driver applies ordered predicates, before the later
 * RKD conversion matrix. Each descriptor is42 packed bytes; compiler local
 * stack slots round these to44 bytes. The reference marker is a full int. */
#include <stddef.h>
#include "fnovload.h"
#include "ptree.h"
#include "pending.h"
#include <recon.h>
#pragma pack(1)
typedef struct {
    PTREE *ptnode;
    TYPE original,reftype,basic,final;
    unsigned short leadflag,refflag,finalflag;
    void *leadbase,*refbase,*finalbase;
    int reference;
} FNOV_TYPE;
#pragma pack()
typedef char FnovTypeSize[(sizeof(FNOV_TYPE)==42)?1:-1];
TYPE TypeModExtract(TYPE type,unsigned short *flags,void **base,unsigned char mask);
TYPE PointerTypeForArray(TYPE type);
int TypeCompareExclude(TYPE source,TYPE target,unsigned char mask);
int TypesSameExclude(TYPE source,TYPE target,unsigned char mask);
void initFNOV_TYPE(FNOV_TYPE *ft)
{
    TYPE basic;
    ft->reference=0;
    ft->reftype=NULL;
    ft->final=NULL;
    ft->refflag=0;
    ft->finalflag=0;
    basic=TypeModExtract(ft->original,&ft->leadflag,&ft->leadbase,9);
    ft->basic=basic;
    if(basic->id==TYP_POINTER && (basic->flag&1)) {
        ft->reference=1;
        ft->reftype=basic;
        basic=TypeModExtract(basic->of,&ft->refflag,&ft->refbase,9);
    } else basic=PointerTypeForArray(ft->basic);
    ft->basic=basic;
}
int errorRank(FNOV_TYPE *source,FNOV_TYPE *target,FNOV_RANK *rank)
{
    initFNOV_TYPE(source);
    initFNOV_TYPE(target);
    if(source->basic->id==TYP_ERROR || source->basic->id==TYP_VOID ||
       target->basic->id==TYP_ERROR || target->basic->id==TYP_VOID) {
        rank->rank=11;
        return 1;
    }
    return 0;
}
int ellipsisRank(FNOV_TYPE *source,FNOV_TYPE *target,FNOV_RANK *rank)
{
    if((target->basic->id==TYP_DOT_DOT_DOT && source->basic->id!=TYP_DOT_DOT_DOT) ||
       (target->basic->id!=TYP_DOT_DOT_DOT && source->basic->id==TYP_DOT_DOT_DOT)) {
        rank->rank=10;
        return 1;
    }
    return 0;
}
int exactRank(FNOV_TYPE *source,FNOV_TYPE *target,FNOV_RANK *rank)
{
    TYPE src=source->basic,tgt=target->basic;
    source->finalflag=target->finalflag=0;
    if(src->id==TYP_POINTER || src->id==TYP_MEMBER_POINTER)
        source->final=TypeModExtract(src->of,&source->finalflag,&source->finalbase,9);
    if(tgt->id==TYP_POINTER || tgt->id==TYP_MEMBER_POINTER)
        target->final=TypeModExtract(tgt->of,&target->finalflag,&target->finalbase,9);
    if(source->reference==target->reference && source->leadflag==target->leadflag &&
       source->refflag==target->refflag && TypesSameExclude(src,tgt,1)) {
        rank->rank=1;
        return 1;
    }
    return 0;
}
int TypesIdentical(TYPE source,TYPE target);
int functionsAreIdentical(TYPE source,TYPE target)
{ return TypesSameExclude(source,target,5); }
int sameRank(FNOV_TYPE *source,FNOV_TYPE *target,FNOV_RANK *rank)
{
    TYPE src=source->basic,tgt=target->basic;
    TYPE sf=source->final,tf=target->final;
    int invalid=0;
    if(tgt->id==TYP_ENUM) {
        if(src->id==TYP_ENUM && !TypesSameExclude(src,tgt,1)) invalid=1;
        else if(src->id>=TYP_CHAR && src->id<TYP_ENUM) invalid=1;
    } else if(tgt->id==TYP_FUNCTION) {
        if(src->id==TYP_FUNCTION) invalid=!functionsAreIdentical(src,tgt);
        else if(src->id==TYP_POINTER && sf->id==TYP_FUNCTION)
            invalid=!functionsAreIdentical(sf,tgt);
    } else if(tgt->id==TYP_POINTER && tf->id==TYP_FUNCTION) {
        if(src->id==TYP_FUNCTION) invalid=!functionsAreIdentical(src,tf);
        else if(src->id==TYP_POINTER && sf->id==TYP_FUNCTION)
            invalid=!functionsAreIdentical(sf,tf);
    } else if(tgt->id==TYP_MEMBER_POINTER && tf->id==TYP_FUNCTION &&
              src->id==TYP_MEMBER_POINTER && sf->id==TYP_FUNCTION)
        invalid=!TypesIdentical(sf,tf);
    if(invalid) rank->rank=11;
    return invalid;
}
int FnovCvFlagsRank(unsigned short source,unsigned short target,FNOV_RANK *rank);
int trivialRank(FNOV_TYPE *source,FNOV_TYPE *target,FNOV_RANK *rank)
{
    int result=0;
    if(target->reference) {
        result=FnovCvFlagsRank(source->reference ? source->refflag : source->leadflag,
                              target->refflag,rank);
        if(result) return 1;
    }
    if(source->basic->id==TYP_POINTER && target->basic->id==TYP_POINTER) {
        if(target->final->id==TYP_VOID && (target->final->flag&1)) return 0;
        result=FnovCvFlagsRank(source->finalflag,target->finalflag,rank);
    } else if(source->basic->id==TYP_MEMBER_POINTER && target->basic->id==TYP_MEMBER_POINTER)
        result=FnovCvFlagsRank(source->finalflag,target->finalflag,rank);
    return result;
}
int functionRank(FNOV_TYPE *source,FNOV_TYPE *target,FNOV_RANK *rank)
{
    TYPE src=source->basic,tgt=target->basic;
    TYPE sf=source->final,tf=target->final;
    if(tgt->id==TYP_FUNCTION) {
        if(src->id==TYP_FUNCTION) {
            if(!functionsAreIdentical(src,tgt)) rank->rank=11;
            return 1;
        }
        if(src->id==TYP_POINTER && sf->id==TYP_FUNCTION) {
            if(!functionsAreIdentical(sf,tgt)) rank->rank=11;
            return 1;
        }
    } else if(tgt->id==TYP_POINTER && tf->id==TYP_FUNCTION) {
        if(src->id==TYP_FUNCTION) {
            if(!functionsAreIdentical(src,tf)) rank->rank=11;
            return 1;
        }
        if(src->id==TYP_POINTER && sf->id==TYP_FUNCTION) {
            if(!functionsAreIdentical(sf,tf)) rank->rank=11;
            return 1;
        }
    }
    return 0;
}
unsigned char arithArithRank(unsigned char source,unsigned char target,int from_enum)
{
    if(source==target) {
        if(!from_enum) return 1;
        if(target>=TYP_SINT && target<=TYP_ULONG) return 4;
        return 5;
    }
    if(source>=TYP_CHAR && source<=TYP_USHORT) return target==TYP_SINT ? 4 : 5;
    if(source==TYP_FLOAT) return target==TYP_DOUBLE ? 4 : 5;
    if(source>=TYP_SINT && source<=TYP_LONG_DOUBLE) return 5;
    /* The actual caller supplies only arithmetic ids here. */
    STUB_TRAP();
    return 11;
}
void FnovMemFlagsRank(unsigned short source,unsigned short target,void *source_scope,void *target_scope,FNOV_RANK *rank);
int arithRank(FNOV_TYPE *source,FNOV_TYPE *target,FNOV_RANK *rank)
{
    unsigned char src=source->basic->id,tgt=target->basic->id,result;
    int handled;
    if(src>=TYP_CHAR && src<=TYP_ENUM && tgt>=TYP_CHAR && tgt<=TYP_ENUM) {
        handled=1;
        if(tgt==TYP_ENUM) result=1;
        else if(src==TYP_ENUM) result=arithArithRank(source->basic->of->id,tgt,1);
        else result=arithArithRank(src,tgt,0);
        if(result==4) { rank->rank=4; rank->u.no_ud.promotion=1; }
        else if(result==5) { rank->rank=5; ++rank->u.no_ud.standard; }
        else if(result==11) rank->rank=11;
    } else handled=functionRank(source,target,rank);
    if(rank->rank<11) {
        if(target->reference) {
            if(source->reference)
                FnovMemFlagsRank(source->refflag,target->refflag,source->refbase,target->refbase,rank);
            else FnovMemFlagsRank(source->leadflag,target->refflag,source->leadbase,target->refbase,rank);
        } else if(tgt==TYP_FUNCTION && src==TYP_FUNCTION)
            FnovMemFlagsRank(source->leadflag,target->leadflag,source->leadbase,target->leadbase,rank);
        else if(tgt==TYP_POINTER) {
            if(src==TYP_POINTER)
                FnovMemFlagsRank(source->finalflag,target->finalflag,source->finalbase,target->finalbase,rank);
            else if(src==TYP_FUNCTION)
                FnovMemFlagsRank(source->leadflag,target->finalflag,source->leadbase,target->finalbase,rank);
        } else if(tgt==TYP_MEMBER_POINTER && src==TYP_MEMBER_POINTER)
            FnovMemFlagsRank(source->finalflag,target->finalflag,source->finalbase,target->finalbase,rank);
    }
    return handled;
}
int NodeIsZeroConstant(PTREE node);
SCOPE TypeScope(TYPE type);
unsigned char ScopeDerivedCount(SCOPE derived,SCOPE base,unsigned *depth);
int fnovScopeDerived(TYPE source,unsigned short source_flags,TYPE target,unsigned short target_flags,FNOV_RANK *rank)
{
    SCOPE src=TypeScope(source),tgt=TypeScope(target);
    unsigned depth;
    unsigned char status;
    if(src && tgt) {
        /*1994 retains the inheritance depth in the secondary conversion
         * rank; the released version instead calls plain ScopeDerived. */
        status=ScopeDerivedCount(src,tgt,&depth);
        if((status==1 || status==2) &&
           !FnovCvFlagsRank(source_flags,target_flags,rank)) {
            rank->rank=6;
            ++rank->u.no_ud.standard;
            rank->u.no_ud.udcnv=(unsigned char)depth;
            return 1;
        }
    }
    return 0;
}
int toPtrRank(FNOV_TYPE *source,FNOV_TYPE *target,FNOV_RANK *rank)
{
    TYPE src=source->basic,tgt=target->basic;
    TYPE sf=source->final,tf=target->final;
    if(tgt->id!=TYP_POINTER) return 0;
    if(source->ptnode!=NULL && *source->ptnode!=NULL && NodeIsZeroConstant(*source->ptnode)) {
        rank->rank=5;
        ++rank->u.no_ud.standard;
        return 1;
    }
    if(tf->id==TYP_VOID) {
        if(src->id==TYP_POINTER) {
            if((tf->flag&2) && sf->id==TYP_VOID) rank->rank=11;
            else { rank->rank=7; ++rank->u.no_ud.standard; }
            return 1;
        }
        if(src->id==TYP_FUNCTION) {
            rank->rank=7;
            ++rank->u.no_ud.standard;
            return 1;
        }
        if(src->id==TYP_CLASS) return 0;
        rank->rank=11;
        return 1;
    }
    if(src->id==TYP_FUNCTION) {
        if(tf->id!=TYP_FUNCTION) rank->rank=11;
        return 1;
    }
    if(src->id==TYP_POINTER) {
        if(TypesSameExclude(sf,tf,5)) return 1;
        if(sf->id==TYP_CLASS && tf->id==TYP_CLASS &&
           fnovScopeDerived(sf,source->finalflag,tf,target->finalflag,rank)) return 1;
        rank->rank=11;
        return 1;
    }
    if(src->id==TYP_CLASS) return 0;
    rank->rank=11;
    return 1;
}
TYPE TypeVoidMemberPtr(void);
TYPE MemberPtrClass(TYPE type,TYPE *unmodified);
int toMbrPtrRank(FNOV_TYPE *source,FNOV_TYPE *target,FNOV_RANK *rank)
{
    TYPE src=source->basic,tgt=target->basic,tf=target->final;
    TYPE any=TypeVoidMemberPtr();
    if(tgt->id!=TYP_MEMBER_POINTER) return 0;
    if(src->id==TYP_MEMBER_POINTER && (src==any || tgt==any)) {
        rank->rank=2;
        rank->u.no_ud.not_exact=1;
        return 1;
    }
    if(source->ptnode!=NULL && *source->ptnode!=NULL && NodeIsZeroConstant(*source->ptnode)) {
        rank->rank=5;
        ++rank->u.no_ud.standard;
        return 1;
    }
    if(src->id==TYP_FUNCTION) {
        if(tf->id!=TYP_FUNCTION) rank->rank=11;
        return 1;
    }
    if(src->id==TYP_MEMBER_POINTER) {
        if(TypesSameExclude(source->final,target->final,1)) {
            if(TypesSameExclude(src,tgt,1)) return 1;
            /* Original parameter evaluation computes both class hosts. */
            if(fnovScopeDerived(MemberPtrClass(tgt,NULL),0,MemberPtrClass(src,NULL),0,rank)) return 1;
        }
        rank->rank=11;
        return 1;
    }
    if(src->id==TYP_CLASS) return 0;
    rank->rank=11;
    return 1;
}
int fnovNumCandidatesGet(void);
void fnovNumCandidatesRestore(int count);
unsigned char ctorFindConversion(TYPE type,arg_list *args,PTREE *nodes,
    FNOV_LIST **match,CGIO_LOCATION *location);
FNOV_LIST *findCtorMatch(FNOV_TYPE *source,FNOV_TYPE *target,FNOV_RANK *rank)
{
    arg_list args;
    FNOV_LIST *match;
    int saved;
    (void)rank;
    InitArgList(&args);
    args.num_args=1;
    args.type_list[0]=source->original;
    saved=fnovNumCandidatesGet();
    ctorFindConversion(target->basic,&args,source->ptnode,&match,NULL);
    fnovNumCandidatesRestore(saved);
    return match;
}
int fnovNumCandidatesGet(void);
void fnovNumCandidatesRestore(int count);
unsigned char UdcLookup(TYPE source,unsigned short flags,TYPE *target,SYMBOL *symbol);
void BuildUdcList(FNOV_LIST **candidates,SYMBOL sym);
unsigned char rankUdcCandidates(FNOV_LIST *candidates,TYPE target,unsigned short flags,FNOV_LIST **rejects,FNOV_LIST **match);
FNOV_LIST *findUdcMatch(FNOV_TYPE *source,FNOV_TYPE *target,FNOV_RANK *rank)
{
    FNOV_LIST *list,*rejects;
    TYPE target_type=target->original;
    SYMBOL sym;
    unsigned short flags;
    int saved;
    (void)rank;
    flags=(source->reference ? source->refflag : source->leadflag)&0x73f;
    saved=fnovNumCandidatesGet();
    if(UdcLookup(source->basic,flags,&target_type,&sym)==0) {
        list=NULL;
        BuildUdcList(&list,sym);
        rankUdcCandidates(list,target->original,flags,&rejects,&list);
    } else list=NULL;
    fnovNumCandidatesRestore(saved);
    return list;
}
int toClsRank(FNOV_TYPE *source,FNOV_TYPE *target,FNOV_RANK *rank)
{
    FNOV_LIST *match;
    if(target->basic->id!=TYP_CLASS || source->basic->id==TYP_CLASS) return 0;
    if(!(rank->control&2) && (match=findCtorMatch(source,target,rank))!=NULL) {
        rank->rank=8;
        rank->userdef=1;
        rank->u.ud.in=match->rankvector->u.no_ud;
        FnovListFree(&match);
    } else rank->rank=11;
    return 1;
}
int fromClsRank(FNOV_TYPE *source,FNOV_TYPE *target,FNOV_RANK *rank)
{
    FNOV_LIST *match;
    if(source->basic->id!=TYP_CLASS || target->basic->id==TYP_CLASS) return 0;
    if(!(rank->control&2) && (match=findUdcMatch(source,target,rank))!=NULL) {
        rank->rank=8;
        rank->userdef=1;
        rank->u.ud.out=match->rankvector->u.no_ud;
        FnovListFree(&match);
    } else rank->rank=11;
    return 1;
}
int clsToClsRank(FNOV_TYPE *source,FNOV_TYPE *target,FNOV_RANK *rank)
{
    TYPE src=source->basic,tgt=target->basic;
    unsigned short sf=source->reference ? source->refflag : source->leadflag;
    unsigned short tf=target->reference ? target->refflag : target->leadflag;
    FNOV_LIST *ctor,*udc;
    if(src->id!=TYP_CLASS || tgt->id!=TYP_CLASS) return 0;
    if(!TypesIdentical(src,tgt) && !fnovScopeDerived(src,sf,tgt,tf,rank)) {
        if(rank->control&2) rank->rank=11;
        else {
            ctor=findCtorMatch(source,target,rank);
            udc=findUdcMatch(source,target,rank);
            if(ctor!=NULL && udc!=NULL) {
                rank->rank=9;
                rank->userdef=1;
                FnovListFree(&ctor);
                FnovListFree(&udc);
            } else if(ctor!=NULL) {
                rank->rank=8;
                rank->userdef=1;
                rank->u.ud.in=ctor->rankvector->u.no_ud;
                FnovListFree(&ctor);
            } else if(udc!=NULL) {
                rank->rank=8;
                rank->userdef=1;
                rank->u.ud.out=udc->rankvector->u.no_ud;
                FnovListFree(&udc);
            } else rank->rank=11;
        }
    }
    return 1;
}
void FnovArgRank(TYPE source,TYPE target,PTREE *pt,FNOV_RANK *rank)
{
    FNOV_TYPE src,tgt;
    src.ptnode=pt;
    src.original=source;
    tgt.original=target;
    tgt.ptnode=NULL;
    if(errorRank(&src,&tgt,rank)) return;
    if(ellipsisRank(&src,&tgt,rank)) return;
    if(exactRank(&src,&tgt,rank)) return;
    if(sameRank(&src,&tgt,rank)) return;
    rank->rank=2;
    rank->u.no_ud.not_exact=1;
    if(!trivialRank(&src,&tgt,rank)) {
        if(arithRank(&src,&tgt,rank)) return;
        if(toPtrRank(&src,&tgt,rank)) return;
        if(toMbrPtrRank(&src,&tgt,rank)) return;
    }
    if(toClsRank(&src,&tgt,rank)) return;
    if(fromClsRank(&src,&tgt,rank)) return;
    if(clsToClsRank(&src,&tgt,rank)) return;
    rank->rank=11;
}
