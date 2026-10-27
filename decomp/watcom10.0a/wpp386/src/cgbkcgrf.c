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


/* Actual1994 call-graph driver. The old48B CALLGRAPH predates scope-call
 * optimization, rescan stacks and the later added throwing analysis. */
#include "compiler.h"
#include "frontend.h"
#include "cmdline.h"
#include "symbol.h"
#include "carve.h"
#include "vstk.h"
#include "cgio.h"
#include "pending.h"
#include "callgrph.h"
#include "ctexcept.h"
#include <recon.h>
#pragma pack(1)
typedef struct vft_defn {
    struct vft_defn *next;
    SYMBOL vft;
    CGFILE *cgfile;
    CGFILE_INS location;
} VFT_DEFN;
#pragma pack()
typedef char VftDefnSize[(sizeof(VFT_DEFN)==18)?1:-1];
static carve_t carve_vft;
static CALLGRAPH *call_graph;
static VFT_DEFN *vft_defs;
static unsigned max_inline_depth,oe_size;
static struct {
    unsigned char inline_recursion:1,any_state_tables:1,only_once_found:1,not_inlined_set:1,reserved:4;
} callGraphFlags;
void SegmentCodeCgInit(void);
void CgrfInit(CALLGRAPH *ctl);
void CgrfFini(CALLGRAPH *ctl);
void CgrfPruneFunction(CALLGRAPH *ctl,CALLNODE *node);
void CgrfWalkFunctions(CALLGRAPH *ctl,int (*walker)(CALLGRAPH *,CALLNODE *));
void CgioWalkFiles(void (*walker)(CGFILE *));
void scanFunction(CGFILE *file);
CALLNODE *addNode(SYMBOL sym);
unsigned char cgbackFuncType(SYMBOL sym);
void scanVftDefn(CALLNODE *node,SYMBOL sym);
CGFILE *nodeCgFile(CALLNODE *node);
void CgioOpenInputFile(CGFILE *file);
CGINTER *CgioReadIC(CGFILE *file,unsigned size);
void CgioCloseInputFile(CGFILE *file);
CGFILE_INS CgioBuffLastRead(void);
void *RingCarveAlloc(carve_t cv,void **head);
TYPE_SIG *BeTypeSignature(TYPE type);
void addAddrOf(CALLNODE *node,SYMBOL sym);
void addTypeSigRefs(CALLNODE *node,TYPE type)
{
    TYPE_SIG *sig=BeTypeSignature(type);
    addAddrOf(node,sig->dtor);
    addAddrOf(node,sig->default_ctor);
    addAddrOf(node,sig->copy_ctor);
}
void *VstkPush(VSTK_CTL *stack);
void CgrfAddAddrOf(CALLGRAPH *ctl,CALLNODE *source,CALLNODE *target);
CALLNODE *addCallee(SYMBOL sym)
{
    CALLNODE *node=addNode(sym);
    unsigned char type;
    if (!node->refs && !node->addrs) {
        type=cgbackFuncType(sym);
        if (type>=1 && type<=3) *(SYMBOL *)VstkPush(&call_graph->calls)=sym;
    }
    return node;
}
void addAddrOf(CALLNODE *node,SYMBOL sym)
{
    unsigned char type;
    if (sym) {
        type=cgbackFuncType(sym);
        if (type>=1 && type<=3)
            CgrfAddAddrOf(call_graph,node,addCallee(sym));
        else if (type!=5) sym->flag|=8;
    }
}
void CgrfAddCall(CALLGRAPH *ctl,CALLNODE *source,CALLNODE *target);
void addCalleeFunc(CALLNODE *node,SYMBOL sym)
{
    unsigned char type=cgbackFuncType(sym);
    if (type>=1 && type<=2) CgrfAddCall(call_graph,node,addCallee(sym));
}
void addCalleeFuncToGen(CALLNODE *node,SYMBOL sym)
{
    addAddrOf(node,sym);
    addCalleeFunc(node,sym);
}
SYMBOL CgBackOpDelete(TYPE type);
/* Inferred name for the old out-of-line effective destructor-method test.
 * The later cgfront source folds a more extensive version into its caller. */
