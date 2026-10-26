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
void ExceptionsCheck(void) { STUB_TRAP(); }
void exceptSpec(TYPE type) { (void)type; STUB_TRAP(); }
void FunctionHasRegistration(void) { STUB_TRAP(); }
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
        /* The switch/try variant is not consumed by beginControl. Its
         * older49-byte layout is reserved until those users are reached. */
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


void DtorPrologue(SYMBOL sym,unsigned *end,CGFILE_INS *begin)
{ (void)sym; (void)end; (void)begin; STUB_TRAP(); }

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
int endOfStmt(int recog_token)
{
    CSTACK *block=currFunction->control;
    for(;;) {
        switch(block->id) {
        case 2:
            /* Switch and exception blocks have extra outgoing jumps and
             * scope bookkeeping; their semantics are still pending. */
            if(block->next->id>=5 && block->next->id<=10) {
                STUB_TRAP(); return 0;
            }
            endBlock();
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
void checkDeclLocation(void) { STUB_TRAP(); }
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
        case 1: case 5: case 11: case 57: case 61: case 62: case 72:
        case 85: case 89: case 98: case 101: case 102: case 107:
        case 114: case 115: case 124:
            STUB_TRAP();
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
void DtorEpilogue(SYMBOL func,unsigned end,CGFILE_INS *begin)
{ (void)func; (void)end; (void)begin; STUB_TRAP(); }
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
