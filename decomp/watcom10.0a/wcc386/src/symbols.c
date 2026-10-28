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
#include "scanner.h"
#include "files.h"
#include "memory.h"
#include "options.h"
#include "declarations.h"
#include "statement.h"
#include <recon.h>
#if defined(__WATCOMC__)
#pragma intrinsic(memcpy)
#endif

static unsigned Cached_sym_num;
static void *Cached_sym_addr;
struct sym_stats { unsigned get,getptr,replace,read,write; } SymStats;
SYM_ENTRY CurFuncSym;
SYMPTR CurFunc;
int PCH_MaxSymHandle;
SYMPTR *PCH_SymArray;
SYM_HASHPTR *HashTab,HashFreeList;
struct tag_entry *TagHead,*DeadTags;
struct label_entry *LabelHead;
int SymLevel;
SYM_HANDLE NextSymHandle,CurFuncHandle,GlobalSym,MainSymHandle;
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

void CSegFree(SEGADDR_T segment);
void SymFini(void)
{
    unsigned seg_num;
    struct seg_info *segment;
    for (seg_num=0;seg_num<MAX_SYM_SEGS;++seg_num) {
        segment=&SymSegs[seg_num];
        if (segment->allocated && segment->in_farmem) CSegFree(segment->index);
    }
    if (CompFlags.extra_stats_wanted) {
        printf("SymStats: get = %u, rep = %u, read = %u, write = %u, typedef = %u\n",
               SymStats.get,SymStats.replace,SymStats.read,SymStats.write,
               (unsigned)SymTypedef);
    }
}

