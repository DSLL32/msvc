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
* Description:  Source-file and cue tracking for code-generator debug information.
*
****************************************************************************/


#include <string.h>
#include "codegen.h"
#include "cgmemory.h"
#include "dbinfo.h"
#include "procdef.h"
#include "stubs.h"
#include <recon.h>
cue_ctl LineInfo;
cue_idx SrcLine;

/* dbsyms.c before the primary source cue was deferred to the first line. */
void InitDbgInfo(void)
{
    char *name;
    unsigned file;
    SrcFileNoInit();
    DBNested(0);
    SourceCueInit(&LineInfo);
    name=FEAuxInfo(NULL,10);
    file=DBSrcFile(name);
    SrcLine=CueAdd(file,1,1);
    if (Model&0x10000000) DFInitDbgInfo();
    else if (Model&0x08000000) CVInitDbgInfo();
    else WVInitDbgInfo();
}

struct fname_ctl DBFiles;

void SrcFileNoInit(void)
{
    DBFiles.lst=NULL;
    DBFiles.count=0;
}

static char Nested;

char DBNested(char nested)
{
    Nested=nested;
    return nested;
}

void SourceCueInit(cue_ctl *ctl)
{
    ctl->head=NULL;
    ctl->state.cue=0;
    ctl->state.fno=-1;
    ctl->state.line=0;
    ctl->state.col=0;
    ctl->count=0;
    ctl->lnk=&ctl->head;
    ctl->next=&ctl->start[0];
    ctl->end=&ctl->start[1];
}

unsigned DBSrcFile(char *name)
{
    struct fname_lst *file,**link;
    unsigned index,length;
    link=&DBFiles.lst;
    index=0;
    while ((file=*link)!=NULL) {
        if (strcmp(name,file->fname)==0) return index;
        link=&file->next;
        ++index;
    }
    length=strlen(name);
    file=CGAlloc(sizeof(*file)+length);
    file->len=length+1;
    file->next=NULL;
    strcpy(file->fname,name);
    ++DBFiles.count;
    *link=file;
    return index;
}

cue_idx CueAdd(int file,int line,int column)
{
    cue_ctl *ctl;
    int changes,difference;
    if (file==0 && column==1 && line<0x8000) return line;
    ctl=&LineInfo;
    changes=0;
    if (file!=ctl->state.fno) changes|=4;
    if (column!=ctl->state.col) changes|=2;
    difference=line-ctl->state.line;
    if (difference!=0) changes|=1;
    if (changes==1 && difference>=0 && difference<20) {
        ctl->state.line=line;
        ctl->state.cue+=difference;
        changes=0;
    }
    if (changes!=0) {
        if (ctl->next>=ctl->end) AddCueBlk(ctl);
        ++ctl->state.cue;
        ctl->state.fno=file;
        ctl->state.line=line;
        ctl->state.col=column;
        *ctl->next=ctl->state;
        ++ctl->next;
        ++ctl->count;
    }
    return ctl->state.cue+0x8000;
}

unsigned short TypeIdx;
short DbgLocals,DbgTypes;

void WVInitDbgInfo(void)
{
    TypeIdx=0;
    DbgLocals=0;
    DbgTypes=0;
}

void WVObjInitInfo(void)
{
    WVSrcCueLoc();
}

static dbg_patch_handle CueInfoOffset;

void WVSrcCueLoc(void)
{
    temp_buff temp;
    BuffStart(&temp,0x13);
    BuffForward(&CueInfoOffset);
    BuffWord(0);
    EndType(0);
}

static temp_buff *CurrBuff;

void BuffStart(temp_buff *temp,unsigned definition)
{
    CurrBuff=temp;
    temp->buff[1]=definition;
    temp->index=2;
    temp->fix_idx=0;
}

void BuffForward(dbg_patch_handle *handle)
{
    BuffRelocatable(handle,0,0);
}

void BuffRelocatable(void *pointer,unsigned char type,unsigned long offset)
{
    CurrBuff->fix[CurrBuff->fix_idx].pos=CurrBuff->index;
    CurrBuff->fix[CurrBuff->fix_idx].p=pointer;
    CurrBuff->fix[CurrBuff->fix_idx].type=type;
    if (type==0) BuffWord(0);
    else {
        BuffOffset(offset);
        CurrBuff->index+=2;
    }
    ++CurrBuff->fix_idx;
}

