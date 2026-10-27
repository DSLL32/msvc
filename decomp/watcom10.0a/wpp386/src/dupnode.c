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
* Description:  WHEN YOU FIGURE OUT WHAT THIS FILE DOES, PLEASE
*               DESCRIBE IT HERE!
*
****************************************************************************/


/* Adapted from initial6b9cb443 bld/plusplus/c/dupnode.c. The actual1994
 * PROMO has four pointers, before the later container tracking field.
 * The source-position filter also still receives the last emitted position. */
#include "compiler.h"
#include "carve.h"
#include "ptree.h"
#include "symbol.h"
#include "pending.h"
#include <recon.h>
typedef struct promo PROMO;
struct promo { PROMO *next; PTREE dup_src,anc_src,anc_tgt; };
typedef char PromoSize[(sizeof(PROMO)==16)?1:-1];
static carve_t carvePromo;
static PROMO *promoRing,*promoHdr;
static CGIO_LOCATION *exprPosition,*emittedPosition;
TYPE NodeType(PTREE node);
SYMBOL TemporaryAlloc(TYPE type);
PTREE MakeNodeSymbol(SYMBOL sym);
PTREE NodeBinary(unsigned char op,PTREE left,PTREE right);
void *RingPrune(void **head,void *entry);
void RingAppend(void **head,void *entry);
PTREE convertToTemp(PTREE node,SYMBOL temp)
{
    node->op=8;
    node->cgop=92;
    node->type=temp->sym_type;
    node->u.subtree[0]=MakeNodeSymbol(temp);
    node->u.subtree[1]=NULL;
    node->flags&=~0x4000;
    return node;
}
PTREE dupPromote(PROMO *promo)
{
    PTREE first=promo->dup_src,second=first->u.dup.partner;
    PTREE expr=second->u.dup.expr,node,ancestor;
    TYPE type=NodeType(expr);
    SYMBOL temp=TemporaryAlloc(type);
    temp->sym_type=type;
    convertToTemp(first,temp);
    convertToTemp(second,temp);
    expr=NodeBinary(13,MakeNodeSymbol(temp),expr);
    expr->type=type;
    expr->flags=0x13;
    ancestor=promo->anc_src;
    ancestor->flags&=~0x4000;
    node=PTreeAlloc();
    *node=*ancestor;
    ancestor->op=9;
    ancestor->cgop=42;
    ancestor->u.subtree[0]=expr;
    ancestor->u.subtree[1]=node;
    return ancestor;
}
void addPromo(PTREE node)
{
    PROMO *promo=CarveAlloc(carvePromo);
    promo->dup_src=node;
    promo->anc_src=node;
    promo->anc_tgt=node->u.dup.partner;
    promo->anc_tgt->flags|=0x4000;
    node->flags|=0x4000;
    RingPush((void **)&promoHdr,promo);
}
void setAncestor(PTREE left,PTREE right,PTREE child)
{
    PROMO *last,*next,*promo;
    PTREE common;
    if(child==NULL || promoHdr==NULL) return;
    last=promoHdr;
    next=last->next;
    do {
        promo=next;
        next=promo->next;
        common=promo->anc_src;
        if(child==common) { common=left; promo->anc_src=left; }
        if(child==promo->anc_tgt) promo->anc_tgt=right;
        if(common==promo->anc_tgt &&
           (common->op!=9 || (common->cgop!=43 && common->cgop!=44))) {
            RingPrune((void **)&promoHdr,promo);
            RingAppend((void **)&promoRing,promo);
        }
    } while(promo!=last);
}
PTREE dupProc(PTREE node)
{
    if(emittedPosition->src_file==NULL ||
       node->locn.line>emittedPosition->line) {
        if(exprPosition->src_file==NULL || node->locn.line<exprPosition->line)
            *exprPosition=node->locn;
    }
    if(promoHdr==NULL) {
        if(node->op==10) addPromo(node);
    } else {
        switch(node->op) {
        case 9:
            setAncestor(node,node,node->u.subtree[1]);
            /* fall through */
        case 8:
            setAncestor(node,node,node->u.subtree[0]);
            break;
        case 10:
            setAncestor(node,node->u.subtree[1],node->u.subtree[0]);
            addPromo(node);
            break;
        }
    }
    return node;
}
PTREE NodePromoteDups(PTREE tree,CGIO_LOCATION *position,CGIO_LOCATION *emitted)
{
    PROMO *last,*next,*current;
    promoHdr=NULL;
    promoRing=NULL;
    exprPosition=position;
    position->src_file=NULL;
    emittedPosition=emitted;
    tree=PTreeTraversePostfix(tree,dupProc);
    last=promoRing;
    if(last) {
        next=last->next;
        do {
            current=next;
            next=current->next;
            dupPromote(current);
            CarveFree(carvePromo,current);
        } while(current!=last);
    }
    return tree;
}
void nodePromoInit(INITFINI *defn)
{
    (void)defn;
    carvePromo=CarveCreate(sizeof(PROMO),16);
}
