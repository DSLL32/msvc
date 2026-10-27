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
#include "procdef.h"
#include "stubs.h"
#include <recon.h>

static void *FrlHead[5],*ConstDefnFrl;
long TypeClassSize[XX+1];

void InitNames(void)
{
    int class;
    for (class=0;class<5;++class) InitFrl(&FrlHead[class]);
    InitFrl(&ConstDefnFrl);
    for (class=U1;class<=XX;++class) TypeClassSize[class]=ClassType(class)->length;
    ReInitNames();
}

union name;
union name *Names[5],*LastTemp,*DummyIndex;
static union name *NullReg,*ConstOne,*ConstZero;
int TempId;

void ReInitNames(void)
{
    int class;
    TempId=0;
    NullReg=NULL;
    ConstOne=NULL;
    ConstZero=NULL;
    LastTemp=NULL;
    DummyIndex=NULL;
    for (class=0;class<5;++class) Names[class]=NULL;
}

name *AllocMemory(void *symbol,long offset,unsigned char memory_type,unsigned char type_class)
{
    return SAllocMemory(symbol,offset,memory_type,type_class,0);
}

/* namelist.c before later memory alignment/visited fields were added. */
name *SAllocMemory(void *symbol,long offset,unsigned char memory_type,unsigned char type_class,unsigned long size)
{
    name *entry,*other,*structure;
    entry=Names[1];
    other=NULL;
    structure=NULL;
    while (entry!=NULL) {
        if (entry->v.symbol==symbol && entry->m.memory_type==memory_type) {
            if (entry->v.offset==offset) {
                if (type_class==XX && size==0) return entry;
                if (entry->n.name_class==type_class && type_class!=XX) return entry;
                if (entry->n.name_class==XX && entry->n.size==size) structure=entry;
            }
            other=entry;
            entry->v.usage|=0x90;
        }
        entry=entry->n.next_name;
    }
    if (structure!=NULL) {
        ZapXX(structure,type_class,size);
        return structure;
    }
    entry=AllocName(1,type_class,size);
    entry->v.symbol=symbol;
    entry->v.offset=offset;
    entry->m.memory_type=memory_type;
    if (other!=NULL) {
        entry->m.same_sym=other->m.same_sym!=NULL ? other->m.same_sym:other;
        other->m.same_sym=entry;
        entry->v.usage=0x90;
    } else {
        entry->v.usage=0x80;
        entry->m.same_sym=NULL;
    }
    if (memory_type==CG_FE && (Model&0x80000000U)) entry->v.usage|=0x10;
    entry->v.block_usage=0;
    entry->v.conflict=NULL;
    if (memory_type==CG_LBL || memory_type==CG_CLB) entry->v.usage|=0x10;
    return entry;
}

static int Size[]={
    sizeof(const_name),sizeof(memory_name),sizeof(temp_name),
    sizeof(register_name),sizeof(indexed_name)
};

name *AllocName(int class,unsigned char type_class,long size)
{
    name *entry;
    entry=AllocFrl(&FrlHead[class],Size[class]);
    entry->n.class=(unsigned char)class;
    entry->n.next_name=Names[class];
    Names[class]=entry;
    entry->n.name_class=type_class;
    entry->n.size=TypeClassSize[type_class];
    if (entry->n.size==0) entry->n.size=size;
    return entry;
}

name *AllocRegName(hw_reg_set reg)
{
    name *entry;
    if (reg==0) {
        if (NullReg==NULL) {
            entry=AllocName(3,RegClass(reg),0);
            entry->r.reg=0;
            entry->r.reg_index=-1;
            NullReg=entry;
        }
        return NullReg;
    }
    entry=Names[3];
    while (entry!=NULL) {
        if (entry->r.reg==reg) return entry;
        entry=entry->n.next_name;
    }
    entry=AllocName(3,RegClass(reg),0);
    entry->r.reg=reg;
    entry->r.reg_index=-1;
    return entry;
}

/* The older signed/unsigned classes share each integer temporary class. */
static unsigned char OneClass[]={U1,U1,U2,U2,U4,U4,CP,PT,FS,FD,XX};
name *SAllocUserTemp(void *symbol,unsigned char type_class,long size)
{
    name *temp;
    type_class=OneClass[type_class];
    temp=LkAddBack(symbol,NULL);
    if (temp==NULL) {
        temp=AllocTemp(type_class);
        temp->v.symbol=symbol;
        if (size!=0) temp->n.size=size;
        LkAddBack(symbol,temp);
        return temp;
    }
    if (temp->n.name_class==type_class && type_class!=XX) return temp;
    return STempOffset(temp,0,type_class,size);
}

