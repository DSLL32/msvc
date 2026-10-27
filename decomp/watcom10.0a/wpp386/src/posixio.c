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
* Description:  Code-generator type identities, aliases and scalar definitions.
*
****************************************************************************/


#include "codegen.h"
#include "memory.h"
#include "dbinfo.h"
#include "pending.h"
#include <recon.h>
static char ObjName[128];
static int ObjFile;
struct obj_buffer;
static struct obj_buffer *BufList,*CurBuf;
int CreateStream(char *name);
struct obj_buffer *NewBuffer(void);
unsigned char CGOpenf(void)
{
    CopyStr(FEAuxInfo(NULL,4),ObjName);
    ObjFile=CreateStream(ObjName);
    if (ObjFile==0) return 0;
    BufList=NewBuffer();
    CurBuf=BufList;
    return 1;
}
int CreateStream(char *name) { (void)name;STUB_TRAP();return 0; }
struct obj_buffer *NewBuffer(void) { STUB_TRAP();return 0; }
