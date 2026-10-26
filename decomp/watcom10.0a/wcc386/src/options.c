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


#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "osi-globals.h"
#include "cmemory.h"
#include "options.h"
#include "macros.h"
#include <recon.h>

unsigned ProcRevision,TargetSwitches;
unsigned PackAmount,GblPackAmount;
struct aux_info DefaultInfo;
unsigned char Save8087Regs;

void GenCOptions(char **cmdline)
{
    InitModInfo();
    InitCPUModInfo();
    ProcOptions(getenv("WCC386"),0);
    for (; *cmdline!=NULL; ++cmdline) ProcOptions(*cmdline,0);
    if (CompFlags.cpp_output_requested) CompFlags.cpp_output=1;
    if (CompFlags.cpp_output) CompFlags.quiet_mode=1;
    CBanner();
    if (!CompFlags.command_line_processed) {
        GblPackAmount=PackAmount;
        SetTargSystem();
        SetGenSwitches();
        Define_Memory_Model();
        if ((ProcRevision & 15U)<3U)
            TargetSwitches &= ~(FLOATING_FS|FLOATING_GS);
        if (!CompFlags.save_restore_segregs) {
            if (TargetSwitches & FLOATING_DS) DefaultInfo.save &= ~HW_DS;
            if (TargetSwitches & FLOATING_ES) DefaultInfo.save &= ~HW_ES;
            if (TargetSwitches & FLOATING_FS) DefaultInfo.save &= ~HW_FS;
            if (TargetSwitches & FLOATING_GS) DefaultInfo.save &= ~HW_GS;
        }
        if (!CompFlags.register_conventions) SetStackConventions();
        if (!Save8087Regs) DefaultInfo.save &= ~HW_FLTS;
        MacroDefs();
        MiscMacroDefs();
    }
}

unsigned GenSwitches;
unsigned char Stack87;
char *CodeClassName,*PCH_FileName,*TextSegName,*DataSegName,*GenCodeGroup;
int TargSys;
void InitCPUModInfo(void)
{
    Stack87=8;
    CodeClassName=NULL;
    PCH_FileName=NULL;
    TextSegName=DataSegName=GenCodeGroup="";
    TargSys=0;
    GenSwitches=0x00100000U; /* MEMORY_LOW_FAILS */
    CompFlags.register_conv_set=0;
    CompFlags.register_conventions=1;
}

/* The 1994 form expands environment indirection recursively (limit10),
 * and borrows the input filename. Later source adds response files and
 * allocated/quoted filename collection; those are absent here. */
void ProcOptions(char *str,unsigned depth)
{
    char *env;
    char ch;
    if (str==NULL) return;
    for (;;) {
        while (*str==' ' || *str=='\t') ++str;
    indirect:
        if (*str=='@' && depth<10) {
            do { ++str; } while (*str==' ' || *str=='\t');
            env=TokenBuf;
            for (;;) {
                ch=*str;
                if (ch=='\0') break;
                ++str;
                if (ch==' ' || ch=='\t' || ch=='-' || ch==SwitchChar) break;
                *env++=ch;
            }
            *env='\0';
            env=getenv(TokenBuf);
            if (env!=NULL) ProcOptions(env,depth+1);
            goto indirect;
        }
        if (*str=='\0') return;
        if (*str=='-' || *str==SwitchChar) {
            str=ProcessOption(str+1);
        } else {
            if (WholeFName==NULL) WholeFName=str;
            else {
                CBanner();
                CErr1(0x473);
            }
            while (*str!='\0' && *str!=' ' && *str!='\t' && *str!=SwitchChar) ++str;
        }
    }
}

/* Actual28848 consumes AL, not a full integer argument; result is EAX0/1. */
int EqualChar(unsigned char character)
{
    return character=='#' || character=='=';
}

