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


/* Adapted initial6b9cb443 datainit.c. Actual1994 CF2f86c..2f8a1
 * uses34-byte nest and12-byte queue records; later code-file state absent. */
#include "compiler.h"
#include "carve.h"
#include "datainit.h"
#include "typesig.h"
#include "ptree.h"
#include "pending.h"
#include <recon.h>
static carve_t carveNEST,carveQUEUE;
static struct data_init *currInit;
void dataInitInit(INITFINI *defn)
{
    (void)defn;
    carveNEST=CarveCreate(34,16);
    carveQUEUE=CarveCreate(12,16);
    currInit=0;
}

int DeclWithInit(DECL_INFO *info);
int SymIsHugeData(SYMBOL sym);
int dataInitCheckHugeAlign(TYPE type);
int SymRequiresDtoring(SYMBOL sym);
unsigned CgTypeSize(TYPE type);
TYPE TypeNeedsCtor(TYPE type);
unsigned CgMemorySize(TYPE type);
int SymIsConstant(SYMBOL sym);
SCOPE SymScope(SYMBOL sym);
int SymIsAutomatic(SYMBOL sym);
int SymIsStaticData(SYMBOL sym);
void DgInitBegin(SYMBOL sym);
void __cdecl CErr(unsigned number,...);
void dataInitPushStack(unsigned char entry,INITIALIZE_INFO *previous);
void dataInitLabel(void);
void dataInitStartLocal(INITIALIZE_DATA *init,DECL_INFO *info,int initializer)
{
    SYMBOL sym=info->sym;
    init->auto_sym=NULL;
    init->ctor_sym=NULL;
    init->prev=currInit;
    init->dinfo=info;
    currInit=init;
    init->sym=sym;
    init->sym_type=sym?sym->sym_type:info->type;
    init->base_type=NULL;
    init->auto_type=NULL;
    init->size_type=0;
    init->nest=NULL;
    init->queue=NULL;
    init->state=DS_IGNORE;
    init->location=DL_IGNORE;
    init->once_only=0; init->label_done=0; init->auto_static=0;
    init->need_storage=initializer;
    init->ctor_reqd=0; init->dtor_reqd=0; init->dtor_done=0; init->const_int=0;
    init->emit_code=0; init->bracketed=0; init->all_zero=1; init->huge_sym=0;
    init->initted=initializer; init->no_size=0;
    if(initializer && !DeclWithInit(info)) return;
    if(SymIsHugeData(sym)) {
        currInit->huge_sym=1;
        currInit->need_storage=1;
        if(dataInitCheckHugeAlign(sym->sym_type)) { CErr(484,sym); return; }
    }
    if(SymRequiresDtoring(sym)) {
        currInit->dtor_reqd=1;
        if(CgTypeSize(sym->sym_type)==0) currInit->no_size=1;
    }
    currInit->base_type=TypeNeedsCtor(sym->sym_type);
    if(currInit->base_type!=NULL) {
        currInit->ctor_reqd=1;
        currInit->size_type=CgMemorySize(currInit->base_type);
    }
    if(SymIsConstant(sym)) currInit->const_int=1;
    if(ScopeType(SymScope(sym),3)) {
        currInit->bracketed=1;
        DgInitBegin(sym);
        currInit->location=SymIsAutomatic(sym)?DL_INTERNAL_AUTO:DL_INTERNAL_STATIC;
    } else if(SymIsStaticData(sym)) {
        currInit->bracketed=1;
        DgInitBegin(sym);
        currInit->location=DL_EXTERNAL_STATIC;
    } else currInit->location=DL_EXTERNAL_PUBLIC;
    dataInitPushStack(DE_ROOT_TYPE,NULL);
    if(currInit->state!=DS_ERROR) {
        currInit->state=DS_EXPRESSION;
        if(initializer && currInit->nest->target!=DT_SCALAR) {
            currInit->state=DS_OPEN_BRACE;
            if(currInit->location!=DL_INTERNAL_AUTO) dataInitLabel();
        }
    }
}
void DataInitStart(INITIALIZE_DATA *data,DECL_INFO *info)
{ dataInitStartLocal(data,info,1); }
int dataInitCheckHugeAlign(TYPE type)
{ (void)type; STUB_TRAP(); return 0; }
void *StackPush(void **head,void *item);
TYPE TypedefModifierRemoveOnly(TYPE type);
void dataInitComputeTarget(INITIALIZE_INFO *info);
unsigned dataInitFieldSize(INITIALIZE_INFO *info);
void dtorableObjectBeg(INITIALIZE_INFO *info);
void dataInitPushStack(unsigned char entry,INITIALIZE_INFO *previous)
{
    INITIALIZE_INFO *top=CarveAlloc(carveNEST);
    StackPush((void **)&currInit->nest,top);
    top->offset=0;
    top->previous=previous;
    top->entry=entry;
    switch(entry) {
    case DE_BRACE:
        top->type=NULL;
        top->base=0;
        top->mem_size=0;
        break;
    case DE_ROOT_TYPE:
        top->type=currInit->sym_type;
        dataInitComputeTarget(top);
        top->mem_size=CgMemorySize(currInit->sym_type);
        top->base=0;
        break;
    case DE_TYPE:
        if(previous->entry==DE_ROOT_TYPE) {
            top->type=previous->type;
            top->target=previous->target;
            top->u=previous->u;
            top->mem_size=previous->mem_size;
        } else if(previous->target==DT_CLASS) {
            top->type=previous->u.c.curr->sym_type;
            dataInitComputeTarget(top);
            top->mem_size=dataInitFieldSize(previous);
        } else {
            top->type=TypedefModifierRemoveOnly(previous->type)->of;
            dataInitComputeTarget(top);
            top->mem_size=CgMemorySize(top->type);
        }
        top->base=previous->base+previous->offset;
        break;
    default: STUB_TRAP(); return;
    }
    dtorableObjectBeg(top);
}
int IsCgTypeAggregate(TYPE type,int string);
SYMBOL ScopeOrderedStart(SCOPE scope);
SYMBOL dataInitScopeOrderedNext(SYMBOL stop,SYMBOL current);
void dataInitComputeTarget(INITIALIZE_INFO *info)
{
    TYPE type;
    if(info->entry==DE_BRACE) return;
    type=TypedefModifierRemoveOnly(info->type);
    if(type->id==TYP_CLASS && type->u.c.info->corrupted) {
        info->target=DT_ERROR;
        currInit->state=DS_ERROR;
    } else if(IsCgTypeAggregate(type,0)) {
        switch(type->id) {
        case TYP_CLASS:
            info->target=DT_CLASS;
            info->u.c.stop=ScopeOrderedStart(type->u.c.scope);
            info->u.c.curr=dataInitScopeOrderedNext(info->u.c.stop,NULL);
            break;
        case TYP_ARRAY:
            info->target=DT_ARRAY;
            info->u.a.index=0;
            break;
        case TYP_BITFIELD:
            info->target=DT_BITFIELD;
            info->u.b.type=type->of;
            info->u.b.mask=0;
            break;
        }
    } else {
        info->target=DT_SCALAR;
        info->u.s.bitf=info->previous!=NULL && info->previous->target==DT_BITFIELD;
    }
}
SYMBOL dataInitScopeOrderedNext(SYMBOL stop,SYMBOL current)
{ (void)stop; (void)current; STUB_TRAP(); return NULL; }

