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


/* Actual1994 language lookup predates the later TypeModFlags/FAR16
 * extension. DefaultInfo and attached AUX_INFO remain genuine owners. */
#include <direct.h>
#include <stdlib.h>
#include <string.h>
#include "symbol.h"
#include "compiler.h"
#include "cmdline.h"
#include "frontend.h"
#include "srcfile.h"
#include "extrf.h"
#include "cpragma.h"
#include "pending.h"
#include <recon.h>
TYPE FunctionDeclarationType(TYPE type);
AUX_INFO *TypeHasPragma(TYPE type);
AUX_INFO *IntrinsicAuxLookup(SYMBOL sym)
{ (void)sym; STUB_TRAP(); return NULL; }
AUX_INFO *getLangInfo(SYMBOL sym)
{
    TYPE type;
    AUX_INFO *info;
    if(!sym) return &DefaultInfo;
    type=FunctionDeclarationType(sym->sym_type);
    if(type) {
        if(type->u.f.pragma) return type->u.f.pragma;
        if((type->flag&0x100) && sym->name) return IntrinsicAuxLookup(sym);
    } else {
        info=TypeHasPragma(sym->sym_type);
        if(info) return info;
    }
    return &DefaultInfo;
}
int IsPragmaAborts(SYMBOL sym)
{ return getLangInfo(sym)->_class&2; }

/* Old aux_class values and direct switch. The1994 body predates the later
 * addDefaultLibs/addDefaultImports list-building helpers. Its SOURCE_NAME
 * tests pointer identity, not the released strcmp/primary-source variant. */
