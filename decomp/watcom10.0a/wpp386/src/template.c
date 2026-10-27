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


/* Adapted initial6b9cb443 template.c, actual1994 CF50a58..50add.
 * The standalone version has no later dll_subsequent reset. Only reached
 * template state is defined; the callback bodies remain explicit traps. */
#include <stddef.h>
#include "compiler.h"
#include "context.h"
#include "carve.h"
#include "pending.h"
#include "types.h"
#include "template.h"
#include "symbol.h"
#include "ptree.h"
#include "rewrite.h"
#include "class.h"
#include "vstk.h"
#include <recon.h>
static carve_t carveTEMPLATE_INFO,carveCLASS_INST;
static carve_t carveTEMPLATE_MEMBER,carveFN_TEMPLATE_DEFN;
static struct {
    unsigned max_depth,curr_depth;
    SYMBOL translate_fn;
    TYPE extra_member_class;
    unsigned keep_going:1,extra_members:1,depth_diagnosed:1;
} templateData;
static struct {
    NESTED_POST_CONTEXT registration;
    struct template_context *inst_stack;
} activeInstantiations;
static SUICIDE_CALLBACK templateSuicide;
void displayActiveInstantiations(NESTED_POST_CONTEXT *block)
{ (void)block; STUB_TRAP(); }
void templateSuicideHandler(void) { STUB_TRAP(); }
void templateInit(INITFINI *defn)
{
    (void)defn;
    templateData.max_depth=100;
    activeInstantiations.registration.call_back=displayActiveInstantiations;
    CtxRegisterPostContext(&activeInstantiations.registration);
    templateSuicide.call_back=templateSuicideHandler;
    RegisterSuicideCallback(&templateSuicide);
    carveTEMPLATE_INFO=CarveCreate(36,16);
    carveCLASS_INST=CarveCreate(24,32);
    carveTEMPLATE_MEMBER=CarveCreate(12,32);
    carveFN_TEMPLATE_DEFN=CarveCreate(24,16);
}

static TEMPLATE_DATA *currentTemplate;
void *StackPush(void **head,void *entry);
void CErrCheckpoint(unsigned *state);
TYPE setArgIndex(SYMBOL sym,unsigned index)
{
    TYPE type=sym->sym_type;
    type->u.g.index=index;
    type=CheckDupType(type);
    sym->sym_type=type;
    return type;
}
void PTreeErrorExpr(PTREE tree,unsigned error)
{ (void)tree; (void)error; STUB_TRAP(); }
void PTreeFreeSubtrees(PTREE tree);
void injectGenericTypes(DECL_INFO *args)
{
    DECL_INFO *curr;
    SYMBOL sym;
    char *name;
    PTREE def_arg;
    unsigned index=1;
    int unnamed_diagnosed=0;
    if (!args) return;
    curr=args;
    do {
        curr=curr->next;
        name=curr->name;
        sym=curr->generic_sym;
        if (sym) {
            curr->type=setArgIndex(sym,index);
            ++index;
            ScopeInsert(CurrScope,sym,name);
            def_arg=curr->defarg_expr;
            if (def_arg) {
                PTreeErrorExpr(def_arg,162);
                PTreeFreeSubtrees(def_arg);
                curr->defarg_expr=NULL;
            }
        } else if (!name && !unnamed_diagnosed) {
            CErr1(180);
            unnamed_diagnosed=1;
        }
    } while (curr!=args);
}
void TemplateDeclInit(TEMPLATE_DATA *data,DECL_INFO *args)
{
    StackPush((void **)&currentTemplate,data);
    CErrCheckpoint(&data->errors);
    data->all_generic=ProcessTemplateArgs(args)!=0;
    data->args=args;
    data->decl_scope=ScopeBegin(4);
    data->defn=NULL;
    data->member_defn=NULL;
    data->template_name=NULL;
    data->symbol_id=0;
    data->defn_found=0;
    data->member_found=0;
    data->defn_added=0;
    injectGenericTypes(args);
}

