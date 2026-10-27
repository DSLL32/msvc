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

/* Reached pragma lookup from the actual 1994 compiler. Original source
 * reference: initial Open Watcom cpragma.c at6b9cb443. */
#include "cpragma.h"
#include "macro.h"
#include "compiler.h"
#include "utility.h"
#include "cmdline.h"
#include "asm.h"
#include "pending.h"
#include <recon.h>
AUX_INFO *CurrInfo;
/* Address identity is encountered before the later pragma initialization.
 * These are genuine zero-initialized compiler objects, not host shims. */
AUX_INFO DefaultInfo;
static AUX_INFO CdeclInfo, PascalInfo;
static AUX_INFO FortranInfo, SyscallInfo, StdcallInfo;

void *PragmaLookup(char *name, unsigned index)
{
    AUX_ENTRY *entry;

    if (index && setAuxInfo(index, 0)) return CurrInfo;
    if (setAuxInfo(lookupMagicKeyword(name), 0)) return CurrInfo;
    entry = AuxLookup(name);
    if (!entry) return 0;
    return entry->info;
}

AUX_ENTRY *AuxLookup(char *name) { (void)name; STUB_TRAP(); return 0; }

int setAuxInfo(unsigned index, int create_new)
{
    switch (index) {
    case 1: CurrInfo = &DefaultInfo; break;
    case 2: CurrInfo = &CdeclInfo; break;
    case 3: CurrInfo = &PascalInfo; break;
    case 4: CurrInfo = &FortranInfo; break;
    case 5: CurrInfo = &SyscallInfo; break;
    case 6: CurrInfo = &StdcallInfo; break;
    default:
        if (!create_new) return 0;
        CreateAux(Buffer);
        break;
    }
    return 1;
}



/* Initial cpragma.c/cprag86.c initialization adapted to the older six
 * conventions and32-bit hardware masks. No later Optlink/8087-save setup. */
static AUX_INFO Far16CdeclInfo, Far16PascalInfo;
AUX_ENTRY *AuxList;
unsigned char InitPriority;

void PragInit(void)
{
    DefaultInfo.use=2;
    CdeclInfo=DefaultInfo;
    PascalInfo=DefaultInfo;
    FortranInfo=DefaultInfo;
    SyscallInfo=DefaultInfo;
    StdcallInfo=DefaultInfo;
    InitPriority=64;
}

void pragmaInitInfo(AUX_INFO *info, unsigned call_type, char *name)
{
    static unsigned stackParms[]={ 0 };
    info->_class=call_type;
    info->objname=strsave(name);
    info->parms=(unsigned *)vctsave((char *)stackParms,sizeof(stackParms));
    info->returns=0;
}

void pragmaInit(INITFINI *entry)
{
    unsigned call_type;
    (void)entry;
    PragInit();
    AuxList=0;
    call_type=DefaultInfo._class & 0x1000;
    FortranInfo.objname=strsave("^");
    pragmaInitInfo(&CdeclInfo,call_type|0x640900,"_*");
    pragmaInitInfo(&PascalInfo,call_type|0x240801,"^");
    pragmaInitInfo(&SyscallInfo,0x200900,"*");
    pragmaInitInfo(&StdcallInfo,call_type|0x800,"_*");
    CdeclInfo.streturn=0x01000003;
    SyscallInfo.streturn=0;
    CdeclInfo.save &= 0xf0ffff00u;
    Far16CdeclInfo=DefaultInfo;
    SyscallInfo.save &= 0xf2ffff0cu;
    PascalInfo.save &= 0xf2ffff0cu;
    Far16PascalInfo=DefaultInfo;
    Far16CdeclInfo._class=0x40640800;
    Far16CdeclInfo.objname="*";
    Far16PascalInfo._class=0x40240801;
    Far16PascalInfo.objname="*";
}

void CreateAux(char *name) { (void)name; STUB_TRAP(); }

/* Initial cprag86.c assembler registration, with the1994 CPU0..5 set. */
void assemblerInit(INITFINI *entry)
{
    int cpu=3;
    (void)entry;
    switch (CpuSwitches & 0x0f) {
    case 0: cpu=0; break;
    case 1: cpu=1; break;
    case 2: cpu=2; break;
    case 3: cpu=3; break;
    case 4: cpu=4; break;
    case 5: cpu=5; break;
    }
    AsmInit(cpu,(CpuSwitches & 0xf0)!=0,1);
}

int ReverseParms(void *pragma) { return (((AUX_INFO *)pragma)->_class&1)!=0; }
