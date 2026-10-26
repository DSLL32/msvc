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
* Description:  Phar Lap format selection and runtime options.
*
****************************************************************************/

#include "cmdline.h"
#include <recon.h>

void SetPharFmt( void )
{
    rtpblock *params;
    Extension = 7;
    LinkState &= ~1;
    params = ChkLAlloc( sizeof(*params) );
    FmtData.u.phar.params = params;
    FmtData.u.phar.mindata = 0;
    FmtData.u.phar.maxdata = 0xffffffffUL;
    FmtData.u.phar.breaksym = 0;
    FmtData.u.phar.stub = 0;
    params->minreal = 0;
    params->maxreal = 0;
    params->callbufs = 0;
    params->realbreak = 0;
    params->minibuf = 1;
    params->maxibuf = 4;
    params->nistack = 4;
    params->istksize = 1;
    params->extender_flags = 0;
    params->unpriv = 0;
    ChkBase4K();
}

static parse_entry PharModels[] = {
    { "EXTended", ProcPharFlat, 0x0800 },
    { "REX", ProcRex, 0x1000 },
    { "SEGmented", ProcPharSegmented, 0x2000 },
    { 0, 0, 0 }
};

unsigned char ProcPharlap( void )
{
    ProcOne( PharModels, SEP_NO, 0 );
    return 1;
}

unsigned char ProcPharSegmented( void )
{
    LinkState |= 1;
    return 1;
}

unsigned char ProcMinData( void )
{
    return GetLong( &FmtData.u.phar.mindata );
}

unsigned char ProcMaxData( void )
{
    return GetLong( &FmtData.u.phar.maxdata );
}

void FreePharFmt( void )
{
    _LnkFree( FmtData.u.phar.breaksym );
    _LnkFree( FmtData.u.phar.params );
    _LnkFree( FmtData.u.phar.stub );
}
