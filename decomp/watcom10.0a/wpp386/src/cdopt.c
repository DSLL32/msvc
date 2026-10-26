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


/* Adapted from initial6b9cb443 bld/plusplus/c/cdopt.c.
 * Actual1994 CF1fc8c..1fd09 initializes five typed allocation pools
 * and the constructor/destructor traversal stack. Sizes are the reached
 * old record sizes, before the later CDOPT_ITER/CDOPT_CACHE extensions.
 * This body has no later allDescriptors reset or ExtraRpt registration. */
#include <stddef.h>
#include "compiler.h"
#include "carve.h"
#include "vstk.h"
#include "types.h"
#include "pending.h"
#include "symbol.h"
#include "cdopt.h"
#include <recon.h>
static carve_t carveCD_DESCR, carveCL_ELEM, carveCDOPT_ITER;
static carve_t carveACC_FUN, carveCDOPT_CACHE;
static VSTK_CTL stackSTKIN;
void cdoptInit(INITFINI *defn)
{
    (void)defn;
    carveCDOPT_CACHE = CarveCreate(8, 16);
    carveCD_DESCR = CarveCreate(20, 32);
    carveCL_ELEM = CarveCreate(28, 64);
    carveCDOPT_ITER = CarveCreate(40, 4);
    carveACC_FUN = CarveCreate(8, 64);
    VstkOpen(&stackSTKIN, 8, 16);
}




TYPE ArrayType(TYPE type);
TYPE ArrayBaseType(TYPE type);
TYPE StructType(TYPE type);
struct class_info *TypeClassInfo(TYPE type);
int TypeRequiresDtoring(TYPE type);
int SymIsVirtual(SYMBOL sym);
SYMBOL RoDtorFindType(TYPE type);
SYMBOL dtorLocate(TYPE type)
{
    type=StructType(type);
    return type ? RoDtorFindType(type) : NULL;
}
int classCanBeDtorOpt(struct class_info *info)
{ (void)info; STUB_TRAP(); return 0; }
int TypeHasDtorableObjects(TYPE type)
{ (void)type; STUB_TRAP(); return 0; }
int typeDtorable(TYPE type,int exact)
{
    TYPE base;
    SYMBOL dtor;
    if (!type) return 0;
    base=ArrayType(type);
    if (base) type=ArrayBaseType(base);
    type=StructType(type);
    if (!type || !TypeRequiresDtoring(type)) return 0;
    dtor=dtorLocate(type);
    if (!exact && SymIsVirtual(dtor)) return 1;
    if (classCanBeDtorOpt(TypeClassInfo(type))) return TypeHasDtorableObjects(type);
    return 1;
}
int TypeReallyDtorable(TYPE type) { return typeDtorable(type,0); }

/* Encountered1994 descriptors: cache nodes are separate8B objects, not
 * the later descriptor's embedded next link. The callback table is a real
 * eight-slot CODE object at1f092 and belongs to the destructor definition. */
