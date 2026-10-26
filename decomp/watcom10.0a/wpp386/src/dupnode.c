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
#include "pending.h"
#include <recon.h>
typedef struct promo PROMO;
struct promo { PROMO *next; PTREE dup_src,anc_src,anc_tgt; };
typedef char PromoSize[(sizeof(PROMO)==16)?1:-1];
static carve_t carvePromo;
static PROMO *promoRing,*promoHdr;
static CGIO_LOCATION *exprPosition,*emittedPosition;
PTREE dupPromote(PROMO *promo)
{ (void)promo; STUB_TRAP(); return NULL; }
void addPromo(PTREE node)
{ (void)node; STUB_TRAP(); }
void setAncestor(PTREE left,PTREE right,PTREE child)
{ (void)left; (void)right; (void)child; STUB_TRAP(); }
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
