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


/* Actual old i86obj.c segment-definition state, adapted locally from
 * the established WCC body and checked at72117..7217b in this image. */
#include <stddef.h>
#include <recon.h>
struct segdef;
struct lname_cache;
static struct segdef *SegDefs;
static struct lname_cache *NameCache,*NameCacheDumped;
static unsigned NameIndex,GroupIndex,DGroupIndex,FlatGIndex,SegsDefd,BackSegIdx;
static unsigned short CodeSeg;
void InitSegDefs(void)
{
    SegDefs=NULL;
    NameCache=NULL;
    NameCacheDumped=NULL;
    NameIndex=0;
    GroupIndex=0;
    DGroupIndex=0;
    FlatGIndex=0;
    SegsDefd=0;
    CodeSeg=0x7fff;
    BackSegIdx=0x7fff;
}
