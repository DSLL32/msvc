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
* Description:  Backend block/control-flow construction.
*
****************************************************************************/


#include "address.h"
#include "cgmemory.h"
#include "genopt.h"
#include "stubs.h"
#include <recon.h>
static unsigned UnrollValue;

void BGGenCtrl(unsigned char op,bool_node *expr,struct code_lbl *label,unsigned char gen)
{
    switch (op) {
    case 86: /* O_LABEL */
        if (HaveCurrBlock) {
            GenBlock(2,1); /* JUMP */
            AddTarget(label,0);
            if (gen) Generate(0);
        }
        EnLink(label,0);
        HaveCurrBlock=1;
        CurrBlock->unroll_count=UnrollValue;
        break;
    case 81: /* O_GOTO */
        if (HaveCurrBlock) {
            GenBlock(2,1);
            AddTarget(label,0);
            if (gen) Generate(0);
        }
        HaveCurrBlock=0;
        break;
    case 85: /* O_INVOKE_LABEL */
        if (HaveCurrBlock) {
            GenBlock(0x40,1); /* CALL_LABEL */
            AddTarget(label,0);
            EnLink(AskForNewLabel(),1);
        }
        break;
    case 88: /* O_LABEL_RETURN */
        if (HaveCurrBlock) {
            AddIns(MakeNop());
            GenBlock(0x80,0);
            if (gen) Generate(0);
        }
        HaveCurrBlock=0;
        break;
    case 83: /* O_IF_TRUE */
        *expr->f=CurrBlock->label;
        *expr->t=label;
        BoolFree(expr);
        if (gen) Generate(0);
        break;
    case 84: /* O_IF_FALSE */
        *expr->t=CurrBlock->label;
        *expr->f=label;
        BoolFree(expr);
        if (gen) Generate(0);
        break;
    }
}

struct address_name *BGName(unsigned char kind,void *symbol,type_def *type)
{
    return MakeAddrName(kind,symbol,type);
}

/* bldins.c decimal constants retain the signed/unsigned conversion split. */
struct address_name *BGInteger(long value,type_def *type)
{
    struct cfloat *constant;
    if (type->attr&2) constant=CFCnvI32F(value);
    else constant=CFCnvU32F((unsigned long)value);
    return MakeConst(constant,type);
}

struct code_lbl *BGGetEntry(void)
{
    return CurrBlock->label;
}

struct address_name *BGUnary(unsigned char op,struct address_name *left,type_def *type)
{
    address_name *result;
    result=NULL;
    switch (op) {
    case 80:result=MakePoints(left,type);break;
    case 36:
        if (type==left->tipe || !NeedPtrConvert(left,type)) result=left;
        else result=CnvRnd(left,type,36);
        break;
    case 37:
        if (type==left->tipe) result=left;
        else result=CnvRnd(left,type,37);
        break;
    case 33:case 34:
        /* This old OSI build still contains the large-model diagnostic. */
        if (left->tipe->length!=4) {
            FEMessage(3,"runtime call cannot be made when DS not pegged");
            left=Unary(36,left,TypeAddress(8));
        }
        break;
    }
    if (result==NULL) result=Unary(op,left,type);
    result->tipe=type;
    return result;
}

struct address_name *BGBinary(unsigned char op,struct address_name *left,
                            struct address_name *right,type_def *type,int fold)
{
    address_name *result;
    instruction *ins;
    result=NULL;
    if (op==1 || (op==3 && (type->attr&4) && (left->tipe->attr&4) &&
                  !(right->tipe->attr&5))) {
        /* Pointer +/- integer operands retain their distinct types. */
    } else {
        left=CheckType(left,type);
        right=CheckType(right,type);
    }
    switch (op) {
    case 1:
        if (fold && (TGIsAddress() || CypAddrPlus(left,right,type)))
            result=AddrPlus(left,right,type);
        break;
    case 13:
        if (fold && (TGIsAddress() || CypAddrShift(left,right,type)))
            result=AddrShift(left,right,type);
        break;
    }
    if (result==NULL) {
        left=CheckType(left,type);
        ins=MakeBinary(op,GenIns(left),GenIns(right),NULL,TypeClass(type));
        result=InsName(ins,type);
        AddIns(ins);
        BGDone(left);
        BGDone(right);
    }
    result->tipe=type;
    return result;
}

