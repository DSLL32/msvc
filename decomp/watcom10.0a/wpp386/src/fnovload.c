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
#include <string.h>
#include "fnovload.h"
#include "carve.h"
#include "ptree.h"
#include "pending.h"
#include <recon.h>
static carve_t carveFNOVLIST,carveManyFNOVRANK;
static FNOV_LIST *diag_ambig,*diag_reject;
static int num_candidates;
void deleteRank(FNOV_RANK *rank,unsigned count)
{
    if (count>16) CMemFree(rank);
    else CarveFree(carveManyFNOVRANK,rank);
}
void FnovListFree(FNOV_LIST **list)
{
    FNOV_LIST *head,*ptr,*next;
    head=*list;
    if (head!=0) {
        ptr=head->next;
        do {
            next=ptr->next;
            deleteRank(ptr->rankvector,ptr->num_args);
            if (ptr->free_args) CMemFree(ptr->alist);
            CarveFree(carveFNOVLIST,ptr);
            if (ptr==head) break;
            ptr=next;
        } while (1);
    }
    *list=0;
}
void FnovFreeDiag(void)
{
    num_candidates=-1;
    FnovListFree(&diag_ambig);
    FnovListFree(&diag_reject);
}
unsigned char IsOverloadedFuncDistinct(SYMBOL *previous,SYMBOL current,char *name)
{
    FnovFreeDiag();
    return doFunctionDistinctCheck(9,previous,current,name);
}
/* Actual1994 CF3b5ad..3b6ba: old explicit list/argument parameters,
 * before FNOV_INFO and per-request diagnostic objects. */
char *CppConversionName(void);
unsigned char doFunctionDistinctCheck(unsigned short control,SYMBOL *previous,SYMBOL current,char *name)
{
    SYMBOL old;
    unsigned char result,overload_result;
    FNOV_LIST *candidates,*match;
    arg_list alist,*args;
    result=6;
    candidates=0;
    match=0;
    old=*previous;
    *previous=0;
    if (name==CppConversionName()) {
        InitArgList(&alist);
        alist.num_args=1;
        alist.type_list[0]=FunctionDeclarationType(current->sym_type)->of;
        alist.qualifier=FunctionThisFlags(current);
        args=&alist;
        control|=4;
    } else args=SymFuncArgList(current);
    buildOverloadListFromSym(control,&candidates,args,old);
    fnovNumCandidatesSet(candidates);
    if (candidates!=0) {
        overload_result=resolveOverload(control,args,0,&candidates,&match);
        if (overload_result!=2) {
            *previous=match->sym;
            if (isRank(match,1)) {
                if (isReturnIdentical(match->sym->sym_type,current->sym_type)) result=3;
                else result=5;
            } else if (isRank(match,2)) result=4;
        }
    }
    setFnovDiagnosticRejectList(&candidates);
    setFnovDiagnosticAmbigList(&match);
    return result;
}
/* Actual old form returns whether a template candidate was seen. */
int buildOverloadListFromSym(unsigned short control,FNOV_LIST **candidates,arg_list *args,SYMBOL old)
{
    SYMBOL ptr;
    int has_template;
    has_template=0;
    if (control&64) has_template=processSym(control,candidates,args,old);
    else if (old!=0) {
        ptr=old->next;
        do {
            has_template|=processSym(control,candidates,args,ptr);
            if (ptr==old) break;
            ptr=ptr->next;
        } while (1);
    }
    return has_template;
}
int isSimpleCandidate(TYPE type,unsigned count)
{
    type=FunctionDeclarationType(type);
    if (type!=0 && TypeArgList(type)->num_args==count) return 1;
    return 0;
}
/* The1994 branch order has no later SymDeAlias call, defarg exclusion,
 * FNOV_INFO or LENT bitmask. The last stack parameter is a free-args flag. */
