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


/* Adapted initial6b9cb443 analclss.c, with the actual1994 prologue
 * layout. The old40B state places its six-byte section position before the
 * iterator; it has no later cached this_type member. */
#include "compiler.h"
#include "types.h"
#include "scope.h"
#include "symbol.h"
#include "ptree.h"
#include "cgio.h"
#include "frontend.h"
#include "pending.h"
#include "cdopt.h"
#include "class.h"
#include "vftable.h"
#include "convdiag.h"
#include <recon.h>
#pragma pack(1)
typedef struct ctor_prologue {
    PTREE member_init,base_init;
    SCOPE scope;
    CGFILE_INS sect_locn;
    CDOPT_ITER *optiter;
    long orig_offset,comp_offset;
    PTREE comp_expr;
    TYPE comp_type;
    unsigned char comp_options;
    unsigned char cdopt:1,dtor_reqd:1,gen_copy:1,excepts:1,obj_dtor:1,
        have_cdopt:1,done_cdopt:1,reserved:1;
} CTOR_PROLOGUE;
#pragma pack()
typedef char CtorPrologueSize[(sizeof(CTOR_PROLOGUE)==40)?1:-1];
char *CppSpecialName(unsigned name);
void checkForGenCopy(CTOR_PROLOGUE *data)
{
    SEARCH_RESULT *result=ScopeFindNaked(CurrScope,CppSpecialName(2));
    if (result) { data->gen_copy=1; ScopeFreeResult(result); }
}
int ExceptionsEnabled(void);
void initCtorPrologue(CTOR_PROLOGUE *data,PTREE init,SCOPE scope)
{
    PTREE current,next,member,*insertion;
    data->member_init=NULL;
    data->base_init=NULL;
    data->scope=scope;
    data->gen_copy=0;
    data->excepts=ExceptionsEnabled()!=0;
    data->cdopt=0;
    data->have_cdopt=0;
    data->done_cdopt=0;
    checkForGenCopy(data);
    for (current=init;current;current=next) {
        next=current->u.subtree[0];
        member=current->u.subtree[1]->u.subtree[0];
        insertion=member->op==6 ? &data->member_init : &data->base_init;
        current->u.subtree[0]=*insertion;
        *insertion=current;
    }
}
int SymClassCorrupted(SYMBOL sym);
PTREE PTreeThis(void);
PTREE PTreeUnary(unsigned char operation,PTREE operand);
PTREE AnalyseReturnExpr(SYMBOL sym,PTREE expr);
PTREE IcEmitExpr(PTREE expr);
SYMBOL SymFunctionReturn(void);
int SymIsDtor(SYMBOL sym);
void CgFrontReturnSymbol(SYMBOL sym);
void returnThis(SYMBOL sym)
{
    PTREE expr=PTreeThis();
    if(!SymIsDtor(sym)) expr=PTreeUnary(40,expr);
    expr=AnalyseReturnExpr(sym,expr);
    IcEmitExpr(expr);
    CgFrontReturnSymbol(SymFunctionReturn());
}
void CtorEpilogue(SYMBOL sym)
{
    if(!SymClassCorrupted(sym)) returnThis(sym);
}
SCOPE SymScope(SYMBOL sym);
TYPE ScopeClass(SCOPE scope);
int TypeReallyDtorable(TYPE type);
int TypeHasNumArgs(TYPE type,unsigned count);
int ScopeHasVirtualBases(SCOPE scope);
void ScopeWalkDirectBases(SCOPE scope,void (*callback)(void *,void *),void *data);
void ctorPrologueBaseGen(void *base,void *data)
{ (void)base; (void)data; STUB_TRAP(); }
PTREE PTreeFree(PTREE tree);
PTREE extractMemberInit(CTOR_PROLOGUE *data,char *name)
{
    PTREE *link=&data->member_init,current,next,init,member;
    for (current=*link;current;current=next) {
        next=current->u.subtree[0];
        init=current->u.subtree[1];
        member=init->u.subtree[0];
        if (member->u.id.name==name) {
            *link=next;
            PTreeFree(current);
            return init;
        }
        link=&current->u.subtree[0];
    }
    return NULL;
}
int SymIsAnonymousMember(SYMBOL sym);
int SymIsThisDataMember(SYMBOL sym);
TYPE ArrayType(TYPE type);
TYPE ReferenceType(TYPE type);
PTREE getRefSymFromFirstParm(SYMBOL sym,unsigned char flags)
{ (void)sym; (void)flags; STUB_TRAP(); return NULL; }
PTREE NodeUnaryCopy(unsigned char op,PTREE expr);
PTREE NodeArg(PTREE expr);
void PTreeErrorExprSym(PTREE expr,unsigned error,SYMBOL sym);
int TypeRequiresCtoring(TYPE type);
TYPE ArrayType(TYPE type);
PTREE NodeOffset(long offset);
PTREE NodeThis(void);
PTREE NodeBinary(unsigned char op,PTREE left,PTREE right);
PTREE addOffset(PTREE node,long offset,TYPE type)
{
    node=NodeBinary(48,node,NodeOffset(offset));
    node->type=type;
    node->flags|=2;
    return node;
}
PTREE addOffsetToThis(long offset,TYPE type)
{ return addOffset(NodeThis(),offset,type); }
SCOPE TypeScope(TYPE type);
int TypeNeedsCtor(TYPE type);
TYPE StructType(TYPE type);
TYPE TypedefModifierRemoveOnly(TYPE type);
void NodeFreeDupedExpr(PTREE expr);
PTREE AnalyseRawExpr(PTREE expr);
unsigned AnalyseTypeCtor(SCOPE access,TYPE type,unsigned control,SYMBOL *ctor,PTREE *expr);
void ConversionTypesSet(TYPE first,TYPE second);
PTREE PTreeExtractLocn(PTREE expr,CGIO_LOCATION *location);
PTREE EffectCtor(PTREE expr,SYMBOL ctor,TYPE type,PTREE target,CGIO_LOCATION *location,unsigned control);
/* Actual old prologue diagnosis: only impossible-conversion is overridden. */
static CNV_DIAG diagProInit={340,0,0,0,0};
PTREE ctorPrologueInit(PTREE target,PTREE expr,TYPE type,unsigned char options,CTOR_PROLOGUE *data)
{
    SCOPE access=data->scope;
    TYPE modified_type=type;
    PTREE right;
    SYMBOL ctor;
    unsigned result;
    CGIO_LOCATION location;
    type=TypedefModifierRemoveOnly(type);
    if (!expr) {
        if (!StructType(type) || !TypeNeedsCtor(type)) {
            PTreeFreeSubtrees(target);
            return NULL;
        }
    } else if (options&2) {
        right=expr->u.subtree[1];
        expr->u.subtree[1]=NULL;
        NodeFreeDupedExpr(expr);
        expr=AnalyseRawExpr(right);
        if (expr && expr->op==0) {
            NodeFreeDupedExpr(expr);
            PTreeFreeSubtrees(target);
            return NULL;
        }
    }
    if (options&4) access=TypeScope(type);
    else if (options&1) access=NULL;
    result=AnalyseTypeCtor(access,type,1,&ctor,&expr);
    if (result) {
        if (access && access->id==1) ConversionTypesSet(ScopeClass(access),modified_type);
        if (ConversionDiagnose(result,expr,&diagProInit)) {
            NodeFreeDupedExpr(expr);
            PTreeFreeSubtrees(target);
            return NULL;
        }
    }
    if (target) PTreeExtractLocn(target,&location);
    else PTreeExtractLocn(expr,&location);
    return EffectCtor(expr,ctor,type,target,&location,(options&1)?1:0);
}
PTREE ctorPrologueArray(PTREE target,PTREE expr,TYPE type,CTOR_PROLOGUE *data)
{ (void)target; (void)expr; (void)type; (void)data; STUB_TRAP(); return NULL; }
TYPE TypedefModifierRemove(TYPE type);
TYPE TypeThis(void);
SYMBOL RoDtorFindType(TYPE type);
PTREE setThisFromOffset(PTREE expr,TYPE type,long offset)
{
    PTREE offset_node;
    if(!expr || expr->op==0 || expr->type->id==TYP_VOID ||
       TypedefModifierRemove(type)->id==TYP_BITFIELD) return expr;
    offset_node=NodeOffset(offset);
    offset_node->cgop=205;
    expr=NodeBinary(120,expr,offset_node);
    expr->type=TypeThis();
    return expr;
}
int TypeExactDtorable(TYPE type);
void CgFrontCodePtr(unsigned char opcode,void *value);
PTREE DataDtorCompClass(PTREE expr,long offset)
{ (void)expr; (void)offset; STUB_TRAP(); return NULL; }
PTREE ctorOfDtorable(PTREE expr,TYPE type,long offset)
{
    if((!expr || expr->op!=0) && TypeExactDtorable(type)) {
        CgFrontCodePtr(71,RoDtorFindType(type));
        expr=DataDtorCompClass(expr,offset);
    }
    return expr;
}
void EmitAnalysedStmt(PTREE expr);
void ctorGenComponent(CTOR_PROLOGUE *data,TYPE type,long offset,PTREE expr)
{
    PTREE target;
    if (expr || TypeRequiresCtoring(type)) {
        target=addOffsetToThis(offset,type);
        if (!ArrayType(type)) expr=ctorPrologueInit(target,expr,type,data->comp_options,data);
        else expr=ctorPrologueArray(target,expr,type,data);
        expr=setThisFromOffset(expr,type,offset);
    }
    expr=ctorOfDtorable(expr,type,offset);
    EmitAnalysedStmt(expr);
}
void ctorPrologueComponents(CTOR_PROLOGUE *data)
{
    int have_cdopt;
    if (!data->have_cdopt && !data->done_cdopt) {
        if (!CDoptIterNextComp(data->optiter)) data->done_cdopt=1;
        else data->have_cdopt=1;
    }
    if (data->done_cdopt) have_cdopt=0;
    else if (data->comp_offset==CDoptIterOffsetExact(data->optiter)) {
        have_cdopt=!data->gen_copy;
        data->have_cdopt=0;
    } else have_cdopt=0;
    if (data->comp_expr)
        ctorGenComponent(data,data->comp_type,data->comp_offset,data->comp_expr);
    else if (have_cdopt) {
        /* The old default-component action dispatcher remains pending. */
        STUB_TRAP();
    }
}
void ctorPrologueMember(SYMBOL sym,void *context)
{
    CTOR_PROLOGUE *data=context;
    PTREE expr;
    if (data->gen_copy && SymIsAnonymousMember(sym)) return;
    if (!SymIsThisDataMember(sym)) return;
    data->comp_type=sym->sym_type;
    data->comp_offset=sym->u.sval;
    data->comp_expr=extractMemberInit(data,sym->name->name);
    if (data->comp_expr) {
        if (ArrayType(sym->sym_type)) {
            PTreeErrorExprSym(data->comp_expr,709,sym);
            PTreeFreeSubtrees(data->comp_expr);
            return;
        }
        data->comp_options=3; /* old CI_EXACT | CI_MEM_INIT */
    } else {
        if (data->gen_copy) {
            expr=getRefSymFromFirstParm(sym,0);
            if (ReferenceType(sym->sym_type)) expr=NodeUnaryCopy(92,expr);
            data->comp_expr=NodeArg(expr);
        }
        data->comp_options=1; /* old CI_EXACT, before later CI_NULL */
    }
    ctorPrologueComponents(data);
}
SYMBOL ScopeOrderedStart(SCOPE scope);
SYMBOL ScopeOrderedNext(SYMBOL stop,SYMBOL current);
void memberWalkWithData(SCOPE scope,void (*callback)(SYMBOL,void *),void *data)
{
    SYMBOL current=NULL,stop=ScopeOrderedStart(scope);
    for (;;) {
        current=ScopeOrderedNext(stop,current);
        if (!current) break;
        callback(current,data);
    }
}
int ScopeHasVirtualFunctions(SCOPE scope);
int TypeCtorDispRequired(TYPE host,TYPE base);
TYPE TypePtrToChar(void);
TYPE TypePtrToUInt(void);
TYPE TypePtrToConstUInt(void);
TYPE MakeVBTableFieldType(int add_consts);
TYPE MakeVFTableFieldType(int add_consts);
TYPE MakePointerTo(TYPE type);
TYPE GetBasicType(type_id id);
SYMBOL SymMakeDummy(TYPE type,char **name);
PTREE PTreeThis(void);
PTREE PTreeType(TYPE type);
PTREE PTreeUnary(unsigned char op,PTREE expr);
PTREE PTreeBinary(unsigned char op,PTREE left,PTREE right);
PTREE PTreeUIntConstant(unsigned value,type_id id);
void EmitStmtExpr(PTREE expr);
/* This1994 form predates IC_GEN_CTOR_DISP. It emits ordinary assignments:
 * load the virtual-base table, obtain each base offset, then store the
 * construction displacement in the DWORD immediately preceding that base. */
