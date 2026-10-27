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


/* Adapted from initial6b9cb443 bld/plusplus/c/fnbody.c.
 * Actual1994 CF38816..38869 creates these four pools with old sizes;
 * the later functionSuicide callback registration is absent. */
#include "compiler.h"
#include "carve.h"
#include "fnbody.h"
#include "label.h"
#include "context.h"
#include "cmdline.h"
#include "frontend.h"
#include "ptree.h"
#include "scan.h"
#include "cgio.h"
#include "pending.h"
#include "class.h"
#include "rewrite.h"
#include "typesig.h"
#include <recon.h>
static carve_t carveCSTACK, carveSWCASE, carveFNLABEL, carveFNCATCH;
void functionInit(INITFINI *defn)
{
    (void)defn;
    carveCSTACK = CarveCreate(79, 16);
    carveSWCASE = CarveCreate(20, 32);
    carveFNLABEL = CarveCreate(19, 8);
    carveFNCATCH = CarveCreate(20, 8);
}

static FUNCTION_DATA *currFunction;
char *CppConstructorName(void);
char *CppDestructorName(void);
char *CppOperatorName(unsigned oper);
TYPE AddFunctionFlag(TYPE type,unsigned short flags);
TYPE RemoveFunctionFlag(TYPE type,unsigned short flags);
CGFILE_INS CgFrontFuncOpen(SYMBOL sym,CGIO_LOCATION *position);
unsigned short CgSegIdFunction(SYMBOL sym);
void LabelInitFunc(LAB_MEM *memory);
void setExceptionSpecs(SYMBOL sym);
void startBlock(void);
void insertFunctionReturn(SYMBOL sym);
PTREE ClassMemInit(SYMBOL sym,void *mem_initial);
void CtorPrologue(SYMBOL sym,PTREE mem,void (*block)(void));
void DtorPrologue(SYMBOL sym,unsigned *end,CGFILE_INS *begin);
void doFnStartup(SYMBOL sym,FUNCTION_DATA *data,void *mem_initial,
                 CGIO_LOCATION *position,unsigned char flags)
{
    PTREE mem;
    data->fn_scope=CurrScope;
    sym->flag|=2;
    if (flags&1) sym->sym_type=RemoveFunctionFlag(sym->sym_type,0x80);
    else if (PragToggle&1) sym->sym_type=AddFunctionFlag(sym->sym_type,0x80);
    data->init_state_tab=CgFrontFuncOpen(sym,position);
    sym->segid=CgSegIdFunction(sym);
    LabelInitFunc(&data->label_mem);
    /* Actual1994 checks the exception specification before startBlock. */
    setExceptionSpecs(sym);
    startBlock();
    insertFunctionReturn(sym);
    if (data->is_ctor) {
        mem=ClassMemInit(sym,mem_initial);
        CtorPrologue(sym,mem,startBlock);
        startBlock();
    } else if (data->is_dtor) {
        DtorPrologue(sym,&data->end_dtor,&data->dtor_beg);
        startBlock();
    }
    data->outer_depth=data->depth;
}
void initFunctionData(SYMBOL sym,FUNCTION_DATA *data)
{
    char *name;
    CtxFunction(sym);
    data->fn_scope=0;
    data->depth=0;
    data->outer_depth=0;
    data->control=0;
    data->labels=0;
    data->block_count=0;
    data->init_var.var=0;
    data->init_var.mask=1;
    data->access_errs=0;
    /* Actual old release clears exactly9 bits; later extra exception and
     * return-optimization fields are not initialized here. */
    data->is_ctor=0;
    data->is_dtor=0;
    data->is_assign=0;
    data->is_defarg=0;
    data->dead_code=0;
    data->dead_diagnosed=0;
    data->ret_reqd=0;
    data->has_mem_init=0;
    data->has_state_tab=0;
    data->flag_count=0;
    data->next=currFunction;
    currFunction=data;
    name=sym->name->name;
    if (name==CppConstructorName()) data->is_ctor=1;
    else if (name==CppDestructorName()) data->is_dtor=1;
    else if (name==CppOperatorName(13)) data->is_assign=1;
}
void FunctionBodyStartup(SYMBOL sym,FUNCTION_DATA *data,unsigned char flags)
{
    initFunctionData(sym,data);
    doFnStartup(sym,data,0,(CGIO_LOCATION *)sym->locn,flags);
}

