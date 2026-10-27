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


#ifndef WPP_CALLGRPH_H
#define WPP_CALLGRPH_H
#include "compiler.h"
#include "carve.h"
#include "vstk.h"
#include "cgio.h"
#include <recon.h>
typedef struct callgraph CALLGRAPH;
typedef struct callnode CALLNODE;
typedef struct dirgraph_ctl DIRGRAPH_CTL;
typedef struct dirgraph_node DIRGRAPH_NODE;
typedef struct dirgraph_edge DIRGRAPH_EDGE;
typedef struct dirgraph_vft DIRGRAPH_VFT;
struct dirgraph_ctl { DIRGRAPH_NODE *objects; DIRGRAPH_VFT *vft; };
struct dirgraph_node { DIRGRAPH_NODE *next; void *object; DIRGRAPH_EDGE *edges; };
struct dirgraph_edge { DIRGRAPH_EDGE *next; DIRGRAPH_NODE *target; };
struct dirgraph_vft {
    void (*init_ctl)(DIRGRAPH_CTL *);
    void (*fini_ctl)(DIRGRAPH_CTL *);
    DIRGRAPH_NODE *(*alloc_node)(DIRGRAPH_CTL *);
    DIRGRAPH_EDGE *(*alloc_edge)(DIRGRAPH_CTL *);
    void (*free_node)(DIRGRAPH_CTL *,DIRGRAPH_NODE *);
    void (*free_edge)(DIRGRAPH_CTL *,DIRGRAPH_EDGE *);
    DIRGRAPH_NODE *(*init_node)(DIRGRAPH_CTL *,DIRGRAPH_NODE *);
    DIRGRAPH_EDGE *(*init_edge)(DIRGRAPH_CTL *,DIRGRAPH_EDGE *);
    DIRGRAPH_EDGE *(*dup_edge)(DIRGRAPH_CTL *,DIRGRAPH_EDGE *);
    void (*prune_node)(DIRGRAPH_CTL *,DIRGRAPH_NODE *);
    void (*prune_edge)(DIRGRAPH_CTL *,DIRGRAPH_EDGE *);
};
struct callgraph {
    DIRGRAPH_CTL base;
    VSTK_CTL calls;
    carve_t carve_nodes,carve_edges;
    unsigned incr_refs,incr_addrs,depth;
    unsigned pruned:1,reserved:31;
};
/* Old40B node: counts precede the byte flags and file pointer. It predates
 * the later unresolved-use and per-scope exception fields. */
struct callnode {
    DIRGRAPH_NODE base;
    unsigned refs,addrs,depth,opcodes,cond_flags;
    unsigned char state_table:1,calls_done:1,is_vft:1,inline_fun:1,
        inlineable:1,inlineable_oe:1,reserved:2;
    unsigned :0; /* period PAD_UNSIGNED before the file pointer */
    CGFILE *cgfile;
};
typedef struct calledge { DIRGRAPH_EDGE base; unsigned refs,addrs; } CALLEDGE;
typedef char CallGraphSize[(sizeof(CALLGRAPH)==48)?1:-1];
typedef char CallNodeSize[(sizeof(CALLNODE)==40)?1:-1];
typedef char CallEdgeSize[(sizeof(CALLEDGE)==16)?1:-1];
#endif
