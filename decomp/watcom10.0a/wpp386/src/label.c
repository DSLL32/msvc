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


/* Initial6b9cb443 label.c, actual1994 labelInit3f1b6..3f1f7.
 * Pool element sizes are the reached old LAB_DEF28/LAB_REF32/COND_BLK20,
 * whose fields remain to be defined when their users are reconstructed. */
#include "compiler.h"
#include "carve.h"
#include "vstk.h"
#include "fnbody.h"
#include "label.h"
#include "pending.h"
#include <recon.h>
static carve_t carveLAB_DEF,carveLAB_REF;
static VSTK_CTL stack_cond;
void labelInit(INITFINI *defn)
{
    (void)defn;
    carveLAB_DEF=CarveCreate(28,16);
    carveLAB_REF=CarveCreate(32,16);
    VstkOpen(&stack_cond,20,16);
}

#pragma pack(1)
typedef struct block_init BLK_INIT;
struct block_init {
    BLK_INIT *next,*containing,*contains;
    SCOPE scope;
    SYMBOL first_init,sym,sym_containing,sym_dtored;
    SYMBOL sym_dtored_containing,dcled_dtored;
    unsigned var_no,var_no_containing;
    void *try_id;
    CGFILE_INS open_ins;
    CGIO_LOCATION locn;
    LAB_POSN switch_posn;
    unsigned flags; /* open_zap/dead_zap/try/catch/free low5 bits */
};
#pragma pack()
typedef char BlkInitSize[(sizeof(BLK_INIT)==86)?1:-1];
static carve_t carveBLK_INIT;
static BLK_INIT *blk_hdr;
void labelMemSave(LAB_MEM *memory)
{
    memory->carve=carveBLK_INIT;
    memory->blk_hdr=blk_hdr;
}
void labelMemLoad(LAB_MEM *memory)
{
    carveBLK_INIT=memory->carve;
    blk_hdr=memory->blk_hdr;
}
void LabelSwitchFunc(LAB_MEM *memory)
{
    LAB_MEM temp;
    labelMemSave(&temp);
    labelMemLoad(memory);
    *memory=temp;
}

SCOPE ScopeFunctionScopeInProgress(void);
int ScopeEnclosed(SCOPE outer,SCOPE inner);
void *RingPrune(void **head,void *entry);
SYMBOL blkDcledSymbol(BLK_INIT *block)
{ return block->dcled_dtored?block->dcled_dtored:block->sym_dtored_containing; }
/* Initial label.c algorithm; old86B record matches every reached field. */
BLK_INIT *labelFindBlk(SCOPE scope)
{
    BLK_INIT *curr,*containing,**container,*cont,*next,*last;
    containing=0;
    container=&blk_hdr;
    for (;;) {
        curr=0;
        last=*container;
        if (last) {
            next=last;
            do {
                next=next->next;
                if (ScopeEnclosed(next->scope,scope)) { curr=next; break; }
            } while (next!=last);
        }
        if (!curr) {
            curr=CarveAlloc(carveBLK_INIT);
            curr->scope=scope;
            curr->contains=0;
            curr->containing=containing;
            if (!containing) {
                curr->var_no_containing=0;
                curr->sym_containing=0;
                curr->sym_dtored_containing=0;
                curr->dcled_dtored=0;
            } else {
                curr->var_no_containing=containing->var_no;
                curr->sym_containing=containing->sym;
                curr->sym_dtored_containing=blkDcledSymbol(containing);
                curr->dcled_dtored=containing->dcled_dtored;
            }
            curr->var_no=0;
            curr->first_init=0;
            curr->sym=0;
            curr->sym_dtored=0;
            curr->locn.src_file=0;
            curr->try_id=0;
            curr->switch_posn.scope=0;
            curr->switch_posn.sym=0;
            curr->switch_posn.var_no=0;
            curr->flags&=~0x1fU;
            last=*container;
            if (last) {
                next=last->next;
                do {
                    cont=next;
                    next=cont->next;
                    if (ScopeEnclosed(scope,cont->scope)) {
                        RingPrune((void **)container,cont);
                        RingAppend((void **)&curr->contains,cont);
                        cont->containing=curr;
                    }
                } while (cont!=last);
            }
            RingAppend((void **)container,curr);
            break;
        }
        if (curr->scope==scope) break;
        containing=curr;
        container=&curr->contains;
    }
    return curr;
}
void LabelInitFunc(LAB_MEM *memory)
{
    labelMemSave(memory);
    carveBLK_INIT=CarveCreate(sizeof(BLK_INIT),16);
    blk_hdr=0;
    labelFindBlk(ScopeFunctionScopeInProgress());
}