void genCtorDispInit(SCOPE scope)
{
    TYPE host=ScopeClass(scope),char_pointer;
    struct class_info *info=host->u.c.info;
    struct base_class *base,*last;
    char *table_name,*offset_name;
    SYMBOL table_symbol,offset_symbol;
    PTREE expr,address,offset;
    int needed=0;
    if(!info->last_vbase) return;
    last=info->bases;
    if(last) {
        base=last;
        do {
            base=base->next;
            if((base->flag&0x18) && TypeCtorDispRequired(host,base->type)) {
                needed=1;
                break;
            }
        } while(base!=last);
    }
    if(!needed) return;
    table_symbol=SymMakeDummy(TypePtrToConstUInt(),&table_name);
    ScopeInsert(CurrScope,table_symbol,table_name);
    offset_symbol=SymMakeDummy(GetBasicType(TYP_UINT),&offset_name);
    ScopeInsert(CurrScope,offset_symbol,offset_name);
    char_pointer=TypePtrToChar();
    address=PTreeBinary(57,PTreeType(char_pointer),PTreeThis());
    address=PTreeBinary(0,address,PTreeUIntConstant(info->vb_offset,TYP_UINT));
    address=PTreeBinary(57,PTreeType(MakePointerTo(MakeVBTableFieldType(0))),address);
    expr=PTreeBinary(13,PTreeId(table_name),PTreeUnary(40,address));
    EmitStmtExpr(expr);
    if(last) {
        base=last;
        do {
            base=base->next;
            if((base->flag&0x18) && TypeCtorDispRequired(host,base->type)) {
                offset=PTreeBinary(24,PTreeId(table_name),
                    PTreeUIntConstant(base->vb_index,TYP_UINT));
                offset=PTreeBinary(0,offset,PTreeUIntConstant(info->vb_offset,TYP_UINT));
                EmitStmtExpr(PTreeBinary(13,PTreeId(offset_name),offset));
                offset=PTreeBinary(1,PTreeId(offset_name),PTreeUIntConstant(4,TYP_UINT));
                address=PTreeBinary(57,PTreeType(char_pointer),PTreeThis());
                address=PTreeBinary(0,address,offset);
                address=PTreeBinary(57,PTreeType(TypePtrToUInt()),address);
                expr=PTreeBinary(13,PTreeUnary(40,address),PTreeId(offset_name));
                EmitStmtExpr(expr);
            }
        } while(base!=last);
    }
}
CLASS_VFTABLE *ScopeCollectVFTable(SCOPE scope);
SYMBOL MakeVFTableSym(SCOPE scope,unsigned count,long offset);
/* The 1994 compiler builds assignments rather than the later vptr IC
 * instructions. The non-exact path first loads the actual virtual-base
 * table and saves its computed displacement in ordinary local symbols. */