name *AllocTemp(unsigned char type_class)
{
    return SAllocTemp(type_class,0);
}

name *SAllocTemp(unsigned char type_class,long size)
{
    name *temp;
    type_class=OneClass[type_class];
    temp=AllocName(2,type_class,size); /* N_TEMP */
    temp->v.symbol=NULL;
    temp->v.offset=0;
    temp->v.usage=0;
    temp->v.block_usage=0;
    temp->v.conflict=NULL;
    temp->t.u.block_id=0xffff; /* NO_BLOCK_ID */
    temp->t.location=0x7fffffffL; /* NO_LOCATION */
    temp->t.temp_flags=0;
    temp->t.possible=26; /* RL_NUMBER_OF_SETS in the old 386 table */
    temp->t.alias=&temp->t;
    temp->t.id.id=++TempId;
    return temp;
}

/* Old namelist.c has a single DWORD integer cache and leaves an existing
 * constant in place; the later move-to-front and 64-bit cache are absent. */
name *AllocConst(struct cfloat *value)
{
    name *constant;
    long integer;
    integer=CFCnvF32(value);
    if (CFTest(value)==0 && ConstZero!=NULL) {
        CFFree(value);
        return ConstZero;
    }
    if (integer==1 && ConstOne!=NULL) {
        CFFree(value);
        return ConstOne;
    }
    for (constant=Names[0];constant!=NULL;constant=constant->n.next_name) {
        if (constant->c.const_type==0 && constant->c.int_value==integer &&
            CFCompare(constant->c.value,value)==0) {
            CFFree(value);
            return constant;
        }
    }
    constant=AllocName(0,XX,0);
    constant->c.static_defn=NULL;
    constant->c.const_type=0; /* CONS_ABSOLUTE */
    constant->c.value=value;
    constant->c.int_value=integer;
    if (ConstOne==NULL && integer==1) ConstOne=constant;
    if (ConstZero==NULL && integer==0 && CFTest(value)==0) ConstZero=constant;
    return constant;
}

name *ScaleIndex(name *index,name *base,long offset,unsigned char type_class,
                 long size,int scale,unsigned char flags)
{
    name *entry;
    type_class=OneClass[type_class];
    entry=Names[4];
    while (entry!=NULL) {
        if (entry->i.base==base && entry->i.index==index &&
            entry->i.constant==offset && entry->i.scale==scale &&
            entry->i.index_flags==flags &&
            ((entry->n.name_class==type_class && type_class!=XX) ||
             (entry->n.name_class==XX && entry->n.size==size))) {
            if (type_class!=XX) {
                entry->n.name_class=type_class;
                entry->n.size=TypeClassSize[type_class];
            }
            if (index->n.class==2) index->t.temp_flags|=8; /* INDEXED */
            return entry;
        }
        entry=entry->n.next_name;
    }
    entry=AllocName(4,type_class,size);
    entry->i.index=index;
    entry->i.base=base;
    entry->i.constant=offset;
    entry->i.index_flags=flags;
    entry->i.scale=scale;
    if (index->n.class==2) index->t.temp_flags|=8;
    return entry;
}

name *STempOffset(name *temp,long offset,unsigned char type_class,long size)
{
    name *entry,*xx;
    type_class=OneClass[type_class];
    offset=(long)((unsigned long)offset+temp->v.offset);
    entry=(name *)temp->t.alias;
    xx=NULL;
    for (;;) {
        if (entry->t.id.id==temp->t.id.id && entry->v.offset==offset) {
            if (type_class==XX && size==0) return entry;
            if (entry->n.name_class==type_class && type_class!=XX) return entry;
            if (entry->n.name_class==XX && entry->n.size==size) xx=entry;
        }
        if (entry==temp) break;
        entry=(name *)entry->t.alias;
    }
    if (xx!=NULL) {
        ZapXX(xx,type_class,size);
        return xx;
    }
    entry=AllocName(2,type_class,size);
    entry->t.id.id=temp->t.id.id;
    entry->v.symbol=temp->v.symbol;
    entry->v.offset=offset;
    entry->v.usage=temp->v.usage;
    entry->v.block_usage=0;
    entry->v.conflict=temp->v.conflict;
    entry->t.u.block_id=temp->t.u.block_id;
    entry->t.alias=temp->t.alias;
    temp->t.alias=&entry->t;
    entry->t.temp_flags=2|(temp->t.temp_flags&0x120); /* ALIAS/PERM_TEMP_FLAGS */
    entry->t.possible=26;
    if (temp->t.location==0x7fffffffL) entry->t.location=0x7fffffffL;
    else entry->t.location=(long)((unsigned long)temp->t.location+offset);
    return entry;
}