void CgFrontCode(int opcode);
CGFILE_INS CgioBuffLastWrite(void);
void LabelBlockOpen(int dead)
{
    BLK_INIT *block;
    block=labelFindBlk(CurrScope);
    if (dead) {
        block->flags|=2;
        CgFrontCode(0x52);
    } else {
        block->flags|=1;
        CgFrontCode(0x4f);
    }
    block->open_ins=CgioBuffLastWrite();
}

static SYMBOL expr_dtor_bound;
static unsigned expr_dtor_depth;
static unsigned char expr_flags;
static CGFILE_INS ins_temp;
SYMBOL scopeDcledSymbol(SCOPE scope);
SYMBOL currDcledSymbol(void)
{
    return scopeDcledSymbol(CurrScope);
}
void LabelExprBegin(void)
{
    expr_dtor_bound=currDcledSymbol();
    expr_dtor_depth=0;
    expr_flags&=~3;
    CgFrontCode(3);
    ins_temp=CgioBuffLastWrite();
}
SYMBOL blkDtorSymbol(BLK_INIT *block);
void emitExprScope(BLK_INIT *block);
SCOPE dtorScope(BLK_INIT *block)
{ (void)block; STUB_TRAP(); return NULL; }
void CgFrontZapUint(CGFILE_INS location,int opcode,unsigned value);
void LabelExprEnd(int dead)
{
    BLK_INIT *block=labelFindBlk(CurrScope);
    if(!dead && (blkDtorSymbol(block)!=expr_dtor_bound || (expr_flags&2))) {
        emitExprScope(block);
        dtorScope(block);
        CgFrontZapUint(ins_temp,28,0);
        CgFrontCode((expr_flags&2) ? 118 : 64);
    }
    block->sym_dtored=block->dcled_dtored;
}

/* Recovered descriptive name for the old expression-scope wrapper. */
SYMBOL blkDtorSymbol(BLK_INIT *block)
{
    return block->sym_dtored ? block->sym_dtored : block->sym_dtored_containing;
}
void CgFrontCodePtr(int opcode,void *value);
void emitExprScope(BLK_INIT *block)
{
    if(!(expr_flags&1)) {
        if(blkDtorSymbol(block)!=expr_dtor_bound)
            CgFrontCodePtr(74,block->dcled_dtored);
        expr_flags|=1;
    }
}
void LabelExprScope(void)
{
    emitExprScope(labelFindBlk(CurrScope));
}

SYMBOL currDtorSymbol(void)
{
    return blkDtorSymbol(labelFindBlk(CurrScope));
}
int popsCatch(BLK_INIT *source,BLK_INIT *target)
{
    while(source!=target) {
        if(source->flags&8) return 1;
        source=source->containing;
    }
    return 0;
}
int FunctionBodyCtor(void);
void LabelReturn(void)
{
    BLK_INIT *outer,*current;
    if(currDtorSymbol()!=NULL) {
        if(FunctionBodyCtor()) CgFrontCode(69);
        CgFrontCode(63);
    } else {
        outer=labelFindBlk(ScopeFunctionScopeInProgress());
        current=labelFindBlk(CurrScope);
        if(popsCatch(current,outer)) {
            if(FunctionBodyCtor()) CgFrontCode(69);
            CgFrontCode(63);
        }
    }
}

void LabelFiniFunc(LAB_MEM *memory)
{
    CarveDestroy(carveBLK_INIT);
    labelMemLoad(memory);
}

