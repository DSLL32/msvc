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

/* Adapted from initial plusplus/c/cmdlnany.c, cmdlnx86.c and cmdln.c at
 * 6b9cb443. The actual1994 GenCOptions has direct option parsing and inline
 * post-analysis rather than the2002 generated OPT_STORAGE/context lifecycle. */
#include <stdlib.h>
#include <string.h>
#include "frontend.h"
#include "compiler.h"
#include "cpragma.h"
#include "cmdline.h"
#include "pending.h"
#include "utility.h"
#include "cmdln-options.h"
#include "macro.h"
#include <recon.h>

unsigned CpuSwitches, TargetSwitches, GenSwitches;
unsigned PackAmount, GblPackAmount;
/* Old parser's explicit x87-clobber override, consumed after stack setup. */
unsigned FloatSaveOverride;
unsigned DataThreshold, OptSize, PragToggle;
unsigned ErrLimit = 20, WngLevel;
char *ObjectFileName, *ModuleName;
unsigned char DestructorMethod, Stack87;
char *CodeClassName, *TextSegName, *DataSegName, *GenCodeGroup;
char *target_name;
unsigned TargetSystem;
unsigned DataPtrSize, CodePtrSize;
char CLIB_Name[8], WCPPLIB_Name[10], MATHLIB_Name[10];
char *EmuLib_Name;

void defineM_IX86Macro(char *buffer);
void setWindowsSystem(void);
void setTargetRegisterConventions(void);
int MergeIncludeFromEnv(char *name);
static RINGNAMECTL undef_names = { NULL, 1 };
/* Hardware ABCD parameter group plus the terminating empty register set. */
static unsigned DefaultParms[] = { 0x0f0000ff, 0 };

enum { FLOATING_FS=0x40, FLOATING_GS=0x80,
       FLOATING_ES=0x100, FLOATING_DS=0x400 };

void GenCOptions(char **argv)
{
    InitModInfo();
    CmdSysInit();
    CompFlags.cmd_file = 0;
    CtxSetContext(2);
    procOptions(getenv("WPP386"));
    CtxSetContext(3);
    while (*argv) procOptions(*argv++);
    CtxSetContext(4);
    if (CompFlags.cpp_output_requested) CompFlags.cpp_output = 1;
    if (CompFlags.cpp_output) CompFlags.quiet_mode = 1;
    CBanner();
    GblPackAmount = PackAmount;
    setFinalTargetSystem();
    setMemoryModel();
    if (!CompFlags.save_restore_segregs) {
        if (TargetSwitches & FLOATING_DS) DefaultInfo.save &= ~0x1000u;
        if (TargetSwitches & FLOATING_ES) DefaultInfo.save &= ~0x2000u;
        if (TargetSwitches & FLOATING_FS) DefaultInfo.save &= ~0x40000000u;
        if (TargetSwitches & FLOATING_GS) DefaultInfo.save &= ~0x80000000u;
    }
    if (CpuSwitches & 0xf0) PreDefineStringMacro("__FPI__");
    if (!CompFlags.register_conventions) setStackConventions();
    if (FloatSaveOverride) DefaultInfo.save &= ~0x00ff0000u;
    macroDefs();
    MiscMacroDefs();
}

