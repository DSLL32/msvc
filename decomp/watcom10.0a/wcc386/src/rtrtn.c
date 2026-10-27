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
#include "block.h"
#include "stubs.h"
#include <recon.h>

/* The old runtime-helper set has82 entries. Handles and labels are
 * initialized independently; no later helper-name inventory is assumed. */
enum { RTSIZE=82 };
struct code_lbl;
static unsigned RTHdls[RTSIZE];
static struct code_lbl *RTLbls[RTSIZE];

void InitRT(void)
{
    int i;
    for (i=0;i<RTSIZE;++i) {
        RTHdls[i]=0;
        RTLbls[i]=NULL;
    }
}

/* The original 386rtrtn.c target hook deliberately leaves no operand fixed. */
unsigned char RTLeaveOp2(instruction *ins)
{
    (void)ins;
    return 0;
}

name *Addressable(name *opnd,unsigned char class) { STUB_TRAP();return NULL; }



int RoutineNum;

void *BEAuxInfo(void *handle,unsigned char request) { STUB_TRAP();return NULL; }