int SymIsReferenced(SYMBOL sym);
int SymIsAutomatic(SYMBOL sym);
TYPE TypeGetActualFlags(TYPE type,unsigned short *flags);
TYPE MakeForceInMemory(TYPE type);
void markCatchVarInMem(SYMBOL sym)
{
    unsigned short flags;
    if(SymIsAutomatic(sym) && SymIsReferenced(sym)) {
        TypeGetActualFlags(sym->sym_type,&flags);
        if(!(flags&0x1000)) sym->sym_type=MakeForceInMemory(sym->sym_type);
    }
}
void ScopeWalkSymbols(SCOPE scope,void (*walker)(SYMBOL));
void LabelBlockClose(int dead)
{
    BLK_INIT *block=labelFindBlk(CurrScope);
    BLK_INIT *outer=block->containing;
    int emitted=0;
    SCOPE scope;
    if(CurrScope->keep || (block->flags&8)) {
        if(block->flags&1) {
            CgFrontZapUint(block->open_ins,79,(unsigned)CurrScope);
            emitted=1;
        } else if(block->flags&2) {
            CgFrontZapUint(block->open_ins,82,(unsigned)CurrScope);
            emitted=1;
        }
        if(!dead) CgFrontCodePtr(80,CurrScope);
        if(emitted) CgFrontCodePtr(84,CurrScope);
        /* Old catch exits mark automatic objects in containing block scopes. */
        if(block->flags&8) {
            for(scope=CurrScope->enclosing;scope->id==3;scope=scope->enclosing)
                ScopeWalkSymbols(scope,markCatchVarInMem);
        }
    } else {
        if(outer) {
            RingPrune((void **)&outer->contains,block);
            CarveFree(carveBLK_INIT,block);
        }
        CgFrontCode(83);
    }
}

void LabelRefFree(LAB_REF *ref)
{
    CarveFree(carveLAB_REF,ref);
}
void LabelDefFree(LAB_DEF *def)
{
    LAB_REF *last,*next,*current;
    if(def) {
        last=def->forward;
        if(last) {
            next=last->next;
            do {
                current=next;
                next=current->next;
                LabelRefFree(current);
            } while(current!=last);
        }
        CarveFree(carveLAB_DEF,def);
    }
}

void labelCurrPosn(LAB_POSN *posn);
BLK_INIT *findCommonBlk(SCOPE source,SCOPE target);
void bypassError(SYMBOL sym) { (void)sym; STUB_TRAP(); }
int labelCheckJump(LAB_POSN *source,LAB_POSN *target,BLK_INIT **common,SYMBOL *sym)
{
    BLK_INIT *src=labelFindBlk(source->scope);
    BLK_INIT *com=findCommonBlk(source->scope,target->scope);
    BLK_INIT *block;
    SYMBOL target_sym=target->sym;
    unsigned target_var=target->var_no,source_var;
    for(block=labelFindBlk(target->scope);;block=block->containing) {
        if(block==com) {
            source_var=source->var_no;
            for(block=src;;block=block->containing) {
                if(block==com) {
                    if(source_var<target_var) {
                        bypassError(block->sym);
                        return 0;
                    }
                    *common=com;
                    *sym=target_sym;
                    return 1;
                }
                source_var=block->var_no_containing;
            }
        }
        if(block->flags&4) {
            CErr1(622);
            CErr2p(614,(const char *)&block->locn);
            return 0;
        }
        if(block->flags&8) {
            CErr1(623);
            CErr2p(615,(const char *)&block->locn);
            return 0;
        }
        if(target_var) {
            bypassError(block->first_init);
            return 0;
        }
        target_sym=block->sym_containing;
        target_var=block->var_no_containing;
    }
}
SYMBOL scopeDcledSymbol(SCOPE scope)
{
    return blkDcledSymbol(labelFindBlk(scope));
}
int ScopeEnclosed(SCOPE outer,SCOPE inner);
BLK_INIT *findCommonBlk(SCOPE source,SCOPE target)
{
    while(!ScopeEnclosed(source,target)) source=source->enclosing;
    ScopeKeep(source);
    return labelFindBlk(source);
}
void CgFrontZapUint(CGFILE_INS location,int opcode,unsigned value);
void SrcFileGetTokenLocn(CGIO_LOCATION *locn);
void LabelDefine(LAB_DEF *label)
{
    LAB_POSN curr;
    BLK_INIT *common,*source;
    SYMBOL sym,entry;
    LAB_REF *ref,*last;
    labelCurrPosn(&curr);
    if(labelCheckJump(&label->posn,&curr,&common,&sym)) {
        labelFindBlk(curr.scope);
        entry=scopeDcledSymbol(curr.scope);
        last=label->forward;
        if(last) {
            ref=last;
            do {
                ref=ref->next;
                source=labelFindBlk(ref->posn.scope);
                if(blkDcledSymbol(source)!=entry || popsCatch(source,common)) {
                    common=findCommonBlk(label->posn.scope,curr.scope);
                    CgFrontZapUint(ref->ins_exit,130,(unsigned)entry);
                }
            } while(ref!=last);
        }
    }
    label->posn=curr;
    SrcFileGetTokenLocn(&label->locn);
}