int processSym(unsigned short control,FNOV_LIST **candidates,arg_list *args,SYMBOL sym)
{
    arg_list *func_args,*mock_args;
    int has_template;
    has_template=0;
    func_args=SymFuncArgList(sym);
    if (control&4) addListEntry(control,candidates,sym,func_args,0);
    else if (control&16) {
        if (isMemberCandidate(sym->sym_type,args->num_args)) {
            mock_args=MakeMemberArgList(sym,args->num_args);
            addListEntry(control,candidates,sym,mock_args,1);
        }
    } else if (isSimpleCandidate(sym->sym_type,args->num_args)) {
        addListEntry(control,candidates,sym,func_args,0);
        if (SymIsFunctionTemplateModel(sym)) has_template=1;
    } else if (!(control&1)) {
        if (isEllipsisCandidate(sym->sym_type,args->num_args)) {
            mock_args=MakeMockArgList(sym->sym_type,args->num_args);
            addListEntry(control,candidates,sym,mock_args,1);
            if (SymIsFunctionTemplateModel(sym)) has_template=1;
        }
    }
    return has_template;
}
int isMemberCandidate(TYPE type,unsigned count)
{
    type=FunctionDeclarationType(type);
    return type && TypeArgList(type)->num_args+1==count;
}
int isEllipsisCandidate(TYPE type,unsigned count)
{
    arg_list *args;
    type=FunctionDeclarationType(type);
    if(!type) return 0;
    args=TypeArgList(type);
    if(!args->num_args) return 0;
    if(count<=args->num_args && count+1!=args->num_args) return 0;
    return args->type_list[args->num_args-1]->id==TYP_DOT_DOT_DOT;
}
void initRankVector(unsigned short control,FNOV_RANK *vector,int count)
{
    FNOV_RANK *rank;
    int i;
    if(count==0) count=1;
    memset(vector,0,count*sizeof(*vector));
    for(i=count,rank=vector;i>0;--i,++rank) {
        rank->rank=12;
        rank->control=control;
    }
    if(control&16) vector->control|=2;
    /* The1994 stdops exclusion is independent of the member bit. */
    if(control&0x100) vector->control|=0x200;
}
void addListEntry(unsigned short control,FNOV_LIST **candidates,SYMBOL sym,arg_list *args,int free_args)
{
    FNOV_LIST *entry=CarveAlloc(carveFNOVLIST);
    entry->sym=sym;
    entry->alist=args;
    entry->num_args=(control&4) ? 1 : args->num_args;
    entry->free_args=free_args;
    entry->member=(control&16)!=0;
    entry->stdops=(control&0x100)!=0;
    entry->rankvector=NULL;
    initRankVector(0,&entry->thisrank,1);
    entry->thisrank.rank=1;
    entry->flags=0;
    entry->next=NULL;
    RingAppend((void **)candidates,entry);
}
arg_list *MakeMemberArgList(SYMBOL sym,unsigned count);
arg_list *MakeMockArgList(TYPE type,unsigned count)
{ (void)type; (void)count; STUB_TRAP(); return 0; }

