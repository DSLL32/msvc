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

/* bld/plusplus/c/iosupp.c at initial commit
 *6b9cb4438959d845a2b4dd4bf19f7908ecd608e9. Actual1994 ioSuppInit
 * only filters three encountered fallback-path vectors; it tail-enters
 * setPaths on the final vector. Later carve/temp initialization is absent. */
#include <direct.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "frontend.h"
#include "compiler.h"
#include "pending.h"
#include "srcfile.h"
#include "utility.h"
#include <recon.h>

char *pathSrc[] = { "..\\cpp", "..\\c", 0 };
char *pathHdr[] = { "..\\h", "..\\include", 0 };
char *pathCmd[] = { "..\\occ", 0 };

void ioSuppInit(INITFINI *entry)
{
    (void)entry;
    setPaths(pathSrc);
    setPaths(pathHdr);
    setPaths(pathCmd);
}

void setPaths(char **paths)
{
    char **dest, **test;
    char *path;

    dest = paths;
    test = paths;
    for (;;) {
        path = *test;
        if (!path) break;
        if (pathExists(path)) *dest++ = path;
        ++test;
    }
    *dest = 0;
}

int pathExists(char *path)
{
    DIR *directory;
    int exists;

    exists = 0;
    directory = opendir(path);
    if (directory) {
        closedir(directory);
        exists = 1;
    }
    return exists;
}

void IoSuppSetLineBuffering(FILE *fp, unsigned size)
{
    set_buffering(fp, size, _IOLBF);
}

/* The 1994 code predates BUF_ALLOC rings: this buffer has the permanent
 * allocator's lifetime and is supplied directly to the CRT. */
void set_buffering(FILE *fp, unsigned size, int mode)
{
    char *buffer;

    buffer = (char *)CPermAlloc(size);
    setvbuf(fp, buffer, mode, size);
}

int IoSuppCloseFile(FILE **file)
{
    if (*file == NULL) return 0;
    SrcFileFClose(*file);
    *file = NULL;
    return 1;
}

#pragma pack(1)
struct path_descr {
    char buffer[260+3];
    char *drv, *dir, *fnm, *ext;
};
#pragma pack()

void splitFileName(char *filename, struct path_descr *path)
{
    _splitpath2(filename,path->buffer,&path->drv,&path->dir,&path->fnm,&path->ext);
}

void HFileListStart(void);
void HFileListNext(char *prefix);
FILE *SrcFileFOpen(char *name, unsigned kind);
int PCHeaderAbsorb(char *name)
{ (void)name; STUB_TRAP(); return 0; }

int openSrc(char *name, int kind)
{
    FILE *file;
    if (SrcFileProcessOnce(name)) {
        SrcFileOpen(0,name);
        return 1;
    }
    file=SrcFileFOpen(name,0);
    if (file==0) return 0;
    if (CompFlags.watch_for_pcheader) {
        CompFlags.watch_for_pcheader=0;
        if (PCHeaderAbsorb(name)) {
            SrcFileSetCreatePCHeader();
            SrcFileOpen(file,name);
        } else {
            SrcFileOpen(0,name);
            fclose(file);
        }
    } else {
        SrcFileOpen(file,name);
        if ((unsigned char)kind==0) SrcFilePrimary();
    }
    return 1;
}

char *openExt(char *extension, struct path_descr *path, int kind)
{
    char name[260], *result;
    _makepath(name,path->drv,path->dir,path->fnm,extension);
    result=extension!=0 ? extension : "";
    if (!openSrc(name,(unsigned char)kind)) result=0;
    return result;
}

char *openSrcExts(char **extensions, struct path_descr *path, int kind)
{
    char *extension;
    if (!path->ext[0]) {
        extension=openExt(0,path,(unsigned char)kind);
        if (extension==0) {
            for (;;) {
                extension=*extensions++;
                if (extension==0) break;
                extension=openExt(extension,path,(unsigned char)kind);
                if (extension!=0) break;
            }
        }
    } else extension=openExt(path->ext,path,(unsigned char)kind);
    return extension;
}

char *concSep(char *position, char *buffer)
{
    char previous;
    if (position>buffer) {
        previous=position[-1];
        if (previous!=':' && previous!='/' && previous!='\\')
            *position++='\\';
    }
    return position;
}

void makeDirName(char *buffer, struct path_descr *path)
{
    _makepath(buffer,0,path->dir,path->fnm,path->ext);
}

