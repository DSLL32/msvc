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
* Description:  Address folding and typed pointer dereferencing.
*
****************************************************************************/


/* cgen2.c's emission-state initializer. In the earlier target the next
 * imported segment starts at -1 directly, before the later SegData field. */
#include "address.h"
#include "stubs.h"
#include <recon.h>

/* addrfold.c before alignment propagation: the actual old descriptor has no
 * alignment field. Preserve each named address class and indexed-name flag. */
name *Points(struct address_name *addr,type_def *type)
{
    unsigned char type_class,flags;
    long size;
    name *result;
    CheckPointer(addr);
    if (addr->format!=0 && addr->format!=2) Zoiks(24);
    type_class=TypeClass(type);
    size=type->length;
    flags=addr->base!=NULL?0x10:0; /* X_FAKE_BASE */
    if (addr->flags&4) flags|=2;
    if (addr->flags&0x80) flags|=0x40;
    switch (addr->class) {
    case 0: /* CL_ADDR_GLOBAL */
        if (addr->u.name->v.offset==addr->offset &&
            addr->u.name->n.name_class==type_class && addr->u.name->n.size==size)
            result=addr->u.name;
        else
            result=SAllocMemory(addr->u.name->v.symbol,addr->offset,
                                addr->u.name->m.memory_type,type_class,size);
        break;
    case 1: /* CL_ADDR_TEMP */
        if (addr->u.name->v.offset==addr->offset &&
            addr->u.name->n.name_class==type_class && addr->u.name->n.size==size)
            result=addr->u.name;
        else result=STempOffset(addr->u.name,addr->offset,type_class,size);
        break;
    case 2: /* CL_POINTER */
        result=ScaleIndex(addr->index,addr->base,addr->offset,type_class,size,0,flags);
        break;
    case 7:case 8: /* CL_GLOBAL_INDEX / CL_TEMP_INDEX */
        result=ScaleIndex(addr->index,addr->u.name,addr->offset,type_class,size,0,flags&~0x10);
        break;
    case 9: /* CL_TEMP_OFFSET */
        result=ScaleIndex(addr->u.name,addr->base,addr->offset,type_class,size,0,flags);
        break;
    case 5:case 6: /* CL_CONS2 / CL_CONS4 */
        addr->u.name=AllocIntConst(addr->offset);
        addr->class=4; /* CL_VALUE */
        LoadTempInt(addr);
        result=ScaleIndex(addr->u.name,addr->base,0,type_class,size,0,flags);
        break;
    default:
        LoadTempInt(addr);
        result=ScaleIndex(addr->u.name,addr->base,0,type_class,size,0,flags);
        break;
    }
    if ((addr->flags&4) && result->n.class!=4) result->v.usage|=0xb0;
    if ((addr->flags&0x80) && result->n.class!=4) result->v.usage|=0x100;
    return result;
}

/* Source action names retained for the actual 32-bit address modes. These
 * encountered semantic tables match the old 81-byte matrix and ten mappings. */
enum mode_action {
    UNEXPECTED,TEMP_L,TEMP_R,XPAND_L,XPAND_R,
    IL,IR,ADD_LA,ADD_RA,ADD_LI,ADD_RI,ADD_I,CODE
};
static unsigned char AddTable[9][9]={
 {0,0,0,0,0,IL,TEMP_L,XPAND_L,ADD_RA},
 {0,0,0,0,0,ADD_RI,ADD_RI,XPAND_L,ADD_RA},
 {0,0,0,0,0,IL,TEMP_L,XPAND_L,ADD_RA},
 {0,0,0,0,0,ADD_RI,ADD_RI,XPAND_L,ADD_RA},
 {0,0,0,0,0,ADD_RI,ADD_RI,XPAND_L,ADD_RA},
 {IR,ADD_LI,IR,ADD_LI,ADD_LI,ADD_I,ADD_I,XPAND_L,ADD_RA},
 {TEMP_R,ADD_LI,TEMP_R,ADD_LI,ADD_LI,ADD_I,CODE,CODE,TEMP_R},
 {XPAND_R,XPAND_R,XPAND_R,XPAND_R,XPAND_R,XPAND_R,CODE,CODE,CODE},
 {ADD_LA,ADD_LA,ADD_LA,ADD_LA,ADD_LA,ADD_LA,TEMP_L,CODE,ADD_LA}
};
static unsigned char Idx[]={0,2,4,7,6,8,8,1,3,5};