unsigned dataInitFieldSize(INITIALIZE_INFO *info)
{ (void)info; STUB_TRAP(); return 0; }
int TypeRequiresDtoring(TYPE type);
CGFILE *dataInitCodeFileOpen(int need_code,SCOPE *saved_scope);
void dataInitCodeFileClose(CGFILE *file,SCOPE saved_scope);
TYPE_SIG *dataInitTypeSigFind(TYPE type,unsigned char access);
PTREE DataDtorObjPush(PTREE expr,TYPE type,SYMBOL sym,unsigned offset);
void CgFrontCode(unsigned opcode);
TYPE dtorableObjectType(INITIALIZE_INFO *info)
{
    if(info!=NULL && (info->entry==DE_ROOT_TYPE || info->entry==DE_TYPE) &&
       TypeRequiresDtoring(info->type)) return info->type;
    return NULL;
}
SYMBOL dtorableObjectInitSymbol(void)
{ return currInit->auto_static?currInit->auto_sym:currInit->sym; }
void dtorableObjectBeg(INITIALIZE_INFO *info)
{
    TYPE type=dtorableObjectType(info);
    CGFILE *file;
    SCOPE saved_scope;
    SYMBOL sym;
    if(type!=NULL) {
        file=dataInitCodeFileOpen(1,&saved_scope);
        sym=dtorableObjectInitSymbol();
        if(info->entry==DE_ROOT_TYPE) CgFrontCode(116);
        dataInitTypeSigFind(type,16);
        DataDtorObjPush(NULL,type,sym,info->base);
        dataInitCodeFileClose(file,saved_scope);
    }
}
CGFILE *CgFrontModInitOpen(SCOPE *saved_scope);
void CgFrontModInitUsed(void);
void CgFrontModInitClose(SCOPE saved_scope,CGFILE *file);
CGFILE *dataInitCodeFileOpen(int need_code,SCOPE *saved_scope)
{
    /* Old internal/ignored paths return an uninitialized saved-file value;
     * its matching close ignores it. Keep that undefined value zero here. */
    CGFILE *file=NULL;
    *saved_scope=NULL;
    switch(currInit->location) {
    case DL_INTERNAL_STATIC:
        if(need_code && currInit->dtor_reqd) CgFrontModInitUsed();
        break;
    case DL_EXTERNAL_STATIC: case DL_EXTERNAL_PUBLIC:
        file=CgFrontModInitOpen(saved_scope);
        if(need_code) CgFrontModInitUsed();
        break;
    }
    return file;
}
void dataInitCodeFileClose(CGFILE *file,SCOPE saved_scope)
{
    if(currInit->location==DL_EXTERNAL_STATIC || currInit->location==DL_EXTERNAL_PUBLIC)
        CgFrontModInitClose(saved_scope,file);
}
void ModuleInitResumeScoped(SCOPE scope);
int SymIsStaticMember(SYMBOL sym);
/* Actual1994 saves the opened code-file and scope explicitly. */
TYPE_SIG *dataInitTypeSigFind(TYPE type,unsigned char access)
{
    TYPE_SIG *sig;
    CGFILE *file;
    SCOPE scope;
    int errors;
    file=dataInitCodeFileOpen(0,&scope);
    if(SymIsStaticMember(currInit->sym))
        ModuleInitResumeScoped(SymScope(currInit->sym));
    sig=TypeSigFind(access,type,NULL,&errors);
    TypeSigReferenced(sig);
    dataInitCodeFileClose(file,scope);
    return sig;
}


