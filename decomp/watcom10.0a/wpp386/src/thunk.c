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


#include "compiler.h"
#include "symbol.h"
#include "vftable.h"
#include "ptree.h"
#include "fnbody.h"
#include "pending.h"
#include <recon.h>
void *TypeHasPragma(TYPE type);
int SymIsDtor(SYMBOL sym);
int SymIsOpDel(SYMBOL sym);
int SymIsOpDelar(SYMBOL sym);
int TypeHasNumArgs(TYPE type,unsigned count);
char *CppSpecialName(unsigned index);
SCOPE SymScope(SYMBOL sym);
TYPE ScopeClass(SCOPE scope);
SEARCH_RESULT *ScopeContainsMember(SCOPE scope,char *name);
TYPE MakeThunkFunction(TYPE type);
SYMBOL AllocTypedSymbol(TYPE type);
unsigned classifyThunk(SYMBOL sym)
{
    if(SymIsDtor(sym)) return 8;
    if(SymIsOpDel(sym)) return 9;
    if(SymIsOpDelar(sym)) return 10;
    if(TypeHasNumArgs(sym->sym_type,1)) return 7;
    return 6;
}
SYMBOL existingThunk(SCOPE scope,char *name)
{
    SEARCH_RESULT *result;
    SYMBOL sym;
    if(scope==FileScope) result=ScopeFindNaked(scope,name);
    else result=ScopeContainsMember(scope,name);
    if(!result) return NULL;
    sym=result->sym_name->name_syms;
    ScopeFreeResult(result);
    return sym;
}
SYMBOL addrThunkSymbol(unsigned kind,SYMBOL sym)
{
    unsigned char id;
    char *name;
    SCOPE scope;
    SYMBOL thunk;
    /* The original leaves id uninitialized outside these five kinds.
     * No caller contract supplies another kind; hold that unknown domain. */
    if(kind>=6 && kind<=8) id=9;
    else if(kind==9 || kind==10) id=2;
    else { STUB_TRAP(); return NULL; }
    name=CppSpecialName(kind);
    scope=SymScope(sym);
    thunk=existingThunk(scope,name);
    if(!thunk) {
        thunk=AllocTypedSymbol(MakeThunkFunction(sym->sym_type));
        thunk->id=id;
        thunk=ScopeInsert(scope,thunk,name);
        thunk->flag|=0x40;
    }
    return thunk;
}
void RtnGenAddSymbol(unsigned char callback,SYMBOL sym);
SYMBOL ClassFunMakeAddressable(SYMBOL sym)
{
    SYMBOL thunk;
    unsigned kind;
    if (!sym) return sym;
    if (sym->id!=7 && !TypeHasPragma(sym->sym_type)) return sym;
    kind=classifyThunk(sym);
    thunk=addrThunkSymbol(kind,sym);
    if (!thunk->u.thunk_calls) {
        thunk->u.thunk_calls=sym;
        RtnGenAddSymbol(1,thunk);
    }
    return thunk;
}

int TypeHasEllipsisArg(TYPE type);
int TypeHasReverseArgs(TYPE type);
void CDtorScheduleArgRemap(SYMBOL sym)
{
    TYPE function=FunctionDeclarationType(sym->sym_type);
    if(TypeHasEllipsisArg(function) || TypeHasReverseArgs(function))
        RtnGenAddSymbol(0,sym);
}

