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

/* Actual1994 hand-written option dispatcher. Initial cmdlnany.c/cmdlnx86.c
 * at6b9cb443 supplies option names/design; its generated OPT_STORAGE parser
 * is later and is not substituted for this original switch/call graph. */
#include <ctype.h>
#include "compiler.h"
#include "frontend.h"
#include "cmdline.h"
#include "cmdln-options.h"
#include "cpragma.h"
#include "pending.h"
#include "utility.h"
#include "scan.h"
#include <recon.h>
unsigned ProEpiDataSize, ZOptionNumber;

void processSwitch(void)
{
    int c, option;
    int value;
    MEPTR macro;
    unsigned char *flags = (unsigned char *)&CompFlags;

    option = CmdScanLowerChar();
    switch (option) {
    case '3': case '4': case '5':
        CpuSwitches = (CpuSwitches & ~0x0fu) | (option-'0');
        scanRegConvention();
        break;
    case '7':
        CpuSwitches &= ~0x80u;
        scanFpuSetup();
        break;
    case 'b':
        switch (CmdScanLowerChar()) {
        case 'd': CompFlags.bd_switch_used=1; break;
        case 'm': CompFlags.bm_switch_used=1; break;
        case 'w': CompFlags.bw_switch_used=1; break;
        case 't': scanTarget(); break;
        default: BadCmdLine(68); break;
        }
        break;
    case 'd':
        c=CmdScanChar();
        if (isdigit((unsigned char)c)) {
            switch (c-'0') {
            case 0: break;
            case 1:
                GenSwitches |= 0x40000000u;
                if (CmdRecogChar('+')) {
                    CompFlags.debug_plus_used=1;
                    GenSwitches |= 0x03000000u;
                    CmdLnWarnDebug();
                }
                break;
            case 3:
                CompFlags.all_debug_type_names=1;
                /* fall through: level3 includes level2 data. */
            case 2:
                GenSwitches |= 0xc3000000u;
                flags[0] &= ~0x20u; /* old debug flag reset */
                if (CmdRecogChar('~')) CompFlags.debug_tilde_used=1;
                break;
            default: BadCmdLine(68); break;
            }
        } else if (c=='+' && CmdScanSwEnd()) {
            CompFlags.extended_defines=1;
        } else if (c=='b' && CmdScanSwEnd()) {
            CompFlags.emit_browser_info=1;
        } else {
            if (c!='+') CmdScanUngetChar();
            macro=DefineCmdLineMacro(c=='+' ? 1 : CompFlags.extended_defines);
            if (macro) macro->macro_flags |= 4;
        }
        break;
    case 'e':
        c=tolower((unsigned char)CmdScanChar());
        switch (c) {
        case 'c': CompFlags.ec_switch_used=1; break;
        case 'e': CompFlags.ee_switch_used=1; break;
        case 'i': CompFlags.ei_switch_used=1; break;
        case 'n': CompFlags.emit_names=1; break;
        case 'p':
            CompFlags.ep_switch_used=1;
            CmdScanRecogEquals();
            getNumber(&ProEpiDataSize,0);
            break;
        case 'w': CompFlags.ew_switch_used=1; break;
        case 'z': TargetSwitches |= 2; break;
        default:
            CmdScanUngetChar();
            getNumber(&ErrLimit,20);
            break;
        }
        break;
    case 'f':
        c=CmdPeekChar();
        switch (c) {
        case 'h': case 'i': case 'o': scanFOption(); break;
        case 't': CmdScanChar(); CompFlags.check_truncated_fnames=1; break;
        case 'x': CmdScanChar(); CompFlags.check_truncated_fnames=0; break;
        case 'p':
            CmdScanChar();
            c=CmdScanLowerChar();
            switch (c) {
            case 'c': CpuSwitches &= ~0xf0u; break;
            case 'i':
                if (scan87Suffix()) CpuSwitches &= ~0x80u;
                else CpuSwitches |= 0x80;
                scanFpuSetup();
                break;
            case '2': case '3':
                scan87Suffix();
                CpuSwitches=(CpuSwitches&~0xf0u)|(CpuSwitches&0x80)|(c=='2'?0x10:0x20);
                break;
            case '5': CpuSwitches=(CpuSwitches&~0xf0u)|(CpuSwitches&0x80)|0x30; break;
            case 'r': Stack87=4; break;
            default: BadCmdLine(68); break;
            }
            break;
        default: BadCmdLine(68); break;
        }
        break;
    case 'g': CmdScanRecogEquals(); scanFileName(&GenCodeGroup,1); break;
    case 'h':
        switch (tolower((unsigned char)CmdScanChar())) {
        case 'w': break;
        case 'd': GenSwitches |= 0x10000000u; break;
        case 'c': GenSwitches |= 0x08000000u; break;
        default: BadCmdLine(68); break;
        }
        break;
    case 'i': scanIncludeOption(); break;
    case 'j': PTypeSignedChar(); CompFlags.signed_char=1; break;
    case 'm': scanMemoryModel(); break;
    case 'n': scanSegmentOption(); break;
    case 'o': scanOptimization(); break;
    case 'p': scanPreprocess(); break;
    case 'r':
        if (CmdRecogChar('i')) CompFlags.returns_promoted=1;
        else CompFlags.save_restore_segregs=1;
        break;
    case 's':
        if (CmdRecogChar('g')) CompFlags.sg_switch_used=1;
        else if (CmdRecogChar('t')) CompFlags.st_switch_used=1;
        else PragToggle &= ~1u;
        break;
    case 'u': undefineMacro(); break;
    case 'v': CompFlags.dump_prototypes=1; break;
    case 'w':
        scanNumber(&value);
        if (value!=-1) WngLevel=value;
        else {
            switch (CmdPeekChar()) {
            case 'e': CompFlags.warnings_cause_bad_exit=1; CmdScanChar(); break;
            case 'o': CompFlags.wo_switch_used=1; CmdLnWarnDebug(); break;
            case 'x': WngLevel=9; CmdScanChar(); break;
            }
        }
        break;
    case 'x':
        c=tolower((unsigned char)CmdScanChar());
        switch(c) {
        case 'c':
            if(scanTwoChars('v','a')) CompFlags.xcva_switch_used=1;
            else if(scanTwoChars('m','b')) CompFlags.xcmb_switch_used=1;
            else BadCmdLine(68);
            break;
        case 'd':
            CompFlags.excs_enabled=0;
            if(CmdRecogChar('t')) DestructorMethod=0;
            else if(CmdRecogChar('s')) DestructorMethod=2;
            else DestructorMethod=0;
            break;
        case 'g':
            if(CmdRecogChar('v')) TargetSwitches |= 0x1000;
            else BadCmdLine(68);
            break;
        case 's':
            CompFlags.excs_enabled=1;
            if(CmdRecogChar('t')) DestructorMethod=1;
            else if(CmdRecogChar('s')) DestructorMethod=4;
            else DestructorMethod=3;
            break;
        default: BadCmdLine(68); break;
        }
        break;
    case 'z':
        c=CmdScanChar();
        if(isdigit((unsigned char)c)) { ZOptionNumber=c-'0'; break; }
        switch(tolower((unsigned char)c)) {
        case 'a':
            CompFlags.extensions_enabled=0;
            CompFlags.za_switch_used=1;
            TargetSwitches &= ~1u;
            break;
        case 'c':
            CompFlags.strings_in_code=1;
            CompFlags.zc_switch_used=1;
            TargetSwitches |= 0x10000;
            break;
        case 'd':
            CompFlags.zd_switch_used=1;
            switch(CmdScanLowerChar()) {
            case 'f': TargetSwitches |= 0x400; break;
            case 'p': TargetSwitches &= ~0x400u; break;
            case 'l': TargetSwitches |= 0x40000; break;
            default: BadCmdLine(68); break;
            }
            break;
        case 'e': CompFlags.extensions_enabled=1; break;
        case 'f':
            CompFlags.zf_switch_used=1;
            switch(CmdScanLowerChar()) {
            case 'f': TargetSwitches |= 0x40; break;
            case 'p': TargetSwitches &= ~0x40u; break;
            default: BadCmdLine(68); break;
            }
            break;
        case 'g':
            switch(CmdScanLowerChar()) {
            case 'f': TargetSwitches |= 0x80; break;
            case 'p': TargetSwitches &= ~0x80u; break;
            default: CmdScanUngetChar(); CompFlags.use_base_types=1; break;
            }
            break;
        case 'i': CompFlags.extra_stats_wanted=1; break;
        case 'k':
            CompFlags.use_unicode=0;
            c=CmdScanLowerChar();
            if(c=='u') { CompFlags.use_unicode=1; loadUnicodeTable(); }
            else if(c>='0' && c<='2') {
                SetDBChar(c-'0');
                if(c=='0' && CmdRecogChar('u')) CompFlags.jis_to_unicode=1;
            } else { CmdScanUngetChar(); SetDBChar(0); }
            break;
        case 'l':
            if(CmdScanLowerChar()=='d') CompFlags.emit_dependencies=0;
            else { CmdScanUngetChar(); CompFlags.emit_library_names=0; }
            break;
        case 'm': CompFlags.zm_switch_used=1; break;
        case 'p':
            c=CmdScanChar();
            if(isdigit((unsigned char)c)) {
                PackAmount=c-'0';
                c=CmdScanChar();
                if(isdigit((unsigned char)c)) PackAmount=PackAmount*10+c-'0';
                else CmdScanUngetChar();
                PackAmount=VerifyPackAmount(PackAmount);
            } else CmdScanUngetChar();
            break;
        case 'q': CompFlags.quiet_mode=1; break;
        case 's': CompFlags.check_syntax=1; break;
        case 't': scanDataThreshold(); break;
        case 'u': CompFlags.zu_switch_used=1; TargetSwitches |= 0x200; break;
        case 'v': CompFlags.zv_switch_used=1; break;
        case 'w': CompFlags.zw_switch_used=1; setTarget("windows",7); break;
        case 'x': FloatSaveOverride=1; break;
        case 'z': CompFlags.zz_switch_used=1; break;
        default: BadCmdLine(68); break;
        }
        break;
    default: BadCmdLine(68); break;
    }
}

