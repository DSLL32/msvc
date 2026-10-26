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
* Description:  Front-end auxiliary information for code generation.
*
****************************************************************************/


#include <stdlib.h>
#include <string.h>
#include "compiler.h"
#include "options.h"
#include "symbols.h"
#include "pragma.h"
#include "scanner.h"
#include "macros.h"
#include "files.h"
#include "codegen.h"
#include "stubs.h"
#include <recon.h>

static unsigned char Alignment[]={2,1,1};
static hw_reg_set TryParms[]={HW_EAX,0};
static hw_reg_set DefaultVarParms[]={0};
static hw_reg_set SaveSet;
unsigned CallClass;
void *FunctionProfileBlock;

/* The old request numbers and inlined library/import iterators precede the
 * later separate NextLibrary/NextImport entry points in cfeinfo.c. */
void *FEAuxInfo(void *symbol,unsigned char request)
{
    unsigned index;
    SYM_HANDLE handle;
    SYM_ENTRY sym;
    struct aux_info *info;
    struct library_list *library;
    struct fname_list *file;
    handle=(SYM_HANDLE)(unsigned)symbol;
    switch (request) {
    case 0: return (void *)(unsigned)handle;
    case 2: GetCallClass(handle);return &CallClass;
    case 3: case 11: case 12: return NULL;
    case 4: return ObjFileName(".obj");
    case 7: return (void *)9;
    case 10:
        if (SrcFName==ModuleName) return _fullpath(Buffer,WholeFName,BUF_SIZE);
        return ModuleName;
    case 13:
        if (!CompFlags.emit_dependencies) return NULL;
        if (symbol==NULL) return FNames;
        return ((struct fname_list *)symbol)->next;
    case 14: return GetTimeStamp(symbol);
    case 15:
        file=symbol;
        return _fullpath(Buffer,file->name,BUF_SIZE);
    case 16: return "C";
    case 29: return SegClassName(handle);
    case 30: return GenCodeGroup;
    case 32: return DataSegName;
    case 34:
        switch (handle) {
        case 1: return CLIB_Name;
        case 2: return MATHLIB_Name;
        case 3: return EmuLib_Name;
        default: return ((struct library_list *)symbol)->name;
        }
    case 36:
        switch (handle) {
        case 0:
            if (!CompFlags.emit_library_with_main) return HeadLibs;
            if (CompFlags.has_main || CompFlags.has_winmain ||
                CompFlags.bd_switch_used || CompFlags.has_libmain) return (void *)1;
        case 1: return (void *)2;
        case 2: if (EmuLib_Name!=NULL) return (void *)3;
        case 3: case 4: return HeadLibs;
        default: library=symbol;return library->next;
        }
    case 35:
        index=handle;
        switch (index) {
        case 0:
            if (CompFlags.has_main || CompFlags.has_winmain || CompFlags.bd_switch_used)
                return (void *)1;
        case 1:
            if (CompFlags.emit_library_with_main && CompFlags.float_used) return (void *)2;
        case 2: case 3:
            if ((CompFlags.pgm_used_8087 || CompFlags.float_used) && (ProcRevision&0x80))
                return (void *)4;
        case 4:
            if ((CompFlags.pgm_used_8087 || CompFlags.float_used) && (ProcRevision&0xf0))
                return (void *)5;
        case 5: if (CompFlags.main_has_parms) return (void *)6;
        case 6: if (CompFlags.bw_switch_used) return (void *)7;
        case 7: if (TargSys==6) return (void *)8;
        case 8: if (TargetSwitches&0x200000) return (void *)9;
        }
        return NULL;
    case 33:
        switch (handle) {
        case 1:
            if (CompFlags.bd_switch_used) return "__DLLstart_";
            if (CompFlags.has_winmain) return "_wstart_";
            return "_cstart_";
        case 2: return "_fltused_";
        case 4: return "__init_387_emulator";
        case 5:
            if (CompFlags.emit_library_with_main && Stack87==4) return "__old_8087";
            return "__8087";
        case 6: return CompFlags.register_conventions ? "__argc":"_argc";
        case 7: return "__init_default_win";
        case 8: return "__WATCOM_Prelude";
        case 9: return "__p5_profile";
        }
        return NULL;
    case 38: CompFlags.pgm_used_8087=1;return NULL;
    case 40: return (void *)(unsigned)Stack87;
    case 41:
        if (OptSize==0) Alignment[1]=4;
        return Alignment;
    case 42: return (void *)ProEpiDataSize;
    case 50: return SegPeggedReg((unsigned)symbol);
    case 52: return FunctionProfileBlock;
    case 53: return (void *)FunctionProfileSegment;
    }
    info=FindInfo(&sym,handle);
    switch (request) {
    case 1: return info->code;
    case 8:
        if (handle!=0) info=LangInfo(sym.attrib,info);
        else sym.attrib=0;
        SaveSet=info->save;
        if (sym.attrib&0x100) SaveSet|=HW_DS|HW_ES|HW_FS|HW_GS|0xc000;
        if (handle==SymTryInit) SaveSet&=~0x800;
        return &SaveSet;
    case 6:
        if (handle!=0) info=LangInfo(sym.attrib,info);
        return &info->returns;
    case 5:
        if (handle==SymTryInit || handle==SymTryFini || handle==SymTryUnwind || handle==SymExcept)
            return TryParms;
        if (handle!=0) {
            info=LangInfo(sym.attrib,info);
            if (VarFunc(&sym)) return DefaultVarParms;
        }
        return info->parms;
    case 37:
        if (handle!=0) info=LangInfo(sym.attrib,info);
        return &info->streturn;
    case 39:
        if (handle==SymTryInit || handle==SymTryFini || handle==SymTryUnwind || handle==SymExcept)
            return "*";
        if (handle!=0) {
            info=LangInfo(sym.attrib,info);
            if (info->objname!=NULL) return info->objname;
            if (!(sym.flags&SYM_FUNCTION)) return "_*";
        }
        if (info->objname!=NULL) return info->objname;
        return "*_";
    }
    return NULL;
}

