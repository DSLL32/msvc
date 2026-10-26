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
* Description:  Basic type initialization and type-node allocation.
*
****************************************************************************/


/* cgen2.c's emission-state initializer. In the earlier target the next
 * imported segment starts at -1 directly, before the later SegData field. */
#include "block.h"
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