void BuffWord(unsigned value)
{
    BuffByte(value&0xff);
    BuffByte(value>>8);
}

void BuffByte(unsigned char value)
{
    if (CurrBuff->index<511) {
        CurrBuff->buff[CurrBuff->index++]=value;
        if (CurrBuff->index>=511) Zoiks(5);
    }
}

void EndType(unsigned char check_size)
{
    if (Model&0x1000000) {
        if (check_size) ChkDbgSegSize(16384,1);
        BuffEnd(DbgTypes);
    }
}

/* This older interface emits OMF line state immediately in the code segment;
 * later dbsyms.c defers that operation by assigning SrcLine alone. */
void DBSrcCue(unsigned file,unsigned line,unsigned column)
{
    unsigned short old;
    cue_idx cue;
    if (!(Model&0x11000000)) {
        if (file==0 && column==1) DBLineNum(line);
    } else {
        cue=CueAdd(file,line,column);
        old=SetOP(AskCodeSeg());
        SrcLine=cue;
        OutSourceLine(cue);
        SetOP(old);
    }
}

void DBLineNum(unsigned line)
{
    unsigned short old;
    old=SetOP(AskCodeSeg());
    SrcLine=(cue_idx)line;
    OutSourceLine((unsigned short)line);
    SetOP(old);
}

void DBAllocReg(name *reg_name,name *opnd) { STUB_TRAP(); }

/* Only the encountered parent-link prefix is established here. */
struct dbg_block { struct dbg_block *parent; };
/* Recovered initial fields of symdbg.h's routine descriptor. No allocation or
 * later debug-member ownership is claimed until its constructor is reached. */
struct dbg_rtn {
    void *reeturn;
    struct dbg_rtn *parent;
    struct dbg_block *blk;
};
typedef char old_debug_block_offset[(offsetof(struct dbg_rtn,blk)==8)?1:-1];
void MkBlock(void) { STUB_TRAP(); }
void AddBlockInfo(struct dbg_block *block,unsigned char start) { STUB_TRAP(); }
void DBBegBlock(void)
{
    if (CurrProc->targ.debug!=NULL) {
        MkBlock();
        AddBlockInfo(((struct dbg_rtn *)CurrProc->targ.debug)->blk,1);
    }
}

void DBEndBlock(void)
{
    struct dbg_block *blk;
    struct dbg_rtn *rtn;
    if (CurrProc->targ.debug!=NULL) {
        rtn=CurrProc->targ.debug;
        blk=rtn->blk;
        AddBlockInfo(blk,0);
        rtn->blk=blk->parent;
    }
}

void CVSetBase(void) { STUB_TRAP(); }
static unsigned long CodeOffset;
unsigned char NeedBaseSet(void);
void BuffBack(struct bck_info *back,long offset) { STUB_TRAP(); }
unsigned short AskOP(void);
void BEFreeBack(void *back);
void WVSetBase(void)
{
    temp_buff temp;
    struct bck_info *back;
    if ((Model&0x02000000U) && NeedBaseSet()) {
        back=BENewBack(NULL);
        back->segment=AskOP();
        OutLabel(back->label);
        CodeOffset=AskAddress(back->label);
        BuffStart(&temp,0x32); /* NEW_BASE + SET_BASE */
        BuffBack(back,0);
        BuffEnd(DbgLocals);
        BEFreeBack(back);
        SetUpObj(0);
    }
}
void DbgSetBase(void)
{
    if (Model&0x10000000U) { /* DWARF has no base action. */ }
    else if (Model&0x08000000U) CVSetBase();
    else WVSetBase();
}

void DbgBlkEnd(void *handle,unsigned long location) { STUB_TRAP(); }

void DbgProEnd(void *handle,unsigned long location) { STUB_TRAP(); }

void DbgRtnEnd(void *handle,unsigned long location) { STUB_TRAP(); }

void DbgRtnBeg(void *handle,unsigned long location) { STUB_TRAP(); }

void DbgBlkBeg(void *handle,unsigned long location) { STUB_TRAP(); }

void DbgEpiBeg(void *handle,unsigned long location) { STUB_TRAP(); }