void CmdLnWarnDebug(void) {  STUB_TRAP(); }
MEPTR DefineCmdLineMacro(int extended)
{
    MEPTR entry;
    int (*old_scanner)(void);
    old_scanner=NextChar;
    NextChar=SrcFileCmdLnGetChar;
    SrcFileCmdLnDummyOpen();
    CurrChar=NextChar();
    PPState=2; /*1994 PPS_EOL only; do not import the later NO_EXPAND bit. */
    entry=MacroScan(extended ? 3 : 2);
    SrcFileCmdLnDummyClose();
    PPState=0;
    NextChar=old_scanner;
    return entry;
}
void undefineMacro(void) {  STUB_TRAP(); }
unsigned VerifyPackAmount(unsigned pack) { (void)pack; STUB_TRAP(); return 0; }
void scanNumber(int *value) { (void)value; STUB_TRAP(); }
void getNumber(unsigned *value, unsigned fallback) { (void)value; (void)fallback; STUB_TRAP(); }
void scanIncludeOption(void) {  STUB_TRAP(); }
void scanFileName(char **name, int quoted) { (void)name; (void)quoted; STUB_TRAP(); }
void scanSegmentOption(void) {  STUB_TRAP(); }
void scanMemoryModel(void)
{
    setMemoryModelOption(CmdScanLowerChar());
}

