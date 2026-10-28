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


#include <string.h>
#include <stdio.h>
#include "compiler.h"
#include "command.h"
#include "options.h"
#include "codegen.h"
#include "symbols.h"
#include "pragma.h"
#include "genquads.h"
#include "stubs.h"
#include <recon.h>
#if defined(__WATCOMC__)
/* The original ten-DWORD aux_info assignment is an inline REP MOVSD. */
#pragma intrinsic(memcpy)
#endif

static struct aux_info InlineInfo;
/* These are backend byte_seq objects, not this reconstruction's code.
 * The source-generated Finally return and TryFini FS:0 store retain their
 * exact one-byte length header used by the period code generator. */
static struct { unsigned char length,code[1]; } FinallyCode={1,{0xc3}};
static struct { unsigned char length,code[6]; } TryFiniCode={6,{0x64,0xa3,0,0,0,0}};
static hw_reg_set TryFiniParms[]={HW_EAX,0};

struct aux_info *FindInfo(SYMPTR sym,SYM_HANDLE handle)
{
    struct aux_info *info;
    struct aux_entry *entry;
    SYM_ENTRY typedef_sym;
    TYPEPTR typ;
    info=&DefaultInfo;
    if (handle==0) return info;
    SymGet(sym,handle);
    if (handle==SymSTOSB || handle==SymSTOSD) return &STOSBInfo;
    if (handle==SymFinally) {
        InlineInfo=DefaultInfo;
        InlineInfo.code=&FinallyCode;
        return &InlineInfo;
    }
    if (handle==SymTryFini) {
        InlineInfo=DefaultInfo;
        InlineInfo.parms=TryFiniParms;
        InlineInfo.code=&TryFiniCode;
        return &InlineInfo;
    }
    if (!(sym->flags&0x20)) {
        info=InfoLookup(sym);
        if (info==&DefaultInfo) info=ModifyLookup(sym);
    }
    typ=sym->sym_type;
    if (info==&DefaultInfo && typ->decl_type==TYPE_TYPEDEF) {
        SymGet(&typedef_sym,typ->u.typedefn);
        if (typedef_sym.name!=NULL) {
            entry=AuxLookup(typedef_sym.name);
            if (entry!=NULL) info=entry->u.info;
        }
    }
    if ((info->flags&1) || (sym->attrib&0x2000)) {
        if ((sym->attrib&0xe0)==0x40 || (info->class&1)) return &Far16PascalInfo;
        return &Far16CdeclInfo;
    }
    return info;
}

int FunctionAborts(SYMPTR sym,SYM_HANDLE handle)
{
    struct aux_entry *entry;
    if (handle!=0) {
        SymGet(sym,handle);
        entry=AuxLookup(SymName(sym,handle));
        if (entry!=NULL && (entry->u.info->class&2)) return 1;
    }
    return 0;
}

struct aux_info *InfoLookup(SYMPTR sym)
{
    char *name;
    struct aux_entry *entry;
    struct inline_funcs *function;
    name=sym->name;
    if (name==NULL) return &DefaultInfo;
    entry=AuxLookup(name);
    if (entry!=NULL) return entry->u.info;
    if (sym->flags&8) return &DefaultInfo;
    if (!(sym->flags&0x4000)) {
        if (memcmp(name,"_inline_",8)!=0) return &DefaultInfo;
        name+=8;
    }
    function=IF_Lookup(name);
    if (function==NULL) return &DefaultInfo;
    if (function->returns==0xc3 || function->returns==0x1100 ||
        function->returns==0x2200 || function->returns==0x230) {
        if (SizeOfArg(sym->sym_type->object)!=4) return &DefaultInfo;
    }
    InlineInfo.class=(DefaultInfo.class&0x1000)|0x10000;
    InlineInfo.code=function->code;
    InlineInfo.parms=function->parms;
    InlineInfo.returns=function->returns;
    if (InlineInfo.returns!=3 && InlineInfo.returns!=0) InlineInfo.class|=0x400;
    InlineInfo.streturn=0;
    InlineInfo.save=function->save;
    InlineInfo.objname=DefaultInfo.objname;
    InlineInfo.use=1;
    return &InlineInfo;
}

/* This x86 build has no target-specific MODIFY lookup. */
struct aux_info *ModifyLookup(SYMPTR sym)
{
    (void)sym;
    return &DefaultInfo;
}