type_def *LastCmpType;
struct bool_node *BGCompare(unsigned char op,struct address_name *left,
                           struct address_name *right,struct code_lbl *entry,type_def *type)
{
    bool_node *result;
    instruction *ins;
    name *newleft,*newright;
    LastCmpType=type;
    newleft=GenIns(left);
    newright=GenIns(right);
    BGDone(left);
    BGDone(right);
    NamesCrossBlocks();
    ins=MakeCondition(op,newleft,newright,0,1,TypeClass(type));
    AddIns(ins);
    GenBlock(4,2); /* CONDITIONAL */
    result=CGAlloc(sizeof(*result));
    AddTarget(NULL,0);
    AddTarget(NULL,0);
    result->format=3;
    result->e=entry;
    result->t=(struct code_lbl **)&CurrBlock->edge[0].destination;
    result->f=(struct code_lbl **)&CurrBlock->edge[1].destination;
    EnLink(AskForNewLabel(),1);
    return result;
}

void BoolFree(bool_node *node)
{
    CGFreeSize(node,sizeof(*node));
}

struct address_name *BGTempName(name *temp,type_def *type)
{
    temp->v.usage|=2; /* USE_IN_ANOTHER_BLOCK */
    return MakeTypeTempAddr(temp,type);
}

struct address_name *BGAssign(struct address_name *dest,struct address_name *source,type_def *type)
{
    address_name *result;
    result=MakeGets(dest,source,type);
    result->tipe=type;
    return result;
}

struct address_name *BGDuplicate(struct address_name *value)
{
    return AddrDuplicate(value);
}

void BGStartBlock(void)
{
    struct code_lbl *label;
    if (MemLow()) {
        label=AskForNewLabel();
        GenBlock(2,1); /* JUMP */
        AddTarget(label,0);
        Generate(0);
        EnLink(label,1);
    } else {
        BlkTooBig();
    }
}

void BGFiniLabel(struct code_lbl *label)
{
    block *blk;
    if (HaveCurrBlock && FiniLabel(label,CurrBlock)) return;
    blk=BlockList;
    while (blk!=NULL) {
        if (FiniLabel(label,blk)) return;
        blk=blk->prev_block;
    }
    GenKillLabel(label);
}

unsigned char FiniLabel(struct code_lbl *label,struct block *blk)
{
    int index;
    index=blk->targets;
    while (--index>=0) {
        if ((struct code_lbl *)blk->edge[index].destination==label) {
            blk->edge[index].flags|=2; /* DEST_LABEL_DIES */
            return 1;
        }
    }
    if (blk->label==label) {
        blk->edge[0].flags|=1; /* BLOCK_LABEL_DIES */
        return 1;
    }
    return 0;
}

struct address_name *Arithmetic(struct address_name *value,type_def *type)
{
    if (value->format==3) {
        if ((type->attr&1) || type->length>TypeInteger->length) {
            value=FlowOut((bool_node *)value,TypeInteger);
            value=Unary(36,value,type);
        } else {
            value=FlowOut((bool_node *)value,type);
        }
    }
    return value;
}

struct address_name *FlowOut(struct bool_node *node,type_def *type)
{
    name *temp;
    struct code_lbl *label;
    label=AskForNewLabel();
    temp=BGNewTemp(type);
    temp->v.usage|=2;
    AddIns(MakeMove(AllocIntConst(FETrue()),temp,temp->n.name_class));
    *node->t=CurrBlock->label;
    GenBlock(2,1);
    AddTarget(label,0);
    EnLink(AskForNewLabel(),1);
    AddIns(MakeMove(AllocIntConst(0),temp,temp->n.name_class));
    *node->f=CurrBlock->label;
    GenBlock(2,1);
    AddTarget(label,0);
    EnLink(label,1);
    NamesCrossBlocks();
    BoolFree(node);
    return AddrName(temp,type);
}