typedef struct cl_elem CL_ELEM;
typedef struct acc_fun ACC_FUN;
typedef struct cdopt_cache CDOPT_CACHE;
typedef struct memb_vfuns MEMB_VFUNS;
#pragma pack(1)
struct cdopt_cache { CDOPT_CACHE *next; CD_DESCR *info; };
struct acc_fun { ACC_FUN *next; SYMBOL fun; };
struct cd_descr {
    const MEMB_VFUNS *mfuns;
    TYPE orig_type;
    CL_ELEM *elements;
    unsigned char opt;
    ACC_FUN *accessed;
    unsigned char err_occurred:1,has_vbt:1,has_vft:1,has_acc:1,chk_acc:1,reserved:3;
};
struct cl_elem {
    CL_ELEM *next;
    TYPE cltype;
    long offset;
    SYMBOL cdtor,sym;
    CD_DESCR *descr;
    unsigned char otype;
    unsigned char elim_intermed:1,cannot_define:1,must_call:1,reserved:5;
};
typedef struct cdopt_defn {
    CDOPT_CACHE **cache;
    const MEMB_VFUNS *vfuns;
    unsigned char otype;
} CDOPT_DEFN;
struct memb_vfuns {
    int (*basePushable)(void *);
    int (*membPushable)(SYMBOL);
    SYMBOL (*find)(CL_ELEM *);
    int (*optimizable)(CL_ELEM *);
    int (*include)(CL_ELEM *);
    SYMBOL (*array_cdtor)(SYMBOL);
    void (*schedule_acc_chk)(TYPE);
    SYMBOL (*array_acc)(TYPE);
};
#pragma pack()
int dtorBasePushable(void *base) { (void)base; STUB_TRAP(); return 0; }
int SymIsThisDataMember(SYMBOL sym);
int SymRequiresDtoring(SYMBOL sym);
int dtorMembPushable(SYMBOL sym)
{ return SymIsThisDataMember(sym) && SymRequiresDtoring(sym); }
SYMBOL dtorFind(CL_ELEM *elem) { (void)elem; STUB_TRAP(); return NULL; }
int dtorOptimizable(CL_ELEM *elem) { (void)elem; STUB_TRAP(); return 0; }
int dtorInclude(CL_ELEM *elem) { (void)elem; STUB_TRAP(); return 0; }
SYMBOL dtorArraySym(SYMBOL sym) { (void)sym; STUB_TRAP(); return NULL; }
void RtnGenAddType(unsigned char callback,TYPE type);
void defDtorSchAccChk(TYPE type) { RtnGenAddType(2,type); }
SYMBOL dtorArrayAcc(TYPE type) { (void)type; STUB_TRAP(); return NULL; }
static CDOPT_CACHE *dtorCache;
static const MEMB_VFUNS mfDtor={dtorBasePushable,dtorMembPushable,dtorFind,
    dtorOptimizable,dtorInclude,dtorArraySym,defDtorSchAccChk,dtorArrayAcc};
