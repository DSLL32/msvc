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


#define WCC_CSUICIDE_ABORTS
#include <string.h>
#include "compiler.h"
#include "symbols.h"
#include <recon.h>
#if defined(__WATCOMC__)
#pragma intrinsic(memcpy)
#endif

static unsigned Cached_sym_num;
static void *Cached_sym_addr;
struct sym_stats { unsigned get,getptr,replace,read,write; } SymStats;
SYM_ENTRY CurFuncSym;
int PCH_MaxSymHandle;
SYMPTR *PCH_SymArray;
SYM_HASHPTR *HashTab,HashFreeList;
struct tag_entry *TagHead,*DeadTags;
struct label_entry *LabelHead;
int SymLevel;
SYM_HANDLE NextSymHandle,CurFuncHandle;
FIELDPTR ErrSym;
unsigned SymBufNum,LastSymBuf,SymBufDirty,SymSegNum;
struct seg_info SymSegs[MAX_SYM_SEGS];
char *SymBuffer,*SymBufPtr;
SEGADDR_T SymSegment;
extern int GblSymCount,LclSymCount;

void SymInit(void)
{
    int i;

    Cached_sym_num=~0U;
    SymLevel=0;
    GblSymCount=0;
    LclSymCount=0;
    HashTab=(SYM_HASHPTR *)SymHashAlloc(SYM_HASH_SIZE*sizeof(SYM_HASHPTR));
    for (i=0;i<SYM_HASH_SIZE;++i) HashTab[i]=NULL;
    TagHead=NULL;
    DeadTags=NULL;
    LabelHead=NULL;
    HashFreeList=NULL;
    NextSymHandle=0;
    ErrSym=(FIELDPTR)CPermAlloc(sizeof(FIELD_ENTRY));
    ErrSym->field_type=TypeDefault();
    SymBufNum=0;
    LastSymBuf=0;
    SymBufDirty=0;
    CurFuncHandle=0;
    for (i=0;i<MAX_SYM_SEGS;++i) SymSegs[i].allocated=0;
    SymBuffer=(char *)CPermAlloc(SYM_BUF_SIZE);
    SymSegment=AllocSegment(&SymSegs[0]);
    SymBufPtr=SymSegment ? (char *)SymSegment : SymBuffer;
    EnumInit();
}

SYM_HANDLE CharSymHandle,TrySymHandle,Sym_CS,Sym_SS,SpecialSyms;
SYM_HANDLE SymSTOW,SymSTOWB,SymMIN,SymMAX,SymMEMCMP,SymSTOD,SymSTOSB,SymSTOSD;
SYM_HANDLE SymTryInit,SymTryFini,SymExcept,SymFinally,SymTryUnwind,SymCover;

void SpcSymInit(void)
{
    TYPEPTR typ;
    SYM_ENTRY sym;

    NewSym();
    memset(&sym,0,sizeof(SYM_ENTRY));
    sym.handle=0;
    sym.name="char";
    sym.sym_type=GetType(TYPE_UCHAR);
    typ=TypeNode(TYPE_TYPEDEF,sym.sym_type);
    CharSymHandle=NextSymHandle;
    typ->u.typedefn=CharSymHandle;
    BaseTypes[TYPE_PLAIN_CHAR]=typ;
    ConstCharType=DupType(typ,1,1);
    ConstCharType->object=DupType(typ->object,1,1);
    SymReplace(&sym,NextSymHandle);
    NewSym();
    TrySymHandle=NextSymHandle;
    sym.stg_class=SC_AUTO;
    sym.name=".try";
    SymReplace(&sym,NextSymHandle);
    Sym_CS=SegSymbol(".CS");
    Sym_SS=SegSymbol(".SS");
    SpecialSyms=NextSymHandle;
    typ=PtrNode(GetType(TYPE_CHAR),0,4);
    typ=TypeNode(TYPE_FUNCTION,typ);
    SymSTOW=MakeFunction("_inline_.stosw",GetType(TYPE_VOID));
    SymSTOWB=MakeFunction("_inline_.stoswb",GetType(TYPE_VOID));
    SymMIN=MakeFunction("_inline_.min",GetType(TYPE_UINT));
    SymMAX=MakeFunction("_inline_.max",GetType(TYPE_UINT));
    SymMEMCMP=MakeFunction("_inline_memcmp",GetType(TYPE_INT));
    typ=TypeNode(TYPE_FUNCTION,GetType(TYPE_INT));
    SymSTOD=MakeFunction("_inline_.stosd",GetType(TYPE_VOID));
    SymSTOSB=MakeFunction("__STOSB",GetType(TYPE_VOID));
    SymSTOSD=MakeFunction("__STOSD",GetType(TYPE_VOID));
    SymTryInit=MakeFunction("__TryInit",typ);
    SymTryFini=MakeFunction("__TryFini",typ);
    SymExcept=MakeFunction("__Except",typ);
    SymFinally=MakeFunction("__Finally",typ);
    SymTryUnwind=MakeFunction("__TryUnwind",typ);
    SymCover=MakeFunction("__COVERAGE",typ);
    MakeFunction("__C",typ);
    SymGet(&sym,NextSymHandle);
    sym.stg_class=SC_STATIC;
    SymReplace(&sym,NextSymHandle);
}