TYPE MakeThunkPragmaFunction(TYPE type);
char *CppThunkName(SCOPE scope,THUNK_ACTION *thunk);
void ScopeBeginFunction(SYMBOL sym);
SYMBOL SymMakeDummy(TYPE type,char **name);
int SymCDtorExtraParm(SYMBOL sym);
void insertArgument(TYPE type)
{
    char *name;
    SYMBOL sym=SymMakeDummy(type,&name);
    ScopeInsert(CurrScope,sym,name);
}
void declareThunkArgs(SYMBOL sym,TYPE type)
{
    arg_list *args;
    TYPE *current;
    unsigned count;
    if (SymCDtorExtraParm(sym)) insertArgument(GetBasicUInt());
    args=type->u.f.args;
    current=args->type_list;
    for (count=args->num_args;count;--count,++current) insertArgument(*current);
}
TYPE TypePtrToChar(void);
TYPE TypePtrToUInt(void);
TYPE MakeVBTableFieldType(int is_const);
PTREE PTreeThis(void);
PTREE PTreeBinary(unsigned char op,PTREE left,PTREE right);
PTREE PTreeUnary(unsigned char op,PTREE expr);
PTREE PTreeUIntConstant(unsigned value,type_id id);
void EmitStmtExpr(PTREE expr);
TYPE TypedefModifierRemoveOnly(TYPE type);
TYPE TypeReferenced(TYPE type);
TYPE ReferenceType(TYPE type);
PTREE MakeNodeSymbol(SYMBOL sym);
PTREE NodeFetchReference(PTREE expr);
PTREE NodeRvalue(PTREE expr);
unsigned char ObjModelArgument(TYPE type);
PTREE NodeArgument(PTREE left,PTREE right);
PTREE NodeReverseArgs(unsigned *count,PTREE initial);
SYMBOL ScopeOrderedStart(SCOPE scope);
SYMBOL ScopeOrderedNext(SYMBOL last,SYMBOL current);
PTREE thunkArgList(SCOPE scope)
{
    char *ret_name=CppSpecialName(1);
    SYMBOL stop=ScopeOrderedStart(scope),sym=NULL;
    PTREE expr,list=NULL;
    TYPE arg_type;
    unsigned count;
    unsigned char model;
    while ((sym=ScopeOrderedNext(stop,sym))!=NULL) {
        if (sym->name->name!=ret_name) {
            arg_type=TypedefModifierRemoveOnly(sym->sym_type);
            expr=NodeFetchReference(MakeNodeSymbol(sym));
            model=ObjModelArgument(TypeReferenced(arg_type));
            if (!ReferenceType(arg_type) && model!=2) expr=NodeRvalue(expr);
            list=NodeArgument(list,expr);
        }
    }
    return NodeReverseArgs(&count,list);
}
PTREE NodeConvert(TYPE type,PTREE expr);
PTREE NodeArg(PTREE expr);
TYPE SymFuncReturnType(SYMBOL sym);
PTREE NodeTemporary(TYPE type);
PTREE NodeMakeCall(SYMBOL sym,TYPE type,PTREE args);
PTREE CallArgsArrange(TYPE type,PTREE call,PTREE args,PTREE thisarg,PTREE cdarg,PTREE retn);
PTREE NodeDtorExpr(PTREE expr,SYMBOL sym);
SYMBOL SymFunctionReturn(void);
PTREE AnalyseNode(PTREE expr);
void EmitAnalysedStmt(PTREE expr);
void setICValue(unsigned value);
void CgFrontCode(int opcode);
void FunctionBodyShutdown(SYMBOL sym,FUNCTION_DATA *data);
TYPE PointerTypeEquivalent(TYPE type);
PTREE NodeDupExpr(PTREE *expr);
PTREE NodeOffset(long offset);
PTREE NodeBinary(unsigned char op,PTREE left,PTREE right);
PTREE NodeCompareToZero(PTREE expr);
PTREE NodeConvertVirtualPtr(PTREE expr,TYPE type,long offset,long index);
PTREE NodeTestExpr(PTREE condition,PTREE yes,PTREE no);
PTREE applyReturnThunk(THUNK_ACTION *thunk,PTREE expr)
{
    long delta=thunk->out.delta;
    TYPE type,ptr;
    PTREE dup1=NULL,dup2;
    if (!thunk->output_virtual && !delta) return expr;
    type=expr->type;
    ptr=PointerTypeEquivalent(type);
    if (ptr && ptr->id==TYP_POINTER && !(ptr->flag&1)) dup1=NodeDupExpr(&expr);
    if (thunk->output_virtual)
        expr=NodeConvertVirtualPtr(expr,type,thunk->out.vb_offset,thunk->out.vb_index);
    if (delta) {
        expr=NodeBinary(0,expr,NodeOffset(delta));
        expr->type=type;
    }
    if (dup1) {
        dup2=NodeDupExpr(&dup1);
        dup1=NodeCompareToZero(dup1);
        expr=NodeTestExpr(dup1,expr,dup2);
    }
    return expr;
}
void EmitVfunThunk(SCOPE scope,THUNK_ACTION *thunk)
{
    SYMBOL sym,thunk_sym,this_sym,return_sym;
    TYPE thunk_type,char_ptr,vb_ptr,override_class,return_type;
    PTREE stmt,args,this_arg,return_node;
    char *name,*this_name;
    SCOPE saved,fn_scope;
    FUNCTION_DATA func_data;
    if (!thunk->override) {
        thunk->thunk=thunk->sym;
        return;
    }
    saved=CurrScope;
    sym=thunk->sym;
    thunk_type=MakeThunkPragmaFunction(sym->sym_type);
    thunk_sym=AllocTypedSymbol(thunk_type);
    name=CppThunkName(scope,thunk);
    scope=SymScope(sym);
    CurrScope=scope;
    thunk_sym=ScopeInsert(scope,thunk_sym,name);
    thunk->thunk=thunk_sym;
    ScopeBeginFunction(thunk_sym);
    fn_scope=CurrScope;
    declareThunkArgs(sym,FunctionDeclarationType(thunk_type));
    FunctionBodyStartup(thunk_sym,&func_data,1);
    /* The1994 thunk adjusts a real local copy of this through generated
     * parse trees; the later IC_VTHUNK instructions are not substituted. */
    char_ptr=TypePtrToChar();
    vb_ptr=MakePointerTo(MakeVBTableFieldType(0));
    this_sym=SymMakeDummy(char_ptr,&this_name);
    ScopeInsert(CurrScope,this_sym,this_name);
    stmt=PTreeBinary(57,PTreeType(char_ptr),PTreeThis());
    EmitStmtExpr(PTreeBinary(13,PTreeId(this_name),stmt));
    if (thunk->delta)
        EmitStmtExpr(PTreeBinary(26,PTreeId(this_name),
            PTreeUIntConstant(thunk->delta,TYP_UINT)));
    if (thunk->ctor_disp) {
        stmt=PTreeBinary(1,PTreeId(this_name),PTreeUIntConstant(4,TYP_UINT));
        stmt=PTreeBinary(57,PTreeType(TypePtrToUInt()),stmt);
        stmt=PTreeUnary(40,stmt);
        EmitStmtExpr(PTreeBinary(26,PTreeId(this_name),stmt));
        if (thunk->input_virtual) {
            EmitStmtExpr(PTreeBinary(25,PTreeId(this_name),
                PTreeUIntConstant(thunk->in.vb_offset,TYP_UINT)));
            stmt=PTreeBinary(57,PTreeType(vb_ptr),PTreeId(this_name));
            stmt=PTreeUnary(40,stmt);
            stmt=PTreeBinary(24,stmt,PTreeUIntConstant(thunk->in.vb_index,TYP_UINT));
            EmitStmtExpr(PTreeBinary(25,PTreeId(this_name),stmt));
        }
        if (thunk->in.delta)
            EmitStmtExpr(PTreeBinary(25,PTreeId(this_name),
                PTreeUIntConstant(thunk->in.delta,TYP_UINT)));
    }
    override_class=ScopeClass(SymScope(thunk->override));
    args=thunkArgList(fn_scope);
    this_arg=NodeRvalue(MakeNodeSymbol(this_sym));
    this_arg=NodeConvert(MakePointerTo(override_class),this_arg);
    this_arg=NodeArg(this_arg);
    return_type=SymFuncReturnType(thunk->override);
    return_node=NULL;
    if (ObjModelArgument(return_type)==2) return_node=NodeTemporary(return_type);
    stmt=NodeMakeCall(thunk->override,return_type,args);
    stmt=CallArgsArrange(thunk->override->sym_type,stmt,args,this_arg,NULL,return_node);
    if (return_node) stmt=NodeDtorExpr(stmt,return_node->u.symcg.symbol);
    return_sym=SymFunctionReturn();
    if (!return_sym) EmitAnalysedStmt(stmt);
    else {
        stmt=applyReturnThunk(thunk,stmt);
        stmt=PTreeBinary(69,PTreeType(return_type),stmt);
        stmt=AnalyseNode(stmt);
        EmitAnalysedStmt(stmt);
        setICValue((unsigned)return_sym);
        CgFrontCode(39);
    }
    FunctionBodyShutdown(thunk_sym,&func_data);
    ScopeEnd(2);
    CurrScope=saved;
}