/* C has no nested procedure frames; this is the actual FE lexical hook. */
int FELexLevel(void *symbol)
{
    (void)symbol;
    return 0;
}

/* cinfo.c parameter promotion; the old interface uses an int type code. */
int FEParmType(void *function,void *parameter,int type)
{
    SYM_HANDLE handle;
    SYMPTR sym;
    (void)parameter;
    switch (type) {
    case T_UINT_1:case T_INT_1:case T_UINT_2:case T_INT_2:
        handle=(SYM_HANDLE)(unsigned)function;
        if (handle!=0) {
            sym=SymGetPtr(handle);
            if (sym->attrib&0x2000) return T_INT_2;
        }
        return T_INTEGER;
    default:
        return type;
    }
}

int FETrue(void)
{
    return 1;
}

int FEStackChk(void *symbol)
{
    SYMPTR sym;
    sym=SymGetPtr((SYM_HANDLE)(unsigned)symbol);
    return (sym->flags&0x40)!=0; /* SYM_CHECK_STACK */
}

/* cinfo.c FEMessage, restored to the actual1994 console host. Later source
 * routed NoteMsg/ConBlip through host callbacks and used a second message
 * buffer. Here ConPrintf owns the real variadic output, and putchar is the
 * SDK's inline stdout operation. The dispatcher consumes only AL. */
static SYM_HANDLE LastFuncOutOfMem;
void FEMessage(unsigned message,void *data)
{
    char text[80];
    switch ((unsigned char)message) {
    case 0:                         /* MSG_INFO_FILE */
    case 5:                         /* MSG_INFO_PROC */
        ConPrintf("\r%s\n",(char *)data);
        break;
    case 1:                         /* MSG_CODE_SIZE */
        if (!CompFlags.quiet_mode) {
            CGetMsg(text,0x7d4);
            ConPrintf("\r%s: %u\n",text,(unsigned)data);
        }
        break;
    case 2:                         /* MSG_DATA_SIZE */
    case 13:                        /* MSG_WANT_MORE_DATA */
        break;
    case 3:                         /* MSG_ERROR */
        CErr2p(0x443,(char *)data);
        break;
    case 4:                         /* MSG_FATAL */
        CErr2p(0x45e,(char *)data);
        CloseFiles();
        /* The actual old host takes its separate _exit callback here,
         * not the ordinary MyExit/exit finalizer route of later source. */
        ImmediateExit(1);
        break;
    case 6:                         /* MSG_BAD_PARM_REGISTER */
        CErr2p(0x460,(char *)data);
        break;
    case 7:                         /* MSG_BAD_RETURN_REGISTER */
        CErr2p(0x461,FEName(data));
        break;
    case 8:                         /* MSG_REGALLOC_DIED */
    case 9:                         /* MSG_SCOREBOARD_DIED */
    case 16:                        /* MSG_SCHEDULER_DIED */
        if (!(GenSwitches&0x80000000U) &&
            LastFuncOutOfMem!=(unsigned)data) {
            ConPrintf("\r");
            CInfoMsg(0x7d0,FEName(data));
            LastFuncOutOfMem=(SYM_HANDLE)(unsigned)data;
        }
        break;
    case 10:                        /* MSG_PEEPHOLE_FLUSHED */
        if (!(GenSwitches&0x80000000U) && WngLevel>=4 &&
            !CompFlags.low_on_memory_printed) {
            ConPrintf("\r");
            CInfoMsg(0x7d1);
            CompFlags.low_on_memory_printed=1;
        }
        break;
    case 11:                        /* MSG_BACK_END_ERROR */
        CErr2p(0x45f,(char *)data);
        break;
    case 12:                        /* MSG_BAD_SAVE */
        CErr2p(0x462,FEName(data));
        break;
    case 14:                        /* MSG_BLIP */
        if (!CompFlags.quiet_mode) {
            putchar('.');
            fflush(stdout);
        }
        break;
    case 15:                        /* MSG_BAD_LINKAGE */
        CErr2p(0x477,FEName(data));
        break;
    case 17:                        /* MSG_NO_SEG_REGS */
        CErr2p(0x488,FEName(data));
        break;
    case 18:                        /* MSG_BAD_PEG_REG */
        CErr2p(0x48a,FEName(data));
        break;
    }
}