static const CDOPT_DEFN dtor_defn={&dtorCache,&mfDtor,0};
typedef struct stkin { CD_DESCR *info; CL_ELEM *elem; } STKIN;
CD_DESCR *cacheFind(TYPE type,CDOPT_CACHE *head)
{
    CDOPT_CACHE *entry;
    if (head) {
        entry=head;
        do {
            entry=entry->next;
            if (entry->info->orig_type==type) return entry->info;
        } while (entry!=head);
    }
    return NULL;
}
typedef struct cl_expand { CD_DESCR *info; STKIN *source; } CL_EXPAND;
void *RingCarveAlloc(carve_t cv,void **head);
void cacheAdd(CD_DESCR *info,CDOPT_CACHE **head)
{
    CDOPT_CACHE *entry=RingCarveAlloc(carveCDOPT_CACHE,(void **)head);
    entry->info=info;
}
void pushVbase(void *base,void *data) { (void)base; (void)data; STUB_TRAP(); }
void pushDbase(void *base,void *data) { (void)base; (void)data; STUB_TRAP(); }
/* Namespaced from type.c's unrelated private pushType helper. */
void cdoptPushType(TYPE type,long offset,SYMBOL sym,CL_EXPAND *data,unsigned char otype)
{ (void)type; (void)offset; (void)sym; (void)data; (void)otype; STUB_TRAP(); }
void pushMember(void *symbol,void *context)
{
    SYMBOL sym=symbol;
    CL_EXPAND *data=context;
    if (data->info->mfuns->membPushable(sym))
        cdoptPushType(sym->sym_type,sym->u.sval,sym,data,ArrayType(sym->sym_type)?3:2);
}
void ScopeWalkVirtualBases(SCOPE scope,void (*callback)(void *,void *),void *data);
void ScopeWalkDirectBases(SCOPE scope,void (*callback)(void *,void *),void *data);
void ScopeWalkDataMembers(SCOPE scope,void (*callback)(void *,void *),void *data);
void *VstkTop(VSTK_CTL *stack);
SCOPE TypeScope(TYPE type);
CD_DESCR *cdoptExpandClass(TYPE type,const CDOPT_DEFN *defn)
{
    CD_DESCR *info=CarveAlloc(carveCD_DESCR);
    struct class_info *class_info;
    CL_EXPAND expansion;
    SCOPE scope;
    cacheAdd(info,defn->cache);
    info->orig_type=type;
    info->mfuns=defn->vfuns;
    info->err_occurred=0;
    info->has_acc=0;
    info->chk_acc=0;
    info->opt=defn->otype;
    info->elements=NULL;
    info->accessed=NULL;
    info->mfuns->schedule_acc_chk(type);
    class_info=TypeClassInfo(type);
    info->has_vft=class_info->has_vfptr;
    info->has_vbt=class_info->has_vbptr;
    expansion.info=info;
    expansion.source=VstkTop(&stackSTKIN);
    scope=TypeScope(type);
    ScopeWalkVirtualBases(scope,pushVbase,&expansion);
    ScopeWalkDirectBases(scope,pushDbase,&expansion);
    ScopeWalkDataMembers(scope,pushMember,&expansion);
    return info;
}
void addInfoElement(CD_DESCR *info,CL_ELEM *elem)
{ (void)info; (void)elem; STUB_TRAP(); }
void addAccFun(CD_DESCR *info,SYMBOL sym)
{ (void)info; (void)sym; STUB_TRAP(); }
void *VstkTop(VSTK_CTL *stack);
void *VstkPop(VSTK_CTL *stack);
CD_DESCR *cdoptBuildOrig(TYPE type,const CDOPT_DEFN *defn)
{
    CD_DESCR *info,*descr;
    STKIN *stacked;
    CL_ELEM *elem;
    TYPE elem_type;
    const MEMB_VFUNS *functions;
    type=StructType(type);
    info=cacheFind(type,*defn->cache);
    if (!info) {
        info=cdoptExpandClass(type,defn);
        functions=info->mfuns;
        for (;;) {
            stacked=VstkTop(&stackSTKIN);
            if (!stacked) break;
            elem=stacked->elem;
            if (elem->otype==3) {
                elem->must_call=1;
                addInfoElement(stacked->info,elem);
                VstkPop(&stackSTKIN);
            } else {
                elem_type=StructType(elem->cltype);
                descr=elem->descr;
                if (!descr) {
                    descr=cacheFind(elem_type,*defn->cache);
                    elem->descr=descr;
                }
                if (!descr) {
                    if (!elem_type) {
                        if (functions->include(elem)) {
                            elem->must_call=1;
                            addInfoElement(stacked->info,elem);
                        }
                        VstkPop(&stackSTKIN);
                    } else elem->descr=cdoptExpandClass(elem_type,defn);
                } else {
                    if (elem->cannot_define) addInfoElement(stacked->info,elem);
                    else if (!functions->optimizable(elem)) {
                        elem->must_call=1;
                        addInfoElement(stacked->info,elem);
                    } else if (functions->include(elem)) addInfoElement(stacked->info,elem);
                    else addAccFun(stacked->info,elem->cdtor);
                    VstkPop(&stackSTKIN);
                }
            }
        }
    }
    return info;
}
CD_DESCR *CDoptDtorBuild(TYPE type) { return cdoptBuildOrig(type,&dtor_defn); }

