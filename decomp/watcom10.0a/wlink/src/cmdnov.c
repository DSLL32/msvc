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

#include <string.h>
#include <time.h>
#include "cmdline.h"
#include <recon.h>

#ifdef __WATCOMC__
#pragma intrinsic(strlen,memcpy)
#endif

static parse_entry NovModels[] = {
    { "NLM", ProcNLM, 0x0100 },
    { "LAN", ProcLAN, 0x0100 },
    { "DSK", ProcDSK, 0x0100 },
    { "NAM", ProcNAM, 0x0100 },
    { 0, 0, 0 }
};

unsigned char ProcNovell( void )
{
    if( !ProcOne( NovModels, SEP_NO, 0 ) ) ProcNLM();
    if( !GetToken( SEP_QUOTE, 1 ) ) {
        FmtData.u.nov.description = 0;
        return 1;
    }
    if( Token.len > 127 ) LnkMsg( 0x7b3b, "s", "description" );
    else FmtData.u.nov.description = tostring();
    return 1;
}

void SetNovFmt( void )
{
    Extension = 8;
    if( !(LinkState & 0x20) && Name != 0 ) {
        FmtData.u.nov.description = FileName( Name, strlen( Name ), 8,
                                               CmdFlags & 0x10 );
    }
}

unsigned char ProcNLM( void )
{
    Extension = 8;
    FmtData.u.nov.moduletype = 0;
    return 1;
}

static parse_entry NovDBIOptions[] = {
    { "ONLyexports", ProcNovDBIExports, 0x0100 },
    { "REFerenced", ProcNovDBIReferenced, 0x0100 },
    { 0, 0, 0 }
};

unsigned char ProcNovDBI( void )
{
    LinkFlags |= 0x800;
    if( ProcOne( NovDBIOptions, SEP_NO, 0 ) ) {
        while( ProcOne( NovDBIOptions, SEP_COMMA, 0 ) );
    }
    return 1;
}

unsigned char ProcNovImport( void )
{
    return ProcArgList( GetNovImport, 1 );
}

unsigned char GetNovImport( void )
{
    symbol *sym;
    sym = SymXOp( 0x26, Token.this, Token.len );
    if( sym != 0 && sym->u.import == 0 ) {
        sym->info = (sym->info & 0xfff0) | 0x4006;
        SetNovImportSymbol( sym );
    }
    return 1;
}

void SetNovImportSymbol( symbol *sym )
{
    sym->u.import = (dll_sym_info *)&FmtData.u.nov;
}

unsigned char ProcNovExport( void )
{
    return ProcArgList( GetNovExport, 1 );
}

unsigned char GetNovExport( void )
{
    symbol *sym;
    sym = SymXOp( 0x12, Token.this, Token.len );
    sym->info |= 0x4000;
    AddNameTable( Token.this, Token.len, 1, &FmtData.u.nov.exports );
    return 1;
}

unsigned char ProcStart( void )
{
    if( !GetToken( SEP_EQUALS, 1 ) ) return 0;
    FmtData.u.nov.startfn = tostring();
    return 1;
}

unsigned char ProcExit( void )
{
    if( !GetToken( SEP_EQUALS, 1 ) ) return 0;
    FmtData.u.nov.exitfn = tostring();
    return 1;
}

unsigned char ProcCheck( void )
{
    if( !GetToken( SEP_EQUALS, 1 ) ) return 0;
    FmtData.u.nov.checkfn = tostring();
    return 1;
}

unsigned char ProcScreenName( void )
{
    if( !GetToken( SEP_NO, 1 ) ) return 0;
    if( Token.len > 71 ) LnkMsg( 0x7a3b, "s", "screenname" );
    else {
        if( FmtData.u.nov.screenname != 0 ) _LnkFree( FmtData.u.nov.screenname );
        FmtData.u.nov.screenname = tostring();
    }
    return 1;
}

unsigned char ProcThreadName( void )
{
    if( !GetToken( SEP_NO, 1 ) ) return 0;
    if( Token.len > 17 ) LnkMsg( 0x7a3b, "s", "threadname" );
    else FmtData.u.nov.threadname = tostring();
    return 1;
}

