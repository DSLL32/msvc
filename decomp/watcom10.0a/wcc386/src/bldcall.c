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
* Description:  Call construction and parameter instruction assignment.
*
****************************************************************************/


/* cgen2.c's emission-state initializer. In the earlier target the next
 * imported segment starts at -1 directly, before the later SegData field. */
#include "block.h"
#include "address.h"
#include "dbinfo.h"
#include "cgmemory.h"
#include "callnode.h"
#include "genopt.h"
#include "stubs.h"
#include <recon.h>

unsigned char AddCallBlock(void *symbol,type_def *type)
{
    unsigned char type_class;
    /* The old blip hook is present unconditionally; its physical entry is
     * the same no-argument progress hook already called by DGBytes. */
    FEName(symbol);
    DGBlip();
    NewProc(FELexLevel(symbol));
    EnLink(AskForSymLabel(symbol,0),0);
    CurrProc->label=CurrBlock->label;
    CurrBlock->class|=0x20;
    type_class=InitCallState(type);
    AddIns(MakeNop());
    return type_class;
}

/* The old register allocator takes only class and parm_state, before the
 * later length/alignment arguments and full call_state interface. */
void BGParmDecl(void *symbol,type_def *type)
{
    type_def *parm_type;
    hw_reg_set reg;
    parm_type=QParmType(AskForLblSym(CurrProc->label),symbol,type);
    reg=ParmReg(TypeClass(parm_type),&CurrProc->state.parm);
    DoParmDecl(symbol,type,reg);
}

/* Earlier bldcall.c emits PARMDEF and links the conversion inline, before
 * those sequences were split into DoParmDef and LinkParmIns helpers. */
name *DoParmDecl(void *symbol,type_def *type,hw_reg_set reg)
{
    type_def *parm_type;
    unsigned char type_class;
    name *temp,*parm_name;
    instruction *parm_def,*ins;
    parm_type=QParmType(AskForLblSym(CurrProc->label),symbol,type);
    type_class=TypeClass(parm_type);
    if (symbol==NULL) temp=AllocTemp(TypeClass(type));
    else {
        temp=SAllocUserTemp(symbol,TypeClass(type),type->length);
        BGDone(MakeAddrName(CG_FE,symbol,type));
    }
    temp->v.usage|=2; /* USE_IN_ANOTHER_BLOCK */
    if (reg==0) {
        if (type_class==XX) {
            parm_name=temp;
            parm_name->t.location=ParmMem(type,&CurrProc->state);
        } else {
            parm_name=AllocTemp(type_class);
            parm_name->t.location=ParmMem(parm_type,&CurrProc->state);
        }
        parm_name->t.temp_flags|=0x20; /* STACK_PARM */
        parm_name->n.size=parm_type->length;
        parm_name->v.usage|=0xd0; /* NEEDS_MEMORY | HAS_MEMORY | USE_MEMORY */
    } else parm_name=AllocRegName(ActualParmReg(reg));
    if (CurrProc->state.attr&0x80) DbgParmLoc(parm_name,symbol);
    parm_def=NewIns(0);
    parm_def->head.opcode=0x2c; /* OP_PARM_DEF */
    parm_def->type_class=type_class;
    parm_def->result=parm_name;
    AddIns(parm_def);
    if (type_class!=XX) {
        ins=MakeConvert(parm_name,temp,TypeClass(type),type_class);
        ins->operands[2]=(name *)parm_def;
        parm_def->operands[2]=(name *)ins;
        ins->ins_flags|=8; /* PARM_LINKAGE */
        parm_def->ins_flags|=8;
        AddIns(ins);
        TRDeclareParm(ins);
        if (temp->n.class==2 && parm_name->n.class==2)
            temp->t.location=parm_name->t.location;
    }
    MaxStack=0;
    return temp;
}

