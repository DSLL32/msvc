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
* Description:  Code-generator type identities, aliases and scalar definitions.
*
****************************************************************************/


#include "codegen.h"
#include "cgmemory.h"
#include <recon.h>

type_def *TypeProcParm,*TypeInteger,*TypeHugeInteger,*TypeLongInteger;
type_def *TypeNearInteger,*TypeUnsigned,*TypeBoolean,*TypeNone,*TypePtr;

/* The 1994 cg_type values are those actually passed by typeinit.c. */
void TypeInit(void)
{
    InitTyping();
    TargTypeInit();
    TypeProcParm=TypeAddress(T_PROC_PARM);
    TypeInteger=TypeAddress(T_INTEGER);
    TypeHugeInteger=TypeAddress(T_HUGE_INTEGER);
    TypeLongInteger=TypeAddress(T_LONG_INTEGER);
    TypeNearInteger=TypeAddress(T_NEAR_INTEGER);
    TypeUnsigned=TypeAddress(T_UNSIGNED);
    TypeBoolean=TypeAddress(T_BOOLEAN);
    TypeNone=TypeAddress(T_DEFAULT);
    TypePtr=TypeAddress(T_POINTER);
}

#pragma pack(4)
typedef struct type_list {
    union {
        struct { type_def *tptr;cg_type refno; } alias;
        type_def type;
    } tipe;
    struct type_list *link;
    unsigned char format;
} type_list;
#pragma pack()
typedef char old_type_list_size[(sizeof(type_list)==20)?1:-1];
static type_list *TypeList;

void InitTyping(void)
{
    TypeList=NULL;
}

void TypeFini(void)
{
    type_list *list,*next;
    for (list=TypeList;list!=NULL;list=next) {
        next=list->link;
        CGFreeSize(list,sizeof(*list));
    }
    /* The original leaves TypeList untouched; InitTyping resets it on
     * the next initialization, rather than this finalizer doing so. */
}

type_def *PTInteger,*PTUnsigned,*PTCodePointer,*PTPointer;

void TargTypeInit(void)
{
    TypeAlias(T_UNSIGNED,T_UINT_4);
    TypeAlias(T_INTEGER,T_INT_4);
    PTInteger=TypeAddress(T_INT_4);
    PTUnsigned=TypeAddress(T_UINT_4);
    if (TargetModel&8) {
        TypeAlias(T_CODE_PTR,T_LONG_CODE_PTR);
        PTCodePointer=TypeAddress(T_LONG_CODE_PTR);
    } else {
        TypeAlias(T_CODE_PTR,T_NEAR_CODE_PTR);
        PTCodePointer=TypeAddress(T_NEAR_CODE_PTR);
    }
    if (TargetModel&4) {
        TypeAlias(T_POINTER,T_LONG_POINTER);
        PTPointer=TypeAddress(T_LONG_POINTER);
    } else {
        TypeAlias(T_POINTER,T_NEAR_POINTER);
        PTPointer=TypeAddress(T_NEAR_POINTER);
    }
    TypeAlias(T_NEAR_INTEGER,T_INT_4);
    TypeAlias(T_LONG_INTEGER,T_INT_4);
    TypeAlias(T_HUGE_INTEGER,T_INT_4);
}

type_def *TypeAlias(cg_type define,cg_type existing)
{
    type_def *type;
    type_list *list;
    type=TypeAddress(existing);
    if (type->refno==T_DEFAULT) return type;
    list=CGAlloc(sizeof(*list));
    list->link=TypeList;
    TypeList=list;
    list->format=1;
    list->tipe.alias.refno=define;
    list->tipe.alias.tptr=type;
    return type;
}

/* Encountered scalar/pointer objects from types.c and 386type.c. The old
 * table has no built-in long-double object; that code falls through to
 * the user-defined list rather than the later released implementation. */
static type_def TUInt1={T_UINT_1,1,0},TInt1={T_INT_1,1,2};
static type_def TUInt2={T_UINT_2,2,0},TInt2={T_INT_2,2,2};
static type_def TUInt4={T_UINT_4,4,0},TInt4={T_INT_4,4,2};
static type_def TSingle={T_SINGLE,4,3},TDouble={T_DOUBLE,8,3};
static type_def TBool={T_BOOLEAN,0,0},TNull={T_DEFAULT,0,0};
static type_def TPascal={T_PROC_PARM,4,0};
type_def TLongCP={T_LONG_CODE_PTR,6,12},TNearCP={T_NEAR_CODE_PTR,4,12};
type_def THugeP={T_HUGE_POINTER,6,4},TLongP={T_LONG_POINTER,6,4};
type_def TNearP={T_NEAR_POINTER,4,4};