void emitVPtrInit(CLASS_TABLE *table,SYMBOL sym,unsigned virtual_base)
{
    PTREE rhs=PTreeId(sym->name->name),address,expr,offset;
    TYPE vptr_type,char_pointer,left_type,vb_type;
    SYMBOL table_symbol,offset_symbol;
    char *table_name,*offset_name;
    if(virtual_base) vptr_type=MakeVBTableFieldType(0);
    else vptr_type=MakeVFTableFieldType(0);
    char_pointer=TypePtrToChar();
    left_type=MakePointerTo(vptr_type);
    if(virtual_base || table->exact_delta==table->delta) {
        address=PTreeBinary(57,PTreeType(char_pointer),PTreeThis());
        address=PTreeBinary(0,address,PTreeUIntConstant(table->exact_delta,TYP_UINT));
    } else {
        vb_type=MakePointerTo(MakeVBTableFieldType(0));
        table_symbol=SymMakeDummy(TypePtrToConstUInt(),&table_name);
        ScopeInsert(CurrScope,table_symbol,table_name);
        offset_symbol=SymMakeDummy(GetBasicType(TYP_UINT),&offset_name);
        ScopeInsert(CurrScope,offset_symbol,offset_name);
        char_pointer=TypePtrToChar();
        address=PTreeBinary(57,PTreeType(char_pointer),PTreeThis());
        address=PTreeBinary(0,address,PTreeUIntConstant(table->vb_offset,TYP_UINT));
        address=PTreeBinary(57,PTreeType(vb_type),address);
        expr=PTreeBinary(13,PTreeId(table_name),PTreeUnary(40,address));
        EmitStmtExpr(expr);
        offset=PTreeBinary(24,PTreeId(table_name),PTreeUIntConstant(table->vb_index,TYP_UINT));
        offset=PTreeBinary(0,offset,PTreeUIntConstant(table->vb_offset+table->delta,TYP_UINT));
        EmitStmtExpr(PTreeBinary(13,PTreeId(offset_name),offset));
        address=PTreeBinary(57,PTreeType(char_pointer),PTreeThis());
        address=PTreeBinary(0,address,PTreeId(offset_name));
    }
    address=PTreeBinary(57,PTreeType(left_type),address);
    expr=PTreeBinary(13,PTreeUnary(40,address),rhs);
    EmitStmtExpr(expr);
}
void EmitVfunThunk(SCOPE scope,THUNK_ACTION *thunk)
{ (void)scope; (void)thunk; STUB_TRAP(); }
void *CgFrontSwitchFile(SYMBOL sym);
void emitThunks(CLASS_VFTABLE *table,SYMBOL ctor)
{
    THUNK_ACTION *thunk=table->data;
    SCOPE scope=SymScope(ctor);
    for(;;++thunk) {
        if(thunk->non_empty) EmitVfunThunk(scope,thunk);
        if(thunk->last_entry) break;
    }
    CgFrontSwitchFile(ctor);
}
int SymIsDtor(SYMBOL sym);
SYMBOL SymMarkRefed(SYMBOL sym);
PTREE NodeMakeCallee(SYMBOL sym);
TYPE TypeVoidFunOfVoid(void);
int DgStoreScalar(PTREE expr,unsigned offset,TYPE type);
void emitVFNPointer(SYMBOL sym)
{
    PTREE expr;
    TYPE type;
    if(sym) {
        sym->flag|=8;
        if(SymIsDtor(sym)) sym=SymMarkRefed(sym);
        expr=NodeMakeCallee(sym);
    } else expr=PTreeIntConstant(0,TYP_SINT);
    type=MakePointerTo(TypeVoidFunOfVoid());
    DgStoreScalar(expr,0,type);
    PTreeFreeSubtrees(expr);
}
void DgSymbolLabel(SYMBOL sym);
unsigned short CgSegId(SYMBOL sym);
void setICValue(unsigned value);
void CgFrontData(int opcode);
int SymIsInitialized(SYMBOL sym);
void genVFPtrCode(SCOPE scope,SYMBOL ctor,CLASS_VFTABLE *table)
{
    SYMBOL sym,vfn;
    THUNK_ACTION *thunk;
    sym=MakeVFTableSym(scope,table->h.count,table->h.exact_delta);
    emitVPtrInit(&table->h,sym,0);
    CgFrontCodePtr(128,sym);
    if(!SymIsInitialized(sym)) {
        emitThunks(table,ctor);
        setICValue((unsigned)sym);
        CgFrontData(127);
        DgSymbolLabel(sym);
        sym->flag|=2;
        thunk=table->data;
        for(;;++thunk) {
            vfn=thunk->sym;
            if(thunk->non_empty) vfn=thunk->thunk;
            else if(thunk->override) vfn=thunk->override;
            emitVFNPointer(vfn);
            if(thunk->last_entry) break;
        }
        CgSegId(sym);
        CgFrontData(92);
    }
}
int genVFPtrInits(SCOPE scope,SYMBOL ctor)
{
    CLASS_VFTABLE *tables=ScopeCollectVFTable(scope),*table;
    int corrupted=0;
    if(tables) {
        table=tables;
        do {
            table=(CLASS_VFTABLE *)table->h.next;
            if(table->corrupted) corrupted=1;
            if(table->h.delta==table->h.exact_delta) genVFPtrCode(scope,ctor,table);
        } while(table!=tables);
        table=tables;
        do {
            table=(CLASS_VFTABLE *)table->h.next;
            if(table->corrupted) corrupted=1;
            if(table->h.delta!=table->h.exact_delta) genVFPtrCode(scope,ctor,table);
        } while(table!=tables);
    }
    RingFree((void **)&tables);
    return corrupted;
}
void CgFrontCode(unsigned char opcode);
void CDoptIterEnd(CDOPT_ITER *iter);
void CtorPrologue(SYMBOL ctor,PTREE init,void (*push_scope)(void))
{
    SCOPE scope;
    CTOR_PROLOGUE data;
    TYPE type;
    CD_DESCR *optinfo;
    if (SymClassCorrupted(ctor)) return;
    scope=SymScope(ctor);
    initCtorPrologue(&data,init,scope);
    type=ScopeClass(scope);
    if (TypeReallyDtorable(type)) CDoptDtorBuild(type);
    optinfo=CDoptDefCtorBuild(type);
    if (CDoptErrorOccurred(optinfo)) return;
    if (TypeHasNumArgs(ctor->sym_type,0)) CDoptNoAccFun(optinfo);
    data.optiter=CDoptIterBeg(optinfo);
    data.cdopt=1;
    (*push_scope)();
    if (ScopeHasVirtualBases(scope)) STUB_TRAP();
    ScopeWalkDirectBases(scope,ctorPrologueBaseGen,&data);
    memberWalkWithData(scope,ctorPrologueMember,&data);
    CgFrontCode(129);
    CDoptIterEnd(data.optiter);
    if(ScopeHasVirtualFunctions(scope)) {
        genCtorDispInit(scope);
        genVFPtrInits(scope,ctor);
    }
    NodeFreeDupedExpr(data.member_init);
    NodeFreeDupedExpr(data.base_init);
}

