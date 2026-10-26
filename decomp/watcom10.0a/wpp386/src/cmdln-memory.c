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

/* Actual1994 cmdlnx86.c memory-model/library selection. The constant copies
 * were compiler intrinsics here, unlike setFinalTargetSystem's strcpy. */
#include <string.h>
#include "frontend.h"
#include "compiler.h"
#include "cpragma.h"
#include "cmdline.h"
#include <recon.h>
#if defined(__WATCOMC__)
#pragma intrinsic(strcpy)
#endif

void setMemoryModel(void)
{
    char convention;
    CodePtrSize=DataPtrSize=4;
    switch (TargetSwitches & 0x0c) {
    case 0:
        if (TargetSwitches & 0x20) {
            PreDefineStringMacro("__FLAT__");
        } else {
            PreDefineStringMacro("M_I86SM");
            PreDefineStringMacro("__SMALL__");
            CompFlags.strings_in_code=0;
        }
        TargetSwitches &= ~0x10000u;
        break;
    case 8:
        CodePtrSize=6;
        DefaultInfo._class |= 0x1000;
        PreDefineStringMacro("M_I86MM");
        PreDefineStringMacro("__MEDIUM__");
        CompFlags.strings_in_code=0;
        TargetSwitches &= ~0x10000u;
        break;
    case 4:
        PreDefineStringMacro("M_I86CM");
        PreDefineStringMacro("__COMPACT__");
        DataPtrSize=6;
        break;
    case 12:
        CodePtrSize=DataPtrSize=6;
        DefaultInfo._class |= 0x1000;
        if (TargetSwitches & 0x10) {
            PreDefineStringMacro("M_I86LM");
            PreDefineStringMacro("__LARGE__");
        } else {
            PreDefineStringMacro("M_I86HM");
            PreDefineStringMacro("__HUGE__");
        }
        break;
    }
    if (CompFlags.register_conventions) {
        convention='r';
        PreDefineStringMacro("__3R__");
    } else {
        convention='s';
        PreDefineStringMacro("__3S__");
    }
    strcpy(CLIB_Name,"1clib3x");
    if (CompFlags.bm_switch_used ||
        (CompFlags.bd_switch_used && TargetSystem!=3))
        strcpy(WCPPLIB_Name,"4plibmt3x");
    else
        strcpy(WCPPLIB_Name,"4plib3x");
    if (CpuSwitches & 0x80) {
        strcpy(MATHLIB_Name,"8math387x");
        EmuLib_Name="9emu387";
    } else if ((CpuSwitches & 0x70)==0) {
        strcpy(MATHLIB_Name,"5math3x");
        EmuLib_Name=NULL;
    } else {
        strcpy(MATHLIB_Name,"8math387x");
        EmuLib_Name="9noemu387";
    }
    *strchr(CLIB_Name,'x')=convention;
    *strchr(MATHLIB_Name,'x')=convention;
    *strchr(WCPPLIB_Name,'x')=convention;
}

/* Initial cgtype.c model predicates; the code predicate returns its bit. */
int IsBigData(void)
{
    return (TargetSwitches & 4)!=0;
}

int IsHugeData(void)
{
    return (TargetSwitches & 0x1c)==0x0c;
}

int IsBigCode(void)
{
    return TargetSwitches & 8;
}
