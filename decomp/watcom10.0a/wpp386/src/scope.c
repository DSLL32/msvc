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


/* Adapted from initial6b9cb443 scope.c/symtype.h. Actual1994
 * CF473cc..474bc has nine pools and one file scope, before namespaces,
 * internal scope, scope-region and namespace allocation/reporting extensions. */
#include "scope.h"
#include "pending.h"
#include "types.h"
#include "symbol.h"
#include "vftable.h"
#include "vstk.h"
#include <recon.h>
static carve_t carveSCOPE, carveSYMBOL, carveSYMBOL_NAME;
static carve_t carveBASE_STACK, carveBASE_PATH, carvePATH_CAP;
static carve_t carveSEARCH_RESULT, carveGEN_LEAP, carveSYMBOL_EXCLUDE;
SCOPE CurrScope, FileScope;
void PCHActivate(void);
void injectGlobalOpNew(void);
void injectGlobalOpDelete(void);
void scopeInit(INITFINI *defn)
{
    (void)defn;
    PCHActivate();
    carveSCOPE=CarveCreate(sizeof(struct scope),64);
    carveSYMBOL=CarveCreate(32,64);
    carveSYMBOL_NAME=CarveCreate(sizeof(struct symbol_name),64);
    carveBASE_STACK=CarveCreate(30,16);
    carveBASE_PATH=CarveCreate(22,16);
    carvePATH_CAP=CarveCreate(34,16);
    carveSEARCH_RESULT=CarveCreate(60,16);
    carveGEN_LEAP=CarveCreate(12,16);
    carveSYMBOL_EXCLUDE=CarveCreate(8,16);
    HashPostInit(0);
    ScopeBegin(0);
    FileScope=CurrScope;
    HashPostInit(FileScope);
    HashMakeMax(FileScope->names);
    injectGlobalOpNew();
    injectGlobalOpDelete();
}
SYMBOL_NAME AllocSymbolName(char *name,SCOPE scope)
{
    SYMBOL_NAME new_sym;
    new_sym=CarveAlloc(carveSYMBOL_NAME);
    new_sym->next=0;
    new_sym->name_type=0;
    new_sym->name_syms=0;
    new_sym->name=name;
    new_sym->containing=scope;
    return new_sym;
}
SCOPE ScopeSetContaining(SYMBOL_NAME name,SCOPE scope)
{
    SCOPE old_containing;
    old_containing=name->containing;
    name->containing=scope;
    return old_containing;
}
/* Original ScopeCreate owns its cold initialization block472c9..472f3. */
SCOPE ScopeCreate(unsigned char id)
{
    SCOPE scope;
    scope=CarveAlloc(carveSCOPE);
    scope->id=id;
    scope->keep=(id==1);
    scope->enclosing=0;
    scope->ordered=0;
    scope->owner=0;
    scope->dtor_reqd=0;
    scope->dtor_naked=0;
    scope->try_catch=0;
    scope->arg_check=0;
    scope->names=HashCreate();
    return scope;
}
void ScopeOpen(SCOPE scope)
{
    scope->enclosing=CurrScope;
    CurrScope=scope;
}
SCOPE ScopeBegin(unsigned char id)
{
    SCOPE scope;
    scope=ScopeCreate(id);
    ScopeOpen(scope);
    return scope;
}
char *CppOperatorName(unsigned oper);
void injectOpNew(TYPE fn_type,char *name)
{
    SYMBOL sym;
    sym=SymCreateFileScope(fn_type,1,0,name);
    LinkageSet(sym,"C++");
}
/* Actual1994 ppops CO_NEW63 / CO_NEW_ARRAY65, before the later
 * reordering. Shared terminal return48bed..48bf4 belongs to this body. */
void injectGlobalOpNew(void)
{
    TYPE pvoid_type,size_type,size_fn_type,placement_fn_type;
    char *simple_name,*array_name;
    pvoid_type=TypePtrToVoid();
    size_type=GetBasicType(TYP_UINT);
    simple_name=CppOperatorName(63);
    array_name=CppOperatorName(65);
    size_fn_type=MakeSimpleFunction(pvoid_type,size_type,(TYPE)0);
    injectOpNew(size_fn_type,simple_name);
    injectOpNew(size_fn_type,array_name);
    placement_fn_type=MakeSimpleFunction(pvoid_type,size_type,pvoid_type,(TYPE)0);
    injectOpNew(placement_fn_type,simple_name);
    injectOpNew(placement_fn_type,array_name);
}
SYMBOL injectOpDel(char *name)
{
    SYMBOL sym;
    sym=SymCreateFileScope(TypeVoidFunOfPtrVoid(),1,0,name);
    LinkageSet(sym,"C++");
    return sym;
}
void injectGlobalOpDelete(void)
{
    injectOpDel(CppOperatorName(58));
    injectOpDel(CppOperatorName(60));
}

/* Actual1994 CF478ce..47917; exact old symbol32 layout. */
SYMBOL AllocSymbol(void)
{
    SYMBOL sym;
    sym=CarveAlloc(carveSYMBOL);
    sym->next=0;
    sym->sym_type=0;
    sym->name=0;
    sym->locn=0;
    sym->flag=0;
    sym->flag2=0;
    sym->id=0;
    sym->segid=0;
    sym->u.tinfo=0;
    sym->thread=0;
    return sym;
}
SYMBOL ScopeInsert(SCOPE scope,SYMBOL sym,char *name)
{
    ScopeCarefulInsert(scope,&sym,name);
    return sym;
}
SYMBOL_NAME scopeInsertName(SCOPE scope,SYMBOL sym,char *name)
{
    SYMBOL_NAME sym_name,check_name;
    SCOPE enclosing;
    sym_name=HashLookup(scope->names,name);
    if (sym_name==0) {
        if (scope->arg_check) {
            enclosing=scope->enclosing;
            while (enclosing->id!=2) enclosing=enclosing->enclosing;
            if (enclosing->id==2) {
                check_name=HashLookup(enclosing->names,name);
                if (check_name!=0) CErr2p(42,check_name->name_syms);
            }
        }
        sym_name=AllocSymbolName(name,scope);
        HashInsert(scope->names,sym_name,name);
        ScopeKeep(scope);
    }
    sym->name=sym_name;
    return sym_name;
}
void ScopeKeep(SCOPE scope) { scope->keep=1; }
/* Actual1994 CF47be2..47c52 predates DC_KEEP_SYMBOL and the masked
 * DC_REDEFINED return: it always frees a replaced input and returns status. */