void InitModInfo(void)
{
    unsigned char *flags;

    RingNameInit(&undef_names);
    PragToggle = 3;              /* check_stack and unreferenced */
    DataThreshold = 32767;
    OptSize = 50;
    WholeFName = NULL;
    ObjectFileName = NULL;
    ModuleName = NULL;
    ErrLimit = 20;
    WngLevel = 1;
    PackAmount = 1;              /* actual1994 default, before later8 */
    DefaultInfo._class = 0;
    DefaultInfo.code = NULL;
    DefaultInfo.parms = DefaultParms;
    DefaultInfo.returns = 0;
    DefaultInfo.streturn = 0;
    DefaultInfo.save = ~0u;
    DefaultInfo.use = 0;
    DefaultInfo.objname = NULL;
    DestructorMethod = 0;
    /* Retain the actual old flag-bit reset, including parser/runtime bits
     * whose names were changed or removed in the later released structure.
     * This preserves unrelated live flags such as banner_printed. */
    flags = (unsigned char *)&CompFlags;
    flags[0] &= 0xc5;
    flags[1] &= 0xfb;
    flags[2] &= 0xac;
    flags[3] &= 0xfb;
    flags[6] |= 0x80;
    flags[8] &= 0xf7;
    flags[9] &= 0xbb;
    flags[10] &= 0xf7;
    flags[0] |= 0x10;
    flags[2] |= 0x40;
    flags[4] &= 0xfe;
    flags[7] &= 0xd8;
    flags[10] |= 0x08;
}
void CmdSysInit(void)
{
    unsigned char *flags;

    GenSwitches = 0x00100000u;   /* MEMORY_LOW_FAILS */
    TargetSwitches = 0x8b0;     /* CHEAP_POINTER|USE_32|FLAT_MODEL|FLOATING_GS */
    /* Old CPU/FPU low byte: 486,387 and emulation. Preserve other flags. */
    CpuSwitches = (CpuSwitches & ~0xffu) | 0xa4;
    Stack87 = 8;
    CodeClassName = NULL;
    TextSegName = DataSegName = GenCodeGroup = "";
    target_name = NULL;
    flags = (unsigned char *)&CompFlags;
    flags[6] &= 0xef;
    flags[7] &= 0xf7;
    CompFlags.register_conventions = 1;
}
static unsigned indirectionLevel;

void procOptions(char *options)
{
    int c;
    const char *filename;
    char *environment;
    unsigned length;
    CMD_SCAN_STATE saved;

    if (indirectionLevel >= 5) {
        BadCmdLine(561);
    } else if (options != NULL) {
        ++indirectionLevel;
        CtxSwitchAddr(options);
        CmdScanInit(options);
        for (;;) {
            c = CmdScanWhiteSpace();
            if (!c) break;
            CmdScanSwitchBegin();
            if (c == '-' || c == SwitchChar) {
                processSwitch();
            } else if (c == '@') {
                CmdScanWhiteSpace();
                CmdScanUngetChar();
                length = CmdScanFilename(&filename);
                environment = get_env(filename, length);
                if (environment) {
                    saveCmdState(&saved);
                    CompFlags.cmd_file = 0;
                    procOptions(environment);
                    restoreCmdState(&saved);
                } else if (openCmdFile(filename, length)) {
                    saveCmdState(&saved);
                    CompFlags.cmd_file = 1;
                    processCmdFile();
                    restoreCmdState(&saved);
                } else {
                    BadCmdLine(560);
                }
            } else {
                CmdScanUngetChar();
                scanInputFile();
            }
        }
        --indirectionLevel;
    }
}
void setFinalTargetSystem(void)
{
    char buffer[128];
    TargetSystem=0; /* TS_OTHER */
    PreDefineStringMacro("__386__");
    PreDefineStringMacro("M_I386");
    defineM_IX86Macro(buffer);
    if (target_name==NULL) {
#if defined(__linux__) || defined(__OSI__)
        switch (__OS) {
        case 0: case 3: setTarget("dos",3); break;
        case 1: setTarget("os2",3); break;
        case 2: setTarget("nt",2); break;
        }
#else
        /* The ordinary Win32 reconstruction runs under the NT CRT. */
        setTarget("nt",2);
#endif
    }
    if (strcmp(target_name,"DOS")==0) {
        TargetSystem=1;
        PreDefineStringMacro("MSDOS");
    } else if (strcmp(target_name,"NETWARE")==0) {
        TargetSystem=6;
        DestructorMethod=4;
        PreDefineStringMacro("__NETWARE_386__");
        setMemoryModelOption('s');
        setTargetRegisterConventions();
    } else if (strcmp(target_name,"PENPOINT")==0) {
        TargetSystem=5;
        setTargetRegisterConventions();
    } else if (strcmp(target_name,"NT")==0) {
        TargetSystem=2;
    } else if (strcmp(target_name,"WINDOWS")==0) {
        TargetSystem=3;
        setWindowsSystem();
    } else if (strcmp(target_name,"CHEAP_WINDOWS")==0) {
        TargetSystem=3;
        strcpy(target_name,"WINDOWS");
        setWindowsSystem();
    } else {
        TargetSystem=0;
    }
    strcpy(buffer,"__");
    strcat(buffer,target_name);
    strcat(buffer,"__");
    PreDefineStringMacro(buffer);
    strcpy(buffer,target_name);
    strcat(buffer,"_INCLUDE");
    MergeIncludeFromEnv(buffer);
    MergeIncludeFromEnv("INCLUDE");
    CMemFree(target_name);
    target_name=NULL;
    CompFlags.target_multi_thread=CompFlags.bm_switch_used ||
        (CompFlags.bd_switch_used && TargetSystem!=3);
}