void TemplateClassDeclaration(char *name)
{ (void)name; STUB_TRAP(); }
REWRITE *ParseGetRecordingInProgress(CGIO_LOCATION **location);
void commonTemplateClass(TEMPLATE_DATA *data,char *name)
{
    data->symbol_id=12; /* old SC_CLASS_TEMPLATE */
    data->template_name=name;
}
int TemplateClassDefinition(char *name)
{
    TEMPLATE_DATA *data=currentTemplate;
    CGIO_LOCATION *location;
    REWRITE *rewrite=ParseGetRecordingInProgress(&location);
    rewrite=RewritePackageClassTemplate(rewrite,location);
    data->defn=rewrite;
    data->defn_found=1;
    commonTemplateClass(data,name);
    return rewrite==NULL;
}

int SymIsClassTemplateModel(SYMBOL sym);
SYMBOL ClassTemplateLookup(char *name)
{
    SEARCH_RESULT *result=ScopeFindNaked(FileScope,name);
    SYMBOL_NAME sym_name;
    SYMBOL sym;
    if (result) {
        sym_name=result->sym_name;
        ScopeFreeResult(result);
        sym=sym_name->name_type;
        if (sym && SymIsClassTemplateModel(sym)) return sym;
    }
    return 0;
}
void mergeClassTemplates(TEMPLATE_DATA *data,SYMBOL sym)
{ (void)data; (void)sym; STUB_TRAP(); }
unsigned getArgList(DECL_INFO *args,TYPE *types,char **names)
{
    DECL_INFO *curr;
    unsigned count=0;
    if (!args) return 0;
    curr=args;
    do {
        curr=curr->next;
        if (types) types[count]=curr->type;
        if (names) names[count]=curr->name;
        ++count;
    } while (curr!=args);
    return count;
}
unsigned getTinfoArgList(DECL_INFO *args,TEMPLATE_INFO *info)
{
    return getArgList(args,info->type_list,info->arg_names);
}
static TEMPLATE_INFO *allClassTemplates;
void *CPermAlloc(unsigned size);
void RingAppend(void **head,void *entry);
TEMPLATE_INFO *newTemplateInfo(TEMPLATE_DATA *data)
{
    DECL_INFO *args=data->args;
    unsigned count=getArgList(args,0,0);
    TEMPLATE_INFO *info=CarveAlloc(carveTEMPLATE_INFO);
    info->instantiations=0;
    info->member_defns=0;
    info->unbound_type=ClassUnboundTemplate(data->template_name);
    info->num_args=count;
    /* Three defined bits are cleared/assigned in the old source; the upper
     * unused allocator bits are uninitialized there and remain zero here. */
    info->flags=data->defn_found ? 2 : 0;
    info->arg_names=CPermAlloc(count*sizeof(char *));
    info->type_list=CPermAlloc(count*sizeof(TYPE));
    info->defn=data->defn;
    getTinfoArgList(args,info);
    RingAppend((void **)&allClassTemplates,info);
    return info;
}
extern TYPE TypeError;
SYMBOL newTemplateSymbol(TEMPLATE_DATA *data)
{
    SYMBOL sym=AllocSymbol();
    sym->id=data->symbol_id;
    sym->u.tinfo=newTemplateInfo(data);
    sym->sym_type=TypeError;
    return sym;
}
int templateArgListsSame(DECL_INFO *args,TEMPLATE_INFO *info)
{ (void)args; (void)info; STUB_TRAP(); return 0; }
char **getUniqueArgNames(DECL_INFO *args,TEMPLATE_INFO *info)
{ (void)args; (void)info; STUB_TRAP(); return 0; }
void addMemberEntry(TEMPLATE_INFO *info,REWRITE *rewrite,char **names)
{ (void)info; (void)rewrite; (void)names; STUB_TRAP(); }
void addClassTemplateMember(TEMPLATE_DATA *data,SYMBOL sym)
{
    TEMPLATE_INFO *info;
    DECL_INFO *args;
    char **names;
    if (!data->member_found) return;
    info=sym->u.tinfo;
    if (info->flags&1) return;
    args=data->args;
    if (!templateArgListsSame(args,info)) { CErr2p(174,sym); return; }
    names=getUniqueArgNames(args,info);
    addMemberEntry(info,data->member_defn,names);
}
void defineAllClassDecls(SYMBOL sym)
{ (void)sym; STUB_TRAP(); }
void FreeArgs(DECL_INFO *args);
int CErrOccurred(unsigned *state);
void *StackPop(void **head);
void TemplateDeclFini(void)
{
    TEMPLATE_DATA *data;
    SYMBOL sym=0;
    ScopeEnd(4);
    data=currentTemplate;
    if (data->template_name && ScopeType(CurrScope,0)) {
        sym=ClassTemplateLookup(data->template_name);
        if (sym) mergeClassTemplates(data,sym);
        else {
            sym=newTemplateSymbol(data);
            sym=ScopeInsert(CurrScope,sym,data->template_name);
        }
        addClassTemplateMember(data,sym);
    }
    FreeArgs(data->args);
    if (CErrOccurred(&data->errors)) {
        if (sym) ((TEMPLATE_INFO *)sym->u.tinfo)->flags|=1;
    } else if (data->defn_added) defineAllClassDecls(sym);
    StackPop((void **)&currentTemplate);
}

