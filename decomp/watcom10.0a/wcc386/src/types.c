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
#include "memory.h"
#include <recon.h>

TYPEPTR TypeHead,FuncTypeHead[6],BaseTypes[22];
TYPEPTR StringType,StringArrayType,VoidParmList[2],ConstCharType;
int CTypeCounts[21];

/* Original CTypeSizes is mutable module DATA, not a CODE literal pool. */
static char CTypeSizes[21]={
    sizeof(signed char),sizeof(unsigned char),sizeof(short),sizeof(unsigned short),
    sizeof(int),sizeof(unsigned),sizeof(long),sizeof(unsigned long),
    sizeof(float),sizeof(double)
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
TYPEPTR PtrNode(TYPEPTR object,unsigned flags,int segment)
{
    return MkPtrNode(object,flags,segment,0);
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
