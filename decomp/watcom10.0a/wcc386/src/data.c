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
#include "quads.h"
#include "compiler.h"
#include "data.h"
#include "stubs.h"
#include "scanner.h"
#include "declarations.h"
#include "expression.h"
#include "options.h"
#include <recon.h>

DATA_QUAD *DataQuadSegs[641],*DataQuadPtr;
int DataQuadIndex,DataQuadSegIndex;
int LastDataQuadIndex,LastDataQuadSegIndex;
SYMPTR InitSym;

/* Source BitMask contains the low n bits for each meaningful width1..32. */
/* A genuine data public keeps its CODE-resident object separately bounded. */
const unsigned long BitMask[32]={
    0x1UL, 0x3UL, 0x7UL, 0xfUL,
    0x1fUL, 0x3fUL, 0x7fUL, 0xffUL,
    0x1ffUL, 0x3ffUL, 0x7ffUL, 0xfffUL,
    0x1fffUL, 0x3fffUL, 0x7fffUL, 0xffffUL,
    0x1ffffUL, 0x3ffffUL, 0x7ffffUL, 0xfffffUL,
    0x1fffffUL, 0x3fffffUL, 0x7fffffUL, 0xffffffUL,
    0x1ffffffUL, 0x3ffffffUL, 0x7ffffffUL, 0xfffffffUL,
    0x1fffffffUL, 0x3fffffffUL, 0x7fffffffUL, 0xffffffffUL,
};

FIELDPTR InitBitField(FIELDPTR field)
{
    TYPEPTR typ;
    unsigned long value,bit_value,offset,mask;
    int token;
    unsigned char int_type;

    token=CurToken;
    if (token==0x0a) NextToken();
    typ=field->field_type;
    switch (typ->u.f.field_type) {
    case TYPE_CHAR:
    case TYPE_UCHAR: int_type=0x64; break;
    case TYPE_SHORT:
    case TYPE_USHORT: int_type=0x4b; break;
    case TYPE_INT:
    case TYPE_UINT: int_type=0x5e; break;
    case TYPE_LONG:
    case TYPE_ULONG: int_type=0x5c; break;
    default: STUB_TRAP();
    }
    offset=field->offset;
    value=0;
    while (typ->decl_type==TYPE_FIELD || typ->decl_type==TYPE_UFIELD) {
        bit_value=0;
        if (CurToken!=0x0b) bit_value=BoolConstExpr();
        if (typ->u.f.field_width==0 || typ->u.f.field_width>32) STUB_TRAP();
        mask=BitMask[typ->u.f.field_width-1];
        ChkConstant(bit_value,mask);
        value|=(bit_value&mask)<<(typ->u.f.field_start&31);
        field=field->next_field;
        if (field==NULL || field->offset!=offset) break;
        typ=field->field_type;
        if (CurToken==1) break;
        if (CurToken!=0x0b) MustRecog(2);
    }
    StoreIValue(int_type,value);
    if (token==0x0a) {
        if (CurToken==2) NextToken();
        MustRecog(0x0b);
    }
    return field;
}

void StoreFloat(int token)
{
    DATA_QUAD dq;
    LEAF leaf;
    unsigned tree;

    dq.opr=token;
    dq.flags=4;
    dq.u.long_values[0]=dq.u.long_values[1]=0;
    if (CurToken!=0x0b) {
        tree=SingleExpr();
        LeafGet(&leaf,tree);
        switch (leaf.leaf_class) {
        case 0x0e: case 0x0f:
            dq.u.double_value=leaf.u.constant.value.floating;
            break;
        case 0x0a: case 0x0c:
            dq.u.double_value=(double)leaf.u.constant.value.integer;
            break;
        case 0x0b: case 0x0d:
            dq.u.double_value=(double)(unsigned)leaf.u.constant.value.integer;
            break;
        default:
            CErr1(0x41e);
        }
        FreeLeaf(tree);
        if (dq.u.long_values[1]!=0) CompFlags.non_zero_data=1;
    }
    GenDataQuad(&dq);
}