int FunctionUsesAllTypes(SYMBOL sym,SCOPE scope,void (*diag)(SYMBOL));
int SymIsFunction(SYMBOL sym);
void ForceNoDefaultArgs(DECL_INFO *info,unsigned error);
void diagnoseUnusedArg(SYMBOL sym)
{
    CErr2p(409,(char *)sym);
}
void TemplateFunctionCheck(SYMBOL sym,DECL_INFO *info)
{
    TEMPLATE_DATA *data;
    if (!SymIsFunction(sym)) { CErr1(408); return; }
    ForceNoDefaultArgs(info,455);
    data=currentTemplate;
    if (!data->all_generic) CErr1(84);
    else if (FunctionUsesAllTypes(sym,CurrScope,diagnoseUnusedArg)) {
        sym->id=13;
        sym->u.tinfo=NULL;
        sym->sym_type=MakePlusPlusFunction(sym->sym_type);
    }
}

/* The old24-byte function-template definition uses separate carve allocation
 * and ring append, before the released RingCarveAlloc helper. */
#pragma pack(1)
typedef struct fn_template_defn {
    struct fn_template_defn *next;
    SYMBOL sym;
    REWRITE *defn;
    unsigned num_args;
    char **arg_names;
    TYPE *type_list;
} FN_TEMPLATE_DEFN;
#pragma pack()
typedef char FnTemplateDefnSize[(sizeof(FN_TEMPLATE_DEFN)==24)?1:-1];
static FN_TEMPLATE_DEFN *allFunctionTemplates;
FN_TEMPLATE_DEFN *buildFunctionDefn(REWRITE *rewrite,SYMBOL sym)
{
    DECL_INFO *args=currentTemplate->args;
    FN_TEMPLATE_DEFN *defn;
    unsigned count;
    char **names;
    TYPE *types;
    if (CErrOccurred(&currentTemplate->errors)) { RewriteFree(rewrite); return NULL; }
    count=getArgList(args,NULL,NULL);
    names=CPermAlloc(count*sizeof(char *));
    types=CPermAlloc(count*sizeof(TYPE));
    defn=CarveAlloc(carveFN_TEMPLATE_DEFN);
    defn->sym=sym;
    defn->defn=rewrite;
    defn->num_args=count;
    defn->arg_names=names;
    defn->type_list=types;
    getArgList(args,types,names);
    RingAppend((void **)&allFunctionTemplates,defn);
    return defn;
}
void TemplateFunctionAttachDefn(DECL_INFO *info)
{
    SYMBOL sym=info->sym;
    REWRITE *rewrite;
    if (!sym) return;
    rewrite=info->body;
    info->body=NULL;
    if (sym->u.tinfo) {
        RewriteFree(rewrite);
        CErr2p(454,(char *)sym);
    } else sym->u.tinfo=buildFunctionDefn(rewrite,sym);
    FreeDeclInfo(info);
}

int TemplateMemberCanBeIgnored(void) { STUB_TRAP(); return 0; }
int TemplateVerifyDecl(SYMBOL sym) { (void)sym; STUB_TRAP(); return 0; }

void TemplateSpecificDefnEnd(void) { STUB_TRAP(); }