unsigned ScopeCarefulInsert(SCOPE scope,SYMBOL *psym,char *name)
{
    unsigned decl_status;
    SYMBOL_NAME sym_name;
    SYMBOL sym,decl_sym;
    sym=*psym;
    sym_name=scopeInsertName(scope,sym,name);
    if (scope==FileScope) LinkageSet(sym,0);
    decl_sym=DeclCheck(sym_name,sym,&decl_status);
    if (decl_sym!=sym) {
        FreeSymbol(sym);
        *psym=decl_sym;
        return decl_status;
    }
    addOrdered(scope,sym);
    if (scope->id==1) ClassMember(scope,sym);
    DeclDefaultStorageClass(scope,sym);
    return 0;
}
void FreeSymbol(SYMBOL sym) { CarveFree(carveSYMBOL,sym); }
/* Actual1994 CF479a5..479ca: ordered symbols form a ring through
 * thread, distinct from the next ring used by overloaded symbol names. */
void addOrdered(SCOPE scope,SYMBOL sym)
{
    SYMBOL last;
    if (sym->id!=7) {
        last=(SYMBOL)scope->ordered;
        if (last==0) sym->thread=sym;
        else { sym->thread=last->thread; last->thread=sym; }
        scope->ordered=sym;
    }
}


int ScopeType(SCOPE scope,unsigned char id) { return scope!=0 && scope->id==id; }

void ScopeBeginFunction(struct symbol *sym)
{
    ScopeBegin(2);
    CurrScope->owner=sym;
    ScopeKeep(CurrScope);
}

SCOPE findFunctionScope(SCOPE scope)
{
    while (scope && scope->id!=2) scope=scope->enclosing;
    return scope;
}
SCOPE ScopeFunctionScopeInProgress(void)
{ return findFunctionScope(CurrScope); }

int ScopeEnclosed(SCOPE outer,SCOPE inner)
{
    while (inner) {
        if (outer==inner) return 1;
        inner=inner->enclosing;
    }
    return 0;
}

void ScopeBeginBlock(unsigned index)
{
    ScopeBegin(3);
    CurrScope->owner=(void *)index;
}


/* Actual old lookup state:52B, without later ignore/fn_sym/namespace and
 * overload-list additions. Paths expose the existing20-byte symbol-name
 * field; their unreached path contents remain owned by searchScope. */
#pragma pack(1)
/* Reached BASE_PATH prefix; the permission tail is owned by its pending
 * allocation/recording helpers. */
struct base_path {
    struct base_path *next,*prev;
    struct base_class *base;
    SCOPE scope;
    unsigned short flag;
    unsigned flags;
};
typedef struct path_cap PATH_CAP;
struct path_cap {
    PATH_CAP *next;
    struct base_path *head,*tail;
    SCOPE access_decl;
    struct symbol_exclude *exclude;
    SYMBOL_NAME sym_name;
    SYMBOL sym;
    unsigned short flag;
    unsigned flags;
};
typedef struct lookup_walk {
    SCOPE start,disambiguate;
    char *name;
    TYPE type,fn_type;
    int (*is_special)(char *name);
    SYMBOL selected_conversion;
    SYMBOL info1;
    SCOPE lexical_scope;
    PATH_CAP *paths;
    unsigned path_count;
    unsigned short perm,this_qualifier;
    unsigned flags;
} LOOKUP_WALK;
#pragma pack()
typedef char LookupWalkSize[(sizeof(LOOKUP_WALK)==52)?1:-1];
enum { LOOKUP_IGNORE_ACCESS=0x1000 };
void newLookupData(LOOKUP_WALK *data,char *name)
{
    data->start=0;
    data->disambiguate=0;
    data->name=name;
    data->type=0;
    data->fn_type=0;
    data->is_special=0;
    data->selected_conversion=0;
    data->info1=0;
    data->lexical_scope=0;
    data->paths=0;
    data->path_count=0;
    data->perm=0;
    data->this_qualifier=0;
    /* The1994 initializer clears the21 defined flag bits while preserving
     *11 unused uninitialized stack bits. Keep those unused bits zero. */
    data->flags=0;
}
#pragma pack(1)
/* Actual1994 inheritance record: offset flag14 and type4. */
typedef struct base_class BASE_CLASS;
struct base_class {
    BASE_CLASS *next;
    TYPE type;
    long delta;
    unsigned short vb_index,flag;
};
typedef struct base_stack BASE_STACK;
struct base_stack {
    BASE_STACK *next,*parent;
    BASE_CLASS *base;
    SCOPE scope;
    SYMBOL access_changed;
    void *hold;
    unsigned short access_perm;
    unsigned used:1,reserved:31;
};
#pragma pack()
typedef char BaseClassSize[(sizeof(BASE_CLASS)==16)?1:-1];
typedef char BaseStackSize[(sizeof(BASE_STACK)==30)?1:-1];
typedef enum { WALK_NORMAL,WALK_FINISH,WALK_ABANDON } WALK_STATUS;
typedef WALK_STATUS (*WALK_ROUTINE)(BASE_STACK *,void *);
void *StackPush(void **head,void *entry);
void *StackPop(void **head);
TYPE ScopeClass(SCOPE scope);
BASE_CLASS *ScopeInherits(SCOPE scope)
{
    TYPE type;
    if (scope->id==1) {
        type=ScopeClass(scope);
        if (type) return type->u.c.info->bases;
    }
    return NULL;
}
void pushBase(BASE_STACK **head,BASE_CLASS *base,SCOPE scope,BASE_STACK *parent)
{
    BASE_STACK *item=CarveAlloc(carveBASE_STACK);
    item->access_changed=NULL;
    item->hold=NULL;
    item->access_perm=0;
    item->parent=parent;
    item->base=base;
    item->scope=scope;
    item->used=0;
    /* The original leaves the unused high allocator bits untouched. */
    item->reserved=0;
    StackPush((void **)head,item);
}
BASE_STACK *popBase(BASE_STACK **head)
{
    BASE_STACK *top;
    for (;;) {
        top=*head;
        if (!top) return NULL;
        if (!top->used) break;
        StackPop((void **)head);
        CarveFree(carveBASE_STACK,top);
    }
    top->used=1;
    return top;
}
void cleanStack(BASE_STACK **head)
{
    while (popBase(head)) ;
}
int notABase(SCOPE scope,SCOPE disambiguate)
{ (void)scope; (void)disambiguate; STUB_TRAP(); return 0; }
PATH_CAP *newCap(void)
{
    PATH_CAP *cap=CarveAlloc(carvePATH_CAP);
    cap->head=NULL;
    cap->tail=NULL;
    cap->access_decl=NULL;
    cap->sym_name=NULL;
    cap->sym=NULL;
    cap->exclude=NULL;
    cap->flag=0;
    /* Original clears the two defined bits, leaving allocator padding. */
    cap->flags=0;
    return cap;
}
struct base_path *newPath(PATH_CAP *cap)
{
    struct base_path *path=CarveAlloc(carveBASE_PATH),*head;
    path->prev=NULL;
    path->base=NULL;
    path->scope=NULL;
    path->flag=0;
    path->flags=0; /* two original defined bits; unused bits are unspecified */
    if (cap->tail) {
        head=cap->head;
        head->prev=path;
        path->next=head;
        cap->head=path;
    } else {
        path->next=NULL;
        cap->head=path;
        cap->tail=path;
    }
    return path;
}
int okToUseAccess(LOOKUP_WALK *data,SYMBOL sym,struct base_path *path)
{ (void)data; (void)sym; (void)path; STUB_TRAP(); return 0; }
PATH_CAP *recordPath(LOOKUP_WALK *data,BASE_STACK *top,SYMBOL_NAME name,SYMBOL sym)
{
    PATH_CAP *cap=newCap();
    struct base_path *path,*last=NULL;
    BASE_CLASS *base;
    SCOPE disambiguate=data->disambiguate,access_scope=NULL,scope;
    SYMBOL access_sym;
    cap->next=data->paths;
    data->paths=cap;
    ++data->path_count;
    cap->flags|=1;
    if (!disambiguate) cap->flags&=~1;
    cap->sym_name=name;
    cap->sym=sym;
    for (;top;top=top->parent) {
        path=newPath(cap);
        scope=top->scope;
        if (scope==disambiguate) cap->flags&=~1;
        path->scope=scope;
        base=top->base;
        path->base=base;
        if (base) {
            if (base->flag&8) cap->flags|=2;
            path->flag=base->flag;
        }
        access_sym=top->access_changed;
        if (access_sym && okToUseAccess(data,access_sym,last)) {
            last->flag=top->access_perm;
            access_scope=scope;
        }
        last=path;
    }
    cap->access_decl=access_scope;
    return cap;
}
unsigned short makePerm(unsigned flags)
{
    if(flags&0x40000000UL) return 1;
    if(flags&0x80000000UL) return 2;
    return 4;
}
SYMBOL foundUserConv(LOOKUP_WALK *data,SYMBOL syms)
{ (void)data; (void)syms; STUB_TRAP(); return NULL; }
int ClassTypeName(SYMBOL_NAME name);
int EnumTypeName(SYMBOL_NAME name);
/* Actual1994 callback predates the virtual-override and ignore-scope
 * extensions in the released memberSearch. */