void NewSym(void)
{
    ++NextSymHandle;
    if (NextSymHandle>=0xffff) {
        CErr1(0x479);
        CSuicide();
    }
}

void SymReplace(SYMPTR sym,SYM_HANDLE handle)
{
    ++SymStats.replace;
    if (handle==CurFuncHandle) memcpy(&CurFuncSym,sym,sizeof(SYM_ENTRY));
    if (handle<PCH_MaxSymHandle) {
        memcpy(PCH_SymArray[handle],sym,sizeof(SYM_ENTRY));
    } else {
        if (handle!=Cached_sym_num) SymAccess(handle);
        memcpy(Cached_sym_addr,sym,sizeof(SYM_ENTRY));
        SymBufDirty=1;
    }
}

/* The 1994 cache tracks the buffer quotient directly, rather than the
 * later FirstSymInBuf range cache in the released source. */
void SymAccess(unsigned sym_num)
{
    unsigned buf_num,seg_num;
    struct seg_info *si;

    Cached_sym_num=sym_num;
    buf_num=sym_num/25;
    if (buf_num!=SymBufNum) {
        if (SymBufDirty) {
            ++SymStats.write;
            si=&SymSegs[SymSegNum];
            if (!si->allocated) SymSegment=AllocSegment(si);
            if (SymSegment==0) {
                PageSeek(si->index*0x4000+(SymBufNum%16)*SYM_BUF_SIZE);
                PageWrite(SymBufPtr,SYM_BUF_SIZE);
            }
            SymBufDirty=0;
        }
        seg_num=buf_num/16;
        si=&SymSegs[seg_num];
        SymSegment=AccessSegment(si);
        if (SymSegment!=0) {
            SymBufPtr=(char *)SymSegment+(buf_num%16)*SYM_BUF_SIZE;
        } else if (buf_num<=LastSymBuf) {
            ++SymStats.read;
            PageSeek(si->index*0x4000+(buf_num%16)*SYM_BUF_SIZE);
            PageRead(SymBufPtr,SYM_BUF_SIZE);
        } else {
            LastSymBuf=buf_num;
            memset(SymBufPtr,0,SYM_BUF_SIZE);
        }
        SymBufNum=buf_num;
        SymSegNum=seg_num;
    }
    Cached_sym_addr=SymBufPtr+(sym_num%25)*sizeof(SYM_ENTRY);
}

SYM_HANDLE SegSymbol(char *name)
{
    SYM_ENTRY sym;
    NewSym();
    memset(&sym,0,sizeof(SYM_ENTRY));
    sym.sym_type=GetType(TYPE_USHORT);
    sym.name=name;
    SymReplace(&sym,NextSymHandle);
    return NextSymHandle;
}

SYM_HANDLE MakeFunction(char *name,TYPEPTR typ)
{
    SYM_ENTRY sym;
    memset(&sym,0,sizeof(SYM_ENTRY));
    sym.stg_class=SC_EXTERN;
    sym.flags=SYM_FUNCTION;
    sym.name=name;
    sym.sym_type=typ;
    sym.handle=SpecialSyms;
    NewSym();
    SpecialSyms=NextSymHandle;
    SymReplace(&sym,NextSymHandle);
    return NextSymHandle;
}

void SymGet(SYMPTR sym,SYM_HANDLE handle)
{
    SYMPTR symptr;
    ++SymStats.get;
    if (handle==CurFuncHandle) {
        symptr=&CurFuncSym;
    } else if (handle<PCH_MaxSymHandle) {
        symptr=PCH_SymArray[handle];
    } else {
        if (handle!=Cached_sym_num) SymAccess(handle);
        symptr=Cached_sym_addr;
    }
    memcpy(sym,symptr,sizeof(SYM_ENTRY));
}