SCOPE SymScope(SYMBOL sym);
int makeSureSymIsAMember(SCOPE scope,SYMBOL sym)
{
    if (templateData.extra_members && !ScopeClass(scope)) {
        SetErrLoc((CGIO_LOCATION *)sym->locn);
        CErr2p(691,templateData.extra_member_class);
        return 1;
    }
    return 0;
}
SYMBOL TemplateFunctionTranslate(SYMBOL sym,SCOPE *parse_scope)
{
    SCOPE scope=SymScope(sym);
    SYMBOL replacement=templateData.translate_fn;
    if (replacement) {
        templateData.translate_fn=NULL;
        sym=replacement;
        *parse_scope=scope->enclosing;
    } else {
        makeSureSymIsAMember(scope,sym);
        *parse_scope=scope;
    }
    return sym;
}

PTREE NodeReverseArgs(unsigned *count,PTREE list);
void NodeFreeDupedExpr(PTREE expr);
PTREE processIndividualParm(TYPE arg_type,PTREE parm)
{ (void)arg_type; (void)parm; STUB_TRAP(); return NULL; }
/* Initial template.c argument classification, exact old error numbers and
 * generic/type ids from CF515d9. Reversal retains the genuine argument nodes. */
PTREE processClassTemplateParms(TEMPLATE_INFO *info,PTREE parms)
{
    unsigned count,index=0,error=0;
    PTREE list,parm;
    TYPE arg_type;
    parms=NodeReverseArgs(&count,parms);
    if(info->flags&1) error=1;
    else if(count!=info->num_args) {
        CErr1(count>info->num_args?214:213);
        error=1;
    } else {
        for(list=parms;list!=NULL;list=list->u.subtree[0]) {
            arg_type=TypedefRemove(info->type_list[index]);
            if(arg_type->id==TYP_GENERIC) arg_type=NULL;
            parm=list->u.subtree[1];
            if(parm->op!=5) {
                if(arg_type!=NULL) {
                    parm=processIndividualParm(arg_type,parm);
                    if(parm->op==0) error=1;
                } else {
                    PTreeErrorExpr(parm,350);
                    error=1;
                }
            } else if(arg_type!=NULL) {
                PTreeErrorExpr(parm,351);
                error=1;
            }
            list->u.subtree[1]=parm;
            ++index;
        }
    }
    if(error) { NodeFreeDupedExpr(parms); parms=NULL; }
    return parms;
}
TYPE instantiateUnboundClass(TEMPLATE_INFO *info,PTREE parms,char *name)
{ (void)info; (void)parms; (void)name; STUB_TRAP(); return NULL; }
SYMBOL ScopeOrderedStart(SCOPE scope);
SYMBOL ScopeOrderedNext(SYMBOL stop,SYMBOL current);
int SymIsConstantInt(SYMBOL sym);
int SymIsTypedef(SYMBOL sym);
SYMBOL SymAddressOf(SYMBOL sym);
int TypesIdentical(TYPE first,TYPE second);
int parmsDifferent(SYMBOL arg,PTREE parm)
{
    SYMBOL sym;
    if(SymIsConstantInt(arg)) {
        if((parm->op==2 || parm->op==3) &&
           arg->u.uval==parm->u.uint_constant) return 0;
    } else if(SymIsTypedef(arg)) {
        if(parm->op==5 && TypesIdentical(arg->sym_type,parm->type)) return 0;
    } else {
        sym=SymAddressOf(arg);
        if(sym!=NULL && parm->op==7 && sym==parm->u.symcg.symbol) return 0;
    }
    return 1;
}
SCOPE sameParms(struct class_inst *instance,PTREE parms)
{
    SCOPE inst_scope=instance->scope;
    SYMBOL current=NULL,stop=ScopeOrderedStart(inst_scope->enclosing);
    for(;;) {
        if(parms==NULL) return inst_scope;
        current=ScopeOrderedNext(stop,current);
        if(current==NULL || parmsDifferent(current,parms->u.subtree[1]))
            return NULL;
        parms=parms->u.subtree[0];
    }
}
SCOPE findInstScope(TEMPLATE_INFO *info,PTREE parms,struct class_inst **instance)
{
    struct class_inst *curr,*stop=info->instantiations;
    SCOPE scope;
    if(stop!=NULL) {
        curr=stop;
        do {
            curr=curr->next;
            scope=sameParms(curr,parms);
            if(scope!=NULL) { *instance=curr; return scope; }
        } while(curr!=stop);
    }
    return NULL;
}
SYMBOL ScopeOrderedFirst(SCOPE scope);
TYPE checkAlreadyDone(TEMPLATE_INFO *info,PTREE parms,struct class_inst **instance)
{
    SCOPE scope=findInstScope(info,parms,instance);
    if(scope!=NULL) return ScopeOrderedFirst(scope)->sym_type;
    return NULL;
}
struct class_inst *newClassInstantiation(TEMPLATE_INFO *info,SCOPE scope)
{
    struct class_inst *instance=CarveAlloc(carveCLASS_INST);
    instance->scope=scope;
    instance->locn.src_file=NULL;
    /* Original clears four owned bits and leaves the unused upper bits
     * indeterminate on fresh allocation; initialize those unused bits to0. */
    instance->flags=0;
    RingAppend((void **)&info->instantiations,instance);
    templateData.keep_going=1;
    return instance;
}
SYMBOL templateArgSym(unsigned char sc,TYPE type)
{
    SYMBOL sym=AllocSymbol();
    sym->id=sc;
    sym->sym_type=type;
    return sym;
}
void DgStoreConstScalar(PTREE expr,TYPE type,SYMBOL sym);
char *NameDummy(void);
/* Actual CF51896..51951 uses one old symbol constructor for both types
 * and values; address aliases always use a pointer to the named symbol. */
