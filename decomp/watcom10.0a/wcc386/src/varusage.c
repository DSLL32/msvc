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
* Description:  Collect per-block backend variable uses and definitions.
*
****************************************************************************/


#include "block.h"
#include "genopt.h"
#include "cgmemory.h"
#include "stubs.h"
#include <recon.h>

void FindReferences(void)
{
    block *curr;
    int word;
    curr=HeadBlock;
    while (curr!=NULL) {
        if (curr->dataflow==NULL) curr->dataflow=CGAlloc(sizeof(data_flow_def));
        for (word=0;word<4;++word) {
            curr->dataflow->def.bits[word]=0;
            curr->dataflow->use.bits[word]=0;
            curr->dataflow->in.bits[word]=0;
            curr->dataflow->out.bits[word]=0;
            curr->dataflow->call_exempt.bits[word]=0;
            curr->dataflow->need_load.bits[word]=0;
            curr->dataflow->need_store.bits[word]=0;
        }
        curr=curr->next_block;
    }
    SearchDefUse();
}

/* Actual 1994 opcode/name values precede later backend enum additions. */
enum { REF_OP_LA=0x28, REF_OP_CALL_INDIRECT=0x29, REF_OP_CALL=0x36,
       REF_OP_BLOCK=0x4b, REF_MEMORY=1, REF_TEMP=2, REF_INDEXED=4,
       REF_USE_ADDRESS=8, REF_USE_OTHER_BLOCK=2, REF_FAKE_BASE=0x10 };

void SearchDefUse(void)
{
    block *blk;
    instruction *ins;
    name *op;
    int i;
    unsigned char touched_non_op;

    blk=HeadBlock;
    for (;;) {
        touched_non_op=0;
        ins=blk->ins.hd.next;
        while (ins->head.opcode!=REF_OP_BLOCK) {
            if ((ins->head.opcode==REF_OP_CALL ||
                 ins->head.opcode==REF_OP_CALL_INDIRECT) &&
                ((ins->flags.call_flags&1)==0 || (ins->flags.call_flags&2)==0)) {
                UseDefGlobals(blk);
                touched_non_op=1;
            }
            i=ins->num_operands;
            while (--i>=0) {
                op=ins->operands[i];
                if (op->n.class==REF_INDEXED) {
                    Use(op->i.index,blk,0);
                    if (op->i.base!=NULL && !(op->i.index_flags&REF_FAKE_BASE))
                        Use(op->i.base,blk,REF_USE_ADDRESS);
                } else {
                    Use(op,blk,ins->head.opcode==REF_OP_LA && i==0 ? REF_USE_ADDRESS : 0);
                }
            }
            op=ins->result;
            if (op!=NULL) {
                if (op->n.class==REF_INDEXED) {
                    Use(op->i.index,blk,0);
                    if (op->i.base!=NULL && !(op->i.index_flags&REF_FAKE_BASE))
                        Use(op->i.base,blk,REF_USE_ADDRESS);
                } else if (op->n.class==REF_MEMORY) {
                    op->v.usage|=REF_USE_OTHER_BLOCK;
                    Define(op,blk);
                } else if (op->n.class==REF_TEMP) {
                    Define(op,blk);
                }
            }
            ins=ins->head.next;
        }
        if (touched_non_op && !BlockByBlock) TransferAllMemBlockUsage();
        else TransferMemBlockUsage(blk);
        TransferTempBlockUsage(blk);
        blk=blk->next_block;
        if (blk==NULL) break;
    }
    TransferTempFlags();
    TransferMemoryFlags();
}