int TypeRequiresCtoring(TYPE type);
int SymRequiresCtoring(SYMBOL sym);
int defCtorBasePushable(void *base) { (void)base; STUB_TRAP(); return 0; }
int defCtorMembPushable(SYMBOL sym)
{
    return SymIsThisDataMember(sym) && (SymRequiresCtoring(sym) || SymRequiresDtoring(sym));
}
SYMBOL defCtorFind(CL_ELEM *elem) { (void)elem; STUB_TRAP(); return NULL; }
int defCtorOptimizable(CL_ELEM *elem) { (void)elem; STUB_TRAP(); return 0; }
int defCtorInclude(CL_ELEM *elem) { (void)elem; STUB_TRAP(); return 0; }
SYMBOL origArraySym(SYMBOL sym) { return sym; }
void defCtorSchAccChk(TYPE type) { RtnGenAddType(1,type); }
SYMBOL defCtorArrayAcc(TYPE type) { (void)type; STUB_TRAP(); return NULL; }
static CDOPT_CACHE *ctorCache;
static const MEMB_VFUNS mfDefCtor={defCtorBasePushable,defCtorMembPushable,
    defCtorFind,defCtorOptimizable,defCtorInclude,origArraySym,
    defCtorSchAccChk,defCtorArrayAcc};
static const CDOPT_DEFN ctor_defn={&ctorCache,&mfDefCtor,1};
CD_DESCR *CDoptDefCtorBuild(TYPE type) { return cdoptBuildOrig(type,&ctor_defn); }
int CDoptErrorOccurred(CD_DESCR *info) { return info->err_occurred; }
void CDoptNoAccFun(CD_DESCR *info) { (void)info; STUB_TRAP(); }
typedef struct cl_iter { CD_DESCR *info; CL_ELEM *elem; } CL_ITER;
#pragma pack(1)
struct cdopt_iter {
    CD_DESCR *info;
    VSTK_CTL stack;
    long off_comp,off_elem,off_vbase;
    void *vbase;
    unsigned char orig_otype;
    unsigned char gened_comp:1,at_end:1,reserved:6;
};
#pragma pack()
void *VstkPush(VSTK_CTL *stack);
CL_ITER *pushClElem(CD_DESCR *info,CDOPT_ITER *iter)
{
    CL_ITER *entry=VstkPush(&iter->stack);
    entry->elem=NULL;
    entry->info=info;
    return entry;
}
CDOPT_ITER *CDoptIterBeg(CD_DESCR *info)
{
    CDOPT_ITER *iter=CarveAlloc(carveCDOPT_ITER);
    iter->info=info;
    iter->orig_otype=0;
    VstkOpen(&iter->stack,8,16);
    pushClElem(iter->info,iter);
    return iter;
}
void VstkClose(VSTK_CTL *stack);
void CDoptIterEnd(CDOPT_ITER *iter)
{
    VstkClose(&iter->stack);
    CarveFree(carveCDOPT_ITER,iter);
}

void *ScopeFindVBase(SCOPE scope,TYPE type);
unsigned char CDoptIterNextComp(CDOPT_ITER *iter)
{
    CD_DESCR *info=iter->info;
    CL_ITER *current;
    CL_ELEM *elem;
    unsigned char result;
    iter->off_vbase=0;
    iter->off_elem=0;
    iter->off_comp=0;
    iter->vbase=NULL;
    iter->gened_comp=0;
    current=VstkTop(&iter->stack);
    if (current->elem==info->elements) result=0;
    else {
        elem=current->elem ? current->elem : info->elements;
        elem=elem->next;
        current->elem=elem;
        iter->off_comp=elem->offset;
        if (elem->otype==3) result=5;
        else if (!elem->cdtor) result=8;
        else switch (elem->otype) {
        case 0:
            iter->off_vbase=elem->offset;
            iter->vbase=ScopeFindVBase(TypeScope(info->orig_type),elem->cltype);
            result=7;
            break;
        case 1: result=6; break;
        case 2: result=5; break;
        default: STUB_TRAP(); result=0; break;
        }
    }
    iter->orig_otype=result;
    return result;
}
long CDoptIterOffsetExact(CDOPT_ITER *iter)
{
    long offset=iter->off_elem;
    CL_ITER *current=VstkTop(&iter->stack);
    if (current->elem) offset+=current->elem->offset;
    return offset;
}

int TypeExactDtorable(TYPE type) { return typeDtorable(type,1); }