WALK_STATUS memberSearch(BASE_STACK *top,void *parm)
{
    LOOKUP_WALK *data=parm;
    SCOPE scope=top->scope,disambiguate;
    SYMBOL_NAME name;
    SYMBOL sym=NULL,syms;
    PATH_CAP *cap;
    if (!data->name) {
        disambiguate=data->disambiguate;
        if (disambiguate) {
            if (notABase(scope,disambiguate)) return WALK_ABANDON;
            if (scope==disambiguate) {
                if (data->flags&8) {
                    name=data->selected_conversion->name;
                    recordPath(data,top,name,data->selected_conversion);
                } else {
                    recordPath(data,top,NULL,NULL);
                    if (!(data->flags&0x800)) return WALK_FINISH;
                }
            }
        }
        return WALK_NORMAL;
    }
    name=HashLookup(scope->names,data->name);
    if (!name) return WALK_NORMAL;
    if (data->flags&0x60) {
        if (data->flags&0x40) {
            if (!ClassTypeName(name)) return WALK_NORMAL;
        } else if (!EnumTypeName(name)) return WALK_NORMAL;
    } else {
        syms=name->name_syms;
        if (syms && syms->id==6) {
            top->access_perm=makePerm(syms->flag);
            top->access_changed=syms;
            return WALK_NORMAL;
        }
        if (data->flags&4) {
            sym=foundUserConv(data,syms);
            if (!sym) return WALK_NORMAL;
        }
    }
    cap=recordPath(data,top,name,sym);
    if (data->flags&0x10) return WALK_NORMAL;
    if (!data->disambiguate || !(cap->flags&1)) return WALK_ABANDON;
    return WALK_NORMAL;
}
void walkOneScope(SCOPE scope,WALK_ROUTINE routine,void *parm)
{
    BASE_STACK *stack=NULL;
    pushBase(&stack,NULL,scope,NULL);
    routine(stack,parm);
    cleanStack(&stack);
}
void walkInheritedDirectBases(SCOPE scope,WALK_ROUTINE routine,void *parm)
{ (void)scope; (void)routine; (void)parm; STUB_TRAP(); }
void walkImmediateBases(SCOPE scope,WALK_ROUTINE routine,void *parm)
{ (void)scope; (void)routine; (void)parm; STUB_TRAP(); }
void walkDirectBases(SCOPE scope,WALK_ROUTINE routine,void *parm)
{
    BASE_STACK *stack=NULL,*top;
    BASE_CLASS *bases,*base;
    WALK_STATUS status;
    pushBase(&stack,NULL,scope,NULL);
    status=routine(stack,parm);
    if (status==WALK_FINISH) {
        cleanStack(&stack);
        return;
    }
    if (status==WALK_ABANDON) popBase(&stack);
    for (;;) {
        top=popBase(&stack);
        if (!top) break;
        bases=ScopeInherits(top->scope);
        if (!bases) continue;
        base=bases;
        do {
            base=base->next;
            if (base->flag&0x40) {
                pushBase(&stack,base,base->type->u.c.scope,top);
                status=routine(stack,parm);
                if (status==WALK_FINISH) {
                    cleanStack(&stack);
                    return;
                }
                if (status==WALK_ABANDON) popBase(&stack);
            }
        } while (base!=bases);
    }
}
void RingWalk(void *head,void (*routine)(void *));
void deletePath(struct base_path *path) { CarveFree(carveBASE_PATH,path); }
void deleteExclude(void *exclude) { CarveFree(carveSYMBOL_EXCLUDE,exclude); }
void deleteCap(PATH_CAP *cap)
{
    struct base_path *path,*next;
    for (path=cap->head;path;path=next) {
        next=path->next;
        deletePath(path);
    }
    RingWalk(cap->exclude,deleteExclude);
    CarveFree(carvePATH_CAP,cap);
}
void removeDead(LOOKUP_WALK *data)
{
    PATH_CAP *cap,*next,**previous=&data->paths;
    for (cap=data->paths;cap;cap=next) {
        next=cap->next;
        if (cap->flags&1) {
            *previous=next;
            deleteCap(cap);
            --data->path_count;
        } else previous=&cap->next;
    }
}
int findBestConversion(LOOKUP_WALK *data)
{ (void)data; STUB_TRAP(); return 0; }
int findVirtualOverride(LOOKUP_WALK *data)
{ (void)data; STUB_TRAP(); return 0; }
void applyDominance(LOOKUP_WALK *data)
{ (void)data; STUB_TRAP(); }
void reduceToOnePath(LOOKUP_WALK *data)
{ (void)data; STUB_TRAP(); }
void findBestAccess(LOOKUP_WALK *data)
{ (void)data; STUB_TRAP(); }
void applyAmbiguity(SCOPE scope,LOOKUP_WALK *data)
{ (void)scope; (void)data; STUB_TRAP(); }
/* The1994 body calls removeDead directly; later source gives that operation
 * the inlined descriptive wrapper applyDisambiguation. */