enum {
    AUX_LOOKUP=0,CALL_BYTES,CALL_CLASS,FREE_SEGMENT,OBJECT_FILE_NAME,
    PARM_REGS,RETURN_REG,REVISION_NUMBER,SAVE_REGS,
    SOURCE_NAME=10,TEMP_LOC_NAME,TEMP_LOC_TELL,NEXT_DEPENDENCY,
    DEPENDENCY_TIMESTAMP,DEPENDENCY_NAME,SOURCE_LANGUAGE,
    DEFAULT_IMPORT_RESOLVE,
    CLASS_NAME=29,CODE_GROUP=30,DATA_GROUP=32,IMPORT_NAME,
    LIBRARY_NAME,NEXT_IMPORT,NEXT_LIBRARY,STRETURN_REG,USED_8087,
    OBJECT_NAME,STACK_SIZE_8087,CODE_LABEL_ALIGNMENT,PROEPI_DATA_SIZE,
    IMPORT_TYPE,CONDITIONAL_IMPORT,NEXT_CONDITIONAL,CONDITIONAL_SYMBOL,
    VIRT_FUNC_REFERENCE,VIRT_FUNC_NEXT_REFERENCE,
    PEGGED_REGISTER=50,CLASS_APPENDED_NAME
};
struct library_list { struct library_list *next; char name[1]; };
struct library_list *HeadLibs;
static char *auxBuffer;
static unsigned char codeAlignment[]={2,1,1};
static SYMBOL dtor_sym;
static EXTRF res_info;
static unsigned save_set,curr_call_class;
static unsigned DefaultVarParms[]={0};
extern char Buffer[];
extern unsigned ProEpiDataSize;
void CMemFreePtr(void **owner);
char *IoSuppOutFileName(unsigned kind);
char *RunTimeCodeString(unsigned code);
SYMBOL RunTimeCallSymbol(unsigned code);
int ExceptionsEnabled(void);
TYPE TypedefModifierRemove(TYPE type);
int SymIsEllipsisFunc(SYMBOL sym);
AUX_INFO *getCallAuxInfo(SYMBOL sym)
{ (void)sym; STUB_TRAP(); return NULL; }
/* Original helper writes the shared result cell; its return is discarded. */
void getCallClass(SYMBOL sym) { (void)sym; STUB_TRAP(); }
char *objectName(SYMBOL sym) { (void)sym; STUB_TRAP(); return NULL; }
unsigned *makeDOSTimeStamp(SRCFILE file)
{ (void)file; STUB_TRAP(); return NULL; }
SRCFILE SrcFileWalkInit(void);
SRCFILE SrcFileWalkNext(SRCFILE file);
char *SrcFileFullName(SRCFILE file);
void CgBackDtorAutoOffset(SYMBOL sym,unsigned offset);
unsigned SegmentBoundReg(unsigned short segment);
char *CppClassPathDebug(SYMBOL sym);
void *FEAuxInfo(SYMBOL sym,unsigned char request)
{
    unsigned index=(unsigned)sym;
    AUX_INFO *info;
    TYPE type;
    struct library_list *lib;
    if (auxBuffer) CMemFreePtr((void **)&auxBuffer);
    switch (request) {
    case SOURCE_LANGUAGE: return "CPP";
    case STACK_SIZE_8087: return (void *)(unsigned)Stack87;
    case CODE_GROUP: return GenCodeGroup;
    case DATA_GROUP: return DataSegName;
    case OBJECT_FILE_NAME: return IoSuppOutFileName(0);
    case REVISION_NUMBER: return (void *)9;
    case AUX_LOOKUP: case CONDITIONAL_SYMBOL: return sym;
    case PROEPI_DATA_SIZE: return (void *)ProEpiDataSize;
    case CODE_LABEL_ALIGNMENT:
        if (!OptSize) codeAlignment[1]=4;
        return codeAlignment;
    case CLASS_NAME: return index==1 ? CodeClassName : NULL;
    case USED_8087: CompFlags.pgm_used_8087=1; return NULL;
    case SOURCE_NAME:
        if (SrcFName==ModuleName) return _fullpath(Buffer,WholeFName,528);
        return ModuleName;
    case CALL_CLASS: getCallClass(sym);return &curr_call_class;
    case FREE_SEGMENT: return NULL;
    case NEXT_LIBRARY:
        switch (index) {
        case 0:
            if (!CompFlags.emit_library_names) return HeadLibs;
            if (CompFlags.has_main || CompFlags.has_winmain ||
                CompFlags.bd_switch_used || CompFlags.has_libmain ||
                CompFlags.extern_C_defn_found || CompFlags.pragma_library)
                return (void *)1;
            return (void *)3;
        case 1: return (void *)2;
        case 2: return (void *)3;
        case 3: return EmuLib_Name ? (void *)4 : HeadLibs;
        case 4: return HeadLibs;
        default: return ((struct library_list *)sym)->next;
        }
    case LIBRARY_NAME:
        switch (index) {
        case 1: return CLIB_Name;
        case 2:
            if (ExceptionsEnabled()) memcpy(WCPPLIB_Name+1,"plbx",4);
            return WCPPLIB_Name;
        case 3: return MATHLIB_Name;
        case 4: return EmuLib_Name;
        default: lib=(struct library_list *)sym;return lib->name;
        }
    case NEXT_IMPORT:
        switch (index) {
        case 0:
            if (CompFlags.has_main || CompFlags.has_winmain || CompFlags.bd_switch_used)
                return (void *)1;
            /* falls through */
        case 1:
            if (CompFlags.emit_library_names && CompFlags.float_used) return (void *)2;
            /* falls through */
        case 2: case 3:
            if ((CompFlags.pgm_used_8087 || CompFlags.float_used) &&
                CompFlags.emit_library_names && (CpuSwitches&0x80)) return (void *)4;
            /* falls through */
        case 4:
            if ((CompFlags.pgm_used_8087 || CompFlags.float_used) &&
                CompFlags.emit_library_names && (CpuSwitches&0xf0)) return (void *)5;
            /* falls through */
        case 5: if (CompFlags.main_has_parms) return (void *)6;
            /* falls through */
        case 6: if (CompFlags.bw_switch_used) return (void *)7;
            /* falls through */
        case 7:
            /* Old file/function-scope DTOR bits select module_dtor_ref. */
            if (CompFlags.reserved8b) return (void *)8;
            /* falls through */
        case 8:
            /* Byte9 bit4 selects stab_init_fun (old RTF_INLINE_FREG). */
            if (CompFlags.reserved9b&1) return (void *)9;
            /* falls through */
        case 9:
            /* Byte9 bit5 selects init_longjmp. */
            if (CompFlags.reserved9b&2) return (void *)10;
            /* falls through */
        case 10:
            if (CompFlags.has_main && TargetSystem==6) return (void *)11;
        }
        return NULL;
    case IMPORT_NAME:
        switch (index) {
        case 1:
            if (CompFlags.bd_switch_used) return "__DLLstart_";
            if (CompFlags.has_winmain) return "_wstart_";
            return "_cstart_";
        case 2: return "_fltused_";
        case 3: return NULL;
        case 4: return "__init_387_emulator";
        case 5: return Stack87==4 ? "__old_8087" : "__8087";
        case 6: return CompFlags.register_conventions ? "__argc" : "_argc";
        case 7: return "__init_default_win";
        case 8: return RunTimeCodeString(16);
        case 9: return RunTimeCodeString(52);
        case 10: return RunTimeCodeString(53);
        case 11: return "__WATCOM_Prelude";
        default:
            /* Original defect: out-of-range import handles return an
             * uninitialized EBX value. Deterministically zero that result. */
            return NULL;
        }
    case SAVE_REGS:
        info=getLangInfo(sym);
        save_set=info->save;
        if (sym) {
            type=TypedefModifierRemove(sym->sym_type);
            /* Original defect: compares the TYPE address with the tag19,
             * rather than its id byte. Ordinary TYPE owners skip this test. */
            if ((unsigned)type==19 && (type->flag&2)) save_set|=0xc000f000UL;
        }
        return &save_set;
    case RETURN_REG: return &getLangInfo(sym)->returns;
    case CALL_BYTES: return getCallAuxInfo(sym)->code;
    case STRETURN_REG: return &getLangInfo(sym)->streturn;
    case PARM_REGS:
        if (SymIsEllipsisFunc(sym)) return DefaultVarParms;
        return getLangInfo(sym)->parms;
    case OBJECT_NAME: return objectName(sym);
    case NEXT_DEPENDENCY:
        if (!CompFlags.emit_dependencies) return NULL;
        return sym ? SrcFileWalkNext((SRCFILE)sym) : SrcFileWalkInit();
    case DEPENDENCY_TIMESTAMP: return makeDOSTimeStamp((SRCFILE)sym);
    case DEPENDENCY_NAME: return SrcFileFullName((SRCFILE)sym);
    case TEMP_LOC_NAME: dtor_sym=sym;return (void *)2;
    case TEMP_LOC_TELL:
        CgBackDtorAutoOffset(dtor_sym,(unsigned)sym);
        /* Original leaves this notification's return value uninitialized. */
        return NULL;
    case DEFAULT_IMPORT_RESOLVE: return ExtrefResolve(sym,&res_info);
    case IMPORT_TYPE: return ExtrefImportType(&res_info);
    case CONDITIONAL_IMPORT: case NEXT_CONDITIONAL: return ExtrefVirtualSymbol(&res_info);
    case VIRT_FUNC_REFERENCE: return ExtrefVfunInfo(sym);
    case VIRT_FUNC_NEXT_REFERENCE: return ExtrefNextVfunSym(sym);
    case PEGGED_REGISTER: return (void *)SegmentBoundReg((unsigned short)index);
    case CLASS_APPENDED_NAME: return CppClassPathDebug(sym);
    default: return NULL;
    }
}