PTREE NodeThis(void);
int SymIsInitialized(SYMBOL sym);
int SymIsDefArg(SYMBOL sym);
SYMBOL CgFrontSaveFile(void);
void CgFrontRestoreFile(SYMBOL sym);
void AddDefaultArgs(SYMBOL sym,PTREE call);
void CgFrontCodePtr(unsigned char opcode,void *value);
CGFILE *CgioLocateFile(SYMBOL sym);
void CgioThunkMarkGen(CGFILE *file);
SCOPE thunkPrologue(SYMBOL thunk_sym,FUNCTION_DATA *data)
{
    SYMBOL orig=thunk_sym->u.thunk_calls;
    SCOPE arg_scope;
    thunk_sym->flag|=2;
    CurrScope=SymScope(thunk_sym);
    ScopeBeginFunction(thunk_sym);
    arg_scope=CurrScope;
    declareThunkArgs(orig,FunctionDeclarationType(orig->sym_type));
    FunctionBodyStartup(thunk_sym,data,0);
    return arg_scope;
}
void thunkEpilogue(SYMBOL thunk_sym,FUNCTION_DATA *data)
{
    FunctionBodyShutdown(thunk_sym,data);
    ScopeEnd(2);
}
void RtnGenCallBackGenThunk(SYMBOL thunk_sym)
{
    SYMBOL orig_sym=thunk_sym->u.thunk_calls,return_sym,curr_func;
    PTREE args,stmt,user_args,this_arg,extra_arg;
    TYPE return_type;
    SCOPE save_scope,fn_scope;
    FUNCTION_DATA func_data;
    unsigned classification,orig_ref;
    CGFILE *cgfile;
    if (thunk_sym->flag&1) orig_sym->flag|=1;
    if (SymIsInitialized(thunk_sym)) return;
    classification=classifyThunk(orig_sym);
    save_scope=CurrScope;
    curr_func=CgFrontSaveFile();
    fn_scope=thunkPrologue(thunk_sym,&func_data);
    return_type=SymFuncReturnType(thunk_sym);
    args=thunkArgList(fn_scope);
    orig_ref=orig_sym->flag&1;
    stmt=NodeMakeCall(orig_sym,return_type,args);
    if (!orig_ref) orig_sym->flag&=~1;
    /* The old AddDefaultArgs is void: later failure-result handling and
     * FunctionCalled are not present at this original call site. */
    AddDefaultArgs(orig_sym,stmt);
    user_args=stmt->u.subtree[1];
    if (SymCDtorExtraParm(orig_sym)) {
        extra_arg=user_args;
        user_args=user_args->u.subtree[0];
    } else extra_arg=NULL;
    if (SymIsThisMember(orig_sym)) this_arg=NodeArg(NodeThis());
    else this_arg=NULL;
    stmt=CallArgsArrange(orig_sym->sym_type,stmt,user_args,this_arg,extra_arg,NULL);
    return_sym=SymFunctionReturn();
    if (return_sym) {
        stmt=PTreeBinary(69,PTreeType(return_type),stmt);
        stmt=AnalyseNode(stmt);
    }
    EmitAnalysedStmt(stmt);
    CgFrontCodePtr(39,return_sym);
    thunkEpilogue(thunk_sym,&func_data);
    CurrScope=save_scope;
    CgFrontRestoreFile(curr_func);
    cgfile=CgioLocateFile(thunk_sym);
    cgfile->flags|=0x80;
    if (SymIsInitialized(orig_sym) || SymIsDefArg(orig_sym)
        || classification==9 || classification==10) CgioThunkMarkGen(cgfile);
}
