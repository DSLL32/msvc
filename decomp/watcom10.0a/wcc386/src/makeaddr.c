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
#include "codegen.h"
#include "address.h"
#include "memory.h"
#include "stubs.h"
#include <recon.h>

struct address_name;
struct address_name *AddrList;
static void *AddrNameFrl;

void InitMakeAddr(void)
{
    InitFrl(&AddrNameFrl);
    AddrList=NULL;
}

/* makeaddr.c before FE_UNALIGNED and the later alignment field. */
struct address_name *MakeAddrName(unsigned char kind,void *symbol,type_def *type)
{
    address_name *addr;
    unsigned attr;
    int level;
    name *op;
    addr=NewAddrName();
    addr->format=0; /* NF_ADDR */
    if (kind!=CG_FE) {
        op=SAllocMemory(symbol,0,kind,TypeClass(type),type->length);
        addr->u.name=op;
        addr->class=0; /* CL_ADDR_GLOBAL */
    } else {
        attr=FEAttr(symbol);
        level=FELexLevel(symbol);
        if (attr&2) { /* FE_STATIC */
            op=SAllocMemory(symbol,0,kind,TypeClass(type),type->length);
            if (attr&0x20) op->v.usage|=0x90;
            addr->u.name=op;
            addr->class=0;
            if (attr&0x800) op->v.usage|=0xb0;
            if (attr&0x10) op->v.usage|=0x100;
        } else if (level!=CurrProc->lex_level) {
            op=Display(symbol,level);
            addr->u.name=op;
            addr->format=1; /* NF_NAME */
        } else {
            op=SAllocUserTemp(symbol,TypeClass(type),type->length);
            if (attr&0x20) op->v.usage|=0x90;
            if (attr&0x400) op->v.usage|=8;
            addr->u.name=op;
            addr->class=1; /* CL_ADDR_TEMP */
            op->v.usage|=2;
            if (attr&0x800) op->v.usage|=0xb0;
        }
    }
    addr->tipe=TypeAddress(NamePtrType(op));
    return addr;
}

address_name *NewAddrName(void)
{
    address_name *addr;
    addr=AllocFrl(&AddrNameFrl,sizeof(*addr));
    addr->link=AddrList;
    AddrList=addr;
    addr->tipe=NULL;
    addr->index=NULL;
    addr->offset=0;
    addr->u.name=NULL;
    addr->format=0;
    addr->flags=0;
    addr->base=NULL;
    return addr;
}

void BGDone(address_name *node)
{
    if (node->format==3) FlowOff((struct bool_node *)node); /* NF_BOOL */
    else AddrFree(node);
}

void AddrFree(address_name *node)
{
    address_name **owner;
    owner=&AddrList;
    while (*owner!=node) owner=&(*owner)->link;
    *owner=node->link;
    if (node->format==4) FreeIns(node->u.ins); /* NF_INS */
    FrlFreeSize(&AddrNameFrl,node,sizeof(*node));
}

void NamesCrossBlocks(void)
{
    address_name *addr,*next,*copy;
    name *temp;
    addr=AddrList;
    while (addr!=NULL) {
        next=addr->link; /* The called conversions may change this list. */
        if (addr->flags&8) addr->flags|=0x10;
        else if (addr->format==4) { /* NF_INS */
            InsToAddr(addr);
            addr->u.name->v.usage|=2;
        } else if (addr->format!=2 && addr->format!=3 &&
            (addr->format!=0 || (addr->class!=0 && addr->class!=1))) {
            temp=GenIns(addr);
            if (temp->n.class==2) temp->v.usage|=2;
            else if (temp->n.class==4 && temp->i.index->n.class==2)
                temp->i.index->v.usage|=2;
            copy=AddrName(temp,addr->tipe);
            CopyAddr(copy,addr);
            AddrFree(copy);
        }
        addr=next;
    }
}

address_name *MakeConst(struct cfloat *value,type_def *type)
{
    return AddrName(AllocConst(value),type);
}

/* addrfold.c's old 32-bit address classes, before widened integer constants. */
address_name *AddrName(name *value,type_def *type)
{
    address_name *addr;
    addr=NewAddrName();
    addr->tipe=type;
    addr->u.name=value;
    if (value->n.class==0 && value->c.const_type==0) {
        addr->format=2; /* NF_CONS */
        if (CFIsI16(value->c.value) && type->length<=4) {
            addr->class=5; /* CL_CONS2 */
            addr->offset=CFCnvF16(value->c.value);
        } else {
            addr->class=4; /* CL_VALUE4 */
            addr->offset=0;
            if (CFIsI32(value->c.value)) {
                addr->class=6; /* CL_CONS4 */
                addr->offset=CFCnvF32(value->c.value);
            }
        }
    } else if (type->attr&4) {
        addr->format=1; /* NF_NAME */
    } else if (type->length==4) {
        addr->format=0; /* NF_ADDR */
        addr->class=value->n.class==2?9:4; /* TEMP_OFFSET / VALUE */
    } else {
        addr->format=1;
    }
    return addr;
}

struct address_name *MakePoints(struct address_name *value,type_def *type)
{
    address_name *result;
    result=AddrName(Points(value,type),type);
    BGDone(value);
    return result;
}

void CheckPointer(struct address_name *addr)
{
    InsToAddr(addr);
    if (addr->format==1 && (addr->tipe->attr&4)) {
        addr->index=Temporary(addr->u.name,addr->tipe);
        addr->u.name=NULL;
        addr->offset=0;
        addr->class=2;
        addr->format=0;
    }
}