struct address_name *AddrPlus(struct address_name *left,struct address_name *right,type_def *type)
{
    unsigned char action;
    address_name *addr;
    CheckPointer(left);
    CheckPointer(right);
    if (!AddToTypeLength(left->offset,right->offset)) return NULL;
    if (left->format==1 || right->format==1) return NULL;
    if (type->refno==7) return NULL;
    addr=NewAddrName();
    for (;;) {
        action=AddTable[Idx[right->class]][Idx[left->class]];
        switch (action) {
        case UNEXPECTED:case CODE:
            AddrFree(addr);
            return NULL;
        case TEMP_L:LoadTempInt(left);break;
        case TEMP_R:LoadTempInt(right);break;
        case XPAND_L:Convert(left,I4);left->class=4;break;
        case XPAND_R:Convert(right,I4);right->class=4;break;
        case IL:
            MoveAddress(right,addr);
            addr->index=left->u.name;
            addr->offset=(long)((unsigned long)addr->offset+left->offset);
            FixIndexClass(addr);
            break;
        case IR:
            MoveAddress(left,addr);
            addr->index=right->u.name;
            addr->offset=(long)((unsigned long)addr->offset+right->offset);
            FixIndexClass(addr);
            break;
        case ADD_LA:
            MoveAddress(left,addr);
            addr->offset=(long)((unsigned long)addr->offset+right->offset);
            break;
        case ADD_RA:
            MoveAddress(right,addr);
            addr->offset=(long)((unsigned long)addr->offset+left->offset);
            break;
        case ADD_LI:
            if (PointLess(left,right)) { AddrFree(addr);return NULL; }
            MoveAddress(left,addr);
            addr->offset=(long)((unsigned long)addr->offset+right->offset);
            AddIndex(addr,left->index,right->u.name);
            break;
        case ADD_RI:
            if (PointLess(left,right)) { AddrFree(addr);return NULL; }
            MoveAddress(right,addr);
            addr->offset=(long)((unsigned long)addr->offset+left->offset);
            AddIndex(addr,right->index,left->u.name);
            break;
        case ADD_I:
            if (PointLess(left,right)) { AddrFree(addr);return NULL; }
            addr->offset=(long)((unsigned long)left->offset+right->offset);
            addr->class=9;
            AddIndex(addr,left->u.name,right->u.name);
            addr->u.name=addr->index;
            addr->index=NULL;
            break;
        }
        if (action>=IL) break;
    }
    BGDone(left);
    BGDone(right);
    addr->tipe=type;
    return addr;
}

unsigned char AddToTypeLength(long left,long right)
{
    unsigned long sum;
    long signed_sum;
    /* Both old type_length and signed_32 are one DWORD on this target.
     * Express their machine wrapping additions without signed-overflow UB. */
    sum=(unsigned long)left+(unsigned long)right;
    signed_sum=(long)((unsigned long)left+(unsigned long)right);
    return (unsigned long)signed_sum==sum;
}