void setMemoryModelOption(int c)
{
    unsigned model = 0;
    switch ((unsigned char)c) {
    case 'c': model=0x14; break; /* BIG_DATA|CHEAP_POINTER */
    case 'f': model=0x20; break; /* FLAT_MODEL */
    case 'l': model=0x1c; break; /* BIG_DATA|BIG_CODE|CHEAP_POINTER */
    case 'm': model=0x18; break; /* BIG_CODE|CHEAP_POINTER */
    case 's': model=0x10; break; /* CHEAP_POINTER */
    default: BadCmdLine(70); break;
    }
    model |= 0x80;             /* every386 model floats GS */
    if (!(model & 0x20)) model |= 0x100;
    if (model & 4) model |= 0x400;
    if (CompFlags.zd_switch_used || CompFlags.zw_switch_used) model &= ~0x400u;
    else TargetSwitches &= ~0x400u;
    if (CompFlags.zf_switch_used) model &= ~0x40u;
    else TargetSwitches &= ~0x40u;
    TargetSwitches &= ~0x13cu;
    TargetSwitches |= model;
}
void scanOptimization(void)
{
    int value;
    unsigned char *flags=(unsigned char *)&CompFlags;
    do {
        switch (CmdScanLowerChar()) {
        case 'a': GenSwitches |= 0x04000000u; break;
        case 'c': TargetSwitches |= 0x00008000u; break;
        case 'd': GenSwitches |= 0x80000000u; break;
        case 'e':
            CmdScanRecogEquals();
            scanNumber(&value);
            if (value==-1) value=100;
            CgBackSetOeSize(value);
            break;
        case 'f':
            TargetSwitches |= 0x00020000u;
            if (CmdRecogChar('+')) DefaultInfo._class |= 1;
            break;
        case 'i': flags[0] |= 0x20; break; /* inline_intrinsics */
        case 'l':
            GenSwitches |= 0x00400000u;
            if (CmdRecogChar('+')) GenSwitches |= 0x00800000u;
            break;
        case 'm': TargetSwitches |= 1; break;
        case 'n': GenSwitches |= 0x00080000u; break;
        case 'o': GenSwitches &= ~0x00100000u; break;
        case 'p': flags[7] |= 4; break;
        case 'r': GenSwitches |= 0x00200000u; break;
        case 's': OptSize=100; GenSwitches &= ~0x80000000u; break;
        case 't': OptSize=0; GenSwitches &= ~0x80000000u; break;
        case 'u': CompFlags.za_switch_used=1; break;
        case 'x':
            PragToggle &= ~1u;
            GenSwitches &= 0x7f9ffffeu;
            flags[0] |= 0x20;
            GenSwitches |= 0x00600001u;
            break;
        case 'z': GenSwitches |= 0x00040000u; break;
        default: BadCmdLine(69); return;
        }
    } while (!CmdDelimitChar());
}
void scanDataThreshold(void) {  STUB_TRAP(); }
void scanFOption(void)
{
    const char *start;
    char *name;
    unsigned length;
    int option, next;
    option=CmdScanLowerChar();
    next=CmdPeekChar();
    if (option=='h' && next=='q') {
        CmdScanLowerChar();
        CompFlags.no_pch_warnings=1;
        return;
    }
    CmdScanRecogEquals();
    length=CmdScanFilename(&start);
    name=NULL;
    if (length) name=vctsave(start,length);
    switch (option) {
    case 'h':
        if (ForceInclude==NULL) {
            if (name) PCHSetFileName(name);
            CompFlags.use_pcheaders=1;
        }
        break;
    case 'i':
        if (name) {
            PCHSetFileName(NULL);
            CompFlags.use_pcheaders=0;
            ForceInclude=name;
        }
        break;
    default:
        if (name) ObjectFileName=name;
        CompFlags.cpp_output_to_file=1;
        break;
    }
}
void scanPreprocess(void) {  STUB_TRAP(); }
void scanFpuSetup(void) {  STUB_TRAP(); }
void scanRegConvention(void)
{
    int c;
    c=CmdScanLowerChar();
    if (c=='r') CompFlags.register_conventions=1;
    else if (c=='s') CompFlags.register_conventions=0;
    else CmdScanUngetChar();
}
int scan87Suffix(void) {  STUB_TRAP(); return 0; }
int scanTwoChars(int first, int second) { (void)first; (void)second; STUB_TRAP(); return 0; }
void loadUnicodeTable(void) {  STUB_TRAP(); }

void PTypeSignedChar(void) {  STUB_TRAP(); }
void SetDBChar(int language) { (void)language; STUB_TRAP(); }

void scanTarget(void)
{
    const char *name;
    unsigned length;
    CmdScanRecogEquals();
    CmdScanChar();
    length = CmdScanId(&name);
    if (name != NULL && length != 0) setTarget(name,length);
}

void setTarget(const char *name, unsigned length)
{
    char *target;
    if (target_name != NULL) CMemFree(target_name);
    target = target_name = (char *)CMemAlloc(length+1);
    while (length) {
        *target++ = (char)toupper((unsigned char)*name++);
        --length;
    }
    *target = '\0';
}

void CgBackSetOeSize(unsigned value) { (void)value; STUB_TRAP(); }

void PCHSetFileName(char *name) { (void)name; STUB_TRAP(); }
