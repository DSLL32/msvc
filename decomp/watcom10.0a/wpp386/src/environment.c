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

/* Adapted from bld/cg/c/envvar.c at initial Open Watcom commit
 * 6b9cb4438959d845a2b4dd4bf19f7908ecd608e9. Actual 1994 CF0xa1599..0xa15ad
 * calls CRT getenv directly, rather than the later FEGetEnv wrapper.
 * The length parameter is unused in the original; CopyStr has no bound.
 * This 20-byte body remains a candidate until a fresh frontier pair passes. */
#include <stdlib.h>
#include "memory.h"
#include <recon.h>

unsigned char GetEnvVar(char *name, char *buffer, int length)
{
    char *value;

    (void)length;
    value = getenv(name);
    if (!value) return 0;
    CopyStr(value, buffer);
    return 1;
}