void InsToAddr(address_name *addr)
{
    address_name *copy;
    instruction *ins;
    if (addr->format==4) {
        ins=addr->u.ins;
        ins->result=BGNewTemp(addr->tipe);
        copy=AddrName(ins->result,addr->tipe);
        copy->flags=addr->flags;
        copy->base=addr->base;
        CopyAddr(copy,addr);
        AddrFree(copy);
    }
}

address_name *AddrCopy(address_name *addr)
{
    address_name *copy;
    InsToAddr(addr);
    copy=NewAddrName();
    CopyAddr(addr,copy);
    return copy;
}

void CopyAddr(address_name *source,address_name *dest)
{
    address_name *link;
    link=dest->link;
    Copy((char *)source,(char *)dest,sizeof(*dest));
    dest->link=link;
}

name *Temporary(name *value,type_def *type)
{
    if (value->n.class!=2) value=LoadTemp(value,TypeClass(type));
    return value;
}

void MoveAddress(address_name *source,address_name *dest)
{
    dest->format=source->format;
    dest->class=source->class;
    dest->u.name=source->u.name;
    dest->index=source->index;
    dest->offset=source->offset;
}

name *GenIns(address_name *addr)
{
    return GetValue(addr,NULL);
}

struct address_name *MakeTypeTempAddr(name *temp,type_def *type)
{
    address_name *addr;
    addr=NewAddrName();
    addr->tipe=type;
    addr->format=0;
    addr->u.name=temp;
    addr->class=1;
    return addr;
}

struct address_name *MakeGets(struct address_name *dest,struct address_name *source,type_def *type)
{
    name *dest_name,*source_name,*temp;
    instruction *ins;
    unsigned char type_class;
    InsToAddr(dest);
    dest_name=Points(dest,type);
    ins=source->u.ins;
    if (source->format==4 && CurrBlock->ins.hd.prev==ins) {
        ins->result=dest_name;
        source->format=0; /* The transferred instruction must not be freed. */
    } else {
        source_name=GetValue(source,dest_name);
        if (source_name!=dest_name) {
            type_class=TypeClass(type);
            source_name=GenIns(source);
            if (dest_name->n.class==4 && !(dest_name->i.index_flags&2)) {
                /* Return a temp rather than extending an index's lifetime. */
                temp=SAllocTemp(dest_name->n.name_class,dest_name->n.size);
                AddIns(MakeMove(source_name,dest_name,type_class));
                AddIns(MakeMove(dest_name,temp,type_class));
                dest_name=temp;
            } else {
                AddIns(MakeMove(source_name,dest_name,type_class));
            }
        }
    }
    BGDone(source);
    BGDone(dest);
    return AddrName(dest_name,type);
}

struct address_name *AddrDuplicate(struct address_name *node)
{
    address_name *copy;
    name *op;
    InsToAddr(node);
    op=GenIns(node);
    copy=AddrName(op,node->tipe);
    CopyAddr(copy,node);
    return copy;
}

address_name *InsName(instruction *ins,type_def *type)
{
    address_name *addr=NewAddrName();
    addr->format=4; /* NF_INS */
    addr->u.ins=ins;
    addr->tipe=type;
    return addr;
}

unsigned char NeedPtrConvert(address_name *addr,type_def *type)
{
    if (addr->format==0 /* NF_ADDR */ &&
        (addr->class==0 || addr->class==1)) {
        switch (type->refno) {
        case T_NEAR_POINTER:case T_LONG_POINTER:case T_HUGE_POINTER:
            return 0;
        }
    }
    return 1;
}

void FixCodePtr(address_name *value)
{
    if (value->format==4) value->u.ins->ins_flags|=0x80; /* CODE_POINTER */
}

void AddrDemote(struct address_name *value)
{
    value->flags|=0x20; /* ADDR_DEMOTED. */
    if (value->format==4) value->u.ins->ins_flags|=4; /* INS_DEMOTED. */
}

unsigned char PointLess(struct address_name *left,struct address_name *right)
{
    if (left->class!=2 && right->class!=2) return 0; /* CL_POINTER. */
    if (left->offset!=0 || right->offset!=0) return 0;
    return 1;
}

name *LoadAddress(name *value,name *suggest,type_def *type)
{
    name *new_name;
    unsigned char type_class;
    if (value->n.class==4 &&
            (value->i.base==NULL || (value->i.index_flags&0x10))) {
        if (value->i.constant!=0) {
            type_class=value->i.index->n.name_class;
            new_name=MaybeTemp(suggest,type_class);
            AddIns(MakeBinary(1,value->i.index,AllocS32Const(value->i.constant),
                              new_name,type_class));
        } else new_name=value->i.index;
    } else {
        if (suggest!=NULL) type_class=suggest->n.name_class;
        else if (type->length==4) type_class=U4;
        else type_class=CP;
        new_name=MaybeTemp(suggest,type_class);
        AddIns(MakeUnary(0x28,value,new_name,type_class));
    }
    return new_name;
}

name *MaybeTemp(name *value,unsigned char type_class)
{
    if (value==NULL) value=AllocTemp(type_class);
    return value;
}

struct address_name *AddrToIns(struct address_name *value)
{
    instruction *ins;
    struct address_name *new_addr;
    if (value->format!=4) { /* NF_INS. */
        ins=MakeMove(GenIns(value),NULL,TypeClass(value->tipe));
        new_addr=InsName(ins,value->tipe);
        new_addr->flags=value->flags;
        /* No later address-name alignment field exists in this target. */
        BGDone(value);
        AddIns(ins);
    } else new_addr=value;
    return new_addr;
}

struct address_name *MakeTempAddr(name *value,type_def *type)
{
    /* This helper describes the address of the temporary: the old target
     * deliberately replaces the supplied value type with near-pointer type. */
    return MakeTypeTempAddr(value,TypeAddress(T_NEAR_POINTER));
}