TYPE ClassTypeForType(TYPE type);
SCOPE TypeScope(TYPE type);
char *CppDestructorName(void);
SYMBOL injectDtorSymbol(TYPE type) { (void)type; STUB_TRAP(); return NULL; }
SEARCH_RESULT *ScopeContainsMember(SCOPE scope,char *name);
SEARCH_RESULT *ScopeFindBaseMember(SCOPE scope,char *name);
void ScopeResultErrLocn(SEARCH_RESULT *result,CGIO_LOCATION *location);
int ScopeCheckSymbol(SEARCH_RESULT *result,SYMBOL sym);
SYMBOL findOrDefineDtor(TYPE type,int check,SCOPE access,CGIO_LOCATION *locn,int del)
{
    SCOPE scope;
    char *name;
    SEARCH_RESULT *result;
    SYMBOL dtor;
    type=ClassTypeForType(type);
    scope=TypeScope(type);
    name=CppDestructorName();
    result=ScopeContainsMember(scope,name);
    if (!result) {
        dtor=injectDtorSymbol(type);
        if (check) {
            if (!access) result=ScopeContainsMember(scope,name);
            else result=ScopeFindBaseMember(scope,name);
        }
    } else {
        if (check) {
            if (access) {
                ScopeFreeResult(result);
                result=ScopeFindBaseMember(scope,name);
            }
        } else {
            dtor=result->sym_name->name_syms;
            ScopeFreeResult(result);
        }
    }
    if (check) {
        ScopeResultErrLocn(result,locn);
        dtor=result->sym_name->name_syms;
        if (ScopeCheckSymbol(result,dtor) && del) dtor=NULL;
        ScopeFreeResult(result);
    }
    return dtor;
}
SYMBOL findOrDefineArrayDtor(TYPE type,int check,SCOPE access,CGIO_LOCATION *locn,int del)
{ (void)type; (void)check; (void)access; (void)locn; (void)del; STUB_TRAP(); return NULL; }
SYMBOL ClassFunMakeAddressable(SYMBOL sym);
TYPE ArrayType(TYPE type);
SYMBOL SymMarkRefed(SYMBOL sym);
SYMBOL RoDtorFindType(TYPE type)
{
    SYMBOL dtor;
    if (!ArrayType(type)) dtor=findOrDefineDtor(type,0,NULL,NULL,0);
    else dtor=findOrDefineArrayDtor(type,0,NULL,NULL,0);
    dtor=ClassFunMakeAddressable(dtor);
    dtor->flag|=8;
    return SymMarkRefed(dtor);
}