void DgSymbolLabel(SYMBOL sym);
void dataInitLabel(void)
{
    if(!currInit->label_done) {
        currInit->label_done=1;
        DgSymbolLabel(currInit->sym);
    }
}


void DataInitExpr(PTREE expr);
void dataInitPushNest(int brace)
{
    INITIALIZE_INFO *old=currInit->nest;
    if(brace) dataInitPushStack(DE_BRACE,NULL);
    dataInitPushStack(DE_TYPE,old);
}
void DataInitSimple(PTREE expr)
{
    if(currInit==NULL) return;
    if(currInit->state>=DS_COMPLETED && currInit->state<=DS_IGNORE)
        PTreeFreeSubtrees(expr);
    else {
        dataInitPushNest(0);
        DataInitExpr(expr);
    }
}
int IsStringConstant(PTREE expr,int *multi_line);
int TypeIsCharString(TYPE type);
void dataInitStringExpr(PTREE expr);
int dataInitAnalyseExpr(PTREE *expr);
void dataInitStashExpr(PTREE expr);
void dataInitPopNest(void);
void dataInitCheckStringConcat(int string,int multi_line,PTREE expr);
void dataInitStashBitfield(PTREE expr);
int dataInitIsFull(INITIALIZE_INFO *info);
void dataInitUpdateState(void);
void DataInitExpr(PTREE expr)
{
    int string,multi_line;
    if(currInit==NULL) return;
    string=IsStringConstant(expr,&multi_line);
    switch(currInit->state) {
    case DS_CLOSE_BRACE: case DS_FINISH:
        CErr(279);
        currInit->state=DS_ERROR;
    case DS_ERROR: case DS_ABANDONED: case DS_COMPLETED: case DS_IGNORE:
        PTreeFreeSubtrees(expr);
        break;
    case DS_OPEN_BRACE:
        if(string && TypeIsCharString(currInit->nest->type)) dataInitStringExpr(expr);
        else if(currInit->nest->target==DT_CLASS) {
            if(dataInitAnalyseExpr(&expr)) {
                dataInitStashExpr(expr);
                dataInitPopNest();
                dataInitUpdateState();
            } else {
                currInit->state=DS_ERROR;
                PTreeFreeSubtrees(expr);
            }
        } else {
            CErr(281,"{");
            currInit->state=DS_ERROR;
            PTreeFreeSubtrees(expr);
        }
        break;
    case DS_EXPRESSION:
        while(IsCgTypeAggregate(currInit->nest->type,string)) dataInitPushNest(0);
        if(string && TypeIsCharString(currInit->nest->type)) dataInitStringExpr(expr);
        else if(dataInitAnalyseExpr(&expr)) {
            dataInitCheckStringConcat(string,multi_line,expr);
            if(currInit->nest->u.s.bitf) dataInitStashBitfield(expr);
            else dataInitStashExpr(expr);
            while(dataInitIsFull(currInit->nest)) dataInitPopNest();
            dataInitUpdateState();
        }
        break;
    }
}
void dataInitStringExpr(PTREE expr) { (void)expr; STUB_TRAP(); }
static PTREE initExpr;
PTREE dataInitLeftSide(INITIALIZE_INFO *info);
void ModuleInitResumeScoped(SCOPE scope);
int SymIsStaticMember(SYMBOL sym);
PTREE AnalyseInitExpr(PTREE expr,int static_init);
PTREE PTreeBinary(unsigned char op,PTREE left,PTREE right);
PTREE PTreeCopySrcLocation(PTREE target,PTREE source);
PTREE dataInitCopyLocn(PTREE node)
{
    PTreeCopySrcLocation(node,initExpr);
    return node;
}
int dataInitCheckAnalyse(PTREE *pexpr)
{
    PTREE expr=*pexpr;
    int ret=1;
    if(expr->op==0 || expr->cgop!=94) {
        PTreeFreeSubtrees(expr);
        currInit->state=DS_ERROR;
        ret=0;
    }
    *pexpr=expr;
    return ret;
}
int dataInitAnalyseExpr(PTREE *pexpr)
{
    PTREE expr=*pexpr,left=dataInitLeftSide(currInit->nest);
    CGFILE *file;
    SCOPE saved_scope;
    int ret;
    initExpr=expr;
    PTreeTraversePostfix(left,dataInitCopyLocn);
    expr=PTreeBinary(72,left,expr);
    switch(currInit->location) {
    case DL_INTERNAL_AUTO: expr=AnalyseInitExpr(expr,0); break;
    case DL_INTERNAL_STATIC: expr=AnalyseInitExpr(expr,1); break;
    case DL_EXTERNAL_STATIC: case DL_EXTERNAL_PUBLIC:
        file=dataInitCodeFileOpen(0,&saved_scope);
        if(SymIsStaticMember(currInit->sym)) ModuleInitResumeScoped(SymScope(currInit->sym));
        expr=AnalyseInitExpr(expr,1);
        dataInitCodeFileClose(file,saved_scope);
        break;
    }
    ret=dataInitCheckAnalyse(&expr);
    *pexpr=expr;
    return ret;
}
PTREE PTreeIdSym(SYMBOL sym);
PTREE dataInitLeftSide(INITIALIZE_INFO *info)
{
    PTREE tree;
    if(info->previous!=NULL) tree=dataInitLeftSide(info->previous);
    else if(info->entry!=DE_ROOT_TYPE) { STUB_TRAP(); return NULL; }
    if(info->entry==DE_ROOT_TYPE) tree=PTreeIdSym(dtorableObjectInitSymbol());
    else if(info!=currInit->nest) {
        switch(info->target) {
        case DT_ARRAY:
            tree=PTreeBinary(24,tree,PTreeIntConstant(info->u.a.index,TYP_UINT));
            break;
        case DT_CLASS:
            tree=PTreeBinary(48,tree,PTreeIdSym(info->u.c.curr));
            break;
        }
    }
    return tree;
}

