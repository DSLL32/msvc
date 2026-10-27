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
#include "procdef.h"
#include "cgmemory.h"
#include "memory.h"
#include "pragma.h"
#include "stubs.h"
#include <recon.h>

/* Call-class bit positions and target prolog state are the older 1994 form.
 * The released i86reg.c adds BP/unused-register exemptions and stack reserve. */
enum {
    CC_SUICIDAL=0x2,CC_CALLER_POPS=0x100,CC_SPECIAL_RETURN=0x400,
    CC_SPECIAL_STRUCT_RETURN=0x800,CC_FAR=0x1000,CC_INTERRUPT=0x2000,
    CC_LOAD_DS_ON_CALL=0x4000,CC_LOAD_DS_ON_ENTRY=0x8000,
    CC_MODIFY_EXACT=0x10000,CC_NO_8087_RETURNS=0x20000,
    CC_NO_FLOAT_REG_RETURNS=0x40000,CC_NO_MEMORY_CHANGED=0x80000,
    CC_NO_MEMORY_READ=0x100000,CC_NO_STRUCT_REG_RETURNS=0x200000,
    CC_ROUTINE_RETURN=0x400000,CC_FAT_WINDOWS_PROLOG=0x800000,
    CC_GENERATE_STACK_FRAME=0x01000000,CC_EMIT_FUNCTION_NAME=0x02000000,
    CC_GROW_STACK=0x04000000,CC_PROLOG_HOOKS=0x08000000,
    CC_THUNK_PROLOG=0x10000000,CC_EPILOG_HOOKS=0x20000000,
    CC_FAR16_CALL=0x40000000
};
#define CC_TOUCH_STACK 0x80000000U

unsigned char CallState(void *aux,type_def *type,call_state *state)
{
    hw_reg_set *regs,*source,parms[10];
    hw_reg_set modified;
    unsigned cclass,count;
    unsigned char type_class;
    state->unalterable=FixedRegs();
    regs=FEAuxInfo(aux,8);
    state->modify=~*regs;
    state->used=state->modify;
    modified=state->used&~StackReg();
    if (state->unalterable&modified) FEMessage(12,aux);
    state->attr=1;
    cclass=*(unsigned *)FEAuxInfo(aux,2);
    if (cclass&CC_INTERRUPT) state->attr|=0x2000;
    else if (cclass&CC_FAR) state->attr|=2;
    else if (cclass&CC_FAR16_CALL) state->attr|=0x2002;
    if (cclass&CC_CALLER_POPS) state->attr&=~1;
    if (cclass&CC_SUICIDAL) state->attr|=0x10;
    if (cclass&CC_ROUTINE_RETURN) state->attr|=0x40;
    if (cclass&CC_NO_STRUCT_REG_RETURNS) state->attr|=0x100;
    if (cclass&CC_NO_FLOAT_REG_RETURNS) state->attr|=0x4200;
    if (cclass&CC_NO_8087_RETURNS) state->attr|=0x4000;
    if (cclass&CC_MODIFY_EXACT) state->attr|=0x400;
    if (cclass&CC_NO_MEMORY_CHANGED) state->attr|=0x800;
    if (cclass&CC_NO_MEMORY_READ) state->attr|=0x1000;
    if (cclass&CC_LOAD_DS_ON_ENTRY) state->attr|=0x8000;
    if (cclass&CC_LOAD_DS_ON_CALL) state->attr|=4;
    if (state==&CurrProc->state) {
        if (cclass&(CC_GENERATE_STACK_FRAME|CC_PROLOG_HOOKS|CC_EPILOG_HOOKS)) {
            CurrProc->prolog_state|=8;
            state->attr|=0x20;
        }
        if (cclass&CC_PROLOG_HOOKS) CurrProc->prolog_state|=0x100;
        if (cclass&CC_EPILOG_HOOKS) CurrProc->prolog_state|=0x200;
        if (cclass&CC_FAT_WINDOWS_PROLOG) CurrProc->prolog_state|=8;
        if (cclass&CC_EMIT_FUNCTION_NAME) CurrProc->prolog_state|=0x10;
        if (cclass&CC_THUNK_PROLOG) CurrProc->prolog_state|=0x20;
        if (cclass&CC_GROW_STACK) CurrProc->prolog_state|=0x40;
        if (cclass&CC_TOUCH_STACK) CurrProc->prolog_state|=0x400;
    }
    type_class=ReturnClass(type,state->attr);
    count=0;
    source=FEAuxInfo(aux,5);
    for (;;) {
        parms[count]=source[count];
        if (parms[count]==0) break;
        if (state->unalterable&parms[count]) FEMessage(12,aux);
        ++count;
    }
    ++count;
    state->parm.table=CGAlloc(count*sizeof(hw_reg_set));
    Copy((char *)parms,(char *)state->parm.table,count*sizeof(hw_reg_set));
    state->parm.used=0;
    state->parm.curr_entry=state->parm.table;
    state->parm.offset=0;
    if (type==TypeNone) {
        state->return_reg=0;
    } else if (type_class==XX) {
        if (cclass&CC_SPECIAL_STRUCT_RETURN) {
            state->return_reg=*(hw_reg_set *)FEAuxInfo(aux,0x25);
            state->attr|=8;
        } else {
            state->return_reg=StructReg();
        }
        if (!(state->attr&0x40)) state->modify|=ReturnReg(U4,0);
    } else {
        if (cclass&CC_SPECIAL_RETURN) {
            state->return_reg=*(hw_reg_set *)FEAuxInfo(aux,6);
            state->attr|=8;
        } else {
            state->return_reg=ReturnReg(type_class,!(state->attr&0x4000));
        }
    }
    UpdateReturn(state,type,type_class,aux);
    return type_class;
}