int findSinglePath(LOOKUP_WALK *data,SCOPE scope)
{
    data->start=scope;
    if (data->flags&0x80) walkOneScope(scope,memberSearch,data);
    else if (data->flags&0x100) walkInheritedDirectBases(scope,memberSearch,data);
    else if (data->flags&0x200) walkImmediateBases(scope,memberSearch,data);
    else walkDirectBases(scope,memberSearch,data);
    removeDead(data);
    if (!data->paths) return 0;
    if (data->flags&0x10) return findBestConversion(data);
    if (data->flags&1) return findVirtualOverride(data);
    if (data->path_count==1) return 1;
    applyDominance(data);
    if (data->path_count==1) return 1;
    if (data->flags&LOOKUP_IGNORE_ACCESS) reduceToOnePath(data);
    else {
        applyAmbiguity(scope,data);
        if (data->path_count!=1) findBestAccess(data);
    }
    return 1;
}
#pragma pack(1)
typedef struct access_data {
    struct base_path *path;
    SCOPE access,member,located;
    unsigned short perm;
    unsigned protected_OK:1;
} ACCESS_DATA;
#pragma pack()
typedef struct qualify_stack {
    struct qualify_stack *next;
    SCOPE reset,access;
} QUALIFICATION;
void *ParseCurrQualification(void);
SCOPE findAccessScope(void)
{
    SCOPE scope=CurrScope;
    QUALIFICATION *qual=ParseCurrQualification();
    if(qual!=NULL) scope=qual->access;
    return scope;
}
SCOPE nextAccessScope(SCOPE scope)
{
    if(scope->id==1) {
        do { scope=scope->enclosing; } while(scope!=NULL && scope->id==1);
    } else scope=scope->enclosing;
    return scope;
}
void newAccessData(ACCESS_DATA *data,unsigned short perm,SCOPE located)
{
    SCOPE access=findAccessScope(),check,member=NULL;
    for(check=access;check!=NULL;check=nextAccessScope(check)) {
        if(check->id==1) { member=check; break; }
    }
    data->path=NULL;
    data->access=access;
    data->member=member;
    data->located=located;
    data->perm=perm;
    data->protected_OK=0;
}
int isScopeFriend(SCOPE access,SCOPE located)
{ (void)access; (void)located; STUB_TRAP(); return 0; }
int protectedPathOK(ACCESS_DATA *data);
unsigned short verifyAccess(ACCESS_DATA *data)
{
    unsigned short perm=data->perm;
    struct base_path *path=data->path;
    if(perm==4) return 4;
    if(path) {
        if(path->flags&1) {
            if(!(path->flags&2)) return 4;
            if(perm==1) return 1;
        }
        path->flags|=1;
    }
    if(data->member==data->located) return 4;
    if(isScopeFriend(data->access,data->located)) return 4;
    if(path) path->flags|=2;
    if(perm==1) return 1;
    if(perm==2 && !data->protected_OK) {
        if(path && protectedPathOK(data)) return 4;
        return 2;
    }
    return 4;
}
unsigned short setAccess(PATH_CAP *cap)
{
    struct base_path *path=cap->head,*next;
    unsigned short perm=4;
    ACCESS_DATA data;
    while(path!=NULL) {
        next=path->next;
        if(next==NULL) break;
        perm=next->flag&7;
        newAccessData(&data,perm,path->scope);
        data.path=path;
        perm=verifyAccess(&data);
        if(perm!=4) break;
        path=next;
    }
    cap->flag=perm;
    return perm;
}
int protectedPathOK(ACCESS_DATA *data)
{ (void)data; STUB_TRAP(); return 0; }
int setProtectedAccess(LOOKUP_WALK *data,PATH_CAP *cap)
{
    struct base_path *path;
    SYMBOL sym,syms;
    SYMBOL_NAME name;
    ACCESS_DATA access;
    int checked=0;
    if(data->flags&0x100000) return 1;
    if(cap==NULL) cap=data->paths;
    path=cap->tail;
    newAccessData(&access,2,path->scope);
    access.path=path;
    sym=cap->sym;
    if(sym!=NULL) {
        if(sym->flag&0x80000000u) {
            checked=1;
            if(protectedPathOK(&access)) data->flags|=0x100000;
        }
    } else {
        name=cap->sym_name;
        if(name!=NULL) {
            syms=name->name_syms;
            if(syms==NULL) syms=name->name_type;
            if(syms!=NULL) {
                sym=syms->next;
                do {
                    if(sym->flag&0x80000000u) {
                        checked=1;
                        if(protectedPathOK(&access)) data->flags|=0x100000;
                        break;
                    }
                    if(sym==syms) break;
                    sym=sym->next;
                } while(1);
            }
        }
    }
    return checked;
}
int findMember(LOOKUP_WALK *data,SCOPE scope)
{
    PATH_CAP *cap;
    if (!findSinglePath(data,scope)) return 0;
    if (!(data->flags&LOOKUP_IGNORE_ACCESS)) {
        cap=data->paths;
        if (!(cap->flag&7)) setAccess(cap);
        setProtectedAccess(data,NULL);
    }
    return 1;
}
int ClassTypeName(SYMBOL_NAME name)
{ (void)name; STUB_TRAP(); return 0; }
int EnumTypeName(SYMBOL_NAME name)
{ (void)name; STUB_TRAP(); return 0; }
void recordLocation(LOOKUP_WALK *data,SCOPE scope,SYMBOL_NAME name)
{
    BASE_STACK *stack=NULL;
    pushBase(&stack,NULL,scope,NULL);
    recordPath(data,stack,name,NULL);
    cleanStack(&stack);
}
int searchScope(LOOKUP_WALK *data,SCOPE scope)
{
    SYMBOL_NAME name;
    if (scope->id==1) return findMember(data,scope);
    if (data->disambiguate) {
        if (scope==FileScope) return findMember(data,data->disambiguate);
        return 0;
    }
    if (data->flags&2) return 0; /* user-defined conversion search */
    name=HashLookup(scope->names,data->name);
    if (!name) return 0;
    if (data->flags&0x40) {
        if (!ClassTypeName(name)) return 0;
    } else if ((data->flags&0x20) && !EnumTypeName(name)) return 0;
    recordLocation(data,scope,name);
    return 1;
}
void delLookupData(LOOKUP_WALK *data)
{
    PATH_CAP *cap;
    for (cap=data->paths;cap;cap=cap->next) cap->flags|=1;
    removeDead(data);
}
SYMBOL_NAME ScopeYYLexical(SCOPE scope,char *name)
{
    LOOKUP_WALK data;
    SYMBOL_NAME found;
    newLookupData(&data,name);
    data.flags|=LOOKUP_IGNORE_ACCESS;
    for (;;) {
        if (!scope) { found=0; break; }
        /* Unlike the later source, old file-scope lookups use HashLookup. */
        if (scope->id==1) {
            if (searchScope(&data,scope)) {
                found=data.paths->sym_name;
                delLookupData(&data);
                break;
            }
        } else {
            found=HashLookup(scope->names,name);
            if (found) break;
        }
        scope=scope->enclosing;
    }
    return found;
}
SYMBOL_NAME ScopeYYMember(SCOPE scope,char *name)
{
    LOOKUP_WALK data;
    SYMBOL_NAME found=0;
    newLookupData(&data,name);
    data.flags|=LOOKUP_IGNORE_ACCESS;
    if (searchScope(&data,scope)) {
        found=data.paths->sym_name;
        delLookupData(&data);
    }
    return found;
}