char *TargetSystemName;
void SetTargName(char *name,unsigned length)
{
    char *p;
    if (TargetSystemName!=NULL) {
        CMemFree(TargetSystemName);
        TargetSystemName=NULL;
    }
    if (name==NULL || length==0) return;
    TargetSystemName=(char *)CMemAlloc(length+1);
    p=TargetSystemName;
    while (length!=0) {
        *p++=toupper((unsigned char)*name++);
        --length;
    }
    *p='\0';
}

#if defined(__WATCOMC__)
#pragma intrinsic(strcpy)
#endif
unsigned char MemoryModel;
void SetTargSystem(void)
{
    char buffer[128];
    PreDefine_Macro("__386__");
    PreDefine_Macro("M_I386");
    if (TargetSystemName==NULL) {
        switch (__OS) {
        case OS_DOS: case OS_WIN: SetTargName("dos",3); break;
        case OS_OS2: SetTargName("os2",3); break;
        case OS_NT: SetTargName("nt",2); break;
        }
    }
    if (strcmp(TargetSystemName,"DOS")==0) TargSys=TS_DOS;
    else if (strcmp(TargetSystemName,"NETWARE")==0) TargSys=TS_NETWARE;
    else if (strcmp(TargetSystemName,"WINDOWS")==0) TargSys=TS_WINDOWS;
    else if (strcmp(TargetSystemName,"CHEAP_WINDOWS")==0) TargSys=TS_CHEAP_WINDOWS;
    else if (strcmp(TargetSystemName,"NT")==0) TargSys=TS_NT;
    else if (strcmp(TargetSystemName,"PENPOINT")==0) TargSys=TS_PENPOINT;
    else if (strcmp(TargetSystemName,"QNX")==0) TargSys=TS_QNX;
    else TargSys=TS_OTHER;
    switch (TargSys) {
    case TS_DOS:
        PreDefine_Macro("MSDOS");
        break;
    case TS_QNX:
        if (MemoryModel==0) MemoryModel=2; /* SW_MS */
        break;
    case TS_NETWARE:
        if (MemoryModel==0) MemoryModel=2;
        PreDefine_Macro("__NETWARE_386__");
        Stack87=4;
        /* fall through */
    case TS_PENPOINT:
        if (!CompFlags.register_conv_set) CompFlags.register_conventions=0;
        break;
    case TS_CHEAP_WINDOWS:
        TargSys=TS_WINDOWS;
        /* fall through */
    case TS_WINDOWS:
        PreDefine_Macro("__WINDOWS_386__");
        if (!PegOptions.fs_used) {
            PegOptions.fs_on=0;
            PegOptions.fs_used=1;
        }
        if (FpuMode==0 || FpuMode==2) FpuMode=3;
        TargetSwitches |= 0x6000U; /* WINDOWS | CHEAP_WINDOWS */
        break;
    }
    strcpy(buffer,"__");
    strcat(buffer,TargetSystemName);
    strcat(buffer,"__");
    PreDefine_Macro(buffer);
}