/* trecurse.c's real parameter-instruction linkage for tail-recursion analysis. */
void TRDeclareParm(instruction *entry)
{
    if (entry==NULL) return;
    entry->u.parm_list=CurrProc->parms_list;
    CurrProc->parms_list=entry;
}

/* bldins.c global temporaries remain live across basic-block boundaries. */
name *BGGlobalTemp(type_def *type)
{
    name *temp;
    temp=BGNewTemp(type);
    temp->v.usage|=2;
    return temp;
}

name *BGNewTemp(type_def *type)
{
    name *temp;
    temp=AllocTemp(TypeClass(type));
    if (temp->n.size==0) temp->n.size=type->length;
    return temp;
}

void BGReturn(struct address_name *value,type_def *type)
{
    instruction *ins,*last_ins,*ret_ins;
    name *result;
    unsigned char type_class,return_class;
    ins=MakeNop();
    last_ins=NULL;
    if (value!=NULL) {
        return_class=ReturnClass(type,CurrProc->state.attr);
        UpdateReturn(&CurrProc->state,type,return_class,
                     FEAuxInfo(AskForLblSym(CurrProc->label),0));
        /* The old build checks its actual call attribute, before the later
         * Model/DBG_LOCALS spelling of the debug-return hook. */
        if (CurrProc->state.attr&0x80) DbgRetLoc();
        type_class=TypeClass(type);
        if (return_class==XX) {
            result=StReturn(value,type,&last_ins);
            ret_ins=MakeMove(GenIns(value),result,type_class);
        } else {
            if (CurrProc->state.return_reg==0) result=StReturn(value,type,&last_ins);
            else result=AllocRegName(CurrProc->state.return_reg);
            if (type->length==result->n.size)
                ret_ins=MakeMove(GenIns(value),result,result->n.name_class);
            else
                ret_ins=MakeConvert(GenIns(value),result,result->n.name_class,type_class);
        }
        AddIns(ret_ins);
        BGDone(value);
    } else {
        ins->zap=(register_name *)AllocRegName(CurrProc->state.return_reg);
        ins->flags.nop_flags|=8; /* NOP_ZAP_INFO */
    }
    AddIns(ins);
    if (last_ins!=NULL) AddIns(last_ins);
    GenBlock(1,0); /* RETURN */
    if (AddrList!=NULL) Zoiks(3);
    Generate(1);
    TargetModel=SaveModel;
}

void BGAutoDecl(void *symbol,type_def *type)
{
    BGDone(MakeAddrName(CG_FE,symbol,type));
}

void TellObjVirtFuncRef(void *cookie);
struct call_node *BGInitCall(struct address_name *value,type_def *type,void *symbol)
{
    struct call_node *call;
    name *memory;
    void *cookie;
    unsigned char type_class;
    if (type->refno==T_DEFAULT) type=TypeInteger;
    call=CGAlloc(24);
    call->state=CGAlloc(sizeof(call_state));
    call->name=value;
    call->tipe=type;
    call->parms=NULL;
    call->ins=NewIns(3);
    if (value->format==0 && value->class==0) { /* NF_ADDR, CL_ADDR_GLOBAL. */
        call->ins->head.opcode=0x36; /* OP_CALL. */
        type_class=CallState(symbol,type,call->state);
        memory=value->u.name;
        value->u.name=SAllocMemory(memory->v.symbol,memory->v.offset,
            memory->m.memory_type,memory->n.name_class,memory->n.size);
    } else {
        call->ins->head.opcode=0x29; /* OP_CALL_INDIRECT. */
        type_class=CallState(symbol,type,call->state);
        cookie=FEAuxInfo(symbol,0x2f); /* VIRT_FUNC_REFERENCE. */
        if (cookie!=NULL) TellObjVirtFuncRef(cookie);
    }
    call->ins->type_class=type_class;
    return call;
}

