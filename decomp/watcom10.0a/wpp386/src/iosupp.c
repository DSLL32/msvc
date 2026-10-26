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
#include "frontend.h"
#include "compiler.h"
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