void SetGenSwitches(void)
{
    if (CpuModel==0) CpuModel=6;
    if (FpuLevel==0) FpuLevel=2;
    if (MemoryModel==0) MemoryModel=1;
    TargetSwitches |= 0x800U; /* USE_32 */
    switch (FpuLevel) {
    case 1: ProcRevision=(ProcRevision & ~0x70U)|0x10U; break;
    case 2: ProcRevision=(ProcRevision & ~0x70U)|0x20U; break;
    case 3: ProcRevision=(ProcRevision & ~0x70U)|0x30U; break;
    }
    switch (FpuMode) {
    case 0: case 2:
        FpuMode=2;
        ProcRevision |= 0x80U;
        break;
    case 1: ProcRevision &= ~0xf0U; break;
    case 3: ProcRevision &= ~0x80U; break;
    }
    ProcRevision=(ProcRevision & ~0x0fU)|(CpuModel-1);
    switch (MemoryModel) {
    case 1: /* flat */
        TargetSwitches |= 0x30U;
        if (!PegOptions.ds_used) PegOptions.ds_on=1;
        if (!PegOptions.es_used) PegOptions.es_on=1;
        if (!PegOptions.fs_used) PegOptions.fs_on=1;
        /* fall through */
    case 2: /* small */
        TargetSwitches |= 0x10U;
        if (!PegOptions.ds_used) PegOptions.ds_on=1;
        break;
    case 3: /* medium */
        TargetSwitches |= 0x18U;
        if (!PegOptions.ds_used) PegOptions.ds_on=1;
        break;
    case 4: TargetSwitches |= 0x14U; break; /* compact */
    case 5: case 6: TargetSwitches |= 0x1cU; break; /* large, huge */
    }
    if (!PegOptions.ds_on) TargetSwitches |= FLOATING_DS;
    if (!PegOptions.es_on) TargetSwitches |= FLOATING_ES;
    if (!PegOptions.fs_on) TargetSwitches |= FLOATING_FS;
    if (!PegOptions.gs_on) TargetSwitches |= FLOATING_GS;
}

int DataPtrSize,CodePtrSize;
char CLIB_Name[8],MATHLIB_Name[10];
char *EmuLib_Name;
void Define_Memory_Model(void)
{
    char model;
    DataPtrSize=CodePtrSize=4;
    switch (TargetSwitches & 0x0cU) {
    case 0:
        if (TargetSwitches & 0x20U) Define_Macro("__FLAT__");
        else {
            Define_Macro("M_I86SM");
            Define_Macro("__SMALL__");
            CompFlags.strings_in_code_segment=0;
        }
        TargetSwitches &= ~0x10000U;
        break;
    case 8:
        CodePtrSize=6;
        DefaultInfo.class |= 0x1000U; /* FAR */
        Define_Macro("M_I86MM");
        Define_Macro("__MEDIUM__");
        CompFlags.strings_in_code_segment=0;
        TargetSwitches &= ~0x10000U;
        break;
    case 4:
        Define_Macro("M_I86CM");
        Define_Macro("__COMPACT__");
        DataPtrSize=6;
        break;
    case 12:
        CodePtrSize=6;
        DefaultInfo.class |= 0x1000U;
        if (TargetSwitches & 0x10U) {
            Define_Macro("M_I86LM");
            Define_Macro("__LARGE__");
        } else {
            Define_Macro("M_I86HM");
            Define_Macro("__HUGE__");
        }
        DataPtrSize=6;
        break;
    }
    model=CompFlags.register_conventions?'r':'s';
    strcpy(CLIB_Name,"1clib3x");
    if (ProcRevision & 0x80U) {
        strcpy(MATHLIB_Name,"8math387x");
        EmuLib_Name="9emu387";
    } else if ((ProcRevision & 0xf0U)==0) {
        strcpy(MATHLIB_Name,"5math3x");
        EmuLib_Name=NULL;
    } else {
        strcpy(MATHLIB_Name,"8math387x");
        EmuLib_Name="9noemu387";
    }
    *strchr(CLIB_Name,'x')=model;
    *strchr(MATHLIB_Name,'x')=model;
}