struct address_name *AddrToIns(struct address_name *value);
void BGAddParm(struct call_node *call,struct address_name *value)
{
    struct parm_node *new_parm;
    new_parm=CGAlloc(20);
    new_parm->name=AddrToIns(value);
    new_parm->name->flags|=8; /* ADDR_OK_ACROSS_BLOCKS. */
    new_parm->next=call->parms;
    call->parms=new_parm;
}

hw_reg_set ParmInLineReg(parm_state *state);
unsigned char CvtOk(unsigned char from,unsigned char to);
void ReverseParmNodeList(struct parm_node **owner);
unsigned PushParms(struct parm_node *parms);
void FPPushParms(struct parm_node *parms,call_state *state);
void ParmIns(struct parm_node *parms);
void LinkParms(instruction *ins,struct parm_node **owner);
hw_reg_set CallZap(call_state *state);
unsigned char AssgnParms(struct call_node *call,unsigned char in_line)
{
    struct parm_node *parm;
    call_state *state;
    instruction *ins;
    unsigned char pushed,parm_type,reg_type;
    int count;
    pushed=0;
    parm=call->parms;
    state=call->state;
    ins=call->ins;
    count=0;
    while (parm!=NULL) {
        if (in_line) {
            parm->regs=ParmInLineReg(&state->parm);
            if (parm->regs==0) {
                if (*state->parm.curr_entry!=0)
                    FEMessage(3,"More parameters than registers in pragma");
                else {
                    ParmMem(parm->name->tipe,state);
                    pushed=1;
                }
            } else {
                reg_type=AllocRegName(parm->regs)->n.name_class;
                parm_type=TypeClass(parm->name->tipe);
                /* The older target predates the FD/U8 exception. */
                if (!CvtOk(parm_type,reg_type)) FEMessage(6,(void *)(count+1));
            }
        } else {
            parm->regs=ParmReg(TypeClass(parm->name->tipe),&state->parm);
            if (parm->regs==0) {
                ParmMem(parm->name->tipe,state);
                pushed=1;
            }
        }
        parm->num=++count;
        parm->ins=NULL;
        parm=parm->next;
    }
    ReverseParmNodeList(&call->parms);
    call->stack_bytes=PushParms(call->parms);
    FPPushParms(call->parms,state);
    ParmIns(call->parms);
    ReverseParmNodeList(&call->parms);
    LinkParms(ins,&call->parms);
    if (ins->head.opcode==0x36) ins->num_operands=2;
    ins->operands[0]=AllocRegName(state->parm.used);
    ins->operands[1]=AllocS32Const(call->stack_bytes);
    ins->zap=AllocRegName(CallZap(state));
    return pushed;
}

void ReverseParmNodeList(struct parm_node **owner)
{
    struct parm_node *parm,*next;
    parm=*owner;
    *owner=NULL;
    while (parm!=NULL) {
        next=parm->next;
        parm->next=*owner;
        *owner=parm;
        parm=next;
    }
}

void PushInSameBlock(instruction *ins);
instruction *PushOneParm(instruction *ins,name *value,unsigned char type_class,
                         unsigned *pushed_bytes);
void FPNotStack(name *value);
/* The 1994 call path accumulates pushed bytes here, before the later
 * per-parameter offset/alignment and call_state interface. */
unsigned PushParms(struct parm_node *parm)
{
    struct parm_node *next;
    instruction *ins,*push_ins;
    struct address_name *addr;
    unsigned pushed_bytes;
    pushed_bytes=0;
    while (parm!=NULL) {
        next=parm->next;
        if (parm->ins==NULL && parm->regs==0) {
            addr=parm->name;
            if (addr->format!=4) Zoiks(43); /* NF_INS. */
            ins=addr->u.ins;
            PushInSameBlock(ins);
            if (ins->head.opcode==0x26 && !IsVolatile(ins->operands[0]) &&
                !(addr->flags&4)) {
                push_ins=PushOneParm(ins,ins->operands[0],ins->type_class,
                                     &pushed_bytes);
                FreeIns(ins);
            } else {
                ins->result=BGNewTemp(addr->tipe);
                FPNotStack(ins->result);
                if (addr->flags&0x10) ins->result->v.usage|=2;
                push_ins=PushOneParm(ins,ins->result,
                                     ins->result->n.name_class,&pushed_bytes);
            }
            addr->format=0; /* NF_ADDR: BGDone now releases the address owner. */
            BGDone(addr);
            parm->ins=push_ins;
        }
        parm=next;
    }
    return pushed_bytes;
}

