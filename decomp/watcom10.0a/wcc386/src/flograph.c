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
* Description:  Construct intervals and nesting in the backend flow graph.
*
****************************************************************************/


#include "block.h"
#include "genopt.h"
#include "cgmemory.h"
#include "stubs.h"
#include <recon.h>

/* The old interval level is a BYTE, followed by allocation padding. */
#pragma pack(4)
typedef struct interval_def {
    struct interval_def *parent;
    block *first_block,*last_block;
    struct interval_def *sub_int,*next_sub_int,*link;
    unsigned char level;
} interval_def;
#pragma pack()
typedef char old_interval_size[(sizeof(interval_def)==28)?1:-1];
static interval_def *Intervals;
interval_def *IntervalNo(block *blk,int level);
void NewInterval(block *blk,int level);
static unsigned char functionDegenerate;
unsigned char FlowDone(block *curr,void *parm);
void FloodForward(block *blk,unsigned char (*func)(block *,void *),void *parm);
void MakeFlowGraph(void)
{
    Irreducable();
    if (CurrProc->state.attr&0x80) return; /* ROUTINE_WANTS_DEBUGGING */
    NoBlocksToSelf();
    Intervals=NULL;
    if (HeadBlock!=NULL) {
        if (!DepthFirstSearch()) {
            Irreducable();
            RestoreLinks();
            Renumber();
            return;
        }
        FixLinks();
        if (FindIntervals()) {
            ReorderBlocks();
            EdgeLevels();
            NestingDepth();
            if (Degenerate()) Irreducable();
        } else Irreducable();
        KillIntervals();
        ReturnsToBottom();
        Renumber();
    }
}
void Irreducable(void)
{
    block *blk;
    for (blk=HeadBlock;blk!=NULL;blk=blk->next_block) {
        blk->class&=~0x110U; /* ITERATIONS_KNOWN | LOOP_HEADER */
        blk->loop_head=NULL;
        blk->depth=1;
    }
}
void NoBlocksToSelf(void)
{
    block *blk,*new_blk;
    block_edge *edge,*new_edge,*curr,**owner,*next;
    for (blk=HeadBlock;blk!=NULL;blk=blk->next_block) {
        edge=blk->input_edges;
        while (edge!=NULL) {
            next=edge->next_source;
            if (edge->source==blk) {
                new_blk=NewBlock(AskForNewLabel(),1);
                new_blk->class=2; /* JUMP */
                new_blk->gen_id=blk->gen_id;
                new_blk->ins.hd.line_num=blk->ins.hd.line_num;
                new_blk->next_block=blk->next_block;
                new_blk->prev_block=blk;
                ++new_blk->targets;
                ++new_blk->inputs;
                new_blk->input_edges=edge;
                blk->next_block=new_blk;
                if (new_blk->next_block!=NULL) new_blk->next_block->prev_block=new_blk;
                edge->destination=new_blk;
                edge->flags&=~2U; /* DEST_LABEL_DIES */
                new_edge=&new_blk->edge[0];
                new_edge->flags|=8;
                new_edge->destination=blk;
                new_edge->source=new_blk;
                owner=&blk->input_edges;
                for (;;) {
                    curr=*owner;
                    if (curr==edge) break;
                    owner=&(*owner)->next_source;
                }
                *owner=new_edge;
                new_edge->next_source=curr->next_source;
                edge->next_source=NULL;
            }
            edge=next;
        }
    }
}
unsigned char DepthFirstSearch(void)
{
    block *blk;
    unsigned char reducible;
    BlockList=NULL;
    MarkVisited(HeadBlock);
    reducible=1;
    for (blk=HeadBlock;blk!=NULL;blk=blk->next_block) {
        if (!(blk->class&0x800)) {
            blk->prev_block=BlockList;
            BlockList=blk;
            reducible=0;
        }
        blk->class&=~0x800U;
    }
    return reducible;
}
void MarkVisited(block *blk)
{
    int i;
    blk->class|=0x800; /* BLOCK_VISITED */
    for (i=0;i<blk->targets;++i) {
        if (blk->edge[i].destination->class&0x800) continue;
        /* Historical SafeRecurse callback ABI: one i386 object pointer.
         * This actual callback is void; all returned EAX values are ignored.
         * The cast is local to this no-result call and does not define a
         * pointer result or qualify the period low-stack relocation path. */
        SafeRecurse((recurse_callback)MarkVisited,blk->edge[i].destination);
    }
    blk->prev_block=BlockList;
    BlockList=blk;
}
void RestoreLinks(void) { STUB_TRAP();  }
void FixLinks(void)
{
    block *blk,*prev;
    int id;
    blk=BlockList;
    HeadBlock=blk;
    prev=NULL;
    id=0;
    for (;;) {
        blk->next_block=blk->prev_block;
        blk->prev_block=prev;
        blk->id=++id;
        prev=blk;
        blk=blk->next_block;
        if (blk==NULL) break;
    }
    BlockList=prev;
}
unsigned char FindIntervals(void)
{
    block               *blk;
    block_edge          *edge;
    interval_def        *curr;
    interval_def        *prev_int;
    interval_def        *test;
    int                 level;
    int                 num;
    int                 prev_num;
    unsigned char       add;

    num = 0;
    blk = HeadBlock;
    for(;;) {
        ++ num;
        curr=CGAlloc(sizeof(interval_def));
        curr->link = Intervals;
        Intervals = curr;
        curr->parent = NULL;
        curr->sub_int = NULL;
        curr->next_sub_int = NULL;
        curr->first_block = blk;
        curr->last_block = blk;
        curr->level = 0;
        blk->depth = 0;
        blk->u.interval = curr;
        blk = blk->next_block;
        if( blk == NULL ) break;
    }
    level = 1;
    for(;;) {
        prev_num = num;
        num = 1;                       /* at least one node at new level*/
        NewInterval( HeadBlock, level );
        blk = HeadBlock;
        for(;;) {
            blk = blk->next_block;
            if( blk == NULL ) break;
            curr = IntervalNo( blk, level - 1 );
            if( curr->parent == NULL ) {
                prev_int = NULL;
                edge = blk->input_edges;
                add = 0;
                for(;;) {
                    test = IntervalNo( edge->source, level - 1 );
                                                /* guess - internal edge*/
                    if( test != curr ) {
                                                /* guess - lower level head*/
                        test = test->parent;
                        if( test == NULL ) {
                            add = 1;
                            break;
                        }
                                                /* guess - no other predecessor*/
                        if( prev_int == NULL ) {
                            prev_int = test;
                        } else {
                                                /* guess - different predecessor*/
                            if( test != prev_int ) {
                                add = 1;
                                break;
                            }
                        }
                    }
                    edge = edge->next_source;
                    if( edge == NULL ) break;
                }
                if( add == 0 ) {
                    curr->parent = prev_int;
                    prev_int->last_block = curr->last_block;
                    test = prev_int->sub_int;
                    while( test->next_sub_int != NULL ) {
                        test = test->next_sub_int;
                    }
                    test->next_sub_int = curr;
                } else {                        /* admit - create a new interval*/
                    NewInterval( blk, level );
                    num ++;
                }
            }
        }
        ++ level;
        if( num == prev_num || num == 1 ) break;
    }
    return( num == 1 );
}
void ReorderBlocks(void)
{
    interval_def *curr;
    block *last_block,*next_block;
    last_block=HeadBlock;
    curr=HeadBlock->u.interval;
    for (;;) {
        for (;;) {
            if (curr->next_sub_int!=NULL) break;
            curr=curr->parent;
            if (curr==NULL) break;
            curr->last_block=last_block;
        }
        if (curr==NULL) break;
        curr=curr->next_sub_int;
        while (curr->level>0) curr=curr->sub_int;
        next_block=curr->first_block;
        next_block->prev_block=last_block;
        last_block->next_block=next_block;
        next_block->id=last_block->id+1;
        last_block=next_block;
    }
    last_block->next_block=NULL;
    BlockList=last_block;
}
void EdgeLevels(void)
{
    block *blk;
    block_edge *edge;
    interval_def *interval;
    unsigned short id;
    blk=HeadBlock;
    for (;;) {
        edge=blk->input_edges;
        while (edge!=NULL) {
            id=edge->source->id;
            interval=blk->u.interval;
            for (;;) {
                if (id>=interval->first_block->id && id<=interval->last_block->id) break;
                edge->join_level=interval->level;
                interval=interval->parent;
                if (interval==NULL) break;
            }
            edge=edge->next_source;
        }
        blk=blk->next_block;
        if (blk==NULL) break;
    }
}
void NestingDepth(void)
{
    block               *blk;
    int                 level;
    interval_def        *interval;
    block_edge          *edge;
    block               *target;
    int                 i;
    unsigned char       change;

    interval = BlockList->u.interval->parent;
    while( interval->parent != NULL ) {
        level = interval->level - 1;
        blk = BlockList;               /* borrow 'next_block'*/
        for(;;) {                            /* identify all back edges at this level*/
            blk->next_block = NULL;
            i = blk->targets;
            while( --i >= 0 ) {
                edge = &blk->edge[ i ];
                target = edge->destination;
                if( target->id <= blk->id ) {     /* if back edge*/
                    if( edge->join_level == level ) {
                        blk->next_block = target;
                        if( blk->loop_head == NULL ) {
                            blk->loop_head = target;
                        }
                        blk->depth ++;
                        break;
                    }
                }
            }
            blk = blk->prev_block;
            if( blk == NULL ) break;
        }
        for(;;) {
            change = 0;
            blk = BlockList;
            for(;;) {
                if( blk->next_block == NULL ) {
                    i = blk->targets;
                    while( --i >= 0 ) {
                        edge = & blk->edge[ i ];
                        if( edge->join_level <= level ) {
                            target = edge->destination->next_block;
                            if( target != NULL ) {
                                blk->depth ++;
                                blk->next_block = target;  /* store head in node*/
                                if( blk->loop_head == NULL && blk != target ) {
                                    blk->loop_head = target;
                                }
                                target->class |= 0x100;
                                change = 1;
                                break;
                            }
                        }
                    }
                }
                blk = blk->prev_block;
                if( blk == NULL ) break;
            }
            if( change == 0 ) break;
        }
        interval = interval->parent;
    }

/*   Restore 'next_block'*/

    blk = BlockList;
    HeadBlock = NULL;
    for(;;) {
        blk->next_block = HeadBlock;
        HeadBlock = blk;
        blk = blk->prev_block;
        if( blk == NULL ) break;
    }
}
unsigned char Degenerate(void)
{
    block *blk;
    block_edge *edge;
    for (blk=HeadBlock;blk!=NULL;blk=blk->next_block) {
        if (!(blk->class&0x100)) continue; /* LOOP_HEADER */
        for (edge=blk->input_edges;edge!=NULL;edge=edge->next_source) {
            if (edge->source->class&8) { /* SELECT */
                functionDegenerate=0;
                FloodForward(edge->source,FlowDone,blk);
                if (functionDegenerate) return 1;
            }
        }
    }
    return 0;
}
void KillIntervals(void)
{
    interval_def *junk;
    block *blk;
    while (Intervals!=NULL) {
        junk=Intervals;
        Intervals=Intervals->link;
        CGFreeSize(junk,sizeof(interval_def));
    }
    blk=HeadBlock;
    while (blk!=NULL) {
        blk->u.interval=NULL;
        blk=blk->next_block;
    }
}
void ReturnsToBottom(void)
{
    block *curr,*prev,*last;
    curr=HeadBlock;
    while (curr->next_block!=NULL) curr=curr->next_block;
    last=curr;
    curr=curr->prev_block;
    while (curr!=NULL) {
        prev=curr->prev_block;
        if (curr->class&1) { /* RETURN */
            if (prev!=NULL) prev->next_block=curr->next_block;
            curr->next_block->prev_block=prev;
            last->next_block=curr;
            curr->prev_block=last;
            curr->next_block=NULL;
            last=curr;
        }
        curr=prev;
    }
    BlockList=last;
}

interval_def *IntervalNo(block *blk,int level)
{
    interval_def *curr;
    curr=blk->u.interval;
    while (--level>=0) curr=curr->parent;
    return curr;
}
void NewInterval(block *blk,int level)
{
    interval_def *prev,*new_int;
    prev=IntervalNo(blk,level-1);
    new_int=CGAlloc(sizeof(interval_def));
    new_int->link=Intervals;
    Intervals=new_int;
    new_int->sub_int=prev;
    new_int->next_sub_int=NULL;
    new_int->level=level;
    new_int->parent=NULL;
    new_int->first_block=blk;
    new_int->last_block=blk;
    prev->parent=new_int;
}

unsigned char FlowDone(block *curr,void *parm) { STUB_TRAP(); return 0; }
void FloodForward(block *blk,unsigned char (*func)(block *,void *),void *parm) { STUB_TRAP(); }