int IsLinkerConstant(PTREE tree,PTREE *ctree,unsigned *offset);
int isDataInitConstant(PTREE tree,PTREE *ctree,unsigned *offset)
{
    PTREE right;
    tree=PTreeOpLeft(tree);
    if(tree->cgop!=72) return 0;
    right=tree->u.subtree[1];
    if(right->flags&1) return 0;
    return IsLinkerConstant(right,ctree,offset);
}
void DgPadBytes(unsigned size);
void DgStoreConstScalar(PTREE expr,TYPE type,SYMBOL sym);
int DgStoreScalar(PTREE expr,unsigned offset,TYPE type);
void NodeFreeSearchResult(PTREE expr);
unsigned StaticInitFuncBeg(void) { STUB_TRAP(); return 0; }
PTREE IcEmitExpr(PTREE expr);
void dataInitEmitExpr(PTREE expr)
{
    SCOPE saved_scope;
    CGFILE *saved_file;
    currInit->emit_code=1;
    saved_file=dataInitCodeFileOpen(1,&saved_scope);
    if(currInit->location==DL_INTERNAL_STATIC && !currInit->once_only) {
        currInit->once_only=1;
        currInit->once_only_label=StaticInitFuncBeg();
    }
    IcEmitExpr(expr);
    dataInitCodeFileClose(saved_file,saved_scope);
}
void *RingPop(void **ring);
void dataInitRunTimeCallHuge(unsigned start,unsigned size)
{ (void)start; (void)size; STUB_TRAP(); }
void dataInitRunTimeCall(unsigned start,unsigned size)
{ (void)start; (void)size; STUB_TRAP(); }
void dataInitFlushQueue(void)
{
    unsigned start,size;
    INITIALIZE_QUEUE *nextq=RingPop((void **)&currInit->queue);
    while(nextq!=NULL && currInit->state!=DS_ERROR) {
        start=nextq->start;
        size=nextq->size;
        CarveFree(carveQUEUE,nextq);
        nextq=RingPop((void **)&currInit->queue);
        while(nextq!=NULL && nextq->start==start+size) {
            size+=nextq->size;
            CarveFree(carveQUEUE,nextq);
            nextq=RingPop((void **)&currInit->queue);
        }
        if(currInit->huge_sym) dataInitRunTimeCallHuge(start,size);
        else dataInitRunTimeCall(start,size);
    }
}
void dataInitCheckHugeSegment(unsigned offset) { (void)offset; STUB_TRAP(); }
TYPE ArrayBaseType(TYPE type);
TYPE StructType(TYPE type);
TYPE ArrayType(TYPE type);
TYPE arrayBaseStructType(TYPE type) { return StructType(ArrayBaseType(type)); }
void IcEmitDtorSym(SYMBOL sym);
void emitDtorInitSymbol(void)
{
    SYMBOL sym;
    if (!currInit->dtor_done) {
        sym=dtorableObjectInitSymbol();
        currInit->dtor_done=1;
        CgFrontCodePtr(117,sym);
        CgFrontCode(26);
        IcEmitDtorSym(sym);
    }
}
PTREE NodeIc(unsigned opcode);
PTREE NodeAddSideEffect(PTREE effect,PTREE expr);
PTREE NodeDtorInitExpr(PTREE expr,SYMBOL sym);
PTREE emitDtorInitExpr(PTREE expr)
{
    SYMBOL sym;
    PTREE done,node;
    if (!currInit->dtor_done) {
        sym=dtorableObjectInitSymbol();
        currInit->dtor_done=1;
        if (NodeIsUnaryOp(expr,94)) {
            done=expr;
            expr=expr->u.subtree[0];
        } else done=NULL;
        if (currInit->nest->entry==DE_ROOT_TYPE) {
            /* Actual old IC_INIT_SYM_END node carries the owner symbol;
             * the later Ptd scope-call optimization is absent. */
            node=NodeIc(117);
            node->u.ic.value=(unsigned)sym;
            expr=NodeAddSideEffect(node,expr);
        }
        expr=NodeDtorInitExpr(expr,sym);
        if (done) {
            done->u.subtree[0]=expr;
            expr=done;
        }
    }
    return expr;
}
PTREE DataDtorCompArrEl(PTREE expr,unsigned index)
{ (void)expr; (void)index; STUB_TRAP(); return NULL; }
PTREE DataDtorCompClass(PTREE expr,long offset);
PTREE dtorableObjectCtored(INITIALIZE_INFO *curr,PTREE expr,int index_updated)
{
    INITIALIZE_INFO *info=curr->previous,*prev;
    SCOPE saved_scope;
    CGFILE *saved_file;
    TYPE type;
    if(info==NULL) {
        type=dtorableObjectType(curr);
        if(type!=NULL) {
            saved_file=dataInitCodeFileOpen(1,&saved_scope);
            if(expr==NULL) emitDtorInitSymbol();
            else expr=emitDtorInitExpr(expr);
            dataInitCodeFileClose(saved_file,saved_scope);
        }
    } else {
        type=dtorableObjectType(info);
        if(type!=NULL) {
            if(info->target==DT_ARRAY) {
                unsigned index=info->u.a.index;
                TYPE eltype=arrayBaseStructType(info->type),artype;
                for(prev=info;;) {
                    prev=prev->previous;
                    if(prev==NULL || prev->target!=DT_ARRAY) break;
                    artype=ArrayType(prev->type);
                    if(eltype!=arrayBaseStructType(artype)) break;
                    index+=prev->u.a.index*artype->u.a.array_size;
                }
                if(index_updated) --index;
                saved_file=dataInitCodeFileOpen(1,&saved_scope);
                expr=DataDtorCompArrEl(expr,index);
                dataInitCodeFileClose(saved_file,saved_scope);
            } else {
                for(prev=curr;prev!=NULL && prev->type==curr->type;
                    prev=prev->previous);
                if(prev!=NULL) {
                    saved_file=dataInitCodeFileOpen(1,&saved_scope);
                    expr=DataDtorCompClass(expr,info->offset);
                    dataInitCodeFileClose(saved_file,saved_scope);
                }
            }
        }
    }
    return expr;
}
void dataInitStashExpr(PTREE expr)
{
    int constant;
    unsigned size,offset;
    PTREE cexpr;
    TYPE type=currInit->nest->type;
    size=CgMemorySize(type);
    constant=isDataInitConstant(expr,&cexpr,&offset);
    switch(currInit->location) {
    case DL_INTERNAL_AUTO:
        if(currInit->auto_static) {
            if(constant) {
                currInit->all_zero &= DgStoreScalar(cexpr,offset,type);
                NodeFreeSearchResult(cexpr);
                PTreeFreeSubtrees(expr);
            } else {
                DgPadBytes(size);
                dataInitFlushQueue();
                dtorableObjectCtored(currInit->nest,expr,0);
                dataInitEmitExpr(expr);
            }
        } else {
            if(constant && currInit->const_int)
                DgStoreConstScalar(cexpr,type,currInit->sym);
            dataInitFlushQueue();
            dtorableObjectCtored(currInit->nest,expr,0);
            dataInitEmitExpr(expr);
        }
        break;
    case DL_INTERNAL_STATIC: case DL_EXTERNAL_STATIC: case DL_EXTERNAL_PUBLIC:
        dataInitLabel();
        if(currInit->huge_sym)
            dataInitCheckHugeSegment(currInit->nest->base+currInit->nest->offset);
        if(constant) {
            if(currInit->const_int) DgStoreConstScalar(cexpr,type,currInit->sym);
            currInit->all_zero &= DgStoreScalar(cexpr,offset,type);
            NodeFreeSearchResult(cexpr);
            PTreeFreeSubtrees(expr);
            dtorableObjectCtored(currInit->nest,NULL,0);
        } else {
            DgPadBytes(size);
            dataInitFlushQueue();
            dtorableObjectCtored(currInit->nest,expr,0);
            dataInitEmitExpr(expr);
        }
        break;
    }
    currInit->nest->offset+=size;
}
void dataInitEnQueue(unsigned offset,unsigned size)
{ (void)offset; (void)size; STUB_TRAP(); }
void dataInitPadOutHuge(INITIALIZE_INFO *info) { (void)info; STUB_TRAP(); }
TYPE dataInitAdvanceField(INITIALIZE_INFO *info)
{ (void)info; STUB_TRAP(); return NULL; }
TYPE DupArrayTypeForArray(TYPE type,unsigned count);
int DgStoreBitfield(TYPE type,unsigned mask);
void dataInitPadOut(INITIALIZE_INFO *top)
{
    unsigned diff;
    switch(top->target) {
    case DT_ARRAY:
        if(top->mem_size==0) {
            if(currInit->auto_static)
                currInit->auto_sym->sym_type=DupArrayTypeForArray(
                    currInit->auto_sym->sym_type,top->u.a.index);
            currInit->sym->sym_type=DupArrayTypeForArray(
                currInit->sym->sym_type,top->u.a.index);
            top->mem_size=top->offset;
            break;
        }
        /* Arrays of known size share the scalar/class fill path. */
    case DT_SCALAR: case DT_CLASS:
        if(top->mem_size<top->offset) top->offset=top->mem_size;
        diff=top->mem_size-top->offset;
        if(diff!=0) {
            if(currInit->ctor_reqd) dataInitEnQueue(top->base+top->offset,diff);
            if(currInit->need_storage) {
                if(currInit->huge_sym) dataInitPadOutHuge(top);
                else DgPadBytes(diff);
            }
        }
        top->offset=top->mem_size;
        break;
    case DT_BITFIELD:
        if(currInit->huge_sym) dataInitCheckHugeSegment(top->base+top->offset);
        currInit->all_zero &= DgStoreBitfield(top->u.b.type,top->u.b.mask);
        while(!dataInitIsFull(top)) top->type=dataInitAdvanceField(top->previous);
        /*1994 has no later padding after the bitfield word. */
        top->offset=top->mem_size;
        break;
    }
}
void dataInitUpdatePrevious(INITIALIZE_INFO *top)
{
    INITIALIZE_INFO *prev=top->previous;
    if(prev!=NULL && prev->entry!=DE_ROOT_TYPE) {
        if(!(top->target==DT_SCALAR && top->u.s.bitf==1))
            prev->offset+=top->mem_size;
        switch(prev->target) {
        case DT_ARRAY: ++prev->u.a.index; break;
        case DT_CLASS:
            if(top->target!=DT_BITFIELD) dataInitAdvanceField(prev);
            break;
        case DT_BITFIELD:
            prev->type=dataInitAdvanceField(prev->previous);
            break;
        }
    }
}
PTREE DataDtorObjPop(PTREE expr);
void dtorableObjectEnd(INITIALIZE_INFO *info)
{
    SCOPE saved_scope;
    CGFILE *saved_file;
    if(dtorableObjectType(info)!=NULL) {
        saved_file=dataInitCodeFileOpen(1,&saved_scope);
        DataDtorObjPop(NULL);
        if(info->entry==DE_ROOT_TYPE) emitDtorInitSymbol();
        dataInitCodeFileClose(saved_file,saved_scope);
    }
}
void *StackPop(void **stack);
void dataInitPopStack(void)
{
    INITIALIZE_INFO *nest;
    dtorableObjectEnd(currInit->nest);
    nest=StackPop((void **)&currInit->nest);
    CarveFree(carveNEST,nest);
}
void dataInitPopNest(void)
{
    if(currInit->nest->entry!=DE_BRACE) {
        dataInitPadOut(currInit->nest);
        dataInitUpdatePrevious(currInit->nest);
    }
    dataInitPopStack();
}
PTREE PTreeSetErrLoc(PTREE expr);
void dataInitCheckStringConcat(int string,int multi_line,PTREE expr)
{
    if(string && multi_line && currInit->nest->previous->target==DT_ARRAY) {
        PTreeSetErrLoc(expr);
        CErr1(438);
    }
}
void dataInitStashBitfield(PTREE expr) { (void)expr; STUB_TRAP(); }
int dataInitIsFull(INITIALIZE_INFO *info)
{
    int retn=0;
    if(info->entry!=DE_BRACE && info->entry!=DE_ROOT_TYPE) {
        switch(info->target) {
        case DT_SCALAR: retn=1; break;
        case DT_ARRAY: retn=info->offset==info->mem_size; break;
        case DT_CLASS: retn=info->u.c.curr==NULL; break;
        case DT_BITFIELD:
            retn=info->type==NULL || info->type->id!=TYP_BITFIELD ||
                info->type->u.b.field_start==0;
            break;
        }
    }
    return retn;
}
void dataInitUpdateState(void)
{
    switch(currInit->state) {
    case DS_ERROR: case DS_ABANDONED: case DS_IGNORE: break;
    default:
        if(currInit->nest->entry==DE_ROOT_TYPE) {
            dataInitFlushQueue();
            currInit->state=DS_FINISH;
        } else if(currInit->nest->entry==DE_BRACE)
            currInit->state=DS_CLOSE_BRACE;
        else currInit->state=DS_EXPRESSION;
    }
}