void fnovNumCandidatesRestore(int count);
void fnovNumCandidatesSet(FNOV_LIST *candidates)
{
    if (num_candidates==-1) fnovNumCandidatesRestore(RingCount(candidates));
}
FNOV_RANK *newRank(unsigned count)
{
    if(count>16) return CMemAlloc(count*sizeof(FNOV_RANK));
    return CarveAlloc(carveManyFNOVRANK);
}
void addRankVector(FNOV_LIST *candidate,unsigned short control)
{
    unsigned count=candidate->num_args;
    FNOV_RANK *rank;
    if(count==0) count=1;
    rank=newRank(count);
    initRankVector(control,rank,count);
    candidate->rankvector=rank;
}
int computeUdcRank(arg_list *args,FNOV_LIST *candidate)
{ (void)args; (void)candidate; STUB_TRAP(); return 0; }
int FnovCvFlagsRank(unsigned short source,unsigned short target,FNOV_RANK *rank)
{
    source&=3;
    target&=3;
    if(source!=target) {
        if(source&~target) { rank->rank=11; return 1; }
        rank->rank=3;
        rank->u.no_ud.trivial=1;
    }
    return 0;
}
int TypeBasesEqual(unsigned short flags,void *source,void *target);
void FnovMemFlagsRank(unsigned short source,unsigned short target,void *source_scope,void *target_scope,FNOV_RANK *rank)
{
    source&=0x73c;
    target&=0x73c;
    if(source!=target || ((source&0x700) && !TypeBasesEqual(target,source_scope,target_scope))) {
        rank->rank=5;
        ++rank->u.no_ud.standard;
    }
}
SYMBOL FunctionSymbol(PTREE node);
int IsOverloadedFunc(SYMBOL sym)
{
    return sym && SymIsFunction(sym) && sym->next!=sym;
}
void computeFuncRank(SYMBOL target,SYMBOL source,TYPE *type,FNOV_RANK *rank)
{ (void)target; (void)source; (void)type; (void)rank; STUB_TRAP(); }
void FnovArgRank(TYPE source,TYPE target,PTREE *plist,FNOV_RANK *rank);
TYPE ReferenceType(TYPE type);
SYMBOL SymIsFunctionTemplateInst(SYMBOL sym);
void doComputeArgRank(SYMBOL func,TYPE source,TYPE target,PTREE *plist,FNOV_RANK *rank)
{
    if((rank->control&0x80) && SymIsFunctionTemplateModel(func)) {
        rank->rank=11;
        return;
    }
    FnovArgRank(source,target,plist,rank);
    if((rank->control&0x200) && rank->rank>3 && ReferenceType(target)!=NULL)
        rank->rank=11;
    else if((rank->control&0x80) && rank->rank>3 && SymIsFunctionTemplateInst(func))
        rank->rank=11;
    else if((rank->control&8) && rank->rank==2)
        FnovArgRank(target,source,NULL,rank);
}
/* Old explicit arguments precede the later FNOV_INFO request object. */
int computeFunctionRank(arg_list *args,PTREE *plist,FNOV_LIST *candidate)
{
    unsigned index=args->num_args;
    TYPE *source=args->type_list,*target=candidate->alist->type_list;
    FNOV_RANK *rank=candidate->rankvector;
    SYMBOL sym;
    for(;index!=0;--index,++source,++target,++rank) {
        sym=NULL;
        if(plist!=NULL) sym=FunctionSymbol(*plist);
        if(sym!=NULL) computeFuncRank(candidate->sym,sym,target,rank);
        else doComputeArgRank(candidate->sym,*source,*target,plist,rank);
        if((rank->control&8) && rank->rank>2) return 0;
        if(rank->rank==11) return 0;
        if(plist!=NULL) ++plist;
    }
    return 1;
}
int SymIsThisFuncMember(SYMBOL sym);
int getRank(unsigned short control,arg_list *args,void *plist,FNOV_LIST *candidate)
{
    int contender;
    if(candidate->rankvector!=NULL) return 1;
    if(candidate->member) control|=16;
    if(candidate->stdops) control|=0x100;
    addRankVector(candidate,control);
    if(control&4) contender=computeUdcRank(args,candidate);
    else contender=computeFunctionRank(args,plist,candidate);
    if(contender && SymIsThisFuncMember(candidate->sym)) {
        if(!FnovCvFlagsRank(args->qualifier,candidate->alist->qualifier,&candidate->thisrank))
            FnovMemFlagsRank(args->qualifier,candidate->alist->qualifier,NULL,NULL,&candidate->thisrank);
    }
    return contender;
}
void *RingPrune(void **head,void *entry);
enum { OV_CMP_SAME,OV_CMP_BETTER_FIRST,OV_CMP_BETTER_SECOND,OV_CMP_UNDEFINED };
unsigned char compareInt(int first,int second)
{
    if (first<second) return OV_CMP_BETTER_FIRST;
    if (first>second) return OV_CMP_BETTER_SECOND;
    return OV_CMP_SAME;
}
/* Actual1994 scalar ranking predates type-based reference/derivation
 * tie breakers. Compare the three-byte conversion counters and flags. */
