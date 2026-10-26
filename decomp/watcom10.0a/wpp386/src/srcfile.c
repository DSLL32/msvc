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

/* bld/plusplus/c/srcfile.c at initial commit6b9cb443. The actual
 * 1994 wrapper avoids closing stdin and tail-calls the CRT otherwise. */
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "frontend.h"
#include "compiler.h"
#include "carve.h"
#include "scan.h"
#include "scanchar.h"
#include "pending.h"
#include "macro.h"
#include "cmdln-options.h"
#include "cmdline.h"
#include "srcfile.h"
#include "cgio.h"
#include "sysdep.h"
#include <recon.h>

int SrcFileFClose(FILE *fp)
{
    if (fp == stdin) return 0;
    return fclose(fp);
}

/* Actual 1994 record sizes precede the later source's expanded SRCFILE and
 * OPEN_FILE layouts. Recover their fields when their objects are consumed. */
static carve_t carveSrcFile, carveOpenFile;
void cleanupOpenFiles(void);

void srcFileInit(INITFINI *defn)
{
    (void)defn;
    carveSrcFile = CarveCreate(53, 16);
    carveOpenFile = CarveCreate(32, 8);
    CMemRegisterCleanup(cleanupOpenFiles);
}

/* Reached1994 layouts: 32-byte OPEN_FILE and53-byte SRCFILE.
 * The final two SRCFILE bytes have not yet been consumed. */
#pragma pack(1)
typedef struct open_file {
    unsigned line, column;
    int currc;
    char *nextc, *lastc, *buff;
    FILE *fp;
    unsigned long pos;
} OPEN_FILE;
struct src_file {
    SRCFILE sister, parent, unique, pch_child;
    unsigned parent_locn;
    OPEN_FILE *active;
    unsigned long time_stamp;
    char *name, *full_name, *ifndef_name;
    MACRO_STATE macro_state;
    unsigned char guard_state;
    unsigned char lib_inc:1, primary:1, alias:1, cmdline:1,
        cmdlneol:1, cmdlneof:1, uncached:1, free:1;
    unsigned char pch_create:1, unclassified_flags:7;
    unsigned char unclassified_33[2];
};
#pragma pack()
static SRCFILE srcFile;
static OPEN_FILE *openFile, *freeFiles;
static char notFilled;
SRCFILE srcFileAlloc(void *fp, char *name);
void set_srcFile(SRCFILE src);
void setGuardState(unsigned state);
void EmitLineNL(unsigned line, char *name);
int SrcFileClose(int close_top_file);
void popSrcFile(SRCFILE file, OPEN_FILE *active);
void PCHeaderCreate(char *name);

int (*NextChar)(void)=GetNextChar;
static int lastChar;
FILE *SrcFileFOpen(char *name, unsigned kind);
unsigned getGuardState(void);
int readBuffer(int close_top_file);
int tryBackSlashNewLine(OPEN_FILE *active);
int getTestCharFromFile(OPEN_FILE **active);
unsigned char translateTriGraph(unsigned char c);
int getCharAfterOneQuestion(void);
int getCharAfterTwoQuestion(void);
int tryTrigraphAgain(void);
int getCharCheck(OPEN_FILE *active, int c)
{
    int next, third, translated;
    if (c!='?') {
        switch (c) {
        case 0:
            if (active->nextc==active->lastc+1) {
                if (!readBuffer(getGuardState()==2)) return GetNextChar();
                return CurrChar;
            }
            break;
        case '\n': ++active->line; active->column=0; break;
        case '\t': active->column+=9-((active->column+1)&7); break;
        case '\\':
            ++active->column;
            CurrChar=c;
            return tryBackSlashNewLine(active);
        case '\r': break;
        default: ++active->column; break;
        }
        CurrChar=c;
        return c;
    }
    ++active->column;
    next=getTestCharFromFile(&active);
    if (next!='?') {
        NextChar=getCharAfterOneQuestion;
        lastChar=next;
        CurrChar=c;
        return c;
    }
    third=getTestCharFromFile(&active);
    translated=translateTriGraph((unsigned char)third);
    if (third!=translated) {
        CurrChar=translated;
        active->column+=2;
        if (translated=='\\') return tryBackSlashNewLine(active);
        return translated;
    }
    lastChar=translated;
    CurrChar=c;
    NextChar=translated=='?' ? tryTrigraphAgain : getCharAfterTwoQuestion;
    return c;
}
int GetNextChar(void)
{
    OPEN_FILE *active=openFile;
    int c=(unsigned char)*active->nextc++;
    if (!(CharSet[c]&C_EX)) {
        CurrChar=c;
        ++active->column;
        return c;
    }
    return getCharCheck(active,c);
}
int SrcFileCmdLnGetChar(void)
{
    int next_char;
    if (srcFile->cmdlneof) next_char=256;
    else if (srcFile->cmdlneol) {
        next_char='\n';
        srcFile->cmdlneof=1;
    } else if (CmdScanSwEnd()) {
        next_char='\r';
        srcFile->cmdlneol=1;
    } else {
        next_char=CmdScanChar();
    }
    CurrChar=next_char;
    return next_char;
}
void SrcFileCmdLnDummyOpen(void)
{
    SRCFILE file;
    file=srcFileAlloc(NULL,NULL);
    file->cmdline=1;
    setGuardState(2);
}
void SrcFileCmdLnDummyClose(void)
{
    SRCFILE file=srcFile;
    SrcFileClose(0);
    CarveFree(carveSrcFile,file);
}