void MacroDefs(void)
{
    if (TargetSwitches & 1U) Define_Macro("__SW_OM");
    switch (MemoryModel) {
    case 1: Define_Macro("__SW_MF"); break;
    case 2: Define_Macro("__SW_MS"); break;
    case 3: Define_Macro("__SW_MM"); break;
    case 4: Define_Macro("__SW_MC"); break;
    case 5: Define_Macro("__SW_ML"); break;
    case 6: Define_Macro("__SW_MH"); break;
    }
    if (TargetSwitches & FLOATING_FS) Define_Macro("__SW_ZFF");
    else Define_Macro("__SW_ZFP");
    if (TargetSwitches & FLOATING_GS) Define_Macro("__SW_ZGF");
    else Define_Macro("__SW_ZGP");
    if (TargetSwitches & FLOATING_DS) Define_Macro("__SW_ZDF");
    else Define_Macro("__SW_ZDP");
    if (TargetSwitches & 0x200U) Define_Macro("__SW_ZU");
    if (TargetSwitches & 0x1000U) Define_Macro("__SW_XGV");
    if (TargetSwitches & 0x2000U) Define_Macro("__SW_ZW");
    if (TargetSwitches & 0x8000U) Define_Macro("__SW_OC");
    if (TargetSwitches & 0x20000U) Define_Macro("__SW_OF");
    if (GenSwitches & 0x80000000U) Define_Macro("__SW_OD");
    if (GenSwitches & 0x04000000U) Define_Macro("__SW_OA");
    if (GenSwitches & 0x00400000U) Define_Macro("__SW_OL");
    if (GenSwitches & 0x00200000U) Define_Macro("__SW_OR");
    if (GenSwitches & 0x00080000U) Define_Macro("__SW_ON");
    if (CompFlags.signed_char) Define_Macro("__SW_J");
    if (PCH_FileName!=NULL) Define_Macro("__SW_PH");
    if (CompFlags.no_pch_warnings) Define_Macro("__SW_FHQ");
    if (CompFlags.inline_functions) Define_Macro("__SW_OI");
    if (CompFlags.zu_switch_used) Define_Macro("__SW_ZU");
    if (CompFlags.unique_functions) Define_Macro("__SW_OU");
    if (CompFlags.register_conventions) Define_Macro("__SW_3R");
    else Define_Macro("__SW_3S");
    if (CompFlags.emit_names) Define_Macro("__SW_EN");
    if (CompFlags.save_restore_segregs) Define_Macro("__SW_R");
    if (CompFlags.make_enums_an_int) Define_Macro("__SW_EI");
    if (CompFlags.zc_switch_used) Define_Macro("__SW_ZC");
    if (!CompFlags.use_unicode) Define_Macro("__SW_ZK");
    if (CompFlags.sg_switch_used) Define_Macro("__SW_SG");
    if (CompFlags.st_switch_used) Define_Macro("__SW_ST");
    if (CompFlags.bm_switch_used) Define_Macro("__SW_BM");
    if (CompFlags.bd_switch_used) Define_Macro("__SW_BD");
    if (CompFlags.bw_switch_used) Define_Macro("__SW_BW");
    if (CompFlags.zm_switch_used) Define_Macro("__SW_ZM");
    if (CompFlags.ep_switch_used) Define_Macro("__SW_EP");
    if (CompFlags.ee_switch_used) Define_Macro("__SW_EE");
    if (CompFlags.ec_switch_used) Define_Macro("__SW_EC");
    switch (ProcRevision & 15U) {
    case 0: Define_Macro("__SW_0"); break;
    case 1: Define_Macro("__SW_1"); break;
    case 2: Define_Macro("__SW_2"); break;
    case 3: Define_Macro("__SW_3"); break;
    case 4: Define_Macro("__SW_4"); break;
    case 5: Define_Macro("__SW_5"); break;
    }
    switch (FpuMode) {
    case 1:
        CompFlags.op_switch_used=0;
        Define_Macro("__SW_FPC");
        break;
    case 2:
        Define_Macro("__SW_FPI");
        Define_Macro("__FPI__");
        break;
    case 3:
        Define_Macro("__SW_FPI87");
        Define_Macro("__FPI__");
        break;
    }
    switch (ProcRevision & 0x70U) {
    case 0x10: Define_Macro("__SW_FP2"); break;
    case 0x20: Define_Macro("__SW_FP3"); break;
    case 0x30: Define_Macro("__SW_FP5"); break;
    }
    if (PegOptions.nd_used) Define_Macro("__SW_ND");
    if (CompFlags.op_switch_used) Define_Macro("__SW_OP");
}