void SetFarHuge(SYMPTR sym,int initializing)
{
    unsigned long size;
    unsigned attrib;
    (void)initializing;
    size=SizeOfArg(sym->sym_type);
    if (TargetSwitches&4) {
        attrib=sym->attrib;
        if (!(attrib&0x1c)) {
            if (size==0) {
                if (sym->stg_class==0x61 && sym->sym_type->decl_type==TYPE_ARRAY) attrib|=8;
            } else if (size>DataThreshold ||
                (CompFlags.strings_in_code_segment && (sym->sym_type->decl_flags&1))) attrib|=8;
            sym->attrib=attrib;
        }
    }
}

void StorePointer(TYPEPTR typ,int token,unsigned maximum)
{
    DATA_QUAD dq;
    LEAF leaf;
    SYM_ENTRY sym;
    TYPEPTR object;
    unsigned tree,flags;
    int address_wanted;

    dq.flags=4;
    dq.u.long_values[0]=dq.u.long_values[1]=0;
    flags=CompFlags.strings_in_code_segment?1:0;
    if (typ->decl_type==TYPE_POINTER) {
        if (typ->decl_flags&0x18) {
            dq.flags|=0x10;
            flags=8;
        } else if (typ->decl_flags&4) {
            dq.flags|=8;
            flags=0;
        } else {
            object=typ->object;
            while (object->decl_type==TYPE_TYPEDEF) object=object->object;
            if (object->decl_type==TYPE_FUNCTION) dq.flags|=0x20;
            else if (TypeSize(typ)==6) dq.flags|=0x10;
        }
    }
    address_wanted=0;
    if (CurToken==T_STRING) {
        dq.opr=T_STRING;
        dq.u.var.sym_handle=StringLeaf(flags);
        CompFlags.non_zero_data=1;
    } else if (CurToken==0x0b) {
        dq.opr=T_ID;
        dq.u.var.sym_handle=0;
        dq.u.var.offset=0;
    } else {
        dq.opr=T_ID;
        tree=SingleExpr();
        LeafGet(&leaf,tree);
        switch (leaf.leaf_class) {
        case 0x12:
            address_wanted=1;
            /* fall through */
        case 0: case 1:
            SymGet(&sym,leaf.u.symbol.handle);
            if (sym.stg_class==0x62) CErr1(0x42b);
            else if (sym.stg_class==0x39) CErr1(0x447);
            else {
                sym.flags|=0x80;
                SymReplace(&sym,leaf.u.symbol.handle);
                if (leaf.leaf_type->decl_type!=TYPE_ARRAY &&
                    leaf.leaf_type->decl_type!=TYPE_FUNCTION && !address_wanted) CErr1(0x41e);
            }
            dq.u.var.sym_handle=leaf.u.symbol.handle;
            dq.u.var.offset=leaf.u.symbol.offset;
            CompFlags.non_zero_data=1;
            break;
        case 0x0a: case 0x0b: case 0x0c: case 0x0d:
            dq.u.var.sym_handle=0;
            dq.u.var.offset=leaf.u.constant.value.integer;
            if (dq.u.var.offset!=0) CompFlags.non_zero_data=1;
            break;
        case 0x0e: case 0x0f:
            dq.u.var.sym_handle=0;
            dq.u.var.offset=(int)leaf.u.constant.value.floating;
            if (dq.u.var.offset!=0) CompFlags.non_zero_data=1;
            break;
        case 0x10:
            dq.opr=T_STRING;
            dq.u.var.sym_handle=tree;
            CompFlags.non_zero_data=1;
            break;
        default:
            CErr1(0x41e);
        }
        if (dq.opr!=T_STRING) FreeLeaf(tree);
    }
    if (typ->decl_type==TYPE_POINTER) GenDataQuad(&dq);
    else if (dq.opr==T_STRING || dq.u.var.sym_handle!=0) {
        if (TypeSize(typ)!=DataPtrSize) CErr1(0x47a);
        GenDataQuad(&dq);
    } else {
        ChkConstant(dq.u.var.offset,maximum);
        StoreIValue((unsigned char)token,dq.u.var.offset);
    }
}