unsigned char compareScalar(FNOV_SCALAR *first,FNOV_SCALAR *second,int user_conversion)
{
    if (first->udcnv!=second->udcnv) return compareInt(first->udcnv,second->udcnv);
    if (first->standard!=second->standard) return compareInt(first->standard,second->standard);
    if (first->promotion!=second->promotion) return compareInt(first->promotion,second->promotion);
    if (first->trivial!=second->trivial) return compareInt(first->trivial,second->trivial);
    if (user_conversion && first->not_exact!=second->not_exact)
        return compareInt(first->not_exact,second->not_exact);
    return OV_CMP_SAME;
}
unsigned char equateExactSame(unsigned char rank)
{
    if (rank==1) rank=2;
    return rank;
}
unsigned char compareArgument(FNOV_RANK *left,FNOV_RANK *right)
{
    unsigned char first=equateExactSame(left->rank);
    unsigned char second=equateExactSame(right->rank);
    if (first!=second) return compareInt(first,second);
    if (first>=3 && first<=7) return compareScalar(&left->u.no_ud,&right->u.no_ud,0);
    if (first==8) return compareScalar(&left->u.ud.out,&right->u.ud.out,1);
    if ((first>=1 && first<=2) || (first>=9 && first<=11)) return OV_CMP_SAME;
    /* Invalid/uncomputed equal ranks leave CL uninitialized in the original.
     * No semantics for that malformed domain have been reconstructed. */
    STUB_TRAP();
    return 0;
}
unsigned char compareFunction(FNOV_LIST *left,FNOV_LIST *right)
{
    unsigned count=left->num_args;
    FNOV_RANK *first=left->rankvector,*second=right->rankvector;
    unsigned char result=OV_CMP_UNDEFINED,argument;
    while (result!=OV_CMP_SAME && count) {
        argument=compareArgument(first,second);
        if (argument==OV_CMP_BETTER_FIRST) {
            if (result==OV_CMP_BETTER_SECOND) result=OV_CMP_SAME;
            else if (result==OV_CMP_UNDEFINED) result=OV_CMP_BETTER_FIRST;
        } else if (argument==OV_CMP_BETTER_SECOND) {
            if (result==OV_CMP_BETTER_FIRST) result=OV_CMP_SAME;
            else if (result==OV_CMP_UNDEFINED) result=OV_CMP_BETTER_SECOND;
        }
        --count;
        ++first;
        ++second;
    }
    if (result==OV_CMP_UNDEFINED) {
        argument=compareArgument(&left->thisrank,&right->thisrank);
        result=(argument==OV_CMP_BETTER_FIRST || argument==OV_CMP_BETTER_SECOND)
            ? argument : OV_CMP_SAME;
    }
    return result;
}
/* The1994 comparison does not receive FNOV_CONTROL, and equal entries
 * remain ambiguous even when their symbol pointers happen to match. */