SRCFILE srcFileAlloc(void *fp, char *name)
{
    SRCFILE fresh;
    OPEN_FILE *active;
    fresh=(SRCFILE)CarveAlloc(carveSrcFile);
    fresh->full_name=NULL;
    fresh->name=name;
    fresh->unique=NULL;
    fresh->pch_child=NULL;
    fresh->lib_inc=0; fresh->primary=0; fresh->alias=0;
    fresh->cmdline=0; fresh->cmdlneol=0; fresh->cmdlneof=0;
    fresh->uncached=0; fresh->free=0;
    fresh->active=NULL;
    fresh->ifndef_name=NULL;
    fresh->sister=fresh;
    fresh->parent=srcFile;
    fresh->pch_create=0;
    MacroStateGet(&fresh->macro_state);
    if (srcFile==NULL) {
        fresh->parent_locn=0;
    } else {
        if (srcFile->pch_create) srcFile->pch_child=fresh;
        fresh->parent_locn=openFile->line;
        openFile->currc=CurrChar;
    }
    active=(OPEN_FILE *)RingPop((void **)&freeFiles);
    if (active==NULL) {
        active=(OPEN_FILE *)CarveAlloc(carveOpenFile);
        active->buff=NULL;
    }
    fresh->active=active;
    active->line=1;
    active->column=0;
    active->nextc=&notFilled;
    active->lastc=&notFilled;
    active->fp=(FILE *)fp;
    fresh->guard_state=7;
    fresh->time_stamp=0;
    fresh->free=0;
    set_srcFile(fresh);
    if (CompFlags.cpp_output) EmitLineNL(1,fresh->name);
    return fresh;
}

void set_srcFile(SRCFILE src)
{
    srcFile=src;
    if (src) openFile=src->active;
    else openFile=NULL;
}

void setGuardState(unsigned state)
{
    SRCFILE file=srcFile;
    while (file->alias) file=file->sister;
    file->guard_state=(unsigned char)state;
}

void EmitLineNL(unsigned line, char *name)
{
    (void)line; (void)name; STUB_TRAP();
}

void SrcFileGuardStateSig(void)
{
    SRCFILE file;
    if (PPState==0) {
        file=srcFile;
        while (file->alias) file=file->sister;
        if (file->guard_state!=4) file->guard_state=1;
    }
}

void SrcFileCurrentLocation(void)
{
    SRCFILE file;
    TokenLine=openFile->line;
    TokenColumn=openFile->column;
    if (PPState==0) {
        file=srcFile;
        while (file->alias) file=file->sister;
        if (file->guard_state!=4) file->guard_state=1;
    }
}

void SrcFileLineLocation(void)
{
    TokenLine=openFile->line;
    TokenColumn=openFile->column;
}

