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
#include "compiler.h"
#include "types.h"
#include "symbols.h"
#include "memory.h"
#include "options.h"
#include <recon.h>

TYPEPTR TypeHead,FuncTypeHead[6],BaseTypes[22];
TYPEPTR StringType,StringArrayType,VoidParmList[2],ConstCharType;
int CTypeCounts[21];

/* Original CTypeSizes is mutable module DATA, not a CODE literal pool. */
static char CTypeSizes[22]={
    sizeof(signed char),sizeof(unsigned char),sizeof(short),sizeof(unsigned short),
    sizeof(int),sizeof(unsigned),sizeof(long),sizeof(unsigned long),
    sizeof(float),sizeof(double),
    0,0,0,0,0,0,0,0,0,0,0,sizeof(char)
};

void CTypeInit(void)
{
    DATA_TYPE base_type;

    __STOSD(FuncTypeHead,0,6);
    TypeHead=NULL;
    TagCount=0;
    FieldCount=0;
    EnumCount=0;
    for (base_type=TYPE_CHAR;base_type<=TYPE_DOT_DOT_DOT;++base_type) {
        CTypeCounts[base_type]=0;
        if (CTypeSizes[base_type] || base_type==TYPE_VOID || base_type==TYPE_DOT_DOT_DOT)
            BaseTypes[base_type]=TypeNode(base_type,NULL);
        else BaseTypes[base_type]=NULL;
    }
    StringArrayType=NULL;
    StringType=PtrNode(BaseTypes[TYPE_UCHAR],0,4);
    VoidParmList[0]=BaseTypes[TYPE_VOID];
    VoidParmList[1]=NULL;
}

TYPEPTR TypeNode(DATA_TYPE base_type,TYPEPTR object)
{
    TYPEPTR typ;

    typ=(TYPEPTR)CPermAlloc(sizeof(TYPEDEFN));
    typ->decl_type=base_type;
    typ->object=object;
    typ->u.tag=NULL;
    typ->next_type=TypeHead;
    TypeHead=typ;
    ++TypeCount;
    ++CTypeCounts[base_type];
    return typ;
}
TYPEPTR ArrayNode(TYPEPTR object)
{
    TYPEPTR typ;

    typ=TypeNode(TYPE_ARRAY,object);
    typ->u.array=(struct array_info *)CPermAlloc(sizeof(struct array_info));
    return typ;
}

TYPEPTR PtrNode(TYPEPTR object,unsigned flags,int segment)
{
    return MkPtrNode(object,flags,segment,0);
}

/* The original has a distinct three-byte entry narrowing CX before it
 * falls through to MkPtrNode. Calls that already own a ushort use the core. */
TYPEPTR BasedPtrNode(TYPEPTR object,unsigned flags,int segment,unsigned based_sym)
{
    return MkPtrNode(object,flags,segment,(unsigned short)based_sym);
}

TYPEPTR MkPtrNode(TYPEPTR object,unsigned flags,int segment,unsigned short based_sym)
{
    TYPEPTR typ;

    if (object!=NULL) {
        for (typ=TypeHead;typ!=NULL;typ=typ->next_type) {
            if (typ->decl_type==TYPE_POINTER && typ->object==object &&
                typ->u.p.segment==segment && typ->u.p.based_sym==based_sym &&
                typ->decl_flags==flags)
                return typ;
        }
    }
    typ=TypeNode(TYPE_POINTER,object);
    typ->decl_flags=flags;
    typ->u.p.segment=segment;
    typ->u.p.based_sym=based_sym;
    return typ;
}

TYPEPTR TypeDefault(void)
{
    return GetType(TYPE_INT);
}

TYPEPTR GetType(DATA_TYPE base_type)
{
    TYPEPTR typ;

    typ=BaseTypes[base_type];
    if (typ==NULL) typ=TypeNode(base_type,NULL);
    return typ;
}

