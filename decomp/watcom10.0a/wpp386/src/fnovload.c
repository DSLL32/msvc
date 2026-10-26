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
{ (void)type; (void)count; STUB_TRAP(); return 0; }
int isEllipsisCandidate(TYPE type,unsigned count)
{ (void)type; (void)count; STUB_TRAP(); return 0; }
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
arg_list *MakeMemberArgList(SYMBOL sym,unsigned count)
{ (void)sym; (void)count; STUB_TRAP(); return 0; }
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
SYMBOL FunctionSymbol(PTREE node)
{ (void)node; STUB_TRAP(); return NULL; }
void computeFuncRank(SYMBOL target,SYMBOL source,TYPE *type,FNOV_RANK *rank)
{ (void)target; (void)source; (void)type; (void)rank; STUB_TRAP(); }
void FnovArgRank(TYPE source,TYPE target,PTREE *plist,FNOV_RANK *rank);
TYPE ReferenceType(TYPE type);
int SymIsFunctionTemplateInst(SYMBOL sym)
{ (void)sym; STUB_TRAP(); return 0; }
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
void updateFnovList(FNOV_LIST **candidates,FNOV_LIST **match,FNOV_LIST *candidate,unsigned char *result)
{ (void)candidates; (void)match; (void)candidate; (void)result; STUB_TRAP(); }
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
{ (void)entry; (void)rank; STUB_TRAP(); return 0; }
int isReturnIdentical(TYPE left,TYPE right)
{ (void)left; (void)right; STUB_TRAP(); return 0; }
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