TYPE AddFunctionFlag(TYPE type,unsigned short flags)
{ (void)type; (void)flags; STUB_TRAP(); return 0; }
TYPE RemoveFunctionFlag(TYPE type,unsigned short flags);
void ExceptionsCheck(void);
void exceptSpec(TYPE type) { (void)type; STUB_TRAP(); }
void FunctionHasRegistration(void)
{
    currFunction->has_state_tab=1;
}
unsigned FunctionRegistrationFlag(void)
{
    FunctionHasRegistration();
    return ++currFunction->flag_count;
}
void setExceptionSpecs(SYMBOL sym)
{
    TYPE *excepts,type;
    int none;
    excepts=SymFuncArgList(sym)->except_spec;
    if (excepts) {
        ExceptionsCheck();
        none=1;
        while ((type=*excepts++)!=0) { exceptSpec(type); none=0; }
        if (none) exceptSpec(0);
        FunctionHasRegistration();
    }
}
#pragma pack(1)
typedef struct block_label { unsigned id; LAB_DEF *defn; } BLK_LABEL;
typedef struct fn_label FNLABEL;
struct fn_label {
    FNLABEL *next;
    BLK_LABEL destination;
    char *name;
    unsigned short block;
    unsigned char defined:1,referenced:1,dangerous:1;
};
typedef char FnLabelSize[(sizeof(FNLABEL)==19)?1:-1];
struct control_stack {
    CSTACK *next;
    BLK_LABEL outside;
    SYMBOL try_var;
    CGIO_LOCATION defined;
    union {
        struct { BLK_LABEL else_part; } i;
        struct { unsigned top_loop; BLK_LABEL continue_loop; } l;
        struct { unsigned short block; unsigned char block_switch; } b;
        struct { void *catches; SCOPE defn_scope; unsigned catch_no;
            SYMBOL try_var; CGIO_LOCATION try_locn; } t;
        /* Remaining switch fields are not yet consumed. */
        unsigned char switch_storage[49];
    } u;
    unsigned char id,flags;
};
#pragma pack()
typedef char ControlStackSize[(sizeof(CSTACK)==79)?1:-1];
void SrcFileGetTokenLocn(CGIO_LOCATION *position);
void *StackPush(void **head,void *entry);
void ScopeBeginBlock(unsigned index);
void LabelBlockOpen(int dead);
CSTACK *beginControl(int id)
{
    CSTACK *block;
    block=CarveAlloc(carveCSTACK);
    block->outside.defn=0;
    block->try_var=0;
    block->id=id;
    SrcFileGetTokenLocn(&block->defined);
    switch (id) {
    case 3: block->u.i.else_part.defn=0; break;
    case 5: case 6: case 7: block->u.l.continue_loop.defn=0; break;
    case 2: block->u.b.block=currFunction->block_count; break;
    }
    block->flags&=~3U;
    StackPush((void **)&currFunction->control,block);
    return block;
}
void openScope(void)
{
    ScopeBeginBlock(currFunction->block_count++);
    LabelBlockOpen(currFunction->dead_code);
}
void startBlock(void)
{
    beginControl(2);
    openScope();
    ++currFunction->depth;
}
unsigned char ObjModelFunctionReturn(TYPE type);
SYMBOL SymAllocReturn(SCOPE scope,TYPE type);
SYMBOL SymFunctionReturn(void);
void insertFunctionReturn(SYMBOL sym)
{
    TYPE function,base;
    function=FunctionDeclarationType(sym->sym_type);
    base=TypedefModifierRemove(function->of);
    switch (ObjModelFunctionReturn(function)) {
    case 0: break;
    case 1: case 3: SymAllocReturn(CurrScope,base); break;
    case 2:
        if (!SymFunctionReturn()) SymAllocReturn(CurrScope->enclosing,base);
        break;
    }
}