void PreDefineStringMacro(char *definition)
{
    char buffer[32], *to, *from;
    MEPTR entry;
    if (!CompFlags.undefine_all_macros) {
        entry=NULL;
        if (undef_names.ring==NULL) {
            entry=defineStringMacro(definition);
        } else {
            to=buffer;
            from=definition;
            for (;;) {
                if (*from=='=') { *to='\0'; break; }
                *to=*from;
                if (*to=='\0') break;
                ++to;
                ++from;
            }
            if (RingNameLookup(&undef_names,buffer)==NULL)
                entry=defineStringMacro(definition);
        }
        if (entry) MacroCanBeRedefined(entry);
    }
}
void setStackConventions(void) { STUB_TRAP(); }

/* Initial cmdln.c keyword table, stopping at _Pascal in this1994 build.
 * The later inline/fastcall/asm/emit aliases and ISO filtering are absent. */
void defineKeywordMacros(void)
{
    static char *mac_table[] = {
        "_far16=__far16", "_Far16=__far16", "near=__near", "far=__far",
        "huge=__huge", "cdecl=__cdecl", "pascal=__pascal",
        "fortran=__fortran", "interrupt=__interrupt", "_near=__near",
        "_far=__far", "_huge=__huge", "_cdecl=__cdecl", "_pascal=__pascal",
        "_fortran=__fortran", "_interrupt=__interrupt", "_export=__export",
        "_loadds=__loadds", "_saveregs=__saveregs", "_based=__based",
        "_self=__self", "_segname=__segname", "_segment=__segment",
        "_syscall=_Syscall", "__syscall=_Syscall", "_System=_Syscall",
        "_Cdecl=__cdecl", "_Pascal=__pascal", NULL
    };
    char **macro;
    for (macro=mac_table; *macro!=NULL; ++macro)
        PreDefineStringMacro(*macro);
}

void MiscMacroDefs(void)
{
    if (CompFlags.inline_intrinsics)
        defineStringMacro("__INLINE_FUNCTIONS__");
    if (!CompFlags.extensions_enabled)
        defineStringMacro("NO_EXT_KEYS");
    else
        defineKeywordMacros();
    if (CompFlags.signed_char) defineStringMacro("__CHAR_SIGNED__");
    PreDefineStringMacro("__WATCOMC__=1000");
    PreDefineStringMacro("__WATCOM_CPLUSPLUS__=1000");
    RingNameFree(&undef_names);
}

char *get_env(const char *name, unsigned length) { (void)name; (void)length; STUB_TRAP(); return 0; }
int openCmdFile(const char *name, unsigned length) { (void)name; (void)length; STUB_TRAP(); return 0; }
void saveCmdState(CMD_SCAN_STATE *state) { (void)state; STUB_TRAP(); }
void restoreCmdState(const CMD_SCAN_STATE *state) { (void)state; STUB_TRAP(); }
void processCmdFile(void) { STUB_TRAP(); }
void BadCmdLine(unsigned message) { (void)message; STUB_TRAP(); }
void scanInputFile(void)
{
    char filename[260];
    const char *name;
    unsigned length;
    length=CmdScanFilename(&name);
    if (WholeFName==NULL) {
        stvcpy(filename,name,length);
        WholeFName=FNameAdd(filename);
    } else {
        CErr1(108);
    }
}

void setWindowsSystem(void) { STUB_TRAP(); }
void setTargetRegisterConventions(void) { STUB_TRAP(); }
void HFileAppend(const char *filename, unsigned length)
{
    (void)filename; (void)length; STUB_TRAP();
}

int MergeIncludeFromEnv(char *name)
{
    char *value;
    value=getenv(name);
    if (value!=NULL) {
        HFileAppend(value,strlen(value));
        return 1;
    }
    return 0;
}
MEPTR defineStringMacro(char *definition)
{
    MEPTR entry;
    const char *save;
    save=CmdScanAddr();
    CmdScanInit(definition);
    entry=DefineCmdLineMacro(0);
    CmdScanInit(save);
    return entry;
}
/* Actual1994 exported helper is exactly one RET, unlike later flags. */
void MacroCanBeRedefined(MEPTR entry) { (void)entry; }
