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


#include <stddef.h>
#include <string.h>
#include "compiler.h"
#include "memory.h"
#include "macros.h"
#include "scanner.h"
#include "symbols.h"
#include "statement.h"
#include <recon.h>

char *MacroSegment,*MacroOffset,*MacroLimit;
struct macro_seg_list *MacSegList;
MEPTR *MacHash;
char Buffer[BUF_SIZE+16],TokenBuf[BUF_SIZE];
int MTokenLen,NestLevel,SkipLevel,MacHashValue,MacroDepth;
char *MacroPtr;
struct cpp_info *CppStack;
struct nested_macros *NestedMacros;
struct macro_token *TokenList;
MEPTR UndefMacroList,NextMacro;
unsigned char InitialMacroFlag;
extern int MacroCount;

void MacroInit(void)
{
    enum { MACRO_LINE,MACRO_FILE,MACRO_DATE,MACRO_TIME,MACRO_STDC };
    static const struct special_macro_name {
        char *name;
        int value;
    } SpcMacros[]={
        {"__LINE__",MACRO_LINE},{"__FILE__",MACRO_FILE},
        {"__DATE__",MACRO_DATE},{"__TIME__",MACRO_TIME},
        {"__STDC__",MACRO_STDC},{NULL,0}
    };
    const struct special_macro_name *mac;
    MEPTR entry;
    int i;

    NestLevel=0;
    SkipLevel=0;
    CppStack=NULL;
    MacroCount=0;
    MacroPtr=NULL;
    NestedMacros=NULL;
    TokenList=NULL;
    UndefMacroList=NULL;
    MTokenLen=0;
    InitialMacroFlag=1;
    AllocMacroSegment(0x1000);
    MacHash=(MEPTR *)MacroSegment;
    MacroOffset=MacroSegment+MACRO_HASH_SIZE*sizeof(MEPTR);
    for (i=0;i<MACRO_HASH_SIZE;++i) MacHash[i]=NULL;
    for (mac=SpcMacros;mac->name!=NULL;++mac) {
        /* Each of these five predefined names has eight characters. */
        entry=(MEPTR)CMemAlloc(offsetof(MEDEFN,macro_name)+sizeof("__LINE__"));
        entry->macro_defn=0;
        entry->macro_len=offsetof(MEDEFN,macro_name)+sizeof("__LINE__");
        entry->parm_count=mac->value;
        strcpy(entry->macro_name,mac->name);
        MacroAdd(entry,Buffer,0,0);
    }
    TimeInit();
}

void AllocMacroSegment(unsigned minimum)
{
    struct macro_seg_list *node;

    (void)minimum;
    MacroSegment=(char *)FEmalloc(0x8000);
    MacroOffset=MacroSegment;
    MacroLimit=MacroOffset+0x7ffe;
    if (MacroSegment==NULL) {
        CErr1(0x464);
        CSuicide();
    }
    node=(struct macro_seg_list *)CMemAlloc(sizeof(struct macro_seg_list));
    node->segment=MacroSegment;
    node->next=MacSegList;
    MacSegList=node;
}

void MacroAdd(MEPTR entry,char *buffer,int length,unsigned flags)
{
    unsigned size;

    size=offsetof(MEDEFN,macro_name)+strlen(entry->macro_name)+1;
    if (length!=0) entry->macro_defn=size;
    entry->macro_len=size+length;
    MacroOverflow(size+length,0);
    MacroCopy((char *)entry,MacroOffset,size);
    if (length!=0) MacroCopy(buffer,MacroOffset+size,length);
    MacLkAdd(entry,size+length,flags);
    CMemFree(entry);
}

void MacroOverflow(unsigned amount,unsigned used)
{
    char *old_offset;

    /* This older implementation has no later four-byte rounding step. */
    if ((unsigned)(MacroLimit-MacroOffset)<amount) {
        old_offset=MacroOffset;
        AllocMacroSegment(amount);
        MacroCopy(old_offset,MacroOffset,used);
    }
}