int TypeDefined(TYPE type);
SYMBOL CgFrontSaveFile(void);
int SymIsFunction(SYMBOL sym);
int SymIsInitialized(SYMBOL sym);
int TemplateMemberCanBeIgnored(void);
SYMBOL TemplateFunctionTranslate(SYMBOL sym,SCOPE *scope);
void skipFunctionBody(unsigned depth)
{ (void)depth; STUB_TRAP(); }
int GeneratedDefaultFunction(SYMBOL sym);
void badFunction(unsigned error,SYMBOL sym)
{ (void)error; (void)sym; STUB_TRAP(); }
struct class_info *TypeClassInfo(TYPE type);
int TypeVAStartWontWork(TYPE type,unsigned *error);
SCOPE SymScope(SYMBOL sym);
int SymIsInline(SYMBOL sym);
void handleDefnChangesToSym(SYMBOL sym)
{
    SCOPE scope=SymScope(sym);
    if (ScopeType(scope,0)) {
        if (sym->id<=1) {
            if (SymIsInline(sym)) sym->id=2;
            else { sym->id=10; CompFlags.external_defn_found=1; }
        }
    } else if (ScopeType(scope,1) && !SymIsInline(sym))
        CompFlags.external_defn_found=1;
}
void ScopeJumpForward(SCOPE scope);
void ScopeBeginFunction(SYMBOL sym);
void InsertArgs(DECL_INFO **args);
void ScopeArgumentCheck(SCOPE scope);
void ClassDtorNullBody(SYMBOL sym);
void ClassCtorNullBody(SYMBOL sym);
void ClassAssignNullBody(SYMBOL sym);
void ParseFlush(void);
int NextToken(void);
void nextYYToken(void) { ParseFlush(); NextToken(); }
CGIO_LOCATION *posnForFunction(DECL_INFO *info)
{
    return info->id ? &info->id->locn : NULL;
}
void initFunctionBody(DECL_INFO *info,FUNCTION_DATA *data)
{
    SYMBOL func=info->sym;
    REWRITE *mem_init;
    initFunctionData(func,data);
    if (info->scope) ScopeJumpForward(info->scope);
    ScopeBeginFunction(func);
    if (!CompFlags.mfi_switch_used &&
        (FunctionDeclarationType(func->sym_type)->flag&0x200))
        TargetSwitches|=0x200;
    InsertArgs(&info->parms);
    mem_init=info->mem_init;
    info->mem_init=NULL;
    if (mem_init && data->is_ctor) data->has_mem_init=1;
    beginControl(1);
    while (CurToken==5) nextYYToken();
    if (CurToken==12) {
        if (data->is_dtor) ClassDtorNullBody(func);
        else if (data->is_ctor && !data->has_mem_init) ClassCtorNullBody(func);
        else if (data->is_assign) ClassAssignNullBody(func);
    }
    doFnStartup(func,data,mem_init,posnForFunction(info),0);
    ScopeArgumentCheck(CurrScope);
}
void *CgFrontSwitchFile(SYMBOL sym);
CGIO_LOCATION SrcPosnEmitCurrent(void);
void set_output(SYMBOL func)
{
    CgFrontSwitchFile(func);
    SrcPosnEmitCurrent();
}
FNLABEL *findLabel(char *name) { (void)name; STUB_TRAP(); return NULL; }
void CgFrontLabdefGoto(unsigned id);
void LabelDefine(LAB_DEF *label);
void LookPastName(void);
void parseLabels(void)
{
    FNLABEL *label=NULL;
    for(;;) {
        if(CurToken==54) LookPastName();
        else if(CurToken!=145) break;
        if(LAToken!=4) break;
        label=findLabel(SavedId);
        if(label->defined) {
            CErr2p(32,label->name);
            CErr2p(574,(const char *)&label->destination.defn->locn);
        } else {
            label->defined=1;
            CgFrontLabdefGoto(label->destination.id);
            LabelDefine(label->destination.defn);
        }
        currFunction->dead_code=0;
        nextYYToken();
        nextYYToken();
    }
    if(label!=NULL && CurToken==12) CErr1(73);
}
void closeScope(void);
void endBlock(void)
{
    if(currFunction->depth>currFunction->outer_depth) closeScope();
    --currFunction->depth;
}
void endControl(void);
void dumpCSLabel(unsigned label);
void dumpBlkLabel(BLK_LABEL *label)
{
    if(label->defn) {
        dumpCSLabel(label->id);
        LabelDefine(label->defn);
    }
}
void dumpOutsideLabel(CSTACK *block)
{
    dumpBlkLabel(&block->outside);
}
void doJUMP(int op,unsigned condition,unsigned destination);
void CgFrontLabfreeCs(unsigned count);
void parseElseStmt(void) { STUB_TRAP(); }
void jumpFwdBlkLabel(BLK_LABEL *label);
void completeTry(CSTACK *block);
int endOfStmt(int recog_token)
{
    CSTACK *block=currFunction->control;
    for(;;) {
        switch(block->id) {
        case 2:
            if(block->next->id==9 || block->next->id==10) {
                if(!currFunction->dead_code) {
                    SrcPosnEmitCurrent();
                    jumpFwdBlkLabel(&block->next->outside);
                }
            } else if(block->next->id>=5 && block->next->id<=8) {
                STUB_TRAP(); return 0;
            }
            endBlock();
            break;
        case 3:
            closeScope();
            if(recog_token) { nextYYToken(); recog_token=0; }
            if(CurToken==103) {
                parseElseStmt();
                return recog_token;
            }
            if(!(block->flags&1)) dumpBlkLabel(&block->u.i.else_part);
            CgFrontLabfreeCs(2);
            break;
        case 9:
            if(recog_token) { nextYYToken(); recog_token=0; }
            if(CurToken==85) return recog_token;
            CErr1(624);
            completeTry(block);
            break;
        case 10:
            if(recog_token) { nextYYToken(); recog_token=0; }
            if(CurToken==85) return recog_token;
            completeTry(block);
            break;
        case 5: case 7:
            if(block->u.l.continue_loop.defn)
                dumpBlkLabel(&block->u.l.continue_loop);
            closeScope();
            doJUMP(7,81,block->u.l.top_loop);
            dumpOutsideLabel(block);
            CgFrontLabfreeCs(3);
            break;
        default:
            STUB_TRAP(); return 0;
        }
        endControl();
        block=currFunction->control;
        if(!block || block->id==1 || block->id==2) return recog_token;
    }
}
void PTreeWarnExpr(PTREE expr,unsigned warning);
PTREE AnalyseStmtExpr(PTREE expr);
PTREE IcEmitExpr(PTREE expr);
void ensureLiveExpr(PTREE expr)
{
    if(currFunction->dead_code && !currFunction->dead_diagnosed) {
        PTreeWarnExpr(expr,13);
        currFunction->dead_diagnosed=1;
    }
}
PTREE emitCodeExpr(PTREE expr)
{
    ensureLiveExpr(expr);
    return IcEmitExpr(expr);
}
void checkDeclLocation(void)
{
    switch(currFunction->control->id) {
    case 3: CErr1(201); break;
    case 4: CErr1(202); break;
    case 5: CErr1(205); break;
    case 6: CErr1(206); break;
    case 7: CErr1(207); break;
    case 8: CErr1(203); break;
    }
}
PTREE ParseExprDecl(void);
void declExprStmt(int for_declaration)
{
    PTREE expr;
    if(CurToken==1) return;
    expr=ParseExprDecl();
    if(expr) {
        ensureLiveExpr(expr);
        expr=AnalyseStmtExpr(expr);
        emitCodeExpr(expr);
    } else if(!for_declaration) checkDeclLocation();
}
void MainProcSetup(SYMBOL sym);
TYPE DefaultIntType(TYPE type);
int returnIsRequired(TYPE type)
{
    TYPE result;
    if(currFunction->dead_code) return 0;
    result=TypedefModifierRemove(type->of);
    if(result->id==TYP_VOID || currFunction->is_ctor || currFunction->is_dtor)
        return 0;
    if(DefaultIntType(result) && !currFunction->ret_reqd) return 0;
    return 1;
}
void CgFrontReturnSymbol(SYMBOL sym);
void CgFrontRestoreFile(SYMBOL symbol);
void FunctionBodyDeadCode(void);
void CgFrontFuncInitFlags(SYMBOL sym);
void functionShutdown(SYMBOL sym,FUNCTION_DATA *data);
void ScopeJumpForward(SCOPE scope);
void RingWalk(void *head,void (*routine)(void *));
void finiLabel(void *label) { (void)label; STUB_TRAP(); }
void freeGotoLabels(void)
{
    RingWalk(currFunction->labels,finiLabel);
    currFunction->labels=NULL;
}
void finiFunctionBody(SYMBOL sym)
{
    CgFrontFuncInitFlags(currFunction->init_var.var);
    freeGotoLabels();
    functionShutdown(sym,currFunction);
    ScopeJumpForward(currFunction->fn_scope);
    ScopeEnd(2);
    currFunction=currFunction->next;
}
void ensureLiveCode(void)
{
    if (currFunction->dead_code && !currFunction->dead_diagnosed) {
        CErr1(13);
        currFunction->dead_diagnosed=1;
    }
}
void MustRecog(int token);
void mustRecog(int token)
{
    ParseFlush();
    MustRecog(token);
}
PTREE ParseExpr(int end_token);
PTREE safeParseExpr(int end_token)
{
    if (CurToken==1) return NULL;
    return ParseExpr(end_token);
}
SYMBOL SymFunctionReturn(void);
PTREE AnalyseReturnExpr(SYMBOL sym,PTREE expr);
void PTreeErrorExpr(PTREE expr,unsigned error);
void parseReturnStmt(SYMBOL func)
{
    PTREE expr;
    SYMBOL return_sym,operand=NULL;
    int expecting;
    ensureLiveCode();
    nextYYToken();
    return_sym=SymFunctionReturn();
    expecting=return_sym!=NULL && !currFunction->is_ctor && !currFunction->is_dtor;
    if (CurToken==5) {
        if (expecting) CErr1(3);
    } else {
        currFunction->ret_reqd=1;
        expr=safeParseExpr(5);
        if (expecting) {
            if (expr) {
                if (expr->op==0) PTreeFreeSubtrees(expr);
                else {
                    emitCodeExpr(AnalyseReturnExpr(func,expr));
                    operand=return_sym;
                }
            }
        } else {
            PTreeErrorExpr(expr,56);
            PTreeFreeSubtrees(expr);
        }
    }
    CgFrontReturnSymbol(operand);
    mustRecog(5);
    FunctionBodyDeadCode();
}
int exprAnalyse(PTREE (*analyse)(PTREE),PTREE *expr)
{
    if(*expr==NULL) return 1;
    *expr=analyse(*expr);
    return (*expr)->op==0 ? 2 : 0;
}
unsigned CgFrontLabelCs(void);
void initBlkLabel(BLK_LABEL *label)
{
    label->id=CgFrontLabelCs();
    label->defn=NULL;
}
unsigned CgFrontLabelCs(void);
CSTACK *beginLoopControl(int id)
{
    CSTACK *loop=beginControl(id);
    initBlkLabel(&loop->outside);
    initBlkLabel(&loop->u.l.continue_loop);
    loop->u.l.top_loop=CgFrontLabelCs();
    return loop;
}
void CgFrontGotoNear(int op,unsigned condition,unsigned destination);
void CgFrontLabdefCs(unsigned label);
void doJUMP(int op,unsigned condition,unsigned destination)
{
    CgFrontGotoNear(op,condition,destination);
    if(condition==81) FunctionBodyDeadCode();
}
void dumpCSLabel(unsigned label)
{
    CgFrontLabdefCs(label);
    currFunction->dead_code=0;
}
LAB_DEF *LabelAllocLabDef(void);
LAB_REF *LabelAllocLabRef(LAB_DEF *def);
LAB_REF *refBlkLabel(BLK_LABEL *label)
{
    if(label->defn==NULL) label->defn=LabelAllocLabDef();
    return LabelAllocLabRef(label->defn);
}
void LabelRefFree(LAB_REF *ref);
void jumpBlkLabel(BLK_LABEL *label,unsigned condition)
{
    LabelRefFree(refBlkLabel(label));
    doJUMP(7,condition,label->id);
}
int NodeIsConstantInt(PTREE expr);
unsigned long NodeConstantValue(PTREE expr);
void warnBoolConstVal(unsigned long value,int parsed_int,PTREE expr)
{ (void)value; (void)parsed_int; (void)expr; STUB_TRAP(); }
void warnBoolConst(PTREE expr,int parsed_int)
{
    /* The1994 interface passes the actual constant value (or known-nonzero1),
     * before the later source changed this helper's argument to a zero flag. */
    if(NodeIsConstantInt(expr))
        warnBoolConstVal(NodeConstantValue(expr),parsed_int,expr);
    else if(expr->flags&0x200) warnBoolConstVal(1,parsed_int,expr);
}
PTREE AnalyseBoolExpr(PTREE expr);
int NodeIsConstantInt(PTREE expr);
void CgFrontCode(int opcode);
unsigned CgFrontLabelCs(void);
void CgFrontLabfreeCs(unsigned count);
int parseBracketExpr(PTREE (*analyse)(PTREE))
{
    PTREE expr;
    int result,constant;
    mustRecog(7);
    expr=safeParseExpr(8);
    constant=NodeIsConstantInt(expr);
    result=exprAnalyse(analyse,&expr);
    if(result!=1) {
        warnBoolConst(expr,constant);
        emitCodeExpr(expr);
    }
    mustRecog(8);
    return result;
}
void parseIfStmt(void)
{
    CSTACK *block;
    ensureLiveCode();
    block=beginControl(3);
    initBlkLabel(&block->outside);
    initBlkLabel(&block->u.i.else_part);
    nextYYToken();
    if(parseBracketExpr(AnalyseBoolExpr)==0) {
        if(block->flags&1) CgFrontCode(26);
        else if(block->flags&2) {
            CgFrontCode(26);
            jumpBlkLabel(&block->u.i.else_part,81);
        } else jumpBlkLabel(&block->u.i.else_part,84);
    } else {
        jumpBlkLabel(&block->u.i.else_part,81);
        currFunction->dead_code=0;
    }
    openScope();
}
void parseForStmt(void)
{
    PTREE test_expr,inc_expr;
    CSTACK *loop;
    int test_type,inc_type,test_was_const;
    unsigned around;
    nextYYToken();
    mustRecog(7);
    if(CurToken==5) mustRecog(5);
    else { ensureLiveCode(); declExprStmt(1); }
    if(CurToken==5) test_type=1;
    else {
        test_expr=safeParseExpr(5);
        test_was_const=NodeIsConstantInt(test_expr);
        test_type=exprAnalyse(AnalyseBoolExpr,&test_expr);
    }
    mustRecog(5);
    if(CurToken!=8) {
        inc_expr=safeParseExpr(8);
        inc_type=exprAnalyse(AnalyseStmtExpr,&inc_expr);
    } else inc_type=1;
    mustRecog(8);
    loop=beginLoopControl(7);
    switch(inc_type) {
    case 0:
        around=CgFrontLabelCs();
        doJUMP(7,81,around);
        dumpCSLabel(loop->u.l.top_loop);
        emitCodeExpr(inc_expr);
        dumpCSLabel(around);
        CgFrontLabfreeCs(1);
        break;
    case 1: dumpCSLabel(loop->u.l.top_loop); break;
    }
    if(test_type==0) {
        warnBoolConst(test_expr,test_was_const);
        emitCodeExpr(test_expr);
        if(loop->flags&1) CgFrontCode(26);
        else jumpBlkLabel(&loop->outside,84);
    }
    openScope();
}
TYPE MakeExpandableType(type_id base);
char *NameDummy(void);
SYMBOL allocTryVar(CSTACK *block)
{
    SYMBOL sym=block->next->try_var;
    if(!sym) {
        sym=SymCreate(MakeExpandableType(TYP_CHAR),3,9,NameDummy(),
                      block->u.t.defn_scope);
        block->next->try_var=sym;
    }
    return sym;
}
void CgFrontSwitchBeg(void);
void CgFrontSwitchDefault(SCOPE scope,int dead,CGIO_LOCATION *posn);
void LabelSwitchBeg(void);
void LabelSwitch(SCOPE scope);
void LabelBlkTry(CGIO_LOCATION *locn,SYMBOL sym);
void parseCatchHandler(void);
void parseTryBlock(void)
{
    CSTACK *block;
    ensureLiveCode();
    ExceptionsCheck();
    /* The 1994 body predates ScopeMarkVisibleAutosInMem/catch_err. */
    block=beginControl(9);
    block->u.t.catches=NULL;
    block->u.t.catch_no=0;
    block->u.t.defn_scope=CurrScope;
    block->u.t.try_locn=SrcPosnEmitCurrent();
    initBlkLabel(&block->outside);
    block->u.t.try_var=allocTryVar(block);
    CgFrontCodePtr(101,block->u.t.try_var);
    CgSetTypeExact(GetBasicType(TYP_SINT));
    CgFrontSwitchBeg();
    LabelSwitchBeg();
    LabelSwitch(block->u.t.defn_scope);
    CgFrontSwitchDefault(block->u.t.defn_scope,0,&block->u.t.try_locn);
    nextYYToken();
    mustRecog(11);
    startBlock();
    LabelBlkTry(&block->u.t.try_locn,block->u.t.try_var);
    FunctionHasRegistration();
}
void FunctionBody(DECL_INFO *info)
{
    unsigned error;
    SYMBOL func=info->sym;
    TYPE fn_type;
    SCOPE parsing_scope,enclosing_scope;
    SYMBOL previous_file;
    FUNCTION_DATA data;
    if (!func || !SymIsFunction(func)) { skipFunctionBody(1); return; }
    func=TemplateFunctionTranslate(func,&parsing_scope);
    info->sym=func;
    CtxFunction(func);
    if (SymIsInitialized(func)) {
        if (TemplateMemberCanBeIgnored()) { skipFunctionBody(1); return; }
        error=GeneratedDefaultFunction(func)?559:229;
        badFunction(error,func);
        return;
    }
    if (!(info->flags&0x20)) {
        SetErrLoc((CGIO_LOCATION *)func->locn);
        badFunction(445,func);
        return;
    }
    fn_type=FunctionDeclarationType(func->sym_type);
    if (!TypeDefined(fn_type->of)) {
        SetErrLoc((CGIO_LOCATION *)func->locn);
        CErr2p(304,TypeClassInfo(fn_type->of)->name);
    }
    if (TypeVAStartWontWork(fn_type,&error)) {
        SetErrLoc((CGIO_LOCATION *)func->locn);
        CErr1(error);
    }
    handleDefnChangesToSym(func);
    previous_file=CgFrontSaveFile();
    enclosing_scope=CurrScope;
    CurrScope=parsing_scope;
    initFunctionBody(info,&data);
    MainProcSetup(func);
    for(;;) {
    set_output(func);
    parseLabels();
    if(CurToken==12) {
        int recog_token;
        if(data.control->id!=2) CErr1(23);
        recog_token=data.depth>data.outer_depth;
        set_output(func);
        if(endOfStmt(recog_token)) nextYYToken();
    } else {
        switch(CurToken) {
        case 1: case 5: case 57: case 72:
        case 89: case 98: case 101: case 107:
        case 114: case 115:
            STUB_TRAP();
            break;
        case 11:
            startBlock();
            /* Switch-owned block fields have not been reconstructed. */
            if(data.control->next->id==8) { STUB_TRAP(); break; }
            nextYYToken();
            continue;
        case 61:
            parseIfStmt();
            continue;
        case 62:
            parseForStmt();
            continue;
        case 85:
            parseCatchHandler();
            continue;
        case 124:
            parseTryBlock();
            continue;
        case 102:
            parseReturnStmt(func);
            break;
        case 43:
            if(data.control->id!=2) {
                if(endOfStmt(1)) nextYYToken();
            } else nextYYToken();
            continue;
        default:
            declExprStmt(0);
            if(data.control->id==2) continue;
            if(endOfStmt(0)) nextYYToken();
            break;
        }
    }
    if(data.depth==0) break;
    }
    if(returnIsRequired(fn_type)) CErr1(3);
    else if(!data.dead_code) {
        CgFrontReturnSymbol(NULL);
        FunctionBodyDeadCode();
    }
    finiFunctionBody(func);
    CurrScope=enclosing_scope;
    CgFrontRestoreFile(previous_file);
    /* Other statement forms remain guarded above; this normal completion
     * returns to the surrounding generated declaration parser. */
}