void injectTemplateParms(TEMPLATE_INFO *info,SCOPE scope,PTREE parms)
{
    char **names=info!=NULL?info->arg_names:NULL;
    char *name;
    PTREE list,parm;
    TYPE type;
    SYMBOL sym,addr_sym;
    for(list=parms;list!=NULL;list=list->u.subtree[0]) {
        parm=list->u.subtree[1];
        type=parm->type;
        sym=NULL;
        switch(parm->op) {
        case 2: case 3:
            sym=templateArgSym(2,type);
            DgStoreConstScalar(parm,type,sym);
            break;
        case 5:
            sym=templateArgSym(5,type);
            break;
        case 7:
            addr_sym=parm->u.symcg.symbol;
            sym=templateArgSym(15,MakePointerTo(addr_sym->sym_type));
            sym->u.sym=addr_sym;
            break;
        }
        if(names!=NULL) name=*names++;
        else name=NameDummy();
        if(sym!=NULL) ScopeInsert(scope,sym,name);
    }
}
#pragma pack(1)
struct template_context {
    struct template_context *next;
    CGIO_LOCATION *locn;
    void *extra;
    unsigned char id;
};
#pragma pack()
void CSuicide(void);
void verifyOKToProceed(CGIO_LOCATION *locn)
{
    ++templateData.curr_depth;
    if(templateData.curr_depth>templateData.max_depth &&
       !templateData.depth_diagnosed) {
        templateData.depth_diagnosed=1;
        SetErrLoc(locn);
        CErr2p(640,(const char *)(unsigned long)(templateData.max_depth*2));
        CSuicide();
    }
}
void pushInstContext(struct template_context *ctx,unsigned char id,
                     CGIO_LOCATION *locn,void *extra)
{
    verifyOKToProceed(locn);
    ctx->id=id;
    ctx->locn=locn;
    ctx->extra=extra;
    StackPush((void **)&activeInstantiations.inst_stack,ctx);
}
void popInstContext(void)
{
    --templateData.curr_depth;
    StackPop((void **)&activeInstantiations.inst_stack);
}
DECL_SPEC *ParseClassInstantiation(REWRITE *defn,int no_definition);
extern TYPE TypeError;
TYPE doParseClassTemplate(TEMPLATE_INFO *info,REWRITE *defn,
                         CGIO_LOCATION *location,unsigned control)
{
    TYPE type=TypeError;
    DECL_SPEC *spec;
    struct template_context context;
    if(!(info->flags&1)) {
        pushInstContext(&context,1,location,CurrScope);
        spec=ParseClassInstantiation(defn,control);
        popInstContext();
        if(spec!=NULL) { type=spec->partial; PTypeRelease(spec); }
        else info->flags|=1;
    }
    return type;
}
void ScopeClear(SCOPE scope);
int TypeDefined(TYPE type);
TYPE StructOpened(TYPE type);
/* Actual1994 binding has FileScope as its enclosing scope, no later
 * directive-state or ScopeSetParmClass calls, and a boolean no-definition
 * fifth argument. The genuine original ABI pops that one stack argument. */
