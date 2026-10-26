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
* Description: Original-source attribution retained for the adapted entry.
*
****************************************************************************/

#ifndef BREAKOUT_H
#define BREAKOUT_H
#include "asmparse.h"
#include <recon.h>
/* CF2678c..26796, packed10-byte definition state. */
#pragma pack(1)
typedef struct a_definition_struct {
    unsigned short struct_depth;
    struct stacknode *struct_stack;
    dir_node *curr_struct;
} a_definition_struct;
#pragma pack()
typedef char definition_size_check[sizeof(a_definition_struct) == 10 ? 1 : -1];
extern a_definition_struct Definition;
int cpu_directive(unsigned short token);
int PubDef(int index);
int conditional_assembly_directive(int index);
int conditional_error_directive(int index);
int StructDef(int index);
int SegDef(int index);
int SetCurrSeg(int index);
int GrpDef(int index);
int ProcDef(int index);
int ProcEnd(int index);
int SimSeg(int index);
void AsmWarn(int level, int message, ...);
int AddAlias(int index);
int ExtDef(int index);
int CommDef(int index);
int GlobalDef(int index);
int Model(int index);
int Startup(int index);
int Include(int index);
int IncludeLib(int index);
int SetAssume(int index);
int ModuleEnd(int count);
int DefineConstant(int index, unsigned char redefine, unsigned char equate);
int MacroDef(int index, unsigned char hidden);
int LocalDef(int index);
int NameDirective(int index);
int LabelDirective(int index);
int OrgDirective(int index);
int AlignDirective(unsigned short token, int index);
int ForDirective(int index, unsigned char character);
#endif
