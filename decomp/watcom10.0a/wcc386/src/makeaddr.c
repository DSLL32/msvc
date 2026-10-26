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
