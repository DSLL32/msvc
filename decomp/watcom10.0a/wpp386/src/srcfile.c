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
#include "frontend.h"
#include "compiler.h"
#include "carve.h"
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
