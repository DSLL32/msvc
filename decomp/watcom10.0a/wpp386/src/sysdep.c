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

/* Initial sysdep.c SysFileTime, exact1994 public fstat/mtime behavior.
 * The original fileno SDK macro reads FILE26.handle; native recon.h supplies
 * the public host descriptor boundary without overlaying either FILE type. */
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <io.h>
#include "sysdep.h"
#include "pending.h"
#include <recon.h>

time_t SysFileTime(FILE *file)
{
    struct stat info;
    if (fstat(fileno(file),&info)) return 0;
    return info.st_mtime;
}

int SysRead(int handle, void *buffer, unsigned size)
{
    return read(handle,buffer,size);
}
long SysSeek(int handle, long offset)
{ (void)handle; (void)offset; STUB_TRAP(); return 0; }
