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


/* Actual1994 intrface.c BEInit, before the separate BEInitCg API.
 * Recovered WCC backend source is reused only after this image's same
 * old call order, global roles and version result were checked. */
#include <stddef.h>
#include "codegen.h"
#include "cgmemory.h"
#include "memory.h"
#include "pending.h"
#include <recon.h>
unsigned Model,TargetModel;
unsigned char OptForSize;
unsigned short CGProcessorVersion;
void BEMemInit(void) { }
void InitWeights(unsigned size);
void MemCoalesce(void);
void CalcMemLimit(void);
unsigned char MemOut;
unsigned Chunks;
void CGMemInit(void)
{
    MemCoalesce();
    MemOut=0; /* MO_FATAL */
    Chunks=0;
    CalcMemLimit();
}
void InitSegDefs(void);
void InitDbgInfo(void);
void TypeInit(void);
void TInit(void);
unsigned char CGOpenf(void);
cg_init_info BEInit(unsigned gen,unsigned target,unsigned size,unsigned short cpu)
{
    cg_init_info info;
    BEMemInit();
    OptForSize=(unsigned char)size;
    if ((cpu&15)<3) cpu=(cpu&~15)|3;
    cpu&=~0x80;
    Model=gen;
    TargetModel=target;
    CGProcessorVersion=cpu;
    InitWeights(size);
    CGMemInit();
    InitSegDefs();
    InitDbgInfo();
    TypeInit();
    TInit();
    /* The original success path leaves unused high bits of its version
     * register unspecified. Clear those bits; only the defined version
     * fields and zero/nonzero success result belong to this interface. */
    info.success=0;
    if (CGOpenf()) {
        info.version.is_large=1;
        info.version.target=1;
        info.version.revision=9;
    }
    return info;
}

/* Old memmgt.c tracked allocation, independently mapped to WPP71fb0. */
int InOptimizer;
void *CGAlloc(int size)
{
    void *chunk;
    MemLow();
    for (;;) {
        chunk=MemAlloc(size);
        if (chunk!=NULL) {
            ++Chunks;
            return chunk;
        }
        if (!MemCheck(size)) break;
    }
    if ((MemOut==MO_FATAL) | (InOptimizer!=0)) FatalError("Out of memory");
    else if (MemOut==MO_SUICIDE) Suicide();
    return NULL;
}
unsigned char MemCheck(int size) { (void)size;STUB_TRAP();return 0; }
void FatalError(char *message) { (void)message;STUB_TRAP(); }
void Suicide(void) { STUB_TRAP(); }