void MacroCopy(const char *source,char *destination,unsigned length)
{
    memcpy(destination,source,length);
}

void MacLkAdd(MEPTR entry,unsigned length,unsigned flags)
{
    MEPTR old_entry;
    char *name;

    MacroCopy((char *)entry,MacroOffset,offsetof(MEDEFN,macro_name)+1);
    entry=(MEPTR)MacroOffset;
    name=entry->macro_name;
    CalcHash(name,strlen(name));
    old_entry=MacroLkUp(name,MacHashValue);
    if (old_entry==NULL) {
        ++MacroCount;
        entry->next_macro=MacHash[MacHashValue];
        MacHash[MacHashValue]=entry;
        MacroOffset+=length;
        entry->macro_flags=InitialMacroFlag|flags;
    } else if (MacroCompare(entry,old_entry)!=0) {
        CErr2p(0x44c,name);
    }
}

SYM_HASHPTR SymHashAlloc(unsigned amount)
{
    char *memory;

    if ((unsigned)(MacroLimit-MacroOffset)<amount)
        AllocMacroSegment(amount);
    memory=MacroOffset;
    MacroOffset+=amount;
    return (SYM_HASHPTR)memory;
}

MEPTR MacroLookup(void)
{
    return MacroLkUp(Buffer,MacHashValue);
}

void DoMacroExpansion(void)
{
    MacroDepth=0;
    TokenList=NestedMacroExpansion();
    if (TokenList==NULL) MacroPtr=NULL;
    else MacroPtr="";
}

struct macro_token *NestedMacroExpansion(void)
{
    return ExpandNestedMacros(MacroExpansion(0),1);
}

struct macro_token *MacroExpansion(int rescanning)
{
    MEPTR mentry;
    struct macro_arg *macro_parms;
    struct macro_token *head,*mtok;
    struct nested_macros *nested;
    char *tokens;
    int len;
    mentry=NextMacro;
    nested=(struct nested_macros *)CMemAlloc(sizeof(struct nested_macros));
    nested->mentry=mentry;
    nested->rescanning=rescanning;
    nested->substituting_parms=0;
    nested->macro_parms=NULL;
    if (mentry->macro_defn==0) {
        CurToken=SpecialMacro(mentry);
        head=BuildAToken(Buffer);
        head->token=CurToken;
        nested->next=NestedMacros;
        NestedMacros=nested;
    } else {
        macro_parms=CollectParms();
        nested->next=NestedMacros;
        NestedMacros=nested;
        nested->macro_parms=macro_parms;
        tokens=(char *)mentry+mentry->macro_defn;
        head=BuildMTokenList(tokens,macro_parms);
        for (mtok=head;mtok!=NULL;mtok=mtok->next) {
            if (mtok->token==0x33) {
                len=strlen(mtok->data)+1;
                for (nested=NestedMacros;nested!=NULL;nested=nested->next) {
                    if (far_strcmp(nested->mentry->macro_name,mtok->data,len)==0 &&
                        nested->substituting_parms==0) {
                        mtok->token=0x8a; /* T_UNEXPANDABLE_ID */
                        break;
                    }
                }
            }
        }
    }
    head=AppendToken(head,0,"Z-<end of macro>");
    return head;
}

/* Before separate CppStackFini/MacroPurge entry points were introduced. */
void MacroFini(void)
{
    struct cpp_info *cpp;
    while ((cpp=CppStack)!=NULL) {
        SetErrLoc(cpp->file_name,cpp->line_num);
        CErr1(0x43f);
        CppStack=cpp->prev_cpp;
        CMemFree(cpp);
    }
}

void FreeMacroSegments(void)
{
    struct macro_seg_list *segment;
    while ((segment=MacSegList)!=NULL) {
        FEfree(segment->segment);
        MacSegList=segment->next;
        CMemFree(segment);
    }
}
