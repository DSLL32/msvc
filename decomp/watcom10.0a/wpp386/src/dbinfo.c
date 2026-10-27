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
* Description:  WHEN YOU FIGURE OUT WHAT THIS FILE DOES, PLEASE
*               DESCRIBE IT HERE!
*
****************************************************************************/


/* Locally adapted established WCC dbsyms/source-cue initialization.
 * This image independently preserves the40B cue control, first source cue,
 * three debug backends and its own frontend FEAuxInfo callback identity. */
#include <string.h>
#include "codegen.h"
#include "cgmemory.h"
#include "dbinfo.h"
#include "pending.h"
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
void DFInitDbgInfo(void) { STUB_TRAP(); }
void CVInitDbgInfo(void) { STUB_TRAP(); }
unsigned short TypeIdx;
short DbgLocals,DbgTypes;
void WVInitDbgInfo(void)
{
    TypeIdx=0;
    DbgLocals=0;
    DbgTypes=0;
}

void AddCueBlk(cue_ctl *ctl) { (void)ctl;STUB_TRAP(); }