name *AllocIntConst(int value)
{
    name *constant;
    constant=FindIntValue(value);
    if (constant!=NULL) return constant;
    return AllocConst(CFCnvI32F(value));
}

name *FindIntValue(long value)
{
    if (value==0 && ConstZero!=NULL) return ConstZero;
    if (value==1 && ConstOne!=NULL) return ConstOne;
    return NULL;
}

name *DeAlias(name *temp)
{
    while (temp->t.temp_flags&2) temp=temp->t.alias; /* ALIAS */
    return temp;
}

/* redefby.c's name attribute query, using the old indexed flag byte. */
unsigned char IsVolatile(name *op)
{
    if (op->n.class==1 || op->n.class==2) return (op->v.usage&0x20)!=0;
    if (op->n.class==4) return (op->i.index_flags&2)!=0;
    return 0;
}

struct constant_defn *GetFloat(name *cons,unsigned char type_class)
{
    struct constant_defn *defn;
    for (defn=cons->c.static_defn;defn!=NULL;defn=defn->next_defn)
        if (defn->const_class==type_class) return defn;
    defn=AllocFrl(&ConstDefnFrl,sizeof(*defn));
    defn->label=NULL;
    defn->const_class=type_class;
    /* Earlier CFCnvTarget accepts the actual class, not its later byte size. */
    CFCnvTarget(cons->c.value,(union cg_float *)defn->value,type_class);
    defn->next_defn=cons->c.static_defn;
    cons->c.static_defn=defn;
    return defn;
}
name *AllocUIntConst(unsigned value)
{
    name *constant;
    constant=FindIntValue(value);
    if (constant!=NULL) return constant;
    return AllocConst(CFCnvU32F(value));
}
void FreeTable(tbl_control *table) { STUB_TRAP(); }
void FreeAName(name *op)
{
    struct constant_defn *defn,*junk;
    if (op->n.class==0) {
        if (op==ConstZero) ConstZero=NULL;
        if (op==ConstOne) ConstOne=NULL;
        if (op->c.const_type==0) {
            CFFree(op->c.value);
            defn=op->c.static_defn;
            while (defn!=NULL) {
                junk=defn;
                defn=defn->next_defn;
                FrlFreeSize(&ConstDefnFrl,junk,sizeof(*junk));
            }
        }
    } else if (op->n.class==1) {
        if (op->m.memory_type==CG_TBL) FreeTable(op->v.symbol);
    } else if (op->n.class==3) {
        if (op==NullReg) NullReg=NULL;
    } else if (op->n.class==2) {
        if (op==LastTemp) LastTemp=op->n.next_name;
        if (op==DummyIndex) DummyIndex=NULL;
    }
    FrlFreeSize(&FrlHead[op->n.class],op,Size[op->n.class]);
}

void FreeNames(void)
{
    int class;
    name *temp,*junk;
    for (class=0;class<5;++class) {
        temp=Names[class];
        Names[class]=NULL;
        while (temp!=NULL) {
            junk=temp;
            temp=temp->n.next_name;
            FreeAName(junk);
        }
    }
}

void ZapXX(name *entry,unsigned char type_class,long size)
{
    if (type_class!=XX) {
        entry->n.name_class=type_class;
        entry->n.size=TypeClassSize[type_class];
    } else if (size!=0) entry->n.size=size;
}

name *SAllocIndex(name *index,name *base,long offset,unsigned char class,unsigned long size)
{
    return ScaleIndex(index,base,offset,class,size,0,0);
}

name *TempOffset(name *value,long offset,unsigned char type_class)
{ return STempOffset(value,offset,type_class,0); }

name *AllocIndex(name *index,name *base,long offset,unsigned char type_class)
{
    return SAllocIndex(index,base,offset,type_class,0);
}

name *AllocS32Const(long value)
{
    name *constant;
    constant=FindIntValue(value);
    if (constant!=NULL) return constant;
    return AllocConst(CFCnvI32F(value));
}

name *AllocAddrConst(name *value,long offset,unsigned char kind,unsigned char type_class)
{
    name *entry;
    for (entry=Names[0];entry!=NULL;entry=entry->n.next_name) {
        if (entry->c.const_type==kind && entry->c.value==value &&
            entry->n.name_class==type_class && entry->c.int_value==offset)
            return entry;
    }
    entry=AllocName(0,type_class,0);
    entry->c.static_defn=NULL;
    entry->c.value=value;
    entry->c.int_value=offset;
    entry->c.const_type=kind;
    return entry;
}