TYPE instantiateClass(TEMPLATE_INFO *info,REWRITE *defn,PTREE parms,
                      CGIO_LOCATION *location,unsigned control)
{
    struct class_inst *instance=NULL;
    TYPE already=checkAlreadyDone(info,parms,&instance),type;
    SCOPE saved,inst_scope,parm_scope;
    if(already!=NULL) {
        int nothing=0;
        if(TypeDefined(already) || control!=0) nothing=1;
        else {
            already=StructType(already);
            if(StructOpened(already)!=NULL) nothing=1;
        }
        if(nothing) { NodeFreeDupedExpr(parms); return already; }
    }
    saved=CurrScope;
    if(already!=NULL) {
        inst_scope=instance->scope;
        parm_scope=inst_scope->enclosing;
        ScopeClear(parm_scope);
        CurrScope=inst_scope;
    } else {
        CurrScope=FileScope;
        parm_scope=ScopeBegin(6);
        inst_scope=ScopeBegin(5);
        instance=newClassInstantiation(info,inst_scope);
        instance->locn=*location;
        instance->flags|=4;
    }
    injectTemplateParms(info,parm_scope,parms);
    NodeFreeDupedExpr(parms);
    type=doParseClassTemplate(info,defn,location,control);
    CurrScope=saved;
    return type;
}
DECL_SPEC *PTypeClassInstantiation(TYPE type,PTREE id);
void NodeFreeDupedExpr(PTREE expr);
extern TYPE TypeError;
/* Initial template.c, actual1994 CF51a97..51b31. The later
 * NO_DECL_SPEC early return and ScopeAccessType helper are absent here. */
DECL_SPEC *TemplateClassInstantiation(PTREE id,PTREE parms,unsigned control)
{
    char *name=id->u.id.name;
    TYPE type=TypeError;
    SYMBOL sym=ClassTemplateLookup(name);
    TEMPLATE_INFO *info;
    if(sym!=NULL) {
        info=(TEMPLATE_INFO *)sym->u.tinfo;
        parms=processClassTemplateParms(info,parms);
        if(parms!=NULL) {
            if(ScopeType(CurrScope,4)) {
                type=info->unbound_type;
                if(currentTemplate->all_generic)
                    type=instantiateUnboundClass(info,parms,name);
                NodeFreeDupedExpr(parms);
            } else {
                type=instantiateClass(info,info->defn,parms,&id->locn,control);
            }
        }
    } else PTreeFreeSubtrees(parms);
    return PTypeClassInstantiation(type,id);
}

/* Actual1994 generator has no later ambiguous-symbol output argument. */
TYPE BindGenericTypes(arg_list *args,SYMBOL fn,CGIO_LOCATION *locn,unsigned *control);
SYMBOL SymCreateAtLocn(TYPE type,unsigned char id,unsigned flags,char *name,
                       SCOPE scope,CGIO_LOCATION *locn);