int DefargBeingCompiled(void)
{
    return currFunction->is_defarg;
}

void FunctionBodyDeadCode(void)
{
    currFunction->dead_code=1;
    currFunction->dead_diagnosed=0;
}

int FunctionBodyCtor(void)
{
    return currFunction->is_ctor;
}

void CgFrontZapUint(CGFILE_INS location,int opcode,unsigned value);
void CgFrontCode(int opcode);
void flushToDepth1(FUNCTION_DATA *data)
{
    unsigned short depth;
    if(data->is_ctor) CgFrontCode(102);
    for(depth=data->outer_depth;depth!=1;--depth) {
        closeScope();
        endControl();
    }
}
void DtorEpilogue(SYMBOL func,unsigned end,CGFILE_INS *begin);
void LabelBlockClose(int dead);
void closeScope(void)
{
    LabelBlockClose(currFunction->dead_code);
    ScopeEnd(3);
}
void *StackPop(void **head);
void labelDefFree(BLK_LABEL *label)
{
    LabelDefFree(label->defn);
    label->defn=NULL;
}
void endControl(void)
{
    CSTACK *old=StackPop((void **)&currFunction->control);
    if(old) {
        labelDefFree(&old->outside);
        switch(old->id) {
        case 3: case 4:
            labelDefFree(&old->u.i.else_part);
            break;
        case 5: case 6: case 7:
            labelDefFree(&old->u.l.continue_loop);
            break;
        }
        CarveFree(carveCSTACK,old);
    }
}
void CgFrontFuncClose(SYMBOL func);
void LabelFiniFunc(LAB_MEM *memory);
TYPE FunctionDeclarationType(TYPE type);
/* The1994 body always has a function, predates exception-flag propagation,
 * and derives floating-SS restoration from the actual function type. */
