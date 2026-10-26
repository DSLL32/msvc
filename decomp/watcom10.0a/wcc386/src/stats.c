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


#include "compiler.h"
#include <recon.h>

int SrcLineCount,IncLineCount,WngCount;
int GblSymCount,LclSymCount,MacroCount;
unsigned MacroSize,QuadIndex,LeafIndex;
int EnumCount,TagCount,FieldCount,TypeCount,TmpSymCount;
int LitPoolSize,LitCount,ReclaimCount,AllocCount,FreeCount,FuncCount;

void InitStats(void)
{
    CompFlags.stats_printed=0;
    CompFlags.extra_stats_wanted=0;
    SrcLineCount=0;
    IncLineCount=0;
    WngCount=0;
    ErrCount=0;
    GblSymCount=0;
    LclSymCount=0;
    MacroCount=0;
    MacroSize=0;
    EnumCount=0;
    TagCount=0;
    FieldCount=0;
    QuadIndex=0;
    LeafIndex=0;
    TypeCount=0;
    TmpSymCount=0;
    LitPoolSize=0;
    LitCount=0;
    ReclaimCount=0;
    AllocCount=0;
    FreeCount=0;
    FuncCount=0;
}