unsigned char updateMatchList(FNOV_LIST **candidates,FNOV_LIST **match,FNOV_LIST *candidate)
{
    FNOV_LIST *last,*entry,*next;
    unsigned char comparison;
    int ambiguous=0;
    last=*match;
    if(last) {
        next=last->next;
        do {
            entry=next;
            next=entry->next;
            comparison=compareFunction(entry,candidate);
            if(comparison==1) return 1;
            if(comparison==2) {
                RingPrune((void **)match,entry);
                RingInsert((void **)candidates,entry,NULL);
            } else if(comparison==0) ambiguous=1;
        } while(entry!=last);
    }
    RingPrune((void **)candidates,candidate);
    RingAppend((void **)match,candidate);
    return ambiguous ? 0 : 2;
}
void updateFnovList(FNOV_LIST **candidates,FNOV_LIST **match,FNOV_LIST *candidate,unsigned char *result)
{
    unsigned char comparison=updateMatchList(candidates,match,candidate);
    if(comparison==0) *result=1;
    else if(comparison==2) *result=0;
}
unsigned char resolveOverload(unsigned short control,arg_list *args,void *plist,FNOV_LIST **candidates,FNOV_LIST **match)
{
    FNOV_LIST *last,*next,*current;
    unsigned char result=2;
    *match=NULL;
    last=*candidates;
    if(last) {
        next=last->next;
        do {
            current=next;
            next=current->next;
            if(getRank(control,args,plist,current))
                updateFnovList(candidates,match,current,&result);
        } while(current!=last);
    }
    return result;
}
int isRank(FNOV_LIST *entry,unsigned char rank)
{
    unsigned count=entry->num_args;
    FNOV_RANK *current=entry->rankvector;
    while(count) {
        if(current->rank>rank) return 0;
        ++current;
        --count;
    }
    return entry->thisrank.rank<=rank;
}
int TypesSameExclude(TYPE left,TYPE right,unsigned control);
int isReturnIdentical(TYPE left,TYPE right)
{
    TYPE right_result=FunctionDeclarationType(right)->of;
    return TypesSameExclude(FunctionDeclarationType(left)->of,right_result,1);
}
void setFnovDiagnosticRejectList(FNOV_LIST **list)
{
    /* Original1994 uses JE-free's inverse here: it calls the free routine
     * when the OLD reject list is NULL. Retain that old leak-prone condition,
     * rather than silently adopting the later source's non-NULL test. */
    if (diag_reject==0) FnovListFree(&diag_reject);
    diag_reject=*list;
}
void setFnovDiagnosticAmbigList(FNOV_LIST **list)
{
    if (diag_ambig!=0) FnovListFree(&diag_ambig);
    diag_ambig=*list;
}
unsigned short FunctionThisFlags(SYMBOL sym);

/* Actual1994 CF3b799..3b7c4;33-byte list records and16x10-byte
 * cached rank vectors. This registration follows the global operator setup,
 * which rejects arity mismatches without allocating overload candidates. */
void fnovInit(INITFINI *defn)
{
    (void)defn;
    carveFNOVLIST=CarveCreate(sizeof(FNOV_LIST),16);
    carveManyFNOVRANK=CarveCreate(16*sizeof(FNOV_RANK),16);
}

int fnovNumCandidatesGet(void) { return num_candidates; }
void fnovNumCandidatesRestore(int count) { num_candidates=count; }
void BuildUdcList(FNOV_LIST **candidates,SYMBOL sym)
{ processSym(4,candidates,NULL,sym); }
unsigned char rankUdcCandidates(FNOV_LIST *candidates,TYPE target,unsigned short flags,FNOV_LIST **rejects,FNOV_LIST **match)
{ (void)candidates; (void)target; (void)flags; (void)rejects; (void)match; STUB_TRAP(); return 0; }

SYMBOL findFunctionTemplate(FNOV_LIST *candidates)
{
    FNOV_LIST *current;
    if(candidates) {
        current=candidates->next;
        do {
            if(SymIsFunctionTemplateModel(current->sym)) return current->sym;
            if(current==candidates) break;
            current=current->next;
        } while(1);
    }
    return NULL;
}
CGIO_LOCATION *extractAGoodLocn(PTREE *nodes)
{ return nodes ? &(*nodes)->locn : NULL; }
unsigned char TemplateFunctionGenerate(SYMBOL *sym,arg_list *args,CGIO_LOCATION *locn,int has_match);
unsigned char genTemplateFunction(arg_list *args,SYMBOL *sym,CGIO_LOCATION *locn,int has_match)
{
    SYMBOL result_sym=*sym;
    unsigned char result=TemplateFunctionGenerate(&result_sym,args,locn,has_match);
    if(result==0) *sym=result_sym;
    return result;
}
void setRank(FNOV_LIST *match,unsigned char rank)
{
    unsigned i;
    for(i=0;i<match->num_args;++i) match->rankvector[i].rank=rank;
    match->thisrank.rank=rank;
}
/* Actual1994 dispatch, before FNOV_INFO and diagnostic request objects.
 * Template failure preserves the ordinary overload result unless ambiguous. */
