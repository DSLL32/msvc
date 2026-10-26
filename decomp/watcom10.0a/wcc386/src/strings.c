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


#include "symbols.h"
#include <string.h>
#include "cstring.h"
#include "compiler.h"
#include "expression.h"
#include "scanner.h"
#include "macros.h"
#include "options.h"
#include "memory.h"
#include "stubs.h"
#include <recon.h>
#ifdef __WATCOMC__
/* Deterministic unused bytes in the final serialized literal chunk need no
 * helper vertex; the original left those bytes unspecified. */
#pragma intrinsic(memset)
#endif

/* The 1994 string lists/hash slots are symbol handles, not the later
 * directly linked string-literal pointers. */
SYM_HANDLE CStringList,CS_StringList,StringHash[30];

void WriteLiteralChunk(void *source,SYM_HANDLE handle)
{
    SymReplace((SYMPTR)source,handle);
}

void StoreLiteral(STRING_LITERAL *literal,SYM_HANDLE handle)
{
    unsigned count,copy_count;
    char *source;
    SYM_ENTRY fragment;

    count=literal->length+11;
    source=(char *)literal;
    for (;;) {
        copy_count=count;
        if (copy_count>sizeof(fragment)) copy_count=sizeof(fragment);
        memset(&fragment,0,sizeof(fragment));
        memcpy(&fragment,source,copy_count);
        WriteLiteralChunk(&fragment,handle);
        if (count<=sizeof(fragment)) break;
        handle=NewLiteralHandle();
        count-=sizeof(fragment);
        source+=sizeof(fragment);
    }
}

/* The old data-generator hook initializes the string's emission state;
 * it does not create an ordinary expression or data quad here. */
void GenStringQuad(LEAFPTR leaf)
{
    leaf->u.string.back_handle=0;
    leaf->u.string.offset=0;
}

void InitStringLeaf(LEAFPTR leaf,unsigned flags,SYM_HANDLE handle)
{
    InitLeaf(leaf);
    leaf->leaf_class=0x10;
    leaf->leaf_flags=flags;
    if (CompFlags.wide_char_string) {
        leaf->leaf_type=ArrayNode(GetType(TYPE_USHORT));
        leaf->leaf_type->u.array->dimension=CLitLength>>1;
    } else if (StringArrayType!=NULL) {
        leaf->leaf_type=StringArrayType;
        StringArrayType->u.array->dimension=CLitLength;
        StringArrayType=NULL;
    } else {
        leaf->leaf_type=ArrayNode(GetType(TYPE_UCHAR));
        leaf->leaf_type->u.array->dimension=CLitLength;
    }
    leaf->u.symbol.offset=0;
    leaf->u.symbol.handle=handle;
}

SYM_HANDLE NewLiteralHandle(void)
{
    NewSym();
    return NextSymHandle;
}

int RemoveEscapes(char *destination)
{
    unsigned c,j,saved_length;
    j=0;
    saved_length=CLitLength;
    CLitLength=0;
    while ((unsigned)CLitLength<saved_length) {
        c=(unsigned char)Buffer[CLitLength];
        if (c=='\\') {
            ++CLitLength;
            c=ESCChar((unsigned char)Buffer[CLitLength],1);
            if (CompFlags.wide_char_string) {
                if (destination) destination[j]=(char)c;
                ++j;
                c>>=8;
            }
        } else {
            ++CLitLength;
            if (CharSet[c]&C_DB) {
                if (CompFlags.jis_to_unicode && CompFlags.wide_char_string) {
                    c=JIS2Unicode((c<<8)+(unsigned char)Buffer[CLitLength]);
                    if (destination) destination[j]=(char)c;
                    c>>=8;
                } else {
                    if (destination) destination[j]=(char)c;
                    c=(unsigned char)Buffer[CLitLength];
                }
                ++j;
                ++CLitLength;
            } else if (CompFlags.wide_char_string) {
                /* The user-supplied Unicode table has not been recovered. */
                if (CompFlags.use_unicode) STUB_TRAP();
                else if (CompFlags.jis_to_unicode) c=JIS2Unicode(c);
                if (destination) destination[j]=(char)c;
                ++j;
                c>>=8;
            }
        }
        if (destination) destination[j]=(char)c;
        ++j;
    }
    return j;
}