void LabelExprSetjmp(void) { STUB_TRAP(); }

int SymRequiresDtoring(SYMBOL sym);
int SymRequiresCtoring(SYMBOL sym);
int SymIsInitialized(SYMBOL sym);
int SymIsAutomatic(SYMBOL sym);
SCOPE SymScope(SYMBOL sym);
int TypeTruncByMemModel(TYPE type);
SYMBOL RoDtorFind(SYMBOL sym);
int SymIsModuleDtorable(SYMBOL sym);
void ScopeKeep(SCOPE scope);
int labelMarkDtorSym(BLK_INIT *block,SYMBOL sym)
{
    SYMBOL dtor;
    if(!SymRequiresDtoring(sym)) return 0;
    if(TypeTruncByMemModel(sym->sym_type)) CErr1(160);
    /* The old compiler always obtains this destructor here; it predates
     * the later TypeExactDtorable distinction. */
    dtor=RoDtorFind(sym);
    dtor->flag|=8;
    if(!SymIsModuleDtorable(sym)) {
        block->sym_dtored=sym;
        block->scope->dtor_reqd=1;
        ScopeKeep(block->scope);
    }
    return 1;
}
void LabelDeclInited(SYMBOL sym)
{
    BLK_INIT *block;
    if(SymRequiresDtoring(sym) && !SymRequiresCtoring(sym))
        CurrScope->dtor_naked=1;
    block=labelFindBlk(SymScope(sym));
    if(SymIsInitialized(sym)) {
        block->sym=sym;
        ++block->var_no;
        if(block->first_init==NULL) block->first_init=sym;
    }
    if(SymIsAutomatic(sym) && labelMarkDtorSym(block,sym))
        block->dcled_dtored=sym;
}
void LabelDeclInitedFileScope(SYMBOL sym) { (void)sym; STUB_TRAP(); }

void ScopeKeep(SCOPE scope);
void labelCurrPosn(LAB_POSN *posn)
{
    BLK_INIT *block;
    ScopeKeep(CurrScope);
    block=labelFindBlk(CurrScope);
    posn->sym=blkDcledSymbol(block);
    posn->scope=block->scope;
    posn->var_no=block->var_no;
}
void SrcFileGetTokenLocn(CGIO_LOCATION *locn);
LAB_DEF *LabelAllocLabDef(void)
{
    LAB_DEF *def=CarveAlloc(carveLAB_DEF);
    labelCurrPosn(&def->posn);
    def->forward=NULL;
    SrcFileGetTokenLocn(&def->locn);
    return def;
}
LAB_REF *LabelAllocLabRef(LAB_DEF *def)
{
    LAB_REF *ref=CarveAlloc(carveLAB_REF);
    ref->next=NULL;
    ref->defn=def;
    ref->ins_exit.block=0;
    ref->ins_exit.offset=0;
    ref->ins_enter.block=0;
    ref->ins_enter.offset=0;
    labelCurrPosn(&ref->posn);
    return ref;
}