void StoreIValue(unsigned char token,unsigned value)
{
    DATA_QUAD dq,*previous;
    dq.opr=0;
    dq.flags=0;
    previous=&dq;
    if (LastDataQuadIndex==DataQuadIndex && LastDataQuadSegIndex==DataQuadSegIndex)
        previous=DataQuadPtr-1;
    if (previous->opr==token && previous->flags==0x84 && previous->u.long_values[0]==value) {
        ++previous->u.long_values[1];
    } else if (previous->opr==token && previous->flags==4) {
        if (previous->u.long_values[0]==value) {
            previous->u.long_values[1]=2;
            previous->flags|=0x80;
        } else {
            previous->u.long_values[1]=value;
            previous->flags|=2;
        }
    } else {
        dq.opr=token;
        dq.flags=4;
        dq.u.long_values[0]=value;
        /* Original second DWORD is unspecified until a two-value or repeat
         * flag makes it meaningful. It is not part of a scalar's value. */
        dq.u.long_values[1]=0;
        if (value!=0) CompFlags.non_zero_data=1;
        GenDataQuad(&dq);
        LastDataQuadIndex=DataQuadIndex;
        LastDataQuadSegIndex=DataQuadSegIndex;
    }
}

void ChkConstant(unsigned value,unsigned maximum)
{
    if (value>maximum && (value|(maximum>>1))!=~0U) CWarn1(6,0x6a);
}

void StoreInt(unsigned char token,unsigned maximum)
{
    unsigned value;
    value=0;
    if (CurToken!=0x0b) {
        value=BoolConstExpr();
        ChkConstant(value,maximum);
    }
    StoreIValue(token,value);
}

int CharArray(TYPEPTR typ)
{
    if (CurToken==T_STRING) {
        while (typ->decl_type==TYPE_TYPEDEF) typ=typ->object;
        if (TypeSize(typ)==1) return 1;
    }
    return 0;
}

int WCharArray(TYPEPTR typ)
{
    if (CurToken==T_STRING && CompFlags.wide_char_string) {
        while (typ->decl_type==TYPE_TYPEDEF) typ=typ->object;
        if (TypeSize(typ)==2) return 1;
    }
    return 0;
}

void InitSymData(TYPEPTR typ,int level)
{
    int token;
    unsigned long n,offset,size;
    FIELDPTR field;
    TYPEPTR field_type;

    while (typ->decl_type==TYPE_TYPEDEF) typ=typ->object;
    if (typ->decl_type==TYPE_ENUM) typ=typ->object;
    token=CurToken;
    if (token==0x0a) {
        NextToken();
        if (CurToken==0x0b || CurToken==2) CErr1(0x458);
    }
    switch (typ->decl_type) {
    case TYPE_ARRAY:
        if (CharArray(typ->object)) InitCharArray(typ);
        else if (WCharArray(typ->object)) InitWCharArray(typ);
        else {
            if (token!=0x0a && level==0) CErr1(0x42e);
            n=TypeSize(typ);
            if (n!=0) {
                for (;;) {
                    InitSymData(typ->object,1);
                    if (--n==0 || CurToken==1) break;
                    if (CurToken==0x0b) {
                        ZeroBytes(n*SizeOfArg(typ->object));
                        break;
                    }
                    MustRecog(2);
                }
            } else {
                do {
                    InitSymData(typ->object,1);
                    ++n;
                    if (CurToken==1 || CurToken==0x0b) break;
                    MustRecog(2);
                } while (CurToken!=0x0b);
                typ->u.array->dimension=n;
            }
        }
        break;
    case TYPE_STRUCT:
        if (token!=0x0a && level==0) CErr1(0x42e);
        size=typ->u.tag->size;
        field=typ->u.tag->u.field_list;
        offset=0;
        while (field!=NULL) {
            if (offset!=field->offset) ZeroBytes(field->offset-offset);
            field_type=field->field_type;
            offset=field->offset+SizeOfArg(field_type);
            if (field_type->decl_type==TYPE_FIELD || field_type->decl_type==TYPE_UFIELD)
                field=InitBitField(field);
            else {
                InitSymData(field_type,1);
                field=field->next_field;
            }
            if (field==NULL || CurToken==1) break;
            if (CurToken!=0x0b) MustRecog(2);
        }
        if (size>offset) ZeroBytes(size-offset);
        break;
    case TYPE_UNION:
        if (token!=0x0a && level==0) CErr1(0x42e);
        field_type=typ->u.tag->u.field_list->field_type;
        size=typ->u.tag->size;
        InitSymData(field_type,1);
        size-=SizeOfArg(field_type);
        if (size!=0) ZeroBytes(size);
        break;
    case TYPE_CHAR:
    case TYPE_UCHAR: StoreInt(0x64,0xff); break;
    case TYPE_SHORT:
    case TYPE_USHORT: StoreInt(0x4b,0xffff); break;
    case TYPE_INT:
    case TYPE_UINT: StorePointer(typ,0x5e,~0U); break;
    case TYPE_LONG:
    case TYPE_ULONG: StorePointer(typ,0x5c,~0U); break;
    case TYPE_FLOAT: StoreFloat(0x56); break;
    case TYPE_DOUBLE: StoreFloat(0x50); break;
    case TYPE_POINTER: StorePointer(typ,0x33,~0U); break;
    }
    if (token==0x0a) {
        if (CurToken==2) NextToken();
        MustRecog(0x0b);
    }
}