struct aux_info *LangInfo(unsigned flags,struct aux_info *info)
{
    if (info==&DefaultInfo) {
        switch (flags&0xe0) {
        case 0x20:info=&CdeclInfo;break;
        case 0x40:info=&PascalInfo;break;
        case 0x60:info=&FortranInfo;break;
        case 0x80:info=&SyscallInfo;break;
        case 0xa0:info=&StdcallInfo;break;
        }
    }
    return info;
}

void GetCallClass(unsigned short handle)
{
    struct aux_info *info;
    SYM_ENTRY sym;
    CallClass=DefaultInfo.class;
    /* Original defect: handle zero skips FindInfo but the common tail reads
     * sym.flags anyway. No stable meaning has been recovered for that domain. */
    if (handle==0) STUB_TRAP();
    info=FindInfo(&sym,handle);
    if (sym.flags&SYM_FUNCTION) {
        switch (sym.attrib&0xe0) {
        case 0x20:
            CallClass=info!=&DefaultInfo ? info->class:CdeclInfo.class;
            break;
        case 0x40:
            CallClass=info!=&DefaultInfo ? info->class:PascalInfo.class;
            break;
        case 0x60: CallClass=FortranInfo.class;break;
        case 0x80: CallClass=SyscallInfo.class;break;
        case 0xa0: CallClass=StdcallInfo.class;break;
        default: CallClass=info->class;break;
        }
        if (CompFlags.emit_names) CallClass|=0x02000000;
        if (sym.attrib&8) {
            CallClass|=0x1000;
            if (sym.attrib&4) CallClass|=0x2000;
        } else if (sym.attrib&4) {
            CallClass&=~0x1000;
        }
        if (sym.attrib&0x400) CallClass|=0x200;
        if (sym.attrib&0x200) CallClass|=0x8000;
    }
    if (VarFunc(&sym)) CallClass|=0x100;
    if (sym.flags&0x800) CallClass|=0x10000000;
    if (CompFlags.ep_switch_used) CallClass|=0x08000000;
    if (CompFlags.ee_switch_used) CallClass|=0x20000000;
    if (CompFlags.sg_switch_used) CallClass|=0x04000000;
    if (CompFlags.st_switch_used) CallClass|=0x80000000U;
}

/* The earlier cfeinfo.c searches the ordinary vararg-library names linearly;
 * the released version replaces this list with a perfect hash. */
static char *VarParmFuncs[]={
    "printf","fprintf","sprintf","cprintf",
    "scanf","fscanf","sscanf","cscanf",
    "execl","execle","execlp","execlpe",
    "spawnl","spawnle","spawnlp","spawnlpe","open","sopen",NULL
};
int VarFunc(SYMPTR sym)
{
    char **name;
    if (sym==NULL || !(sym->flags&SYM_FUNCTION)) return 0;
    for (name=VarParmFuncs;*name!=NULL;++name)
        if (strcmp(sym->name,*name)==0) return 1;
    return VarParm(sym);
}