type_def *TypeAddress(cg_type type)
{
    type_list *list;
    switch (type) {
    case T_UINT_1:return &TUInt1;
    case T_INT_1:return &TInt1;
    case T_UINT_2:return &TUInt2;
    case T_INT_2:return &TInt2;
    case T_UINT_4:return &TUInt4;
    case T_INT_4:return &TInt4;
    case T_LONG_POINTER:return &TLongP;
    case T_HUGE_POINTER:return &THugeP;
    case T_NEAR_POINTER:return &TNearP;
    case T_LONG_CODE_PTR:return &TLongCP;
    case T_NEAR_CODE_PTR:return &TNearCP;
    case T_SINGLE:return &TSingle;
    case T_DOUBLE:return &TDouble;
    case T_DEFAULT:return &TNull;
    case T_INTEGER:return PTInteger;
    case T_UNSIGNED:return PTUnsigned;
    case T_POINTER:return PTPointer;
    case T_CODE_PTR:return PTCodePointer;
    case T_BOOLEAN:return &TBool;
    case T_PROC_PARM:return &TPascal;
    default:
        for (list=TypeList;list!=NULL;list=list->link) {
            if (list->format==0) {
                if (list->tipe.type.refno==type) return &list->tipe.type;
            } else if (list->tipe.alias.refno==type) return list->tipe.alias.tptr;
        }
        return NULL;
    }
}

unsigned char TypeClass(type_def *type)
{
    switch (type->refno) {
    case T_INT_1:return I1;
    case T_UINT_1:return U1;
    case T_INT_2:return I2;
    case T_UINT_2:return U2;
    case T_INT_4:return I4;
    case T_UINT_4:return U4;
    default:return ReturnClass(type,0);
    }
}

static cg_type Types[]={
    T_UINT_1,T_INT_1,T_UINT_2,T_INT_2,T_UINT_4,T_INT_4,
    T_LONG_POINTER,T_HUGE_POINTER,T_SINGLE,T_DOUBLE,T_DEFAULT
};

type_def *ClassType(unsigned char class)
{
    return TypeAddress(Types[class]);
}

unsigned char ReturnClass(type_def *type,unsigned short attributes)
{
    switch (type->refno) {
    case T_UINT_1:case T_INT_1:case T_UINT_2:case T_INT_2:case T_UINT_4:case T_INT_4:
        return MapIntReturn(type->refno);
    case T_LONG_POINTER:case T_HUGE_POINTER:case T_NEAR_POINTER:
    case T_LONG_CODE_PTR:case T_NEAR_CODE_PTR:
        return MapPointer(type->refno);
    case T_SINGLE:case T_DOUBLE:
        return MapFloat(type->refno,attributes);
    default:return MapStruct(type->length,attributes);
    }
}

unsigned char MapPointer(cg_type type)
{
    switch (type) {
    case T_NEAR_POINTER:case T_NEAR_CODE_PTR:return U4;
    case T_LONG_POINTER:case T_HUGE_POINTER:case T_LONG_CODE_PTR:return CP;
    default:Zoiks(78);return XX;
    }
}

/* types.c TypeDef: this earlier build has no per-type alignment field. */
type_def *TypeDefn(cg_type type,unsigned long size)
{
    type_list *list;
    list=CGAlloc(sizeof(*list));
    list->format=0;
    list->tipe.type.attr=0;
    list->link=TypeList;
    TypeList=list;
    list->tipe.type.length=size;
    list->tipe.type.refno=type;
    return &list->tipe.type;
}

unsigned long TypeLength(cg_type type)
{
    return TypeAddress(type)->length;
}

unsigned char MapFloat(cg_type type,unsigned short attributes)
{
    if (attributes&0x200) return XX;
    return type==T_SINGLE ? FS:FD;
}

unsigned char MapIntReturn(cg_type type)
{
    switch (type) {
    case T_UINT_1:return U1;
    case T_INT_1:return I1;
    case T_UINT_2:return U2;
    case T_INT_2:return I2;
    case T_UINT_4:return U4;
    case T_INT_4:return I4;
    default:Zoiks(78);return XX;
    }
}

unsigned char MapStruct(unsigned long size,unsigned short attr)
{
    if (!(attr&0x100)) {
        if (size==1) return U1;
        if (size==2) return U2;
        if (size==4) return U4;
    }
    return XX;
}