unsigned char doOverload(unsigned short control,FNOV_LIST **resolved,arg_list *args,
                         void *nodes,FNOV_LIST *candidates)
{
    FNOV_LIST *match=NULL;
    SYMBOL sym;
    unsigned char result=2,template_result;
    *resolved=NULL;
    if(candidates) {
        result=resolveOverload(control,args,nodes,&candidates,&match);
        if(result==0 && isRank(match,2)) goto matched;
        if((control&0x88)==0x80) {
            sym=findFunctionTemplate(candidates);
            template_result=2;
            if(sym) template_result=genTemplateFunction(args,&sym,
                                       extractAGoodLocn(nodes),result==0);
            if(template_result==0) {
                FnovListFree(&match);
                addListEntry(0,&match,sym,SymFuncArgList(sym),0);
                addRankVector(match,control);
                result=0;
                setRank(match,2);
            } else if(template_result==1) {
                result=1;
                FnovListFree(&match);
            }
        }
        if(result==0) {
matched:
            *resolved=match;
        }
    }
    if(result!=0) setFnovDiagnosticAmbigList(&match);
    setFnovDiagnosticRejectList(&candidates);
    return result;
}
unsigned char resolveFunctionOverload(unsigned short control,FNOV_LIST **match,
                                       SYMBOL syms,arg_list *args,void *nodes)
{
    FNOV_LIST *candidates=NULL;
    FnovFreeDiag();
    if(buildOverloadListFromSym(control,&candidates,args,syms)) control|=0x80;
    fnovNumCandidatesSet(candidates);
    return doOverload(control,match,args,nodes,candidates);
}
unsigned char FuncOverloaded(SYMBOL *ctor,SYMBOL candidates,arg_list *args,PTREE *nodes)
{
    FNOV_LIST *match;
    unsigned char result;
    *ctor=NULL;
    result=resolveFunctionOverload(0,&match,candidates,args,nodes);
    if(match) {
        *ctor=match->sym;
        FnovListFree(&match);
    }
    return result;
}

unsigned char AreFunctionsDistinct(SYMBOL *previous,SYMBOL current,char *name)
{
    return doFunctionDistinctCheck(0x49,previous,current,name);
}

/* Actual1994 operator overload set, before namespaces/region diagnostics. */
unsigned char OpOverloaded(SYMBOL *resolved,SEARCH_RESULT *member,
                 SEARCH_RESULT *nonmember,SYMBOL scalars,arg_list *args,PTREE *nodes)
{
    FNOV_LIST *candidates=NULL,*match=NULL;
    unsigned short control=0;
    int has_template=0;
    unsigned char result;
    FnovFreeDiag();
    *resolved=NULL;
    if(member) has_template=buildOverloadListFromSym(16,&candidates,args,
                                                    member->sym_name->name_syms);
    if(nonmember) has_template|=buildOverloadListFromSym(0,&candidates,args,
                                                    nonmember->sym_name->name_syms);
    fnovNumCandidatesSet(candidates);
    if(scalars) has_template|=buildOverloadListFromSym(256,&candidates,args,scalars);
    if(has_template) control|=128;
    result=doOverload(control,&match,args,nodes,candidates);
    if(match) {
        *resolved=match->sym;
        FnovListFree(&match);
    }
    return result;
}