void lexicalLookup(LOOKUP_WALK *data,SCOPE scope);
SEARCH_RESULT *makeResult(LOOKUP_WALK *data);
SEARCH_RESULT *ScopeFindLexicalClassType(SCOPE scope,char *name)
{
    LOOKUP_WALK data;
    newLookupData(&data,name);
    data.flags|=0x40;
    lexicalLookup(&data,scope);
    return makeResult(&data);
}
void deleteResult(SEARCH_RESULT *result)
{
    CarveFree(carveSEARCH_RESULT,result);
}
void ScopeFreeResult(SEARCH_RESULT *result)
{
    if(result) deleteResult(result);
}
int searchError(SEARCH_RESULT *result,SYMBOL sym,unsigned message)
{ (void)result; (void)sym; (void)message; STUB_TRAP(); return 0; }
int diagnoseAmbiguity(SEARCH_RESULT *result,SYMBOL sym)
{
    if(result->flags&0x10) return searchError(result,sym,147);
    return 0;
}
int ScopeAmbiguousSymbol(SEARCH_RESULT *result,SYMBOL sym)
{
    if(result->scope->id!=1) return 0;
    if((result->flags&0x20) && sym->id!=2) result->flags|=0x10;
    return diagnoseAmbiguity(result,sym);
}
int DefargBeingCompiled(void);
int ScopeCheckSymbol(SEARCH_RESULT *result,struct symbol *sym)
{
    SCOPE located=result->scope;
    unsigned message;
    unsigned short perm;
    ACCESS_DATA access;
    if((result->flags&0x40) && sym->id>=3 && sym->id<=4) {
        if(DefargBeingCompiled()) message=ScopeType(located,2)?255:256;
        else message=ScopeType(located,2)?365:352;
        return searchError(result,sym,message);
    }
    if(located->id!=1) return 0;
    if(ScopeAmbiguousSymbol(result,sym)) return 1;
    perm=result->perm;
    if(perm!=4) return searchError(result,sym,perm==2?548:547);
    perm=makePerm(sym->flag);
    newAccessData(&access,perm,located);
    if(result->flags&0x80) access.protected_OK=1;
    perm=verifyAccess(&access);
    if(perm!=4) return searchError(result,sym,perm==2?149:148);
    return 0;
}

int SymIsInline(SYMBOL sym);
int SymIsTemporary(SYMBOL sym);
int SymIsAnonymous(SYMBOL sym);
void diagnoseTypedef(SYMBOL sym)
{ (void)sym; STUB_TRAP(); }
void diagnoseSym(SYMBOL sym)
{
    if(!(sym->flag&0x11) && !SymIsInline(sym) && !SymIsTemporary(sym)
       && sym->id!=7)
        CErr2p(SymIsAnonymous(sym) ? 18 : 14,(const char *)sym);
}
void scopeEnsureRefed(SYMBOL sym)
{
    if(sym->id>=2 && sym->id<=4) diagnoseSym(sym);
}
void handleBlockSyms(SYMBOL sym)
{
    if(sym->id==5) diagnoseTypedef(sym);
    else scopeEnsureRefed(sym);
}
void handleFileSyms(SYMBOL sym)
{ (void)sym; STUB_TRAP(); }
void ScopeWalkOrderedSymbols(SCOPE scope,void (*walker)(SYMBOL))
{
    SYMBOL last=scope->ordered;
    SYMBOL next,sym;
    if(last) {
        next=last->thread;
        do {
            sym=next;
            next=sym->thread;
            walker(sym);
        } while(sym!=last);
    }
}
void ScopeWalkNames(SCOPE scope,void (*walker)(SYMBOL_NAME));
void HashDestroy(HASHTAB table);
void RingWalk(void *head,void (*walker)(void *));
void FreeSymbolName(SYMBOL_NAME name)
{
    RingWalk(name->name_syms,(void (*)(void *))FreeSymbol);
    FreeSymbol(name->name_type);
    CarveFree(carveSYMBOL_NAME,name);
}
void ScopeBurn(SCOPE scope)
{
    ScopeWalkNames(scope,FreeSymbolName);
    HashDestroy(scope->names);
    CarveFree(carveSCOPE,scope);
}
SCOPE ScopeEnd(unsigned char id)
{
    SCOPE dropping_scope=CurrScope;
    (void)id; /* release1994 build has no debug scope-kind check */
    CurrScope=dropping_scope->enclosing;
    if (!HashEmpty(dropping_scope->names)) {
        dropping_scope->keep=1;
        switch (dropping_scope->id) {
        case 3: ScopeWalkOrderedSymbols(dropping_scope,handleBlockSyms); break;
        case 0: ScopeWalkOrderedSymbols(dropping_scope,handleFileSyms); break;
        case 2: ScopeWalkOrderedSymbols(dropping_scope,scopeEnsureRefed); break;
        }
    }
    if (CurrScope) CurrScope->keep|=dropping_scope->keep;
    if (!dropping_scope->keep) {
        ScopeBurn(dropping_scope);
        dropping_scope=0;
    }
    return dropping_scope;
}