name *GetValue(struct address_name *addr,name *suggest)
{
    name *op;
    instruction *ins;
    op=NULL;
    InsToAddr(addr);
    if (addr->format==2 || addr->format==1) {
        op=addr->u.name;
    } else if (addr->format==0) {
        switch (addr->class) {
        case 3:case 4:op=addr->u.name;break;
        case 9:
            if (addr->offset!=0) {
                op=MaybeTemp(suggest,addr->u.name->n.name_class);
                ins=MakeBinary(1,addr->u.name,AllocS32Const(addr->offset),
                               op,TypeClass(addr->tipe));
                if (addr->flags&0x20) ins->ins_flags|=4;
                AddIns(ins);
            } else op=addr->u.name;
            break;
        case 0:
            op=AllocMemory(addr->u.name->v.symbol,addr->offset,
                           addr->u.name->m.memory_type,XX);
            op=LoadAddress(op,suggest,addr->tipe);
            break;
        case 1:
            op=TempOffset(addr->u.name,addr->offset,XX);
            op=LoadAddress(op,suggest,addr->tipe);
            break;
        case 7:case 8:case 2:
            op=AllocIndex(addr->index,addr->u.name,addr->offset,XX);
            op=LoadAddress(op,suggest,addr->tipe);
            break;
        }
    } else if (addr->format==3) {
        Zoiks(25);
    }
    /* The original leaves op uninitialized for invalid format/class and if
     * Zoiks returns. Those unsupported malformed owners trap in both builds. */
    if (op==NULL) STUB_TRAP();
    if (op->n.class==2 && op->v.symbol==NULL &&
        (addr->flags&2) && !(addr->flags&0x40)) FPSetStack(op);
    return op;
}

unsigned char CypAddrPlus(address_name *left,address_name *right,type_def *type)
{
    unsigned char action;
    if (type->refno==T_HUGE_POINTER) return 0;
    CheckPointer(left);
    if (left->format==1) return 0;
    CheckPointer(right);
    if (right->format==1) return 0;
    action=AddTable[Idx[right->class]][Idx[left->class]];
    return action==ADD_LA || action==ADD_RA || action==XPAND_R || action==XPAND_L;
}

unsigned char ShiftConst(address_name *right)
{
    if (right->format!=2 || right->class!=5) return 0;
    if (right->offset<0 || right->offset>8) return 0;
    return 1;
}
unsigned char CypAddrShift(address_name *left,address_name *right,type_def *type)
{
    (void)type;
    CheckPointer(left);
    if (left->format==1 || left->class!=9 || left->offset==0) return 0;
    return ShiftConst(right)!=0;
}

name *AddrToName(address_name *addr)
{
    switch (addr->format) {
    case 2:case 1: /* NF_CONS, NF_NAME */
        return addr->u.name;
    case 0:
        switch (addr->class) {
        case 3:case 4: /* CL_VALUE2, CL_VALUE4 */
            return addr->u.name;
        case 9: /* CL_TEMP_OFFSET */
            if (addr->offset==0) return addr->u.name;
            break;
        }
        break;
    }
    return NULL;
}

unsigned char ShiftToTypeLength(long offset,unsigned short shift)
{ STUB_TRAP();return 0; }

address_name *AddrShift(address_name *left,address_name *right,type_def *type)
{
    address_name *addr;
    instruction *ins;
    short shift;
    unsigned char type_class;
    if (!ShiftConst(right)) return NULL;
    CheckPointer(left);
    if (left->format==1 || left->class!=9 || left->offset==0) return NULL;
    shift=(short)right->offset;
    if (!ShiftToTypeLength(left->offset,(unsigned short)shift)) return NULL;
    addr=NewAddrName();
    /* The actual i386 shift masks the count and retains the DWORD result. */
    addr->offset=(unsigned long)left->offset<<((unsigned)shift&31);
    type_class=TypeClass(type);
    ins=MakeBinary(13,left->u.name,AllocIntConst(shift),AllocTemp(type_class),type_class);
    addr->u.name=ins->result;
    AddIns(ins);
    addr->class=9;
    BGDone(left);
    BGDone(right);
    addr->tipe=type;
    return addr;
}

void FixIndexClass(address_name *addr)
{
    addr->class=addr->u.name->n.class==2?8:7;
}