unsigned char dataInitSegIdControl(void)
{
    unsigned char control=0;
    if(currInit->all_zero) control|=1;
    if(currInit->emit_code) control|=2;
    return control;
}
void dataInitAbandon(void) { STUB_TRAP(); }
unsigned short CgSegIdData(SYMBOL sym,unsigned char control);
void DgInitDone(void);
int SymIsInitialized(SYMBOL sym);
int SymIsClass(SYMBOL sym);
int SymIsClassArray(SYMBOL sym);
SCOPE ModuleInitResume(void);
void ModuleInitRestore(SCOPE scope);
void LabelDeclInited(SYMBOL sym);
void LabelDeclInitedFileScope(SYMBOL sym);
void StaticInitFuncEnd(unsigned label) { (void)label; STUB_TRAP(); }
void DataInitFinish(INITIALIZE_DATA *init)
{
    SYMBOL sym;
    SCOPE saved_scope,scope;
    CGFILE *file;
    if(init!=currInit) return;
    switch(currInit->state) {
    case DS_EXPRESSION: case DS_OPEN_BRACE: case DS_CLOSE_BRACE: case DS_ERROR:
        dataInitAbandon();
        break;
    case DS_COMPLETED: case DS_ABANDONED: case DS_IGNORE:
        currInit=currInit->prev;
        break;
    case DS_FINISH:
        sym=currInit->sym;
        if(currInit->auto_static) {
            CgSegIdData(sym,dataInitSegIdControl());
            if(currInit->auto_type!=NULL)
                currInit->auto_type->u.a.array_size=CgTypeSize(sym->sym_type);
        }
        sym=currInit->dinfo->sym;
        file=dataInitCodeFileOpen(0,&saved_scope);
        if(currInit->bracketed) DgInitDone();
        if(currInit->emit_code) sym->flag|=1;
        else sym->flag&=~1;
        if(currInit->initted) sym->flag|=2;
        else sym->flag&=~2;
        switch(currInit->location) {
        case DL_INTERNAL_AUTO: case DL_INTERNAL_STATIC:
            if(SymIsInitialized(sym) || SymIsStaticData(sym) ||
               SymIsClass(sym) || SymIsClassArray(sym)) LabelDeclInited(sym);
            break;
        case DL_EXTERNAL_STATIC: case DL_EXTERNAL_PUBLIC:
            if(currInit->dtor_reqd) {
                scope=ModuleInitResume();
                LabelDeclInitedFileScope(sym);
                ModuleInitRestore(scope);
            }
            break;
        }
        sym->flag|=2;
        CgSegIdData(sym,dataInitSegIdControl());
        dataInitCodeFileClose(file,saved_scope);
        dataInitPopStack();
        if(currInit->once_only) StaticInitFuncEnd(currInit->once_only_label);
        currInit->state=DS_COMPLETED;
        currInit=currInit->prev;
        break;
    }
}

/* Initial datainit.c, actual2ff5e..2fff6: constructors use the same old
 * analysis entry as ordinary expressions, before the later split helper. */
void DataInitConstructorParms(PTREE expr)
{
    if(currInit==NULL) return;
    switch(currInit->state) {
    case DS_CLOSE_BRACE: case DS_FINISH:
        CErr(279);
        currInit->state=DS_ERROR;
        /* fall through */
    case DS_ERROR: case DS_ABANDONED: case DS_COMPLETED: case DS_IGNORE:
        PTreeFreeSubtrees(expr);
        break;
    case DS_OPEN_BRACE:
        if(currInit->nest->target!=DT_CLASS) {
            CErr(281,"{");
            currInit->state=DS_ERROR;
            PTreeFreeSubtrees(expr);
            break;
        }
        /* fall through */
    case DS_EXPRESSION:
        if(dataInitAnalyseExpr(&expr)) {
            dataInitStashExpr(expr);
            dataInitUpdateState();
        }
        break;
    }
}