/* Private variable-use successors remain genuine pending application entries. */
void UseDefGlobals(block *blk)
{
    conflict_node *conf;
    unsigned short usage;
    int word;
    for (conf=ConfList;conf!=NULL;conf=conf->next_conflict) {
        if (conf->name->n.class==REF_MEMORY) {
            usage=conf->name->v.block_usage;
            Use(conf->name,blk,0);
            Define(conf->name,blk);
            for (word=0;word<4;++word)
                blk->dataflow->def.bits[word]|=conf->id.out_of_block.bits[word];
            if (!(usage&4)) {
                for (word=0;word<4;++word)
                    blk->dataflow->use.bits[word]|=conf->id.out_of_block.bits[word];
            }
        }
    }
}
void Use(name *op,block *blk,unsigned short usage)
{
    int word;
    /* The old compiler lowered the released source's tail recursion to
     * this loop. CONS_TEMP_ADDR is 4 in this target. */
    while (op->n.class==0 && op->c.const_type==4) {
        op=(name *)op->c.value;
        usage=REF_USE_ADDRESS;
    }
    if (op->n.class==REF_TEMP) op=DeAlias(op);
    else if (op->n.class!=REF_MEMORY) return;
    if (op->v.block_usage&4) op->v.block_usage|=1; /* defined/used within block */
    else {
        op->v.block_usage|=REF_USE_OTHER_BLOCK;
        if (op->v.conflict!=NULL) {
            for (word=0;word<4;++word)
                blk->dataflow->use.bits[word]|=op->v.conflict->id.out_of_block.bits[word];
        }
    }
    op->v.block_usage|=usage;
}
void Define(name *op,block *blk)
{
    name *actual;
    int word;
    actual=op;
    actual->v.block_usage|=4; /* DEF_WITHIN_BLOCK */
    if (op->n.class==REF_TEMP) op=DeAlias(op);
    if (op->v.block_usage&REF_USE_OTHER_BLOCK) return;
    if (actual!=op && !CoveringDefinitions(op)) return;
    op->v.block_usage|=4;
    if (op->v.conflict==NULL) return;
    for (word=0;word<4;++word)
        blk->dataflow->def.bits[word]|=op->v.conflict->id.out_of_block.bits[word];
}
unsigned char CoveringDefinitions(name *op) { STUB_TRAP(); return 0; }
void TransferAllMemBlockUsage(void)
{
    name *mem;
    for (mem=Names[REF_MEMORY];mem!=NULL;mem=mem->n.next_name)
        TransferBlockUsage(mem);
}
void TransferMemBlockUsage(block *blk)
{
    instruction *ins;
    int i;
    ins=blk->ins.hd.next;
    while (ins->head.opcode!=REF_OP_BLOCK) {
        i=ins->num_operands;
        while (--i>=0) TransferOneMemBlockUsage(ins->operands[i]);
        if (ins->result!=NULL) TransferOneMemBlockUsage(ins->result);
        ins=ins->head.next;
    }
}
void TransferOneMemBlockUsage(name *op)
{
    /* The base recursion is an actual tail loop in the 1994 body. */
    for (;;) {
        if (op->n.class==REF_INDEXED) {
            TransferOneMemBlockUsage(op->i.index);
            if (op->i.base!=NULL && !(op->i.index_flags&REF_FAKE_BASE)) {
                op=op->i.base;
                continue;
            }
        } else if (op->n.class==REF_MEMORY) {
            TransferBlockUsage(op);
        }
        return;
    }
}
void TransferBlockUsage(name *op)
{
    op->v.usage|=op->v.block_usage;
    if (op->v.usage&REF_USE_OTHER_BLOCK) op->v.usage&=~1U;
    op->v.block_usage=0;
}
void TransferTempBlockUsage(block *blk)
{
    instruction *ins;
    int i;
    ins=blk->ins.hd.next;
    while (ins->head.opcode!=REF_OP_BLOCK) {
        i=ins->num_operands;
        while (--i>=0) TransferOneTempBlockUsage(ins->operands[i]);
        if (ins->result!=NULL) TransferOneTempBlockUsage(ins->result);
        ins=ins->head.next;
    }
}
void TransferOneTempBlockUsage(name *op)
{
    name *alias;
    for (;;) {
        while (op->n.class==0 && op->c.const_type==4) op=(name *)op->c.value;
        if (op->n.class==REF_INDEXED) {
            TransferOneTempBlockUsage(op->i.index);
            if (op->i.base!=NULL && !(op->i.index_flags&REF_FAKE_BASE)) {
                op=op->i.base;
                continue;
            }
        } else if (op->n.class==REF_TEMP) {
            alias=(name *)op->t.alias;
            for (;;) {
                TransferBlockUsage(alias);
                if (alias==op) break;
                alias=(name *)alias->t.alias;
            }
        }
        return;
    }
}
void TransferTempFlags(void)
{
    block *blk;
    instruction *ins;
    int i;
    for (blk=HeadBlock;blk!=NULL;blk=blk->next_block) {
        ins=blk->ins.hd.next;
        while (ins->head.opcode!=REF_OP_BLOCK) {
            i=ins->num_operands;
            while (--i>=0) TransferOneTempFlag(ins->operands[i]);
            if (ins->result!=NULL) TransferOneTempFlag(ins->result);
            ins=ins->head.next;
        }
    }
    for (blk=HeadBlock;blk!=NULL;blk=blk->next_block) TransferTempBlockUsage(blk);
}
void TransferOneTempFlag(name *op)
{
    name *alias;
    unsigned short usage;
    for (;;) {
        if (op->n.class==REF_TEMP) {
            op=DeAlias(op);
            alias=op;
            usage=0;
            do {
                usage|=alias->v.usage;
                alias=(name *)alias->t.alias;
            } while (alias!=op);
            alias=op;
            do {
                alias->v.usage|=usage;
                alias=(name *)alias->t.alias;
            } while (alias!=op);
        } else if (op->n.class==REF_INDEXED) {
            TransferOneTempFlag(op->i.index);
            if (op->i.base!=NULL && !(op->i.index_flags&REF_FAKE_BASE)) {
                op=op->i.base;
                continue;
            }
        }
        return;
    }
}
void TransferMemoryFlags(void)
{
    name *mem,*same_sym;
    unsigned short usage;
    /* This old implementation revisits same-symbol rings; the released
     * version's later MF_VISITED field/pass is absent from the 1994 body. */
    for (mem=Names[REF_MEMORY];mem!=NULL;mem=mem->n.next_name) {
        if (mem->m.same_sym!=NULL) {
            usage=mem->v.usage;
            same_sym=mem->m.same_sym;
            while (same_sym!=mem) {
                usage|=same_sym->v.usage;
                same_sym=same_sym->m.same_sym;
            }
            mem->v.usage=usage;
            same_sym=mem->m.same_sym;
            while (same_sym!=mem) {
                same_sym->v.usage|=usage;
                same_sym=same_sym->m.same_sym;
            }
        }
    }
}