void lexicalLookup(LOOKUP_WALK *data,SCOPE scope)
{
    data->lexical_scope=scope;
    while (scope) {
        if (scope->id==1) {
            if (data->flags&0x18000) data->flags=(data->flags&~0x18000UL)|0x10000;
            else data->flags|=0x8000;
        } else data->flags&=~0x18000UL;
        if (searchScope(data,scope)) break;
        if (scope->id==2) data->flags|=0x20000;
        scope=scope->enclosing;
    }
}
SEARCH_RESULT *newResult(SCOPE scope,SYMBOL_NAME name)
{
    SEARCH_RESULT *result=CarveAlloc(carveSEARCH_RESULT);
    result->sym=NULL;
    result->start=NULL;
    result->access_decl=NULL;
    result->delta=0;
    result->exact_delta=0;
    result->vb_index=0;
    result->vb_offset=0;
    result->offset=0;
    result->vf_offset=0;
    result->perm=0;
    result->errlocn.src_file=NULL;
    result->sym_name=name;
    result->scope=scope;
    result->flags=0;
    /* These coordinates and the unused second flag byte are not initialized
     * by the old allocator. Keep their reconstruction values deterministic. */
    result->errlocn.line=0;
    result->errlocn.column=0;
    result->flags2=0;
    return result;
}
BASE_CLASS *ScopeFindVBase(SCOPE scope,TYPE type)
{
    BASE_CLASS *last=ScopeInherits(scope),*base;
    if(last!=NULL) {
        base=last->next;
        do {
            if(base->type==type && (base->flag&0x18)) return base;
            if(base==last) break;
            base=base->next;
        } while(1);
    }
    return NULL;
}
void addClassPath(LOOKUP_WALK *data,SEARCH_RESULT *result)
{
    PATH_CAP *cap=data->paths;
    struct base_path *path;
    BASE_CLASS *base,*vbase;
    TYPE last_virtual=NULL;
    struct class_info *info;
    SCOPE scope;
    long delta=0;
    for(path=cap->head;path!=NULL;path=path->next) {
        base=path->base;
        if(base!=NULL) {
            if(base->flag&8) { last_virtual=base->type; delta=0; }
            else delta+=base->delta;
        }
    }
    result->delta=delta;
    if(last_virtual!=NULL) {
        scope=cap->head->scope;
        info=ScopeClass(scope)->u.c.info;
        vbase=ScopeFindVBase(scope,last_virtual);
        result->vb_offset=info->vb_offset;
        result->vb_index=vbase->vb_index;
        delta+=vbase->delta;
    } else result->flags|=2;
    result->exact_delta=delta;
    result->vf_offset=ScopeClass(cap->tail->scope)->u.c.info->vf_offset;
}
void makeClassResult(LOOKUP_WALK *data,SEARCH_RESULT *result)
{
    PATH_CAP *cap=data->paths;
    SYMBOL sym;
    if(data->flags&0x2000) result->flags|=0x10;
    if(data->flags&0x4000) result->flags|=0x20;
    if(data->flags&0x8000) result->flags|=4;
    if(data->flags&0x10000) result->flags|=8;
    if(data->flags&0x100000) result->flags|=0x80;
    if(data->flags&2) result->sym=cap->sym;
    result->start=data->start;
    result->perm=cap->flag;
    result->access_decl=cap->access_decl;
    addClassPath(data,result);
    sym=result->sym_name->name_syms;
    if(sym!=NULL && sym->id==9) result->offset=sym->u.sval;
}
SEARCH_RESULT *makeResult(LOOKUP_WALK *data)
{
    PATH_CAP *cap=data->paths;
    SEARCH_RESULT *result;
    SCOPE scope;
    if (!cap) return 0;
    scope=cap->tail->scope;
    result=newResult(scope,cap->sym_name);
    if (scope->id==1) makeClassResult(data,result);
    else {
        result->flags|=1; /* simple FILE/BLOCK lookup */
        if (data->flags&0x20000) result->flags|=0x40; /* cannot be auto */
    }
    if (data->lexical_scope && data->lexical_scope!=scope &&
        data->lexical_scope->id==1) result->flags2|=1; /* ignore access */
    if (data->flags&8) result->sym=data->selected_conversion;
    delLookupData(data);
    return result;
}
SEARCH_RESULT *ScopeFindScopedNaked(SCOPE scope,SCOPE disambiguate,char *name)
{
    LOOKUP_WALK data;
    newLookupData(&data,name);
    data.disambiguate=disambiguate;
    lexicalLookup(&data,scope);
    return makeResult(&data);
}
SEARCH_RESULT *ScopeFindNaked(SCOPE scope,char *name)
{
    return ScopeFindScopedNaked(scope,0,name);
}

SCOPE ScopeEstablishEnclosing(SCOPE scope,SCOPE enclosing)
{
    SCOPE old=scope->enclosing;
    scope->enclosing=enclosing;
    return old;
}
void ScopeSetClassOwner(SCOPE scope,TYPE type)
{
    scope->owner=type;
}

TYPE ScopeClass(SCOPE scope)
{
    return scope->id==1 ? (TYPE)scope->owner : NULL;
}

SYMBOL ScopeOrderedStart(SCOPE scope)
{
    return (SYMBOL)scope->ordered;
}
SYMBOL ScopeOrderedNext(SYMBOL stop,SYMBOL curr)
{
    if (!stop) return NULL;
    if (!curr) return stop->thread;
    if (curr==stop) return NULL;
    return curr->thread;
}

SCOPE findFunctionScope(SCOPE scope);
int ScopeLocalClass(SCOPE scope)
{
    return scope->id==1 && findFunctionScope(scope)!=NULL;
}

int ScopeFindVirtual(SCOPE scope,SYMBOL *sym,char *name,unsigned *return_thunk)
{ (void)scope; (void)sym; (void)name; (void)return_thunk; STUB_TRAP(); return 0; }