void functionShutdown(SYMBOL func,FUNCTION_DATA *data)
{
    if(data->has_state_tab)
        CgFrontZapUint(data->init_state_tab,65,data->flag_count);
    if(data->dead_code) CgFrontCode(60);
    if(data->is_ctor) {
        flushToDepth1(data);
        CtorEpilogue(func);
        FunctionBodyDeadCode();
    } else if(data->is_dtor) {
        flushToDepth1(data);
        DtorEpilogue(func,data->end_dtor,&data->dtor_beg);
        FunctionBodyDeadCode();
    }
    closeScope();
    CgFrontFuncClose(func);
    LabelFiniFunc(&data->label_mem);
    endControl();
    if(!CompFlags.zu_switch_used &&
       (FunctionDeclarationType(func->sym_type)->flag&0x200))
        TargetSwitches&=~0x200U;
}
void FunctionBodyShutdown(SYMBOL func,FUNCTION_DATA *data)
{
    functionShutdown(func,data);
    currFunction=currFunction->next;
}

void LabelGotoFwd(LAB_REF *ref);
void jumpFwdBlkLabel(BLK_LABEL *label)
{
    SrcPosnEmitCurrent();
    LabelGotoFwd(refBlkLabel(label));
    doJUMP(7,81,label->id);
}
DECL_INFO *ParseException(void);
TYPE AbstractClassType(TYPE type);
typedef struct fn_catch FNCATCH;
struct fn_catch { FNCATCH *next; TYPE type; CGIO_LOCATION defined; };
typedef char FnCatchSize[(sizeof(FNCATCH)==20)?1:-1];
TYPE TypeCanonicalThr(TYPE type);
TYPE TypeReferenced(TYPE type);
TYPE StructType(TYPE type);
TYPE TypePointedAt(TYPE type,unsigned short *flags);
int IsBigData(void);
int TypesIdentical(TYPE left,TYPE right);
unsigned char TypeDerived(TYPE derived,TYPE base);
TYPE getCatchTypeAttrs(TYPE specified,TYPE *tester,unsigned char *attrs)
{
    TYPE canonical=TypeCanonicalThr(specified);
    TYPE type=TypeReferenced(canonical),test=StructType(type);
    unsigned char flags;
    unsigned short pointed_flags;
    if(!test) {
        test=TypePointedAt(type,&pointed_flags);
        if(!test) { test=type; flags=0; }
        else {
            type=test;
            test=StructType(test);
            if(!test) { test=type; flags=2; }
            else flags=3;
            TypePointedAt(specified,&pointed_flags);
            if(pointed_flags&0x710) flags|=4;
            else if((pointed_flags&8) && !IsBigData()) flags|=8;
        }
    } else flags=1;
    *attrs=flags;
    *tester=test;
    return canonical;
}
int makeFNCATCH(TYPE type,CSTACK *block,CGIO_LOCATION *posn)
{
    TYPE new_test,old_test;
    unsigned char new_attrs,old_attrs;
    int errors;
    FNCATCH *entry,*test,*last;
    SetErrLoc(posn);
    type=getCatchTypeAttrs(type,&new_test,&new_attrs);
    if(new_attrs&4) CErr1(61);
    else if(new_attrs&8) CErr1(209);
    /* Unlike the released version the old body requests no copy/dtor access
     * and keeps building the catch entry after diagnostics. */
    TypeSigFind(0,type,NULL,&errors);
    last=block->u.t.catches;
    if(last) {
        test=last;
        do {
            unsigned message=0;
            test=test->next;
            if(!test->type) message=612;
            else {
                getCatchTypeAttrs(test->type,&old_test,&old_attrs);
                if(new_attrs==old_attrs) {
                    if(TypesIdentical(new_test,old_test)) message=616;
                    else if((new_attrs&2) && old_test->id==TYP_VOID) message=616;
                    else if((new_attrs&1) && TypeDerived(new_test,old_test)) message=616;
                }
            }
            if(message) {
                CErr1(message);
                CErr2p(615,(const char *)&test->defined);
                break;
            }
        } while(test!=last);
    }
    SetErrLoc(NULL);
    entry=CarveAlloc(carveFNCATCH);
    entry->type=type;
    entry->defined=*posn;
    RingAppend(&block->u.t.catches,entry);
    CgFrontCodePtr(103,block->next->try_var);
    CgFrontCodePtr(104,type);
    return 1;
}
unsigned long CgMemorySize(TYPE type);
SYMBOL updateTryVar(CSTACK *block,TYPE type)
{
    SYMBOL sym=block->next->try_var;
    TYPE array=TypedefModifierRemove(sym->sym_type);
    unsigned long size=CgMemorySize(type);
    if(size>array->u.a.array_size) array->u.a.array_size=size;
    return sym;
}
SYMBOL SymCreateCurrScope(TYPE type,unsigned char id,unsigned flags,char *name);
SYMBOL makeCatchVar(SYMBOL try_var,DECL_INFO *info)
{
    char *name=info->id?info->id->u.id.name:NameDummy();
    SYMBOL sym=SymCreateCurrScope(info->type,11,1,name);
    sym->u.alias=try_var;
    return sym;
}
void CgFrontEmitSwitchLabels(void);
void CgFrontSwitchCase(SCOPE scope,int dead,CGIO_LOCATION *posn,unsigned value);
void LabelBlkCatch(CGIO_LOCATION *posn,CSTACK *block);
void IcEmitDtorSym(SYMBOL sym);
void LabelDeclInited(SYMBOL sym);
int SymRequiresDtoring(SYMBOL sym);
void parseCatchHandler(void)
{
    DECL_INFO *info;
    SYMBOL try_var=NULL,catch_var;
    CSTACK *block;
    CGIO_LOCATION posn,blk;
    SrcFileGetTokenLocn(&posn);
    block=currFunction->control;
    if(block->id!=9 && block->id!=10) { CErr1(611); block=NULL; }
    ExceptionsCheck();
    nextYYToken();
    mustRecog(7);
    if(CurToken==48) { nextYYToken(); info=NULL; }
    else {
        info=ParseException();
        if(!info) block=NULL;
        else if(AbstractClassType(info->type)) CErr1(684);
    }
    mustRecog(8);
    currFunction->dead_code=0;
    SrcPosnEmitCurrent();
    SrcFileGetTokenLocn(&blk);
    mustRecog(11);
    startBlock();
    SrcPosnEmitCurrent();
    if(block) {
        if(!info) makeFNCATCH(NULL,block,&posn);
        else {
            makeFNCATCH(info->type,block,&posn);
            try_var=updateTryVar(block,info->type);
        }
        block->id=10;
        ++block->u.t.catch_no;
        LabelSwitch(block->u.t.defn_scope);
        SrcFileGetTokenLocn(&posn);
        CgFrontSwitchCase(block->u.t.defn_scope,0,&posn,block->u.t.catch_no);
        CgFrontEmitSwitchLabels();
        LabelBlkCatch(&blk,block);
        if(info) {
            catch_var=makeCatchVar(try_var,info);
            FreeDeclInfo(info);
            if(catch_var) {
                LabelDeclInited(catch_var);
                if(SymRequiresDtoring(catch_var)) IcEmitDtorSym(catch_var);
            }
        }
    }
}


void SrcPosnEmit(CGIO_LOCATION *posn);
void CgFrontSwitchEnd(void);
void LabelTryComplete(SYMBOL try_var);
/* This old callback was folded into RingCarveFree in the released source. */
void finiCatch(void *entry)
{
    CarveFree(carveFNCATCH,entry);
}
void completeTry(CSTACK *block)
{
    SrcPosnEmit(&block->u.t.try_locn);
    CgFrontSwitchEnd();
    SrcPosnEmitCurrent();
    dumpOutsideLabel(block);
    CgFrontLabfreeCs(1);
    RingWalk(block->u.t.catches,finiCatch);
    LabelTryComplete(block->u.t.try_var);
}