void TypesPurge(void);
void SymsPurge(void)
{
    struct label_entry *label;
    while ((label=LabelHead)!=NULL) {
        LabelHead=label->next_label;
        CMemFree(label);
    }
    /* Tags are in the permanent area; like the actual csym.c body, drop
     * their lists here without freeing the individual permanent objects. */
    TagHead=NULL;
    DeadTags=NULL;
    TypesPurge();
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

void SymCreate(SYMPTR sym,char *name)
{
    memset(sym,0,sizeof(SYM_ENTRY));
    sym->name=CMemAlloc(strlen(name)+1);
    strcpy(sym->name,name);
    if (SrcFile!=NULL) sym->defn_file_index=SrcFile->src_flist->index;
    sym->d.defn_line=TokenLine;
}

SYM_HANDLE SymLook(int hash,char *name)
{
    int len;
    SYM_HASHPTR hsym;
    len=strlen(name)+1;
    for (hsym=HashTab[hash];hsym;hsym=hsym->next_sym)
        if (far_strcmp(hsym->name,name,len)==0) return hsym->handle;
    return 0;
}

SYM_HANDLE SymLookTypedef(int hash,char *name,SYMPTR sym)
{
    int len;
    SYM_HASHPTR hsym;
    len=strlen(name)+1;
    for (hsym=HashTab[hash];hsym;hsym=hsym->next_sym) {
        if (far_strcmp(hsym->name,name,len)==0) {
            if (hsym->sym_type==NULL) break;
            sym->sym_type=hsym->sym_type;
            sym->stg_class=0x58;
            sym->level=hsym->level;
            return hsym->handle;
        }
    }
    return 0;
}

SYM_HANDLE SymAdd(int hash,SYMPTR sym)
{
    SYM_HASHPTR node,*slot;
    if (sym==NULL) return 0;
    if (SymLevel==0) ++GblSymCount;
    else ++LclSymCount;
    NewSym();
    sym->level=SymLevel;
    node=SymHash(sym,NextSymHandle);
    sym->info.hash_value=hash;
    slot=&HashTab[hash];
    while (*slot!=NULL && ((*slot)->level&0x7f)>SymLevel) slot=&(*slot)->next_sym;
    node->next_sym=*slot;
    *slot=node;
    return node->handle;
}

SYM_HANDLE SymAddL0(int hash,SYMPTR sym)
{
    SYM_HASHPTR node,*slot;
    if (sym==NULL) return 0;
    ++GblSymCount;
    NewSym();
    sym->level=0;
    node=SymHash(sym,NextSymHandle);
    node->next_sym=NULL;
    sym->info.hash_value=hash;
    slot=&HashTab[hash];
    while (*slot!=NULL) slot=&(*slot)->next_sym;
    *slot=node;
    return node->handle;
}

SYM_HASHPTR SymHash(SYMPTR sym,SYM_HANDLE handle)
{
    SYM_HASHPTR node;
    TYPEPTR typ;
    int length;
    length=strlen(sym->name);
    node=SymHashAlloc(sizeof(struct sym_hash_entry)+length);
    node->sym_type=NULL;
    if (sym->stg_class==0x58) {
        typ=sym->sym_type;
        /* The older compiler retains simple aliases directly unless debug,
         * browser, prototype, based or function identity requires a node. */
        if (CompFlags.dump_prototypes || CompFlags.emit_browser_info ||
            (GenSwitches&0x19000000) || (sym->attrib&0x800) ||
            typ->decl_type==TYPE_FUNCTION) {
            typ=TypeNode(TYPE_TYPEDEF,typ);
            if (sym->sym_type->decl_type==TYPE_FUNCTION)
                typ->decl_flags=sym->sym_type->decl_flags;
            typ->u.typedefn=handle;
            sym->sym_type=typ;
        }
        node->sym_type=typ;
    }
    node->level=sym->level;
    memcpy(node->name,sym->name,length+1);
    if (sym->name[0]!='.') CMemFree(sym->name);
    sym->name=NULL;
    node->handle=handle;
    return node;
}

/* cutil.c: names retained in the symbol hash, except the predefined char type. */
char *SymName(SYMPTR sym,SYM_HANDLE handle)
{
    SYM_HASHPTR node;
    if (handle==CharSymHandle) return "char";
    node=HashTab[sym->info.hash_value];
    while (node->handle!=handle) node=node->next_sym;
    return node->name;
}

/* The old symbol-dump hook is a genuine empty one-byte RET. */
void SymDump(void)
{
}

int VarParm(SYMPTR sym)
{
    TYPEPTR typ, *parms;
    if (sym!=NULL && (sym->flags&SYM_FUNCTION)) {
        typ=sym->sym_type;
        while (typ->decl_type==TYPE_TYPEDEF) typ=typ->object;
        parms=typ->u.parms;
        if (parms==NULL) return 0;
        while (*parms!=NULL) {
            if ((*parms)->decl_type==TYPE_DOT_DOT_DOT) return 1;
            ++parms;
        }
    }
    return 0;
}

extern int TmpSymCount;
SYM_HANDLE MakeNewSym(SYMPTR sym,char id,TYPEPTR typ,int stg_class)
{
    SYM_HANDLE handle;
    handle=GetNewSym(sym,id,typ,stg_class);
    if (SymLevel!=0) {
        sym->handle=CurFunc->u.func.locals;
        CurFunc->u.func.locals=handle;
    }
    ++TmpSymCount;
    return handle;
}

SYM_HANDLE GetNewSym(SYMPTR sym,char id,TYPEPTR typ,int stg_class)
{
    char name[3];
    SYM_HANDLE handle;
    name[0]='.';
    name[1]=id;
    name[2]='\0';
    SymCreate(sym,name);
    handle=SymAdd(0,sym);
    sym->sym_type=typ;
    sym->stg_class=stg_class;
    sym->flags|=0x22; /* SYM_REFERENCED | SYM_TEMP */
    return handle;
}

void EndBlock(void)
{
    FreeEnums();
    FreeTags();
    FreeSym();
    if (SymLevel!=0) {
        if (SymLevel==1) AsgnSegs(CurFunc->u.func.locals);
        else AsgnSegs(BlockStack->sym_head);
    }
    --SymLevel;
}

void FreeTags(void)
{
    struct tag_entry *tag;
    while ((tag=TagHead)!=NULL && tag->level==SymLevel) {
        TagHead=tag->next_tag;
        tag->next_tag=DeadTags;
        DeadTags=tag;
    }
}

SYM_HASHPTR FreeSym(void)
{
    SYM_HASHPTR node,next,list;
    SYM_ENTRY sym;
    SYM_HANDLE head[7],tail[7],previous_tail;
    int bucket,length;
    for (bucket=0;bucket<7;++bucket) head[bucket]=tail[bucket]=0;
    list=GetSymList();
    for (node=list;node!=NULL;node=next) {
        SymGet(&sym,node->handle);
        next=node->next_sym;
        if (sym.stg_class==0x58 || SymLevel!=0 || (sym.flags&0x0a) || sym.stg_class==SC_NULL) {
            if (SymLevel==0) {
                bucket=SymBucket(&sym);
                sym.handle=head[bucket];
                if (!(sym.flags&SYM_FUNCTION)) {
                    if (sym.stg_class==SC_NULL && sym.sym_type->decl_type==TYPE_ARRAY &&
                        sym.sym_type->u.array->dimension==0) sym.sym_type->u.array->dimension=1;
                    AssignSeg(&sym);
                } else ChkFunction(&sym,node->name);
                if (tail[bucket]==0) tail[bucket]=node->handle;
                head[bucket]=node->handle;
            }
            length=far_strlen_plus1(node->name);
            sym.name=CPermAlloc(length);
            far_memcpy(sym.name,node->name,length);
            sym.info.backinfo=NULL;
            SymReplace(&sym,node->handle);
        }
        ChkIncomplete(&sym,node->name);
        if (SymLevel==0) ChkDefined(&sym,node->name);
        else {
            ChkReference(&sym,node->name);
            if (sym.stg_class==SC_STATIC && !(sym.flags&SYM_FUNCTION)) {
                CurFunc->attrib&=~0x8000;
                SymReplace(CurFunc,CurFuncHandle);
            }
        }
    }
    if (SymLevel==0) {
        GlobalSym=0;
        previous_tail=0;
        for (bucket=6;bucket>=0;--bucket) {
            if (head[bucket]!=0) {
                if (GlobalSym==0) GlobalSym=head[bucket];
                if (previous_tail!=0) {
                    SymGet(&sym,previous_tail);
                    sym.handle=head[bucket];
                    SymReplace(&sym,previous_tail);
                }
                previous_tail=tail[bucket];
            }
        }
    }
    return list;
}

SYM_HASHPTR GetSymList(void)
{
    SYM_HASHPTR node,next,list,last;
    SYM_HASHPTR segments[MAX_SYM_SEGS],buffers[16],tails[16];
    unsigned i,j;
    list=NULL;
    for (i=0;i<SYM_HASH_SIZE;++i) {
        for (node=HashTab[i];node!=NULL;node=next) {
            if ((node->level&0x7f)!=SymLevel) break;
            next=node->next_sym;
            node->next_sym=list;
            list=node;
        }
        HashTab[i]=node;
    }
    if (SymLevel==0) {
        for (i=0;i<MAX_SYM_SEGS;++i) segments[i]=NULL;
        for (node=list;node!=NULL;node=next) {
            next=node->next_sym;
            /* Old segments contain16 buffers of25 forty-byte symbols. */
            i=node->handle/400;
            node->next_sym=segments[i];
            segments[i]=node;
        }
        list=last=NULL;
        for (i=0;i<MAX_SYM_SEGS;++i) {
            for (j=0;j<16;++j) buffers[j]=tails[j]=NULL;
            for (node=segments[i];node!=NULL;node=next) {
                next=node->next_sym;
                j=(node->handle/25)%16;
                node->next_sym=buffers[j];
                buffers[j]=node;
                if (tails[j]==NULL) tails[j]=node;
            }
            for (j=0;j<16;++j) {
                node=buffers[j];
                if (node!=NULL) {
                    if (list==NULL) list=node;
                    if (last!=NULL) last->next_sym=node;
                    last=tails[j];
                }
            }
        }
    }
    return list;
}

void ChkIncomplete(SYMPTR sym,char *name)
{
    TYPEPTR typ;
    if (sym->stg_class==0x58 || (sym->flags&0x21)) return;
    if (!(sym->flags&2) && sym->stg_class==SC_EXTERN) return;
    typ=sym->sym_type;
    while (typ->decl_type==TYPE_TYPEDEF) typ=typ->object;
    if (SizeOfArg(typ)==0 && typ->decl_type!=TYPE_FUNCTION && typ->decl_type!=TYPE_DOT_DOT_DOT) {
        if (typ->decl_type==TYPE_ARRAY && SizeOfArg(typ->object)!=0 && sym->stg_class==SC_EXTERN) return;
        SetSymLoc(sym);
        CErr(0x414,name);
    }
}

void ChkReference(SYMPTR sym,char *name)
{
    if ((sym->flags&8) && sym->stg_class!=SC_EXTERN) {
        if (!(sym->flags&2)) {
            if (!(sym->flags&0x100)) {
                SetSymLoc(sym);
                CWarn(0x12,0xca,name);
            }
        } else if (!(sym->flags&0x40) && sym->sym_type->decl_type!=TYPE_ARRAY && sym->stg_class!=SC_STATIC) {
            SetSymLoc(sym);
            CWarn(0x10,0xc8,name);
        }
    }
}

void AsgnSegs(SYM_HANDLE handle)
{
    SYM_ENTRY sym;
    for (;handle!=0;handle=sym.handle) {
        SymGet(&sym,handle);
        if (!(sym.flags&SYM_FUNCTION)) {
            AssignSeg(&sym);
            SymReplace(&sym,handle);
        }
    }
}

int SymBucket(SYMPTR sym)
{
    unsigned long size;
    if (sym->flags&SYM_FUNCTION) return 0;
    size=SizeOfArg(sym->sym_type);
    switch (size&7) {
    case 0: return 6;
    case 4: return 5;
    case 1: case 3: case 5: case 7: return size==1 ? 1:2;
    case 2: case 6: return size==2 ? 3:4;
    }
    return 0;
}

void ChkFunction(SYMPTR sym,char *name)
{
    TYPEPTR typ;
    if (sym->stg_class!=SC_STATIC) return;
    if (sym->flags&0x80) {
        if (CompFlags.using_overlays) CWarn(27,0x77,name);
    } else if (!CompFlags.pcode_was_generated && !(sym->attrib&0x0c) &&
               (TargetSwitches&8) && !CompFlags.multiple_code_segments) {
        sym->flags|=4;
        sym->attrib|=4;
        /* The original reloads the symbol's type here rather than advancing
         * through a typedef. Keep that old behavior; ordinary function
         * symbols have a direct function type. */
        do { typ=sym->sym_type; } while (typ->decl_type==TYPE_TYPEDEF);
        sym->sym_type=FuncNode(typ->object,typ->decl_flags|4,typ->u.parms);
    }
}

void ChkDefined(SYMPTR sym,char *name)
{
    if (sym->flags&8) {
        if (sym->stg_class==SC_STATIC && !(sym->flags&2) && !(sym->flags&0x100)) {
            SetSymLoc(sym);
            CWarn(18,0xca,name);
        }
    } else if (sym->flags&2) {
        if (sym->stg_class==SC_STATIC) {
            if (sym->flags&SYM_FUNCTION) {
                SetSymLoc(sym);
                CErr(0x40b,name);
            }
        } else if (sym->stg_class==0x73) {
            SetSymLoc(sym);
            sym->stg_class=SC_EXTERN;
            CWarn(20,0x12d,name);
        }
    }
}

SYMPTR SymGetPtr(SYM_HANDLE handle)
{
    ++SymStats.getptr;
    if (handle==CurFuncHandle) return &CurFuncSym;
    if (handle<PCH_MaxSymHandle) return PCH_SymArray[handle];
    if (handle!=Cached_sym_num) SymAccess(handle);
    return Cached_sym_addr;
}