int openSrcPath(char *prefix, char **extensions, struct path_descr *path, int kind)
{
    struct path_descr expanded;
    char directory[260*2], *position, *extension;
    splitFileName(prefix,&expanded);
    if (!path->drv[0]) position=stpcpy(directory,prefix);
    else if (!expanded.drv[0]) {
        position=stpcpy(directory,path->drv);
        position=stpcpy(position,prefix);
    } else position=0;
    if (position==0) return 0;
    position=concSep(position,directory);
    makeDirName(position,path);
    splitFileName(directory,&expanded);
    extension=openSrcExts(extensions,&expanded,(unsigned char)kind);
    if (extension==0) return 0;
    if (kind==0 && extension!=path->ext) {
        _makepath(directory,path->drv,path->dir,path->fnm,extension);
        WholeFName=FNameAdd(directory);
    }
    return 1;
}

int doIoSuppOpenSrc(struct path_descr *fd, int kind)
{
    static char *extsSrc[]={ ".cpp", ".cc", ".c", 0 };
    static char *extsHdr[]={ ".hpp", ".h", 0 };
    static char *extsCmd[]={ ".occ", 0 };
    char **paths, **exts, *path;
    char buffer[260], previous[260];
    struct path_descr included;
    SRCFILE current;
    unsigned line;
    switch ((unsigned char)kind) {
    case 0:
        if (!fd->fnm[0] && fd->ext[0]=='.' && !fd->ext[1]) {
            if (ErrCount) return 0;
            WholeFName=FNameAdd("stdin");
            SrcFileOpen(stdin,WholeFName);
            goto found;
        }
        paths=pathSrc; exts=extsSrc; break;
    case 1: case 2:
        paths=pathHdr; exts=extsHdr; break;
    case 3:
        paths=pathCmd; exts=extsCmd; break;
    default: STUB_TRAP(); return 0;
    }
    switch ((unsigned char)kind) {
    case 1:
        if (openSrcPath("",exts,fd,kind)) goto found;
        /* The1994 instructions compare the directory POINTER to separator
         * values, not its first byte. Preserve this original source bug. */
        if ((unsigned)fd->dir==':' || (unsigned)fd->dir=='/' ||
            (unsigned)fd->dir=='\\') goto include_paths;
        previous[0]=(char)0xff; previous[1]=0;
        current=SrcFileCurrent();
        while (current!=0) {
            splitFileName(SrcFileName(current),&included);
            _makepath(buffer,included.drv,included.dir,0,0);
            if (strcmp(buffer,previous)!=0 && openSrcPath(buffer,exts,fd,1))
                goto found;
            current=SrcFileIncluded(current,&line);
            strcpy(previous,buffer);
        }
        break;
    case 0: case 3:
        if (openSrcPath("",exts,fd,kind)) goto found;
        break;
    }
include_paths:
    if (kind==1 || kind==2) {
        HFileListStart();
        for (;;) {
            HFileListNext(buffer);
            if (!buffer[0]) break;
            if (openSrcPath(buffer,exts,fd,kind)) goto found;
        }
    }
    if (kind==0 || kind==1 || kind==3) {
        /* Same pointer comparison defect; the original branches to success. */
        if ((unsigned)fd->dir==':' || (unsigned)fd->dir=='/' ||
            (unsigned)fd->dir=='\\') goto found;
        for (;;) {
            path=*paths++;
            if (!path) return 0;
            if (openSrcPath(path,exts,fd,kind)) goto found;
        }
    }
    return 0;
found:
    if (kind==2) SrcFileLibrary();
    else if (kind==3) SrcFileCommand();
    return 1;
}

int IoSuppOpenSrc(char *filename, int kind)
{
    struct path_descr path;
    splitFileName(filename,&path);
    if (doIoSuppOpenSrc(&path,(unsigned char)kind)) return 1;
    if (CompFlags.check_truncated_fnames) {
        if (strlen(path.fnm)<=8) return 0;
        path.fnm[8]=0;
        if (doIoSuppOpenSrc(&path,(unsigned char)kind)) return 1;
    }
    return 0;
}

/* Actual1994 temp block counter3e48d..3e49e; no I/O occurs here. */
static unsigned tempBlock;
unsigned IoSuppTempNextBlock(unsigned count)
{
    unsigned block;
    block=tempBlock+1;
    tempBlock+=count;
    return block;
}

void IoSuppTempRead(unsigned block,unsigned length,void *dest)
{ (void)block; (void)length; (void)dest; STUB_TRAP(); }