int SymIsDtor(SYMBOL sym);
unsigned char CgEffectiveDtorMethod(SYMBOL func,unsigned char method)
{
    if (method==2 && !SymIsDtor(func)) method=0;
    return method;
}
void scanFunctionBody(CALLNODE *node,CGFILE *file)
{
    CGINTER *ins;
    SYMBOL func;
    unsigned opcodes=0,method;
    int skip_vft=0;
    if (!file || node->calls_done) return;
    node->calls_done=1;
    func=file->symbol;
    if (func) func->flag&=~8UL;
    CgioOpenInputFile(file);
    for (;;) {
        ins=CgioReadIC(file,5);
        if (!ins->opcode) break;
        if (skip_vft) {
            if (ins->opcode==92) skip_vft=0; /* IC_INIT_DONE */
            continue;
        }
        /* Actual1994 IC values. The default counts instructions; the old
         * mask retains these additional metadata tags without counting them.
         * Unlike the later driver there is no scope-resolution pass here. */
        switch (ins->opcode) {
        case 128: case 100: case 99: case 98: case 97:
        case 92: case 91: case 90: case 89: case 84: case 82:
        case 61: case 60: case 57: case 56: case 55: case 54:
        case 53: case 52: case 51: case 50: case 49: case 47:
        case 46: case 45: case 43: case 38: case 27: case 26:
        case 25: case 10: case 9: case 8: case 7: case 6:
        case 5: case 4: case 3: case 2:
            continue;
        case 65: /* IC_FUNCTION_STAB */
            node->state_table=1;
            node->cond_flags=ins->value.uvalue;
            continue;
        case 107: /* IC_THROW_RO_BLK */
          {
            THROW_CNV_CTL ctl;
            TYPE type;
            unsigned offset;
            ThrowCnvInit(&ctl,ins->value.pvalue);
            while ((type=ThrowCnvType(&ctl,&offset))!=NULL)
                addTypeSigRefs(node,type);
            ThrowCnvFini(&ctl);
          } continue;
        case 36: /* IC_EXPR_TS */
            addTypeSigRefs(node,ins->value.pvalue);
            continue;
        case 48: case 37: case 21: /* address-bearing symbol ICs */
            addAddrOf(node,ins->value.pvalue);
            continue;
        case 62: /* IC_FUNCTION_DTM */
            method=CgEffectiveDtorMethod(func,(unsigned char)ins->value.uvalue);
            if (method==2) {
                node->depth=max_inline_depth+1;
                callGraphFlags.not_inlined_set=1;
                file->flags|=0x200;
            }
            continue;
        case 71: /* destructor call, selected by the function's DTM */
            switch (method) {
            case 0: addCalleeFuncToGen(node,ins->value.pvalue); break;
            case 1: addCalleeFuncToGen(node,ins->value.pvalue);
                /* falls through */
            case 2: case 3: case 4:
                addAddrOf(node,ins->value.pvalue); break;
            }
            continue;
        case 119:
            if (method>0 && method<=4) addAddrOf(node,ins->value.pvalue);
            continue;
        case 72: /* IC_TRY */
            node->state_table=1;
            continue;
        case 41: /* IC_CALL_SETUP */
            addCalleeFunc(node,ins->value.pvalue);
            continue;
        case 105: case 104: /* IC_EXCEPT_SPEC, IC_CATCH */
            ++opcodes;
            addTypeSigRefs(node,ins->value.pvalue);
            continue;
        case 122: /* IC_NEW_CTORED */
            ++opcodes;
            addAddrOf(node,CgBackOpDelete(ins->value.pvalue));
            continue;
        case 127: /* IC_VFT_BEG */
          {
            VFT_DEFN *vft=RingCarveAlloc(carve_vft,(void **)&vft_defs);
            SYMBOL sym=ins->value.pvalue;
            vft->vft=sym;
            sym->flag&=~1UL;
            vft->cgfile=file;
            vft->location=CgioBuffLastRead();
            skip_vft=1;
          } continue;
        default:
            ++opcodes;
        }
    }
    node->opcodes=opcodes;
    CgioCloseInputFile(file);
}
int SymIsRegularStaticFunc(SYMBOL sym);
void CgioFreeFile(CGFILE *file);
void removeCodeFile(CALLNODE *node)
{
    SYMBOL func=node->base.object;
    CGFILE *file=nodeCgFile(node);
    if (node->inline_fun) {
        CgioFreeFile(file);
        func->flag&=~1UL;
    } else if (SymIsRegularStaticFunc(func)) CgioFreeFile(file);
}
int SymIsRegularStaticFunc(SYMBOL sym);
int pruneFunction(CALLGRAPH *ctl,CALLNODE *node)
{
    SYMBOL func=node->base.object;
    unsigned char type=cgbackFuncType(func);
    if (type==1 || (type==2 && SymIsRegularStaticFunc(func))) {
        if (!node->refs && !node->addrs) {
            removeCodeFile(node);
            CgrfPruneFunction(ctl,node);
        }
    }
    return 0;
}
int procStaticFunction(CALLGRAPH *ctl,CALLNODE *node)
{ (void)ctl; (void)node; STUB_TRAP(); return 0; }
int procInlineFunction(CALLGRAPH *ctl,CALLNODE *node)
{ (void)ctl; (void)node; STUB_TRAP(); return 0; }
void SegmentMarkUsed(unsigned short segid);
SYMBOL ModuleInitFuncSym(void);
void markAsGen(CALLNODE *node)
{
    CGFILE *file;
    SYMBOL func;
    if (!node->is_vft) {
        file=nodeCgFile(node);
        if (file && !(file->flags&2)) {
            file->flags|=2;
            func=node->base.object;
            if (!func) func=ModuleInitFuncSym();
            SegmentMarkUsed(func->segid);
        }
    }
}
void genFunction(CALLNODE *node)
{
    unsigned char type=cgbackFuncType(node->base.object);
    if (type==1) {
        if (node->depth<=max_inline_depth) return;
    } else if (type==3) return;
    markAsGen(node);
}
int procFunction(CALLGRAPH *ctl,CALLNODE *node)
{
    SYMBOL func=node->base.object;
    CGFILE *file;
    (void)ctl;
    if (node->state_table) callGraphFlags.any_state_tables=1;
    if (node->addrs) {
        if (func) func->flag|=8;
        markAsGen(node);
    } else if (GenSwitches&0x02000000UL) markAsGen(node);
    else genFunction(node);
    file=nodeCgFile(node);
    if (file) {
        file->flags=(file->flags&~0x10U)|(node->state_table?0x10:0);
        file->cond_flags=node->cond_flags;
        if (file->flags&8) func->flag|=0x08000000UL;
        if (file->flags&0x200) func->flag&=~0x08000000UL;
    }
    return 0;
}
int setFunctionStab(CALLGRAPH *ctl,CALLNODE *node)
{ (void)ctl; (void)node; STUB_TRAP(); return 0; }
void *VstkPop(VSTK_CTL *stack);
void VstkClose(VSTK_CTL *stack);
void MarkFuncsToGen(unsigned bounding_depth)
{
    CALLGRAPH ctl;
    SYMBOL *pfunc,sym;
    CALLNODE *node;
    CGFILE *code_file;
    carve_vft=CarveCreate(sizeof(VFT_DEFN),16);
    SegmentCodeCgInit();
    /* Old DBG_LOCALS disables inline expansion, rather than the later
     * separate CompFlags.inline_functions/scope_call_opt decisions. */
    if (GenSwitches&0x02000000UL) max_inline_depth=0;
    else max_inline_depth=bounding_depth;
    call_graph=&ctl;
    CgrfInit(&ctl);
    VstkOpen(&ctl.calls,sizeof(SYMBOL),32);
    CgioWalkFiles(scanFunction);
    while ((pfunc=VstkPop(&ctl.calls))!=NULL) {
        sym=*pfunc;
        node=addNode(sym);
        if (cgbackFuncType(sym)==3) scanVftDefn(node,sym);
        else {
            code_file=nodeCgFile(node);
            if (!code_file) CgrfPruneFunction(&ctl,node);
            else scanFunctionBody(node,code_file);
        }
    }
    VstkClose(&ctl.calls);
    do {
        ctl.pruned=0;
        CgrfWalkFunctions(&ctl,pruneFunction);
    } while (ctl.pruned);
    if (max_inline_depth) {
        /* The old INLINEE stack element is two pointers (8B). */
        VstkOpen(&ctl.calls,8,32);
        if (oe_size) CgrfWalkFunctions(&ctl,procStaticFunction);
        CgrfWalkFunctions(&ctl,procInlineFunction);
        CgrfWalkFunctions(&ctl,procFunction);
        if (callGraphFlags.any_state_tables) CgrfWalkFunctions(&ctl,setFunctionStab);
        VstkClose(&ctl.calls);
    } else CgrfWalkFunctions(&ctl,procFunction);
    CgrfFini(&ctl);
    CarveDestroy(carve_vft);
}

