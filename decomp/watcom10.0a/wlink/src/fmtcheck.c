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

#include "cmdline.h"
#include <recon.h>

void PreAddrCalcFormatSpec( void )
{
    if( FmtData.type & 0x0010 ) ChkPEData();
    else if( FmtData.type & 0x000f ) ChkOS2Data();
    if( FmtData.type & 0x0100 ) FindExportedSyms();
    if( (FmtData.type & 0x0800) && (LinkState & 0x2000)
        && !(CmdFlags & 0x0800) ) LnkMsg( 0x3285, 0 );
}

extern void AllocPETransferTable( void );
extern void ChkOS2Exports( void );
extern void SetQNXSegFlags( void );

void PostAddrCalcFormatSpec( void )
{
    if( FmtData.type & 0x0010 ) AllocPETransferTable();
    else if( FmtData.type & 0x000f ) ChkOS2Exports();
    if( FmtData.type & 0x4200 ) SetQNXSegFlags();
}