void ScopeWalkSymbols(SCOPE scope,void (*walker)(SYMBOL))
{ (void)scope; (void)walker; STUB_TRAP(); }

/* This older lookup returns a symbol ring and updates the desired type.
 * It predates ScopeConversionList's FNOV_LIST result in the released source. */
TYPE FunctionDeclarationType(TYPE type);
char *CppConversionName(void);
SYMBOL ScopeBestConversion(SCOPE scope,unsigned short flags,TYPE *target)
{
    LOOKUP_WALK data;
    SYMBOL result=NULL;
    newLookupData(&data,CppConversionName());
    data.type=*target;
    data.flags|=0x12;
    data.this_qualifier=flags;
    if(searchScope(&data,scope)) {
        if(data.flags&0x2000) {
            result=data.paths->sym_name->name_syms;
            *target=NULL;
        } else {
            result=data.paths->sym;
            *target=FunctionDeclarationType(result->sym_type)->of;
        }
        delLookupData(&data);
    }
    return result;
}

int ScopeHasVirtualFunctions(SCOPE scope)
{
    TYPE type=ScopeClass(scope);
    return type!=NULL && type->u.c.info->has_vfn;
}

SEARCH_RESULT *ScopeContainsMember(SCOPE scope,char *name)
{
    LOOKUP_WALK data;
    newLookupData(&data,name);
    data.flags|=0x80;
    searchScope(&data,scope);
    return makeResult(&data);
}

struct friend_entry *ScopeFriends(SCOPE scope)
{
    TYPE type;
    if(scope->id!=1) return NULL;
    type=ScopeClass(scope);
    return type!=NULL ? type->u.c.info->friends : NULL;
}
void HashWalk(HASHTAB hash,void (*walker)(SYMBOL_NAME));
void ScopeWalkNames(SCOPE scope,void (*walker)(SYMBOL_NAME))
{ HashWalk(scope->names,walker); }

void ScopeJumpForward(SCOPE scope) { CurrScope=scope; }
void ScopeArgumentCheck(SCOPE scope) { scope->arg_check=1; }

SYMBOL ScopeFunction(SCOPE scope)
{
    return scope->id==2 ? scope->owner : NULL;
}
SYMBOL ScopeFunctionInProgress(void)
{
    SCOPE scope=findFunctionScope(CurrScope);
    return scope ? ScopeFunction(scope) : NULL;
}

SEARCH_RESULT *ScopeFindBaseMember(SCOPE scope,char *name)
{ (void)scope; (void)name; STUB_TRAP(); return NULL; }

void ScopeWalkVirtualBases(SCOPE scope,void (*callback)(void *,void *),void *data)
{
    BASE_CLASS *head=ScopeInherits(scope),*base;
    if (head) {
        base=head;
        do { base=base->next; if (base->flag&0x18) callback(base,data); } while (base!=head);
    }
}
void ScopeWalkDirectBases(SCOPE scope,void (*callback)(void *,void *),void *data)
{
    BASE_CLASS *head=ScopeInherits(scope),*base;
    if (head) {
        base=head;
        do { base=base->next; if (!(base->flag&0x18)) callback(base,data); } while (base!=head);
    }
}
int SymIsThisDataMember(SYMBOL sym);
void ScopeWalkDataMembers(SCOPE scope,void (*callback)(void *,void *),void *data)
{
    SYMBOL stop=ScopeOrderedStart(scope),current=NULL;
    if (stop) {
        for (;;) {
            current=ScopeOrderedNext(stop,current);
            if (!current) break;
            if (SymIsThisDataMember(current)) callback(current,data);
        }
    }
}

int ScopeHasVirtualBases(SCOPE scope)
{
    TYPE type=ScopeClass(scope);
    return type && type->u.c.info->last_vbase!=0;
}

/* Actual56B virtual-table walk state. Only its diagnostic control exists
 * here; the later cached-table/already_done/thunk_code fields are absent. */