#pragma pack(1)
typedef struct conditional_block {
    CGFILE_INS ins_true,ins_false;
    SYMBOL sym_dtored;
    unsigned char flags;
    unsigned char unused[3];
} COND_BLK;
#pragma pack()
typedef char CondBlockSize[(sizeof(COND_BLK)==20)?1:-1];
void *VstkPush(VSTK_CTL *stack);
void CgFrontCodeInt(int opcode,int value);
void LabelCondTrue(void)
{
    COND_BLK *block;
    /* Actual1994 emits before pushing; later source reordered these. */
    CgFrontCodeInt(66,0);
    block=VstkPush(&stack_cond);
    block->ins_true=CgioBuffLastWrite();
    block->sym_dtored=currDtorSymbol();
    block->flags&=~1;
}
void *VstkTop(VSTK_CTL *stack);
void LabelCondFalse(void)
{
    COND_BLK *block;
    CgFrontCodeInt(67,0);
    block=VstkTop(&stack_cond);
    block->flags|=1;
    block->ins_false=CgioBuffLastWrite();
}

void *VstkPop(VSTK_CTL *stack);
unsigned FunctionRegistrationFlag(void);
void LabelCondEnd(void)
{
    COND_BLK *block=VstkPop(&stack_cond);
    if(block->sym_dtored!=currDtorSymbol()) {
        FunctionRegistrationFlag();
        if(block->flags&1) {
            CgFrontZapUint(block->ins_true,66,1);
            CgFrontZapUint(block->ins_false,67,1);
            CgFrontCodeInt(68,0);
        } else {
            CgFrontZapUint(block->ins_true,66,2);
            CgFrontCodeInt(68,1);
        }
    }
}

void LabelSwitchBeg(void)
{
    labelCurrPosn(&labelFindBlk(CurrScope)->switch_posn);
}
void LabelSwitch(SCOPE scope)
{
    LAB_POSN curr;
    BLK_INIT *common;
    SYMBOL sym;
    labelCurrPosn(&curr);
    common=labelFindBlk(scope);
    labelCheckJump(&common->switch_posn,&curr,&common,&sym);
}
void LabelBlkTry(CGIO_LOCATION *locn,SYMBOL sym)
{
    BLK_INIT *block=labelFindBlk(CurrScope);
    block->flags|=4;
    CurrScope->try_catch=1;
    block->locn=*locn;
    ScopeKeep(CurrScope);
    CgFrontCodePtr(77,sym);
}

void CgFrontEmitSwitchLabels(void);
void LabelSwitchLabel(SCOPE scope,SCOPE label_scope,int dead)
{
    /* The actual1994 tail consumes none of these compatibility arguments. */
    (void)scope; (void)label_scope; (void)dead;
    CgFrontEmitSwitchLabels();
}

CGFILE_INS CgioBuffLastWrite(void);
void LabelGotoFwd(LAB_REF *ref)
{
    /* Old1994 code brackets the forward reference in an expression scope. */
    CgFrontCode(132);
    CgFrontCode(3);
    ref->ins_exit=CgioBuffLastWrite();
    CgFrontCode(133);
    RingAppend((void **)&ref->defn->forward,ref);
}

void LabelBlkCatch(CGIO_LOCATION *posn,void *try_id)
{
    BLK_INIT *block=labelFindBlk(CurrScope);
    block->try_id=try_id;
    block->flags|=8;
    CurrScope->try_catch=1;
    block->locn=*posn;
    ScopeKeep(CurrScope);
    CgFrontCode(131);
}

void LabelSwitchEnd(void) { CgFrontCode(13); }
void LabelTryComplete(SYMBOL sym) { CgFrontCodePtr(110,sym); }

void VstkClose(VSTK_CTL *stack);
void labelFini(INITFINI *defn)
{
    (void)defn;
    CarveDestroy(carveLAB_DEF);
    CarveDestroy(carveLAB_REF);
    VstkClose(&stack_cond);
}