int SrcFileClose(int close_top_file)
{
    SRCFILE old, actual, child;
    OPEN_FILE *active;
    unsigned lines;
    int result;
    if (CompFlags.scanning_c_comment) {
        SrcFileLineLocation();
        CErr1(51);
    }
    old=srcFile;
    PpStartFile();
    active=old->active;
    if (active->fp) {
        SrcFileFClose(active->fp);
        active->fp=NULL;
    }
    actual=old;
    while (actual->alias) actual=actual->sister;
    if (actual->guard_state>1) {
        if (actual->guard_state==5)
            actual->guard_state=actual->ifndef_name ? 2 : 0;
        else actual->guard_state=1;
    }
    if (old->cmdline) {
        popSrcFile(old,active);
        CurrChar=256;
        result=0;
    } else {
        lines=active->line-1;
        if (old->parent==NULL) {
            if (close_top_file) popSrcFile(old,active);
            CurrChar=256;
            SrcLineCount+=lines;
            result=0;
        } else {
            popSrcFile(old,active);
            active=openFile;
            IncLineCount+=lines;
            CurrChar=active->currc;
            if (CompFlags.cpp_output) EmitLineNL(active->line+1,srcFile->name);
            result=1;
        }
    }
    if (srcFile && srcFile->pch_create) {
        srcFile->pch_create=0;
        child=srcFile->pch_child;
        srcFile->pch_child=NULL;
        SrcFileLineLocation();
        PCHeaderCreate(child->name);
    }
    return result;
}

void popSrcFile(SRCFILE file, OPEN_FILE *active)
{
    RingPush((void **)&freeFiles,active);
    file->active=NULL;
    set_srcFile(file->parent);
}

void PCHeaderCreate(char *name) { (void)name; STUB_TRAP(); }


SRCFILE srcFileAddUnique(SRCFILE current);
void setJustOpenedGuardState(void)
{
    setGuardState(CompFlags.cpp_output_requested ? 1 : 3);
}
void setReOpenedGuardState(unsigned state) { (void)state; STUB_TRAP(); }

/* The1994 body has no result move/return, unlike later SrcFileOpen.
 * Its reached callers consume only the installed srcFile/openFile state. */
void SrcFileOpen(FILE *stream, char *name)
{
    SRCFILE fresh, old;
    OPEN_FILE *active;
    fresh=srcFileAlloc(stream,FNameAdd(name));
    active=openFile;
    if (!active->buff && stream) active->buff=(char *)CMemAlloc(4096+1);
    if (!stream) setGuardState(2);
    else {
        old=srcFileAddUnique(fresh);
        if (!old) {
            fresh->time_stamp=SysFileTime(stream);
            setJustOpenedGuardState();
        } else {
            fresh->time_stamp=old->time_stamp;
            while (old->alias) old=old->sister;
            setReOpenedGuardState(old->guard_state);
        }
    }
    CurrChar='\n';
}
int SrcFilesOpen(void)
{
    return srcFile && !srcFile->cmdline;
}
SRCFILE SrcFileCurrent(void)
{
    return srcFile;
}
char *SrcFileName(SRCFILE file) { (void)file; STUB_TRAP(); return 0; }
SRCFILE SrcFileIncluded(SRCFILE file, unsigned *line)
{ (void)file; (void)line; STUB_TRAP(); return 0; }
void SrcFileCommand(void) { STUB_TRAP(); }
void SrcFileLibrary(void) { STUB_TRAP(); }

static SRCFILE srcFilesUnique;

SRCFILE srcFileGetUnique(const char *name)
{
    SRCFILE search;
    for (search=srcFilesUnique;
         search!=0 && strcmp(search->name,name)!=0;
         search=search->unique) {}
    return search;
}

int MacroExists(char *name);
int SrcFileProcessOnce(char *name)
{
    SRCFILE old;
    old=srcFileGetUnique(name);
    if (old!=0 && old->guard_state==2 && MacroExists(old->ifndef_name)) {
        SrcFileOpen(0,name);
        return 1;
    }
    return 0;
}
void SrcFilePrimary(void)
{
    srcFile->primary=1;
}
void SrcFileSetCreatePCHeader(void) { STUB_TRAP(); }

int MacroStateMatchesCurrent(MACRO_STATE *state);

int recursiveIncludeDetected(char *name)
{
    SRCFILE current;
    for (current=srcFile; current!=0; current=current->parent) {
        if (!MacroStateMatchesCurrent(&current->macro_state)) break;
        if (strcmp(name,current->name)==0) {
            CErr2p(710,name);
            return 1;
        }
    }
    return 0;
}
int srcFileCacheClose(int source_file)
{ (void)source_file; STUB_TRAP(); return 0; }

FILE *SrcFileFOpen(char *name, unsigned kind)
{
    static char *file_kind[]={ "rb", "r", "rb", "w", "wb" };
    FILE *file;
    char *mode;
    unsigned index=(unsigned char)kind;
    if (index>=5) { STUB_TRAP(); return 0; }
    mode=file_kind[index];
    for (;;) {
        file=fopen(name,mode);
        if (file!=0) {
            if (recursiveIncludeDetected(name)) {
                fclose(file);
                file=0;
            }
            break;
        }
        /* Actual old Watcom errno values: ENOMEM5/ENFILE10/EMFILE11. */
        if (errno!=5 && errno!=10 && errno!=11) break;
        if (!srcFileCacheClose(index==0)) break;
    }
    return file;
}