typedef struct vftable_walk {
    CLASS_VFTABLE *tables,*curr;
    BASE_STACK *top,*parent;
    SCOPE start,final,base,derived;
    void *thunk;
    VSTK_CTL disambig;
    unsigned OK_to_diagnose:1,reserved:31;
} VFTABLE_WALK;
typedef char VfTableWalkSize[(sizeof(VFTABLE_WALK)==56)?1:-1];
void findPtrOffset(BASE_STACK *top,SCOPE start,CLASS_TABLE *location)
{
    TYPE virtual_base=NULL;
    BASE_CLASS *base;
    long delta=location->delta;
    while(top && (base=top->base)!=NULL) {
        if(base->flag&8) {
            virtual_base=base->type;
            break;
        }
        delta+=base->delta;
        top=top->parent;
    }
    location->delta=delta;
    if(virtual_base) {
        location->vb_offset=ScopeClass(start)->u.c.info->vb_offset;
        base=ScopeFindVBase(start,virtual_base);
        location->vb_index=base->vb_index;
        delta+=base->delta;
    }
    location->exact_delta=delta;
}
void assignLocation(CLASS_TABLE *table,CLASS_TABLE *location)
{
    table->vb_offset=location->vb_offset;
    table->vb_index=location->vb_index;
    table->delta=location->delta;
    table->exact_delta=location->exact_delta;
}
void HashWalkData(HASHTAB hash,void (*walker)(SYMBOL_NAME,void *),void *data);
void checkAmbiguousOverride(THUNK_ACTION *thunk,VFTABLE_WALK *data)
{ (void)thunk; (void)data; STUB_TRAP(); }
SCOPE SymScope(SYMBOL sym);
SYMBOL findOverride(BASE_STACK *top,SYMBOL base)
{ (void)top; (void)base; STUB_TRAP(); return NULL; }
void pushDisambig(VFTABLE_WALK *data,SYMBOL sym)
{ (void)data; (void)sym; STUB_TRAP(); }
void fillInput(THUNK_ACTION *thunk,VFTABLE_WALK *data,TYPE virtual_base)
{ (void)thunk; (void)data; (void)virtual_base; STUB_TRAP(); }
void fillOutput(THUNK_ACTION *thunk,VFTABLE_WALK *data)
{ (void)thunk; (void)data; STUB_TRAP(); }
void analyseVFN(THUNK_ACTION *thunk,VFTABLE_WALK *data)
{
    SYMBOL base_sym=thunk->sym,test_sym,last_override=NULL;
    BASE_STACK *first=data->top,*top;
    BASE_CLASS *base;
    TYPE virtual_base=NULL,fn_type;
    long delta=0,previous=0;
    for(top=first;top;top=top->parent) {
        if(!virtual_base) {
            delta+=previous;
            base=top->base;
            if(base) {
                if(base->flag&8) {
                    previous=0;
                    virtual_base=base->type;
                } else previous=base->delta;
            }
        }
        if(top!=first) {
            test_sym=findOverride(top,base_sym);
            if(test_sym) {
                last_override=test_sym;
                if(!thunk->override) pushDisambig(data,base_sym);
                pushDisambig(data,test_sym);
                if(virtual_base) {
                    thunk->possible_ambiguity=1;
                    data->curr->ambiguities=1;
                }
                thunk->override=test_sym;
                thunk->delta=delta;
            }
        }
    }
    if(!last_override) return;
    if(thunk->override) {
        data->final=SymScope(thunk->override);
        if(virtual_base) fillInput(thunk,data,virtual_base);
        if(thunk->delta || thunk->ctor_disp) thunk->non_empty=1;
        fillOutput(thunk,data);
        if(thunk->out.delta || thunk->output_virtual) thunk->non_empty=1;
    } else {
        fn_type=FunctionDeclarationType(thunk->sym->sym_type);
        if(fn_type->flag&0x40) thunk->non_empty=1;
    }
}
void handleVFN(VFTABLE_WALK *data,SYMBOL sym)
{
    CLASS_VFTABLE *table=data->curr;
    THUNK_ACTION *thunk=&table->data[sym->u.uval-1];
    if(!thunk->sym) {
        thunk->sym=sym;
        --table->amt_left;
    } else if(thunk->override) {
        if(thunk->possible_ambiguity) checkAmbiguousOverride(thunk,data);
        sym=NULL;
    }
    if(sym) analyseVFN(thunk,data);
}
void scanForVFNs(SYMBOL_NAME name,void *parm)
{
    VFTABLE_WALK *data=parm;
    SYMBOL last=name->name_syms,sym;
    TYPE fn_type;
    if(last) {
        sym=last;
        do {
            sym=sym->next;
            fn_type=FunctionDeclarationType(sym->sym_type);
            if(!fn_type) break;
            if((fn_type->flag&0x10) && sym->id!=7 && sym->u.uval)
                handleVFN(data,sym);
        } while(sym!=last);
    }
}
void doubleCheckEntries(VFTABLE_WALK *data,CLASS_VFTABLE *table)
{
    THUNK_ACTION *thunk=table->data;
    for(;;++thunk) {
        if(thunk->sym && thunk->possible_ambiguity) handleVFN(data,thunk->sym);
        if(thunk->last_entry) break;
    }
}
void initThunk(THUNK_ACTION *thunk)
{
    thunk->sym=NULL;
    thunk->thunk=NULL;
    thunk->delta=0;
    thunk->in.delta=0;
    thunk->override=NULL;
    thunk->out.delta=0;
    thunk->ctor_disp=0;
    thunk->input_virtual=0;
    thunk->output_virtual=0;
    thunk->non_empty=0;
    thunk->last_entry=0;
    thunk->possible_ambiguity=0;
}
WALK_STATUS collectVFTable(BASE_STACK *top,void *parm)
{
    VFTABLE_WALK *data=parm;
    SCOPE scope=top->scope;
    struct class_info *info;
    CLASS_VFTABLE *curr,*table;
    CLASS_TABLE location;
    THUNK_ACTION *thunk;
    unsigned max_vfn,i;
    data->top=top;
    info=ScopeClass(scope)->u.c.info;
    if(!info->has_vfn) return WALK_ABANDON;
    if(!info->has_vfptr) return WALK_NORMAL;
    /* The old nonvirtual path copies untouched vb_offset/vb_index stack
     * cells through assignLocation. They have no virtual-base role there;
     * keep those uninitialized reconstruction bytes deterministic. */
    location.vb_offset=0;
    location.vb_index=0;
    location.delta=info->vf_offset;
    findPtrOffset(top,data->start,&location);
    curr=data->tables;
    if(curr) {
        table=curr;
        do {
            curr=(CLASS_VFTABLE *)curr->h.next;
            if(curr->h.exact_delta==location.exact_delta) {
                top->hold=curr;
                data->curr=curr;
                if(curr->amt_left) HashWalkData(scope->names,scanForVFNs,data);
                else {
                    HashWalkData(scope->names,scanForVFNs,data);
                    if(curr->ambiguities) doubleCheckEntries(data,curr);
                }
                return WALK_NORMAL;
            }
        } while(curr!=table);
    }
    max_vfn=info->last_vfn;
    table=RingAlloc((void **)&data->tables,sizeof(CLASS_VFTABLE)+
        (max_vfn-1)*sizeof(THUNK_ACTION));
    assignLocation(&table->h,&location);
    table->h.count=max_vfn;
    table->amt_left=max_vfn;
    table->ambiguities=0;
    table->corrupted=0;
    thunk=table->data;
    for(i=0;i<max_vfn;++i,++thunk) initThunk(thunk);
    (--thunk)->last_entry=1;
    top->hold=table;
    data->curr=table;
    HashWalkData(scope->names,scanForVFNs,data);
    return WALK_NORMAL;
}
void VstkClose(VSTK_CTL *stack);
CLASS_VFTABLE *doCollectVFTable(SCOPE scope,unsigned diagnose)
{
    VFTABLE_WALK data;
    data.tables=NULL;
    data.final=NULL;
    data.curr=NULL;
    data.start=scope;
    data.top=NULL;
    data.OK_to_diagnose=diagnose&1;
    VstkOpen(&data.disambig,sizeof(SCOPE),8);
    walkDirectBases(scope,collectVFTable,&data);
    VstkClose(&data.disambig);
    return data.tables;
}
CLASS_VFTABLE *ScopeCollectVFTable(SCOPE scope)
{
    return doCollectVFTable(scope,1);
}

SYMBOL ScopeAlreadyExists(SCOPE scope,char *name)
{
    SEARCH_RESULT *result=ScopeFindNaked(scope,name);
    SYMBOL sym=NULL;
    if(result) {
        sym=result->sym_name->name_syms;
        ScopeFreeResult(result);
    }
    return sym;
}

/* Only block scopes interpret the owner word as their numbered index. */
unsigned ScopeIndex(SCOPE scope)
{
    return scope->id==3 ? (unsigned)scope->owner : 0;
}

unsigned char ScopeDerived(SCOPE derived,SCOPE possible_base)
{ (void)derived; (void)possible_base; STUB_TRAP(); return 0; }