void UpdateReturn(call_state *state,type_def *type,unsigned char type_class,void *aux)
{
    hw_reg_set normal;
    if ((CGProcessorVersion&0x70)>=0x10 && (type->attr&1) && !(state->attr&0x4000))
        state->return_reg&=0x10000; /* retain ST0 only */
    else state->return_reg&=~HW_FLTS;
    if (type==TypeNone) {
        if (state->return_reg==0) return;
        FEMessage(7,aux);
        state->return_reg=0;
        state->attr&=~8;
    } else if (type_class==XX) {
        normal=ReturnReg(U4,!(state->attr&0x4000));
        if (state->return_reg==normal || state->return_reg==0) return;
        if (IsRegClass(state->return_reg,U4)) return;
        FEMessage(7,aux);
        state->return_reg=normal;
        state->attr&=~8;
    } else {
        normal=ReturnReg(type_class,!(state->attr&0x4000));
        if (state->return_reg==normal) return;
        if (IsRegClass(state->return_reg,type_class)) return;
        FEMessage(7,aux);
        state->return_reg=normal;
        state->attr&=~8;
    }
}

hw_reg_set FullReg(hw_reg_set regs);
hw_reg_set DisplayReg(void);
hw_reg_set MustSaveRegs(void)
{
    hw_reg_set save,tmp;
    save=~CurrProc->state.modify;
    if (CurrProc->state.attr&0x400) save&=~CurrProc->state.return_reg;
    else {
        tmp=CurrProc->state.parm.used|CurrProc->state.return_reg;
        tmp=FullReg(tmp);
        save&=~tmp;
    }
    save&=~StackReg();
    if (CurrProc->state.return_reg==0)
        save&=~ReturnReg(U4,!(CurrProc->state.attr&0x4000));
    tmp=CurrProc->state.unalterable;
    tmp&=~DisplayReg();
    tmp&=~StackReg();
    save&=~tmp;
    return save;
}

hw_reg_set SaveRegs(void)
{
    hw_reg_set save;
    save=MustSaveRegs();
    return save&CurrProc->state.used;
}

hw_reg_set CallZap(call_state *state)
{
    hw_reg_set zap;
    zap=state->modify;
    if (!(state->attr&0x400)) {
        zap=FullReg(zap|state->parm.used|state->return_reg);
        zap|=ReturnReg(U4,!(state->attr&0x4000));
    }
    return zap;
}