TYPEPTR DupType(TYPEPTR typ,unsigned short flags,int force_duplicate)
{
    TYPEPTR next,newtype;

    if (!force_duplicate) {
        for (next=TypeHead;next!=NULL;next=next->next_type) {
            if (next->decl_type==typ->decl_type && next->object==typ->object &&
                next->u.tag==typ->u.tag && next->decl_flags==flags)
                return next;
        }
    }
    newtype=TypeNode(typ->decl_type,typ->object);
    next=newtype->next_type;
    memcpy(newtype,typ,sizeof(TYPEDEFN));
    newtype->next_type=next;
    newtype->decl_flags=flags;
    return newtype;
}

/* Earlier cinfo helper: recursively constant aggregate fields may reside
 * in CONST even when their outer tag itself lacks a const qualifier. */
int IsConstType(TYPEPTR typ)
{
    struct field_entry *field;
    for (;;) {
        while (typ->decl_type==TYPE_TYPEDEF) typ=typ->object;
        if (typ->decl_type!=TYPE_POINTER && typ->decl_type!=TYPE_ARRAY) break;
        if ((typ->decl_flags&3)!=1) return 0;
        typ=typ->object;
    }
    if (typ->decl_type==TYPE_STRUCT || typ->decl_type==TYPE_UNION) {
        if ((typ->decl_flags&3)==1) return 1;
        field=typ->u.tag->u.field_list;
        if (field==NULL || ErrCount!=0) return 0;
        for (;field;field=field->next_field)
            if (!IsConstType(field->field_type)) return 0;
        return 1;
    }
    return (typ->decl_flags&3)==1;
}

unsigned long TypeSize(TYPEPTR typ)
{
    unsigned long size;
    if (typ==NULL) return 0;
    while (typ->decl_type==TYPE_TYPEDEF) typ=typ->object;
    switch (typ->decl_type) {
    case TYPE_CHAR:
    case TYPE_UCHAR:
    case TYPE_SHORT:
    case TYPE_USHORT:
    case TYPE_INT:
    case TYPE_UINT:
    case TYPE_LONG:
    case TYPE_ULONG:
    case TYPE_FLOAT:
    case TYPE_DOUBLE:
    case TYPE_FUNCTION:
    case TYPE_VOID:
    case TYPE_DOT_DOT_DOT:
    case TYPE_PLAIN_CHAR:
        size=(unsigned char)CTypeSizes[typ->decl_type];
        break;
    case TYPE_POINTER:
        if (typ->decl_flags&0x18) size=6;
        else if (typ->decl_flags&4) size=4;
        else {
            do { typ=typ->object; } while (typ->decl_type==TYPE_TYPEDEF);
            if (typ->decl_type==TYPE_FUNCTION) size=CodePtrSize;
            else size=DataPtrSize;
        }
        break;
    case TYPE_ARRAY:
        size=typ->u.array->dimension;
        break;
    case TYPE_STRUCT:
        size=typ->u.tag->size;
        if (typ->object!=NULL) size+=SizeOfArg(typ->object);
        break;
    case TYPE_UNION:
    case TYPE_ENUM:
        size=typ->u.tag->size;
        break;
    case TYPE_FIELD:
    case TYPE_UFIELD:
        size=(unsigned char)CTypeSizes[typ->u.f.field_type];
        break;
    default:
        size=0;
    }
    return size;
}

/* ctype.c TypesPurge has its entire freeing loop under#if0 because
 * type entries are in permanent memory. Exact empty19dd4 C3 is physically
 * shared with TypeSize; no distinct observed hook frontier is claimed. */
void TypesPurge(void)
{
}

unsigned long SizeOfArg(TYPEPTR typ)
{
    unsigned long size;
    while (typ->decl_type==TYPE_TYPEDEF) typ=typ->object;
    size=TypeSize(typ);
    while (typ->decl_type==TYPE_ARRAY) {
        typ=typ->object;
        while (typ->decl_type==TYPE_TYPEDEF) typ=typ->object;
        size*=TypeSize(typ);
    }
    return size;
}