void GenDataQuad(DATA_QUAD *quad)
{
    if ((unsigned)DataQuadIndex>=1638) {
        if (DataQuadSegIndex==641) {
            CErr1(0x479);
            CSuicide();
        }
        DataQuadIndex=0;
        ++DataQuadSegIndex;
        DataQuadPtr=(DATA_QUAD *)FEmalloc(16384);
        DataQuadSegs[DataQuadSegIndex]=DataQuadPtr;
    }
    memcpy(DataQuadPtr,quad,sizeof(DATA_QUAD));
    ++DataQuadPtr;
    ++DataQuadIndex;
    DataQuadPtr->opr=1;
}

void StaticInit(SYMPTR sym,SYM_HANDLE sym_handle)
{
    DATA_QUAD dq;
    TYPEPTR typ,struct_typ;
    FIELDPTR field;

    dq.opr=0x5d;
    dq.flags=4;
    /* The old symbol quad leaves two unused union bytes uninitialized.
     * They have no meaning for this opcode; keep them deterministic. */
    dq.u.long_values[0]=dq.u.long_values[1]=0;
    dq.u.var.sym_handle=sym_handle;
    GenDataQuad(&dq);
    CompFlags.non_zero_data=0;
    InitSym=sym;
    struct_typ=NULL;
    typ=sym->sym_type;
    for (;;) {
        while (typ->decl_type==TYPE_TYPEDEF) typ=typ->object;
        if (typ->decl_type!=TYPE_STRUCT) break;
        if (struct_typ==NULL) struct_typ=typ;
        field=typ->u.tag->u.field_list;
        if (field==NULL) break;
        while (field->next_field!=NULL) field=field->next_field;
        typ=field->field_type;
    }
    if (typ->decl_type==TYPE_ARRAY && TypeSize(typ)==0) {
        if (struct_typ==NULL) {
            sym->sym_type=ArrayNode(typ->object);
            sym->sym_type->decl_flags=typ->decl_flags;
        } else {
            sym->sym_type=TypeNode(TYPE_STRUCT,ArrayNode(typ->object));
            sym->sym_type->u.tag=struct_typ->u.tag;
            struct_typ=sym->sym_type;
        }
    } else struct_typ=NULL;
    SymReplace(sym,sym_handle);
    InitSymData(sym->sym_type,0);
    SymGet(sym,sym_handle);
    if (struct_typ!=NULL) {
        struct_typ->object->u.array->dimension=typ->u.array->dimension;
        typ->u.array->dimension=0;
    }
    if (sym->u.var.segment==0) {
        SetFarHuge(sym,0);
        SetSegment(sym);
        SetSegAlign(sym);
    }
}

void InitDataQuads(void)
{
    /* 16KiB segments hold1638 of the old10-byte data quads. */
    DataQuadIndex=1638;
    DataQuadSegIndex=-1;
    memset(DataQuadSegs,0,sizeof(DataQuadSegs));
}

int StartDataQuadAccess(void)
{
    if (DataQuadSegIndex!=-1) {
        DataQuadSegIndex=0;
        DataQuadIndex=0;
        DataQuadPtr=DataQuadSegs[0];
        return 1;
    }
    return 0;
}

DATA_QUAD *NextDataQuad(void)
{
    DATA_QUAD *quad;
    if ((unsigned)DataQuadIndex>=1638) {
        ++DataQuadSegIndex;
        DataQuadIndex=0;
        DataQuadPtr=DataQuadSegs[DataQuadSegIndex];
    }
    ++DataQuadIndex;
    quad=DataQuadPtr++;
    return quad;
}

void FreeDataQuads(void)
{
    unsigned i;
    for (i=0;i<641 && DataQuadSegs[i]!=NULL;++i) FEfree(DataQuadSegs[i]);
    InitDataQuads();
}
