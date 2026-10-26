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
* Description:  Emit the compiler's initialized data quads.
*
****************************************************************************/


#include "compiler.h"
#include "codegen.h"
#include "data.h"
#include "declarations.h"
#include "macros.h"
#include "scanner.h"
#include "cstring.h"
#include "genquads.h"
#include <recon.h>

void EmitDataQuads(void)
{
    DATA_QUAD *quad;
    if (StartDataQuadAccess()!=0) {
        for (;;) {
            quad=NextDataQuad();
            if (quad->opr==1) break;
            for (;;) {
                EmitDQuad(quad);
                if (!(quad->flags&0x80)) break;
                if (--quad->u.long_values[1]==0) break;
            }
        }
    }
}

/* Earlier cgendata.c emits software-FP text through ftoa/DGFloat. */
void EmitDQuad(DATA_QUAD *quad)
{
    cg_type data_type;
    int item_size;
    unsigned long amount;
    SYM_ENTRY sym;
    static int segment;
    static unsigned long size;
    if (quad->flags&8) { data_type=T_NEAR_POINTER;item_size=4; }
    else if (quad->flags&0x10) { data_type=T_LONG_POINTER;item_size=6; }
    else if (quad->flags&0x20) { data_type=T_CODE_PTR;item_size=4; }
    else { data_type=T_POINTER;item_size=4; }
    switch (quad->opr) {
    case 0x5d: /* static definition */
        SymGet(&sym,quad->u.var.sym_handle);
        segment=sym.u.var.segment;
        BESetSeg(segment);
        AlignIt(sym.sym_type);
        DGLabel(FEBack(quad->u.var.sym_handle));
        size=0;
        break;
    case 0x64:
        DGInteger(quad->u.long_values[0],T_UINT_1);
        ++size;
        if (quad->flags&2) { DGInteger(quad->u.long_values[1],T_UINT_1);++size; }
        break;
    case 0x4b:
        DGInteger(quad->u.long_values[0],T_UINT_2);
        size+=2;
        if (quad->flags&2) { DGInteger(quad->u.long_values[1],T_UINT_2);size+=2; }
        break;
    case 0x5e:
        DGInteger(quad->u.long_values[0],T_INTEGER);
        size+=4;
        if (quad->flags&2) { DGInteger(quad->u.long_values[1],T_INTEGER);size+=4; }
        break;
    case 0x5c:
        DGInteger(quad->u.long_values[0],T_UINT_4);
        size+=4;
        if (quad->flags&2) { DGInteger(quad->u.long_values[1],T_UINT_4);size+=4; }
        break;
    case 0x56:
        ftoa(quad->u.double_value,Buffer);
        DGFloat(Buffer,T_SINGLE);
        size+=4;
        break;
    case 0x50:
        ftoa(quad->u.double_value,Buffer);
        DGFloat(Buffer,T_DOUBLE);
        size+=8;
        break;
    case T_STRING:
        EmitStrPtr(quad->u.var.sym_handle,data_type);
        size+=item_size;
        break;
    case T_ID:
        if (quad->u.var.sym_handle==0) DGInteger(quad->u.var.offset,data_type);
        else DGFEPtr((void *)(unsigned)quad->u.var.sym_handle,data_type,quad->u.var.offset);
        size+=item_size;
        break;
    case 0x5a:
        size+=EmitBytes(quad->u.var.sym_handle);
        break;
    case T_CONSTANT:
        amount=quad->u.long_values[0];
        EmitZeros(amount);
        size+=amount;
        break;
    }
}

/* Earlier per-function pool uses serialized literal handles, before the later
 * direct-pointer CS_StringList implementation. */
void FreeCS_Strings(void)
{
    union { SYM_ENTRY raw;STRING_LITERAL literal; } entry;
    LEAF leaf;
    while (CStringList!=0) {
        ReadLiteralChunk(&entry,CStringList);
        LeafGet(&leaf,entry.literal.leaf_index);
        BEFiniBack((void *)leaf.u.string.back_handle);
        BEFreeBack((void *)leaf.u.string.back_handle);
        CStringList=entry.literal.next_string;
    }
}