STRING_LITERAL *GetLiteral(void)
{
    unsigned len,len2;
    STRING_LITERAL *literal,*p;

    len=RemoveEscapes(NULL);
    literal=(STRING_LITERAL *)CMemAlloc(sizeof(STRING_LITERAL)+len);
    RemoveEscapes(literal->literal);
    NextToken();
    while (CurToken==T_STRING) {
        len2=RemoveEscapes(NULL);
        --len;
        if (CompFlags.wide_char_string && len!=0) --len;
        p=(STRING_LITERAL *)CMemAlloc(sizeof(STRING_LITERAL)+len+len2);
        memcpy(p->literal,literal->literal,len);
        RemoveEscapes(p->literal+len);
        CMemFree(literal);
        NextToken();
        len+=len2;
        literal=p;
    }
    CLitLength=len;
    literal->flags=0;
    literal->length=len;
    return literal;
}

unsigned StringLeaf(unsigned flags)
{
    STRING_LITERAL *new_lit,*previous;
    char saved[sizeof(SYM_ENTRY)];
    SYM_HANDLE strlit,leaf_index;
    LEAF leaf;

    new_lit=GetLiteral();
    strlit=0;
    if ((TargetSwitches&4) && !CompFlags.strings_in_code_segment && new_lit->length>DataThreshold)
        flags=8;
    if (flags==8) CompFlags.far_strings=1;
    previous=(STRING_LITERAL *)saved;
    if (new_lit->length<30 && (Toggles&0x20)) {
        strlit=StringHash[new_lit->length];
        while (strlit!=0) {
            ReadLiteralChunk(saved,strlit);
            if (previous->length==new_lit->length && previous->flags==flags &&
                memcmp(previous->literal,new_lit->literal,new_lit->length)==0) break;
            strlit=previous->next_string;
        }
    }
    if (strlit==0) {
        new_lit->flags=flags;
        ++LitCount;
        LitPoolSize+=CLitLength;
        strlit=NewLiteralHandle();
        InitStringLeaf(&leaf,flags,strlit);
        if (SizeOfCount==0) {
            leaf.u.symbol.offset=0;
            if (DeadCode==0) {
                leaf.u.symbol.offset=1;
                GenStringQuad(&leaf);
            }
        }
        leaf_index=GenLeaf(&leaf);
        new_lit->next_string=0;
        new_lit->cg_back_handle=0;
        new_lit->leaf_index=leaf_index;
        if (new_lit->length<30) {
            new_lit->next_string=StringHash[new_lit->length];
            StringHash[new_lit->length]=strlit;
        } else if (CompFlags.strings_in_code_segment) {
            new_lit->next_string=CS_StringList;
            CS_StringList=strlit;
        } else {
            new_lit->next_string=CStringList;
            CStringList=strlit;
        }
        StoreLiteral(new_lit,strlit);
    } else if (SizeOfCount==0) {
        leaf_index=previous->leaf_index;
        LeafGet(&leaf,leaf_index);
        if (DeadCode==0) ++leaf.u.symbol.offset;
        LeafReplace(&leaf,leaf_index);
    } else {
        InitStringLeaf(&leaf,flags,strlit);
        leaf_index=GenLeaf(&leaf);
    }
    CMemFree(new_lit);
    if (CurFunc!=NULL) CurFunc->attrib&=~0x8000;
    return leaf_index;
}

void StringInit(void)
{
    int i;
    CStringList=0;
    CS_StringList=0;
    for (i=0;i<30;++i) StringHash[i]=0;
}

void ReadLiteralChunk(void *destination,SYM_HANDLE handle)
{
    SymGet((SYMPTR)destination,handle);
}