int SymIsThunk(SYMBOL sym);
int SymIsRegularStaticFunc(SYMBOL sym);
void SegmentMarkUsed(unsigned short segid);
void scanFunction(CGFILE *file)
{
    SYMBOL func=file->symbol;
    CALLNODE *node=addNode(func);
    if (SymIsThunk(func)) SegmentMarkUsed(func->segid);
    switch (cgbackFuncType(func)) {
    case 1:
        if (!node->inlineable) node->depth=max_inline_depth+1;
        break;
    case 2:
        if (SymIsRegularStaticFunc(func)) break;
        /* falls through: members and other emitted functions are scanned */
    default:
        scanFunctionBody(node,file);
    }
}

int SymIsFunction(SYMBOL sym);
int SymIsInitialized(SYMBOL sym);
int SymIsInline(SYMBOL sym);
int SymIsVft(SYMBOL sym);
int SymIsEllipsisFunc(SYMBOL sym);
void *TypeHasPragma(TYPE type);
int funcInlineable(SYMBOL sym)
{
    if (!max_inline_depth || !SymIsInitialized(sym) || SymIsEllipsisFunc(sym))
        return 0;
    if (sym && TypeHasPragma(sym->sym_type)) return 0;
    return 1;
}
int canBeOeInlined(CALLNODE *node) { (void)node; STUB_TRAP(); return 0; }
CALLNODE *CgrfAddFunction(CALLGRAPH *ctl,SYMBOL sym);
CGFILE *CgioLocateFile(SYMBOL sym);
unsigned char cgbackFuncType(SYMBOL sym)
{
    if (!sym) return 0;
    if (!SymIsFunction(sym)) return SymIsVft(sym)?3:5;
    if (!SymIsInitialized(sym)) return 4;
    if (SymIsInline(sym)) return 1;
    if (oe_size || SymIsRegularStaticFunc(sym)) return 2;
    return 4;
}
CGFILE *nodeCgFile(CALLNODE *node)
{
    if (!node->cgfile) node->cgfile=CgioLocateFile(node->base.object);
    return node->cgfile;
}
CALLNODE *addNode(SYMBOL sym)
{
    CALLNODE *node=CgrfAddFunction(call_graph,sym);
    switch (cgbackFuncType(sym)) {
    case 3:
        node->is_vft=1;
        break;
    case 1:
        node->inline_fun=1;
        node->inlineable=funcInlineable(sym)!=0;
        if (node->inlineable) sym->flag|=0x08000000UL;
        break;
    case 2:
    case 4:
        node->inlineable=funcInlineable(sym)!=0;
        if (oe_size) node->inlineable_oe=canBeOeInlined(node)!=0;
        break;
    }
    return node;
}