TYPE attemptGen(arg_list *args,SYMBOL model,CGIO_LOCATION *locn,unsigned *control)
{
    arg_list *formals;
    TYPE bound;
    struct template_context context;
    formals=TypeArgList(FunctionDeclarationType(model->sym_type));
    if(!formals || formals->num_args!=args->num_args) return NULL;
    pushInstContext(&context,4,locn,model);
    bound=BindGenericTypes(args,model,locn,control);
    popInstContext();
    return bound;
}
SYMBOL buildTemplateFn(TYPE bound,SYMBOL model,CGIO_LOCATION *locn)
{
    SYMBOL sym;
    if(!bound) return NULL;
    /* Original flags preserve only0x80000000 and add template-instance. */
    sym=SymCreateAtLocn(bound,0,(model->flag&0x80000000UL)|0x40000000UL,
                        model->name->name,FileScope,locn);
    sym->u.sym=model;
    return sym;
}
unsigned char TemplateFunctionGenerate(SYMBOL *psym,arg_list *args,CGIO_LOCATION *locn,int has_match)
{
    SYMBOL syms=*psym,sym,previous,models[2],generated;
    TYPE bound,types[2][2];
    unsigned bind_control,control,index;
    if(!ScopeType(SymScope(syms),0)) return 2;
    types[0][0]=types[0][1]=types[1][0]=types[1][1]=NULL;
    /* Original model slots are uninitialized when no type was bound;
     * buildTemplateFn ignores them with NULL type. Zero those bytes here. */
    models[0]=models[1]=NULL;
    bind_control=!has_match;
    if(syms) {
        previous=NULL;
        sym=syms->next;
        while(previous!=syms) {
            if(SymIsFunctionTemplateModel(sym)) {
                control=bind_control;
                bound=attemptGen(args,sym,locn,&control);
                if(bound) {
                    index=(control!=0);
                    types[index][1]=types[index][0];
                    types[index][0]=bound;
                    models[index]=sym;
                    if(types[0][1]) return 1;
                }
            }
            previous=sym;
            sym=sym->next;
        }
    }
    bound=types[0][0];
    sym=models[0];
    if(!bound) {
        sym=models[1];
        bound=types[1][0];
        if(types[1][1]) return 1;
    }
    generated=buildTemplateFn(bound,sym,locn);
    if(generated) { *psym=generated; return 0; }
    return 2;
}