address_name *CheckType(address_name *value,type_def *type)
{
    if (value->format!=0) return value;
    return BGUnary(0x24,value,type); /* O_CONVERT */
}

unsigned char NeedConvert(type_def *from,type_def *to);
struct address_name *BGOpGets(unsigned char op,struct address_name *left,
    struct address_name *right,type_def *type,type_def *optype)
{
    address_name *result,*leftp;
    name *temp,*left_name;
    instruction *ins;
    unsigned char class,opclass;
    leftp=MakePoints(left,type);
    left_name=GenIns(leftp);
    class=TypeClass(type);
    opclass=TypeClass(optype);
    if (NeedConvert(type,optype)) {
        temp=AllocTemp(opclass);
        ins=MakeConvert(left_name,temp,opclass,class);
        AddIns(ins);
        AddIns(MakeBinary(op,temp,GenIns(right),temp,opclass));
        ins=MakeConvert(temp,left_name,class,opclass);
        AddIns(ins);
    } else {
        ins=MakeBinary(op,left_name,GenIns(right),left_name,opclass);
        if (type!=optype) ins->ins_flags|=4; /* INS_DEMOTED */
        AddIns(ins);
    }
    result=AddrName(left_name,type);
    BGDone(right);
    BGDone(leftp);
    return result;
}

unsigned char NeedConvert(type_def *from,type_def *to)
{
    if (from==to) return 0;
    if ((from->attr&1) || (to->attr&1)) return 1;
    return from->length!=to->length;
}

void FixCodePtr(address_name *value);
address_name *BGConvert(address_name *value,type_def *type)
{
    unsigned char attr;
    address_name *result;
    attr=value->tipe->attr;
    result=BGUnary(36,value,type);
    if (attr&8) FixCodePtr(result);
    return result;
}

struct address_name *CnvRnd(struct address_name *value,type_def *type,unsigned char op)
{
    if (NeedConvert(value->tipe,type)) value=Unary(op,value,type);
    else if (value->tipe!=type) AddrDemote(value);
    value->tipe=type;
    return value;
}

struct bool_node *BGFlow(unsigned char op,struct bool_node *left,
                         struct bool_node *right)
{
    struct code_lbl **temp;
    if (op==0x4f) { /* O_FLOW_NOT. */
        temp=left->t;
        left->t=left->f;
        left->f=temp;
        return left;
    }
    switch (op) {
    case 0x4c: /* O_FLOW_AND. */
        *left->t=right->e;
        *left->f=CurrBlock->label;
        *right->f=CurrBlock->label;
        GenBlock(2,1);
        AddTarget(NULL,0);
        left->f=(struct code_lbl **)&CurrBlock->edge[0].destination;
        left->t=right->t;
        break;
    case 0x4d: /* O_FLOW_OR. */
        *left->f=right->e;
        *left->t=CurrBlock->label;
        *right->t=CurrBlock->label;
        GenBlock(2,1);
        AddTarget(NULL,0);
        left->t=(struct code_lbl **)&CurrBlock->edge[0].destination;
        left->f=right->f;
        break;
    default:
        /* No result is established outside the three actual flow operators. */
        STUB_TRAP();
        return NULL;
    }
    BoolFree(right);
    EnLink(AskForNewLabel(),1);
    return left;
}

void BGControl(unsigned char op,bool_node *value,struct code_lbl *label)
{
    BGGenCtrl(op,value,label,0);
}

struct address_name *Unary(unsigned char op,struct address_name *value,type_def *type)
{
    instruction *ins;
    address_name *result;
    ins=MakeNary(op,GenIns(value),NULL,NULL,TypeClass(type),
                 TypeClass(value->tipe),1);
    result=InsName(ins,type);
    AddIns(ins);
    BGDone(value);
    return result;
}

/* Initial bldins.c floating leaf owns a parsed cfloat, not host-double state. */
struct address_name *BGFloat(char *value,type_def *type)
{
    return MakeConst(CFCnvSF(value,value+Length(value)),type);
}
