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


/* The 1994 cpragma initializer has five aux copies, before OptlinkInfo.
 * Unlike the later source it clears six packed pragma-state bits rather
 * than assigning the later PackInfo/EnumInfo list pointers. */
#include <string.h>
#include "pragma.h"
#include "scanner.h"
#include "files.h"
#include "macros.h"
#include "declarations.h"
#include <recon.h>

void PragInit(void)
{
    CdeclInfo=DefaultInfo;
    PascalInfo=DefaultInfo;
    FortranInfo=DefaultInfo;
    SyscallInfo=DefaultInfo;
    CompFlags.pragma_state=0;
    CdeclInfo.use=2;
    PascalInfo.use=2;
    FortranInfo.use=2;
    SyscallInfo.use=2;
    StdcallInfo=DefaultInfo;
    DefaultInfo.use=2;
}

/* The surviving released declaration names this hook; the1994 binary's
 * actual complete body is a shared C3. It contributes no independent logic. */
void ChkPragmas(void) {}

void CPragma(void)
{
    if (CompFlags.cpp_output) {
        if (!CppPrinting()) return;
        CppPrtf("#pragma");
        CompFlags.pre_processing=1;
        for (;;) {
            GetNextToken();
            if (CurToken==T_NULL) break;
            PrtToken();
        }
    } else {
        NextToken();
        if (PragRecog("on")) PragFlag(1);
        else if (PragRecog("off")) PragFlag(0);
        else if (PragRecog("aux") || PragRecog("linkage")) PragAux();
        else if (PragRecog("library")) PragLibs();
        else if (PragRecog("pack")) PragPack();
        else if (PragRecog("alloc_text")) PragAllocText();
        else if (PragRecog("code_seg")) PragCodeSeg();
        else if (PragRecog("data_seg")) PragDataSeg();
        else if (PragRecog("disable_message")) PragEnableDisableMessage(0);
        else if (PragRecog("enable_message")) PragEnableDisableMessage(1);
        else if (PragRecog("intrinsic")) PragIntrinsic(1);
        else if (PragRecog("function")) PragIntrinsic(0);
        else if (PragRecog("unroll")) PragUnroll();
        else return;
        EndOfPragma();
    }
}

void EndOfPragma(void)
{
    if (CurToken==5) NextToken();
    if (CurToken!=T_NULL) Expecting(Tokens[T_NULL]);
    while (CurToken!=T_NULL && CurToken!=1) NextToken();
}

int PragRecog(char *name)
{
    char *p;
    if (!(CurToken==T_ID || (CurToken>=54 && CurToken<101))) return 0;
    p=Buffer;
    if (*p=='_') ++p;
    if (*p=='_') ++p;
    if (stricmp(name,p)==0) {
        NextToken();
        return 1;
    }
    return 0;
}

/* This predates pack(push/pop); the numeric parser was still in this body. */
void PragPack(void)
{
    if (CurToken!=6) return;
    CompFlags.pre_processing=1;
    NextToken();
    CompFlags.pre_processing=2;
    if (CurToken==7) {
        NextToken();
        if (CurToken==5) NextToken();
        PackAmount=GblPackAmount;
        return;
    }
    if (CurToken==T_CONSTANT) {
        PackAmount=Constant;
        switch (PackAmount) {
        case 1: case 2: case 4: case 8: case 16: break;
        default: PackAmount=1;
        }
    } else CErr1(0x41e);
    NextToken();
    MustRecog(7);
}

struct aux_entry *AuxList;
void FreeInfo(struct aux_info *info)
{
    if (info->code!=NULL) {
        CMemFree(info->code);
        info->code=NULL;
    }
    if (info->parms!=NULL && info->parms!=DefaultParms) {
        CMemFree(info->parms);
        info->parms=NULL;
    }
    if (info->objname!=NULL) {
        CMemFree(info->objname);
        info->objname=NULL;
    }
}
/* caux.c before OptlinkInfo: the actual finalizer releases these five
 * standard descriptors, deliberately excluding StdcallInfo/Far16 copies. */
void PragmaFini(void)
{
    struct aux_entry *entry,*next;
    struct library_list *library;
    entry=AuxList;
    while (entry!=NULL) {
        next=entry->next;
        if (entry->u.info!=NULL) {
            if (entry->u.info->use!=1) --entry->u.info->use;
            else {
                FreeInfo(entry->u.info);
                if (entry->u.info!=&DefaultInfo) CMemFree(entry->u.info);
            }
        }
        CMemFree(entry);
        entry=next;
    }
    FreeInfo(&DefaultInfo);
    FreeInfo(&CdeclInfo);
    FreeInfo(&PascalInfo);
    FreeInfo(&SyscallInfo);
    FreeInfo(&FortranInfo);
    AuxList=NULL;
    while ((library=HeadLibs)!=NULL) {
        HeadLibs=library->next;
        CMemFree(library);
    }
}

struct aux_entry *AuxLookup(char *name)
{
    struct aux_entry *entry;
    for (entry=AuxList;entry!=NULL;entry=entry->next)
        if (strcmp(entry->name,name)==0) break;
    return entry;
}
