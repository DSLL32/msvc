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


/* Adapted from cprag86.c PragmaInit; this 1994 body predates OptlinkInfo
 * and calls AsmInit every time. The original aux_info occupies 40 bytes. */
#include <string.h>
#include "pragma.h"
#include <recon.h>

struct aux_info CdeclInfo,PascalInfo,FortranInfo,SyscallInfo,StdcallInfo;
struct aux_info STOSBInfo,Far16CdeclInfo,Far16PascalInfo;
struct library_list *HeadLibs;
static hw_reg_set StackParms[]={0};
static hw_reg_set STOSBParms[]={HW_EAX,HW_EDX,HW_ECX,0};

void PragmaInit(void)
{
    unsigned call_type;
    int cpu,fpu;
    cpu=3;
    switch (ProcRevision&15) {
    case 0: cpu=0; break;
    case 1: cpu=1; break;
    case 2: cpu=2; break;
    case 3: cpu=3; break;
    case 4: cpu=4; break;
    case 5: cpu=5; break;
    }
    fpu=(ProcRevision&0xf0)!=0;
    AsmInit(cpu,fpu,1);
    PragInit();
    HeadLibs=NULL;
    call_type=DefaultInfo.class&0x1000U;
    CdeclInfo.class=call_type|0x640900U;
    CdeclInfo.objname=CStrSave("_*");
    CdeclInfo.parms=(hw_reg_set *)CMemAlloc(sizeof(StackParms));
    memcpy(CdeclInfo.parms,StackParms,sizeof(StackParms));
    CdeclInfo.returns=0;
    PascalInfo.class=call_type|0x240801U;
    PascalInfo.parms=(hw_reg_set *)CMemAlloc(sizeof(StackParms));
    memcpy(PascalInfo.parms,StackParms,sizeof(StackParms));
    PascalInfo.objname=CStrSave("^");
    PascalInfo.returns=0;
    FortranInfo.objname=CStrSave("^");
    StdcallInfo.class=call_type|0x800U;
    StdcallInfo.objname=CStrSave("_*");
    StdcallInfo.parms=(hw_reg_set *)CMemAlloc(sizeof(StackParms));
    memcpy(StdcallInfo.parms,StackParms,sizeof(StackParms));
    StdcallInfo.returns=0;
    CdeclInfo.streturn=HW_EAX;
    CdeclInfo.save &= ~(HW_EAX|HW_EBX|HW_ECX|HW_EDX);
    PascalInfo.save &= ~(HW_EAX|HW_EBX|HW_ECX|HW_EDX);
    StdcallInfo.save &= ~(HW_EAX|HW_ECX|HW_EDX);
    SyscallInfo.parms=(hw_reg_set *)CMemAlloc(sizeof(StackParms));
    memcpy(SyscallInfo.parms,StackParms,sizeof(StackParms));
    SyscallInfo.objname=CStrSave("*");
    SyscallInfo.class=0x200900U;
    SyscallInfo.streturn=0;
    SyscallInfo.save &= ~(HW_EAX|HW_EBX|HW_ECX|HW_EDX);
    SyscallInfo.save |= HW_EBX;
    STOSBInfo=DefaultInfo;
    STOSBInfo.class=0x240800U;
    STOSBInfo.parms=STOSBParms;
    STOSBInfo.objname="*";
    Far16CdeclInfo=CdeclInfo;
    Far16CdeclInfo.class |= 0x40000000U;
    Far16PascalInfo=PascalInfo;
    Far16PascalInfo.class |= 0x40000000U;
}

/* cfeinfo.c without REVERSE: the x86 build keeps declared parameter order. */
int ParmsToBeReversed(int flags,struct aux_info *info)
{
    (void)flags;
    (void)info;
    return 0;
}