SRCFILE srcFileAddUnique(SRCFILE current)
{
    SRCFILE search;
    search=srcFileGetUnique(current->name);
    if (!search) {
        current->unique=srcFilesUnique;
        srcFilesUnique=current;
    }
    return search;
}

void SrcFileGetTokenLocn(CGIO_LOCATION *position)
{
    position->src_file=srcFile;
    position->line=TokenLine;
    position->column=TokenColumn;
}

char *SrcFileNameCurrent(void) { STUB_TRAP(); return NULL; }
unsigned SrcFileLine(void) { STUB_TRAP(); return 0; }

unsigned getGuardState(void)
{
    SRCFILE file=srcFile;
    while (file->alias) file=file->sister;
    return file->guard_state;
}
int SysRead(int handle, void *buffer, unsigned size);
long SysSeek(int handle, long offset);
void __cdecl CErr(unsigned number, ...);
int readBuffer(int close_top_file)
{
    OPEN_FILE *active=openFile;
    SRCFILE file;
    int amount;
    for (;;) {
        file=srcFile;
        while (file->alias) file=file->sister;
        if (file->uncached) {
            active->fp=SrcFileFOpen(file->name,0);
            file->uncached=0;
            if (!active->fp) CErr(626,file->name,strerror(errno));
            else SysSeek(fileno(active->fp),active->pos);
        }
        if (active->fp) {
            if (close_top_file) close_top_file=0;
            else {
                active->nextc=active->buff;
                amount=SysRead(fileno(active->fp),active->nextc,4096);
                if (amount>0) {
                    active->lastc=active->buff+amount;
                    *active->lastc=0;
                    return 0;
                }
                if (amount==-1) CErr(626,file->name,strerror(errno));
            }
        }
        if (!SrcFileClose(0)) return 1;
        active=openFile;
        if (active->nextc<active->lastc) return 1;
    }
}
int tryBackSlashNewLine(OPEN_FILE *active)
{ (void)active; STUB_TRAP(); return 0; }
int getTestCharFromFile(OPEN_FILE **active)
{
    OPEN_FILE *file;
    int c;
    for (;;) {
        file=openFile;
        c=(unsigned char)*file->nextc++;
        if (c || file->nextc!=file->lastc+1) break;
        if (readBuffer(getGuardState()==2)) { c=CurrChar; break; }
    }
    *active=openFile;
    return c;
}
unsigned char translateTriGraph(unsigned char c)
{ (void)c; STUB_TRAP(); return 0; }
int getCharAfterOneQuestion(void)
{
    OPEN_FILE *active=openFile;
    int c=lastChar;
    NextChar=GetNextChar;
    if (c=='?') {
        ++active->column;
        CurrChar=c;
        return c;
    }
    return getCharCheck(active,c);
}
int getCharAfterTwoQuestion(void) { STUB_TRAP(); return 0; }
int tryTrigraphAgain(void) { STUB_TRAP(); return 0; }

void __cdecl CErr(unsigned number, ...)
{ (void)number; STUB_TRAP(); }

unsigned IfDepthInSrcFile(void);
void srcFileGuardReject(void) { setGuardState(1); }
void SrcFileGuardPpIf(void)
{
    unsigned state=getGuardState();
    if (state!=1 && state!=0 && !IfDepthInSrcFile()) {
        if (state==3) setGuardState(4);
        else srcFileGuardReject();
    }
}
char *CStrSave(const char *name);
void SrcFileGuardPpIfndef(char *name)
{
    SRCFILE file=srcFile;
    while (file->alias) file=file->sister;
    if (!file->ifndef_name) file->ifndef_name=CStrSave(name);
}

int SrcFileGuardedIf(void) { STUB_TRAP(); return 0; }

void SrcFileGuardPpElse(void)
{
    if (IfDepthInSrcFile()<=1) srcFileGuardReject();
}
void SrcFileGuardPpEndif(void)
{
    if (!IfDepthInSrcFile() && getGuardState()==4) setGuardState(5);
}

int IsSrcFileLibrary(void) { STUB_TRAP(); return 0; }

void SrcFileResetTokenLocn(CGIO_LOCATION *location)
{ (void)location; STUB_TRAP(); }
