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
* Description:  QNX386 format options and old segment-protection rules.
*
****************************************************************************/

#include <string.h>
#include "cmdline.h"
#include <recon.h>

static parse_entry QNXFormats[] = {
    { "FLat", ProcQNXFlat, 0x4000 },
    { 0, 0, 0 }
};

unsigned char ProcQNX( void )
{
    if( !ProcOne( QNXFormats, SEP_NO, 0 ) ) HintFormat( 0x0200 );
    return 1;
}

void SetQNXFmt( void )
{
    Extension = 14;
    FmtData.u.qnx.flags = 0;
    FmtData.u.qnx.priv_level = 0;
    FmtData.u.qnx.heapsize = 4096;
    FmtData.u.qnx.seg_flags = 0;
    FmtData.u.qnx.resource = 0;
    FmtData.u.qnx.reloc_flags |= 1;
    FmtData.u.qnx.reloc_flags &= ~2;
    ChkBase4K();
}

/* Distinct from the old OS/2 64K base checker. */
void ChkBase4K( void )
{
    if( FmtData.base != 0xffffffffUL && (FmtData.base & 4095) != 0 ) {
        LnkMsg( 0x7a62, 0 );
        FmtData.base = (FmtData.base + 4095) & ~4095UL;
    }
}

unsigned char ProcQNXFlat( void )
{
    return 1;
}

unsigned char ProcQNXHeapSize( void )
{
    unsigned short value;
    unsigned char ret;
    if( !HaveEquals( 0 ) ) return 0;
    ret = getatoi( &value );
    if( ret != 0 || value == 0 ) LnkMsg( 0x7a3c, "s", "heapsize" );
    else FmtData.u.qnx.heapsize = value;
    return 1;
}

unsigned char ProcLonglived( void )
{
    FmtData.u.qnx.flags |= 1;
    return 1;
}

unsigned char ProcLinearRelocs( void )
{
    FmtData.u.qnx.reloc_flags |= 2;
    return 1;
}

unsigned char ProcPrivilege( void )
{
    unsigned short value;
    unsigned char ret;
    if( !HaveEquals( 0 ) ) return 0;
    ret = getatoi( &value );
    if( ret != 0 || value > 3 ) LnkMsg( 0x7a3c, "s", "privilege" );
    else FmtData.u.qnx.priv_level = value;
    return 1;
}

unsigned char ProcQNXResource( void )
{
    if( GetToken( SEP_EQUALS, 3 ) ) FmtData.u.qnx.flags |= 0x80;
    else if( !GetToken( SEP_NO, 1 ) ) return 0;
    FmtData.u.qnx.resource = tostring();
    return 1;
}

unsigned char ProcQNXSegment( void )
{
    return ProcArgList( GetQNXSegFlags, 1 );
}

/* Both old tables use the same three-byte constant-return callback. */
static parse_entry QNXSegDesc[] = {
    { "Class", ProcQNXFlat, 0x4200 },
    { 0, 0, 0 }
};
static parse_entry QNXSegModel[] = {
    { "EXECUTEOnly", ProcQNXExecuteonly, 0x4200 },
    { "EXECUTERead", ProcQNXExecuteread, 0x4200 },
    { "READOnly", ProcQNXReadOnly, 0x4200 },
    { "READWrite", ProcQNXReadWrite, 0x4200 },
    { 0, 0, 0 }
};

unsigned char GetQNXSegFlags( void )
{
    unsigned char isclass;
    seg_flags *entry;

    Token.thumb = 1;
    isclass = ProcOne( QNXSegDesc, SEP_NO, 0 );
    if( !GetToken( SEP_NO, 1 ) ) return 0;
    entry = ChkLAlloc( sizeof(*entry) );
    entry->flags = 0;
    entry->name = tostring();
    entry->type = isclass;
    entry->next = FmtData.u.qnx.seg_flags;
    FmtData.u.qnx.seg_flags = entry;
    return ProcOne( QNXSegModel, SEP_NO, 0 );
}

unsigned char ProcQNXExecuteread( void )
{
    FmtData.u.qnx.seg_flags->flags = 2;
    return 1;
}

unsigned char ProcQNXReadOnly( void )
{
    FmtData.u.qnx.seg_flags->flags = 1;
    return 1;
}

unsigned char ProcQNXReadWrite( void )
{
    FmtData.u.qnx.seg_flags->flags = 0;
    return 1;
}

void CmdQNXFini( void )
{
    if( !(FmtData.u.qnx.reloc_flags & 3) ) LinkState &= ~1;
}

void FreeQNXFmt( void )
{
    FreeSegFlags( FmtData.u.qnx.seg_flags );
    _LnkFree( FmtData.u.qnx.resource );
}