unsigned char ProcCopyright( void )
{
    time_t thetime;
    unsigned year;
    char *copy_year;
    if( !GetToken( SEP_NO, 1 ) ) {
        if( FmtData.u.nov.copyright != 0 ) _LnkFree( FmtData.u.nov.copyright );
        FmtData.u.nov.copyright = ChkLAlloc( 48 );
        memcpy( FmtData.u.nov.copyright,
            "Copyright 1991 Novell, Inc. All rights reserved", 48 );
        copy_year = FmtData.u.nov.copyright + 14;
        thetime = time( 0 );
        year = localtime( &thetime )->tm_year + 1900;
        while( year > 0 ) {
            *copy_year-- = '0' + year % 10;
            year /= 10;
        }
    } else if( Token.len > 255 ) LnkMsg( 0x7a3b, "s", "copyright" );
    else {
        if( FmtData.u.nov.copyright != 0 ) _LnkFree( FmtData.u.nov.copyright );
        FmtData.u.nov.copyright = tostring();
    }
    return 1;
}

unsigned char ProcMultiLoad( void )
{
    FmtData.u.nov.flags |= 2;
    return 1;
}

unsigned char ProcReentrant( void )
{
    FmtData.u.nov.flags |= 1;
    return 1;
}

unsigned char ProcSynch( void )
{
    FmtData.u.nov.flags |= 4;
    return 1;
}

unsigned char ProcPseudoPreemption( void )
{
    FmtData.u.nov.flags |= 8;
    return 1;
}

unsigned char ProcModule( void )
{
    return ProcArgList( GetNovModule, 1 );
}

unsigned char GetNovModule( void )
{
    AddNameTable( Token.this, Token.len, 0, &FmtData.u.nov.modules );
    return 1;
}

unsigned char ProcCustom( void )
{
    if( !GetToken( SEP_EQUALS, 3 ) ) return 0;
    FmtData.u.nov.customdata = tostring();
    return 1;
}

unsigned char ProcMessages( void )
{
    if( !GetToken( SEP_EQUALS, 3 ) ) return 0;
    FmtData.u.nov.messages = tostring();
    return 1;
}

unsigned char ProcHelp( void )
{
    if( !GetToken( SEP_EQUALS, 3 ) ) return 0;
    FmtData.u.nov.help = tostring();
    return 1;
}

unsigned char ProcXDCData( void )
{
    if( !GetToken( SEP_EQUALS, 3 ) ) return 0;
    FmtData.u.nov.rpcdata = tostring();
    return 1;
}

void CmdNovFini( void )
{
    if( FmtData.u.nov.description == 0 && Name != 0 ) {
        FmtData.u.nov.description = FileName( Name, strlen( Name ), Extension,
                                                CmdFlags & 0x10 );
    }
}

void FreeNovFmt( void )
{
    _LnkFree( FmtData.u.nov.screenname );
    _LnkFree( FmtData.u.nov.description );
    _LnkFree( FmtData.u.nov.checkfn );
    _LnkFree( FmtData.u.nov.startfn );
    _LnkFree( FmtData.u.nov.exitfn );
    _LnkFree( FmtData.u.nov.customdata );
    _LnkFree( FmtData.u.nov.threadname );
    _LnkFree( FmtData.u.nov.copyright );
    _LnkFree( FmtData.u.nov.messages );
    _LnkFree( FmtData.u.nov.help );
    _LnkFree( FmtData.u.nov.rpcdata );
    _LnkFree( FmtData.u.nov.sharednlm );
    FreeList( (path_entry *)FmtData.u.nov.exports );
    FreeList( (path_entry *)FmtData.u.nov.modules );
}

unsigned char ProcLAN( void )
{
    Extension = 9;
    FmtData.u.nov.moduletype = 1;
    return 1;
}

unsigned char ProcNovDBIExports( void )
{
    FmtData.u.nov.flags |= 0x80;
    return 1;
}

unsigned char ProcNovDBIReferenced( void )
{
    FmtData.u.nov.flags |= 0x20;
    return 1;
}

unsigned char ProcSharelib( void )
{
    if( !GetToken( SEP_EQUALS, TOK_INCLUDE_DOT | TOK_IS_FILENAME ) ) return 0;
    FmtData.u.nov.sharednlm = FileName( Token.this, Token.len, 8, 0 );
    return 1;
}
