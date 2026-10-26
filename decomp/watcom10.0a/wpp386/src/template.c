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
#include <recon.h>
static carve_t carveTEMPLATE_INFO,carveCLASS_INST;
static carve_t carveTEMPLATE_MEMBER,carveFN_TEMPLATE_DEFN;
static struct {
    unsigned max_depth,curr_depth;
    SYMBOL translate_fn;
    TYPE extra_member_class;
    unsigned extra_members:1;
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
