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

/* Initial cmdlnx86.c macroDefs, adapted to the1994 direct macro strings
 * and old switch bits. Later DefSwitchMacro/generated-options helpers are
 * absent from this target. */
#include "frontend.h"
#include "cmdline.h"
#include <recon.h>

void defEmu(void)
{
    if (CpuSwitches & 0x80) PreDefineStringMacro("__SW_FPI");
    else PreDefineStringMacro("__SW_FPI87");
}

void macroDefs(void)
{
    if (TargetSwitches & 1) PreDefineStringMacro("__SW_OM");
    switch (TargetSwitches & 0x3c) {
    case 0: PreDefineStringMacro("__SW_MS"); break;
    case 8: PreDefineStringMacro("__SW_MM"); break;
    case 0x14: PreDefineStringMacro("__SW_MC"); break;
    case 0x1c: PreDefineStringMacro("__SW_ML"); break;
    case 0xc: PreDefineStringMacro("__SW_MH"); break;
    case 0x20: PreDefineStringMacro("__SW_MF"); break;
    }
    if (TargetSwitches & 0x40) PreDefineStringMacro("__SW_ZFF");
    else PreDefineStringMacro("__SW_ZFP");
    if (TargetSwitches & 0x80) PreDefineStringMacro("__SW_ZGF");
    else PreDefineStringMacro("__SW_ZGP");
    if (TargetSwitches & 0x400) PreDefineStringMacro("__SW_ZDF");
    else PreDefineStringMacro("__SW_ZDP");
    if (TargetSwitches & 0x200) PreDefineStringMacro("__SW_ZU");
    if (TargetSwitches & 0x1000) PreDefineStringMacro("__SW_XGV");
    if (TargetSwitches & 0x2000) PreDefineStringMacro("__SW_ZW");
    if (TargetSwitches & 0x100000) PreDefineStringMacro("__SW_ZWS");
    if (TargetSwitches & 0x8000) PreDefineStringMacro("__SW_OC");
    if (TargetSwitches & 0x20000) PreDefineStringMacro("__SW_OF");
    if (GenSwitches & 0x80000000u) PreDefineStringMacro("__SW_OD");
    if (GenSwitches & 0x04000000u) PreDefineStringMacro("__SW_OA");
    if (GenSwitches & 0x00400000u) PreDefineStringMacro("__SW_OL");
    if (GenSwitches & 0x00200000u) PreDefineStringMacro("__SW_OR");
    if (CompFlags.signed_char) PreDefineStringMacro("__SW_J");
    if (CompFlags.use_pcheaders) PreDefineStringMacro("__SW_FH");
    if (CompFlags.no_pch_warnings) PreDefineStringMacro("__SW_FHQ");
    if (CompFlags.inline_intrinsics) PreDefineStringMacro("__SW_OI");
    if (CompFlags.zu_switch_used) PreDefineStringMacro("__SW_ZU");
    if (CompFlags.za_switch_used) PreDefineStringMacro("__SW_OU");
    if (CompFlags.register_conventions) PreDefineStringMacro("__SW_3R");
    else PreDefineStringMacro("__SW_3S");
    if (CompFlags.emit_names) PreDefineStringMacro("__SW_EN");
    if (CompFlags.save_restore_segregs) PreDefineStringMacro("__SW_R");
    if (CompFlags.zc_switch_used) PreDefineStringMacro("__SW_ZC");
    if (CompFlags.use_unicode) PreDefineStringMacro("__SW_ZK");
    if (CompFlags.sg_switch_used) PreDefineStringMacro("__SW_SG");
    if (CompFlags.st_switch_used) PreDefineStringMacro("__SW_ST");
    if (CompFlags.bm_switch_used) PreDefineStringMacro("__SW_BM");
    if (CompFlags.bd_switch_used) PreDefineStringMacro("__SW_BD");
    if (CompFlags.bw_switch_used) PreDefineStringMacro("__SW_BW");
    if (CompFlags.zm_switch_used) PreDefineStringMacro("__SW_ZM");
    if (CompFlags.ep_switch_used) PreDefineStringMacro("__SW_EP");
    if (CompFlags.ee_switch_used) PreDefineStringMacro("__SW_EE");
    if (CompFlags.ec_switch_used) PreDefineStringMacro("__SW_EC");
    switch (DestructorMethod) {
    case 0:
        PreDefineStringMacro("__SW_XDT");
        PreDefineStringMacro("__SW_XD"); break;
    case 1: PreDefineStringMacro("__SW_XST"); break;
    case 2: PreDefineStringMacro("__SW_XDS"); break;
    case 3: PreDefineStringMacro("__SW_XS"); break;
    case 4: PreDefineStringMacro("__SW_XSS"); break;
    }
    switch (CpuSwitches & 0x0f) {
    case 1: PreDefineStringMacro("__SW_1"); break;
    case 2: PreDefineStringMacro("__SW_2"); break;
    case 3: PreDefineStringMacro("__SW_3"); break;
    case 4: PreDefineStringMacro("__SW_4"); break;
    case 5: PreDefineStringMacro("__SW_5"); break;
    }
    switch (CpuSwitches & 0x70) {
    case 0:
        CompFlags.op_switch_used=0;
        PreDefineStringMacro("__SW_FPC"); break;
    case 0x10: PreDefineStringMacro("__SW_FP287"); defEmu(); break;
    case 0x20: PreDefineStringMacro("__SW_FP387"); defEmu(); break;
    case 0x30: PreDefineStringMacro("__SW_FP5"); defEmu(); break;
    }
    if (CompFlags.op_switch_used) PreDefineStringMacro("__SW_OP");
    if (CompFlags.extensions_enabled) PreDefineStringMacro("SOMLINK=__syscall");
}