void ParmIns(struct parm_node *parm)
{
    name *reg,*value;
    instruction *ins;
    struct address_name *addr;
    while (parm!=NULL) {
        if (parm->ins!=NULL) {
            parm=parm->next;
            continue;
        }
        addr=parm->name;
        if (addr->format!=4) Zoiks(43);
        reg=AllocRegName(ActualParmReg(parm->regs));
        ins=addr->u.ins;
        ins->result=BGNewTemp(addr->tipe);
        if (addr->flags&0x10) ins->result->v.usage|=2;
        value=ins->result;
        if (addr->tipe->length==reg->n.size) {
            ins=MakeMove(value,reg,TypeClass(addr->tipe));
            AddIns(ins);
        } else if (!CvtOk(TypeClass(addr->tipe),reg->n.name_class) ||
                   reg->r.reg==0xffU) { /* HW_ABCD. */
            FEMessage(6,(void *)parm->num);
            ins=NULL;
        } else {
            ins=MakeConvert(value,reg,reg->n.name_class,TypeClass(addr->tipe));
            AddIns(ins);
        }
        addr->format=0;
        BGDone(addr);
        parm->ins=ins;
        parm=parm->next;
    }
}

void TRAddParm(instruction *call_ins,instruction *parm_ins);
void LinkParms(instruction *call_ins,struct parm_node **owner)
{
    struct parm_node *parm,*next;
    parm=*owner;
    *owner=NULL;
    while (parm!=NULL) {
        /* This assertion remains present in the older target. */
        if (parm->ins==NULL) Zoiks(78);
        TRAddParm(call_ins,parm->ins);
        next=parm->next;
        CGFreeSize(parm,sizeof(struct parm_node));
        parm=next;
    }
}
void TRAddParm(instruction *call_ins,instruction *parm_ins)
{
    if (call_ins==NULL || parm_ins==NULL) return;
    parm_ins->u.parm_list=call_ins->u.parm_list;
    call_ins->u.parm_list=parm_ins;
}

void PreCall(struct call_node *call);
void TNZapParms(void);
void AddCallIns(instruction *ins,struct call_node *call)
{
    PreCall(call);
    /* This older body predates the direct-to-data-label conversion path. */
    if (ins->head.opcode==0x36) ins->operands[2]=call->name->u.name;
    else ins->operands[2]=GenIns(call->name);
    AddIns(ins);
    TNZapParms();
    /* The actual PreCall and PostCall sites share the same sole RET. Both
     * released definitions are empty void(call_node*) hooks; preserve one
     * physical vertex under the representative PreCall name. */
    PreCall(call);
}

void BGZapBase(name *base,type_def *type)
{
    instruction *ins;
    if (base==NULL || !(Model&0x20000000U) || !(type->attr&4)) return;
    ins=MakeNop();
    if (DummyIndex==NULL) DummyIndex=AllocTemp(U4);
    ins->result=ScaleIndex(DummyIndex,base,0,XX,type->length,0,0x10);
    ins->flags.nop_flags|=8;
    AddIns(ins);
}

void FreeCallNode(struct call_node *call)
{
    BGDone(call->name);
    /* Pointer-only CGFree is folded into CGFreeSize in this target. Zero
     * represents its unused size transport, not an observed original EDX. */
    CGFreeSize(call->state->parm.table,0);
    CGFreeSize(call->state,sizeof(call_state));
    CGFreeSize(call,sizeof(struct call_node));
}
