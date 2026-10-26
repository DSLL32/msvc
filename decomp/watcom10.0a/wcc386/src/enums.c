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
#include <limits.h>
#include "memory.h"
#include "symbols.h"
#include "declarations.h"
#include "compiler.h"
#include "expression.h"
#include "scanner.h"
#include "macros.h"
#include "stubs.h"
#include <recon.h>

#pragma pack(1)
struct enum_entry {
    struct enum_entry *next_enum,*thread;
    struct tag_entry *parent;
    int hash,value;
    char name[1];
};
#pragma pack()
typedef struct enum_entry *ENUMPTR;
ENUMPTR EnumTable[SYM_HASH_SIZE];
int EnumRecSize;

void EnumInit(void)
{
    __STOSD(EnumTable,0,SYM_HASH_SIZE);
    EnumRecSize=0;
}

int EnumLookup(int hash,char *name,struct enum_info *info)
{
    ENUMPTR sym;
    for (sym=EnumTable[hash];sym;sym=sym->next_enum) {
        if (strcmp(sym->name,name)==0) {
            info->value=sym->value;
            info->parent=sym->parent;
            info->level=sym->parent->level;
            return 1;
        }
    }
    info->level=-1;
    return 0;
}

TYPEPTR EnumDecl(unsigned flags)
{
    TYPEPTR typ;
    struct tag_entry *tag;
    ENUMPTR entry,prev;
    int value,low,high,overflow;
    NextToken();
    if (CurToken==T_ID) {
        tag=TagLookup();
        NextToken();
        if (CurToken!=0x0a) {
            typ=tag->sym_type;
            if (typ==NULL) {
                CErr1(0x45b);
                typ=TypeDefault();
            } else {
                if (typ->decl_type!=TYPE_ENUM) CErr2p(0x3fb,tag->name);
                if (typ->decl_flags!=flags) typ=DupType(typ,flags,0);
                typ->u.tag=tag;
            }
            return typ;
        }
        tag=VfyNewTag(tag,TYPE_ENUM);
    } else tag=NullTag();
    typ=TypeNode(TYPE_ENUM,GetType(TYPE_INT));
    typ->u.tag=tag;
    typ->decl_flags=flags;
    tag->size=4;
    tag->u.enum_list=NULL;
    tag->sym_type=typ;
    prev=NULL;
    /* Without a body the old compiler reads uninitialized range locals.
     * This malformed anonymous-enum path remains unsupported in both builds. */
    if (CurToken!=0x0a) STUB_TRAP();
    NextToken();
    if (CurToken==0x0b) CErr1(0x453);
    overflow=0;
    low=INT_MAX;
    high=INT_MIN;
    value=0;
    while (CurToken==T_ID) {
        entry=EnumLkAdd(tag);
        if (prev!=NULL) SetEnumThread(prev,entry);
        NextToken();
        prev=entry;
        if (CurToken==0x0e) {
            NextToken();
            value=BoolConstExpr();
            overflow=0;
        }
        if (!CompFlags.extensions_enabled && overflow) CErr1(0x468);
        SetEnumValue(entry,value);
        if (value<low) low=value;
        if (value>high) high=value;
        if (CurToken==0x0b) break;
        if (value==INT_MAX) overflow=1;
        value=(unsigned)value+1;
        MustRecog(2);
        if (CurToken==0x0b && !CompFlags.extensions_enabled) ExpectIdentifier();
    }
    MustRecog(0x0b);
    if (CompFlags.extensions_enabled && !CompFlags.make_enums_an_int) {
        if (low>=0) {
            if ((unsigned)high<=0xff) { typ->object=GetType(TYPE_UCHAR); tag->size=1; }
            else if ((unsigned)high<=0xffff) { typ->object=GetType(TYPE_USHORT); tag->size=2; }
            else typ->object=GetType(TYPE_UINT);
        } else if (low>=-128 && high<=127) {
            typ->object=GetType(TYPE_CHAR);
            tag->size=1;
        } else if (low>=-32768 && high<=32767) {
            typ->object=GetType(TYPE_SHORT);
            tag->size=2;
        }
    }
    return typ;
}

struct enum_entry *EnumLkAdd(struct tag_entry *tag)
{
    ENUMPTR entry;
    int size;
    VfyNewSym(HashValue,Buffer);
    size=sizeof(struct enum_entry)+strlen(Buffer);
    if (size>EnumRecSize) EnumRecSize=size;
    entry=CPermAlloc(size);
    strcpy(entry->name,Buffer);
    entry->parent=tag;
    entry->next_enum=EnumTable[HashValue];
    EnumTable[HashValue]=entry;
    entry->hash=HashValue;
    ++EnumCount;
    if (tag->u.enum_list==NULL) tag->u.enum_list=entry;
    return entry;
}

void SetEnumValue(struct enum_entry *entry,int value)
{
    entry->value=value;
}

void SetEnumThread(struct enum_entry *entry,struct enum_entry *next)
{
    entry->thread=next;
}

void FreeEnums(void)
{
    int bucket;
    ENUMPTR entry;
    for (bucket=0;bucket<SYM_HASH_SIZE;++bucket) {
        while ((entry=EnumTable[bucket])!=NULL && entry->parent->level==SymLevel)
            EnumTable[bucket]=entry->next_enum;
    }
}