void scanVftDefn(CALLNODE *node,SYMBOL sym)
{
    VFT_DEFN *vft=NULL,*last=vft_defs,*entry;
    CGFILE_INS pos;
    CGINTER *ins;
    SYMBOL func;
    if (node->calls_done) return;
    node->calls_done=1;
    sym->flag|=1;
    if (last) {
        entry=last->next;
        for (;;) {
            if (entry->vft==sym) { vft=entry; break; }
            if (entry==last) break;
            entry=entry->next;
        }
    }
    /* Original invariant: every referenced VFT has an encountered definition.
     * The older reader locates it by scanning rather than the later seek API. */
    CgioOpenInputFile(vft->cgfile);
    do {
        CgioReadIC(vft->cgfile,5);
        pos=CgioBuffLastRead();
    } while (pos.block!=vft->location.block || pos.offset!=vft->location.offset);
    for (;;) {
        ins=CgioReadIC(vft->cgfile,5);
        if (ins->opcode==92) break; /* IC_INIT_DONE */
        if (ins->opcode==48) {      /* IC_DATA_PTR_SYM */
            func=ins->value.pvalue;
            func->flag|=0x10000000UL; /* SF_IN_VFT */
            addAddrOf(node,func);
        }
    }
    CgioCloseInputFile(vft->cgfile);
}