int SymIsFunctionTemplateModel(SYMBOL sym);
int BindFunction(SYMBOL sym,SYMBOL model);
int SymIsInitialized(SYMBOL sym);
SYMBOL SymIsFunctionTemplateInst(SYMBOL sym);
int processSymDefn(SYMBOL sym,FN_TEMPLATE_DEFN *defn)
{
    SYMBOL model,previous;
    struct template_context context;
    int bound;
    if(sym->id==7 || SymIsInitialized(sym)) return 0;
    model=defn->sym;
    previous=SymIsFunctionTemplateInst(sym);
    if(previous && previous!=model) return 0;
    pushInstContext(&context,5,(CGIO_LOCATION *)sym->locn,model);
    bound=BindFunction(sym,model);
    popInstContext();
    if(!bound) return 0;
    /* This older pass marks the original symbol only after its body was
     * instantiated, rather than the later callback setting these fields. */
    sym->u.alias=model;
    sym->flag|=0x40000000UL;
    return 1;
}
void processFunctionTemplateDefns(void)
{
    FN_TEMPLATE_DEFN *defn,*last=allFunctionTemplates;
    SYMBOL sym,stop;
    if(last) {
        defn=last;
        do {
            defn=defn->next;
            stop=defn->sym->name->name_syms;
            if(stop) {
                sym=stop;
                do {
                    sym=sym->next;
                    if(!SymIsFunctionTemplateModel(sym)) processSymDefn(sym,defn);
                } while(sym!=stop);
            }
        } while(defn!=last);
    }
}
int TypeDefined(TYPE type);
TYPE classTemplateType(struct class_inst *instance)
{ return ScopeOrderedFirst(instance->scope)->sym_type; }
int classTemplateWasDefined(struct class_inst *instance)
{ return TypeDefined(classTemplateType(instance)); }
void instantiateMember(TEMPLATE_INFO *info,struct class_inst *instance,TEMPLATE_MEMBER *member)
{ (void)info; (void)instance; (void)member; STUB_TRAP(); }
int TypeDefined(TYPE type);
SYMBOL ScopeOrderedStart(SCOPE scope);
int SymIsFunction(SYMBOL sym);
void processNewFileSyms(SYMBOL old_last,SYMBOL current_last)
{
    SYMBOL curr=old_last,stop=ScopeOrderedStart(FileScope),fn,last;
    FN_TEMPLATE_DEFN *defn;
    for(;;) {
        curr=ScopeOrderedNext(stop,curr);
        if(!curr) break;
        if(SymIsFunction(curr)) {
            last=curr->name->name_syms;
            if(last) {
                fn=last;
                do {
                    fn=fn->next;
                    if(SymIsFunctionTemplateModel(fn)) {
                        defn=fn->u.tinfo;
                        if(defn && processSymDefn(curr,defn)) break;
                    }
                } while(fn!=last);
            }
        }
        if(curr==current_last) break;
    }
}
void freeDefns(void)
{
    REWRITE *rewrite;
    TEMPLATE_INFO *info,*last_info=allClassTemplates;
    TEMPLATE_MEMBER *member,*last_member;
    FN_TEMPLATE_DEFN *fn,*last_fn=allFunctionTemplates;
    if(last_info) {
        info=last_info;
        do {
            info=info->next;
            rewrite=info->defn;
            info->defn=NULL;
            RewriteFree(rewrite);
            last_member=info->member_defns;
            if(last_member) {
                member=last_member;
                do {
                    member=member->next;
                    rewrite=member->defn;
                    member->defn=NULL;
                    RewriteFree(rewrite);
                } while(member!=last_member);
            }
        } while(info!=last_info);
    }
    if(last_fn) {
        fn=last_fn;
        do {
            fn=fn->next;
            rewrite=fn->defn;
            fn->defn=NULL;
            RewriteFree(rewrite);
        } while(fn!=last_fn);
    }
}
SYMBOL ScopeOrderedStart(SCOPE scope);
void TemplateProcessInstantiations(void)
{
    TEMPLATE_INFO *info,*last_info;
    struct class_inst *instance,*last_instance;
    TEMPLATE_MEMBER *member,*last_member;
    SYMBOL old_last=ScopeOrderedStart(FileScope),current_last;
    processFunctionTemplateDefns();
    for(;;) {
        verifyOKToProceed(NULL);
        templateData.keep_going=0;
        templateData.extra_members=1;
        last_info=allClassTemplates;
        if(last_info) {
            info=last_info;
            do {
                info=info->next;
                last_instance=info->instantiations;
                if(!last_instance) continue;
                instance=last_instance;
                do {
                    instance=instance->next;
                    if(instance->flags&3) continue;
                    if(!classTemplateWasDefined(instance)) continue;
                    templateData.extra_member_class=classTemplateType(instance);
                    instance->flags|=1;
                    last_member=info->member_defns;
                    if(!last_member) continue;
                    member=last_member;
                    do {
                        member=member->next;
                        instantiateMember(info,instance,member);
                    } while(member!=last_member);
                } while(instance!=last_instance);
            } while(info!=last_info);
        }
        templateData.extra_members=0;
        current_last=ScopeOrderedStart(FileScope);
        if(current_last!=old_last) {
            processNewFileSyms(old_last,current_last);
            templateData.keep_going=1;
            old_last=current_last;
        }
        if(!templateData.keep_going) break;
    }
    templateData.extra_members=0;
    freeDefns();
}

void injectFunctionTemplateArgs(SYMBOL model)
{
    FN_TEMPLATE_DEFN *defn=model->u.tinfo;
    unsigned i;
    TYPE type;
    SYMBOL parm;
    for(i=0;i<defn->num_args;++i) {
        type=TypedefModifierRemove(defn->type_list[i]);
        parm=templateArgSym(5,type->of);
        ScopeInsert(CurrScope,parm,defn->arg_names[i]);
    }
}
void clearGenericBindings(VSTK_CTL *stack);
void ParseFunctionInstantiation(REWRITE *rewrite);
void TemplateFunctionInstantiate(SYMBOL sym,SYMBOL model,void *bindings)
{
    SCOPE save_scope=CurrScope;
    SYMBOL save_fn;
    FN_TEMPLATE_DEFN *defn=model->u.tinfo;
    struct template_context context;
    /* The actual old path starts at FileScope and has no ScopeSetParmFn. */
    CurrScope=FileScope;
    ScopeBegin(6);
    injectFunctionTemplateArgs(model);
    ScopeBegin(5);
    clearGenericBindings(bindings);
    save_fn=templateData.translate_fn;
    templateData.translate_fn=sym;
    pushInstContext(&context,2,(CGIO_LOCATION *)sym->locn,sym);
    ParseFunctionInstantiation(defn->defn);
    popInstContext();
    CurrScope=save_scope;
    templateData.translate_fn=save_fn;
}
