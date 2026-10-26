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

void SetOS2Fmt( void )
{
    if( LinkState & 0x400 ) return;
    LinkState |= 0x400;
    FmtData.u.os2.flags = 2;
    FmtData.u.os2.heapsize = 0;
    FmtData.u.os2.res_module_name = 0;
    FmtData.u.os2.stub_file_name = 0;
    FmtData.u.os2.os2_seg_flags = 0;
    FmtData.u.os2.description = 0;
    FmtData.u.os2.exports = 0;
    FmtData.u.os2.old_lib_name = 0;
    FmtData.u.os2.segment_shift = 0;
    if( FmtData.type & 8 ) FmtData.def_seg_flags |= 0x40;
    Extension = 4;
    ChkBase();
}

void ChkBase( void )
{
    if( FmtData.base != 0xffffffffUL && (FmtData.base & 0xffffUL) )
        LnkMsg( 0x7a7b, 0 );
}

static parse_entry SubFormats[] = {
    { "DLl", ProcOS2DLL, 0x001f },
    { "FLat", ProcLE, 0x0004 },
    { "LE", ProcLE, 0x0002 },
    { "LX", ProcLE, 0x0004 },
    { "NT", ProcPE, 0x0010 },
    { "PE", ProcPE, 0x0010 },
    { "OLDPE", ProcOldPE, 0x0010 },
    { 0, 0, 0 }
};
static parse_entry OS2FormatKeywords[] = {
    { "PM", ProcPM, 0x0007 },
    { "PMCompatible", ProcPMCompatible, 0x0007 },
    { "FULLscreen", ProcFullScreen, 0x0007 },
    { "PHYSdevice", ProcPhysDevice, 0x0006 },
    { "VIRTdevice", ProcVirtDevice, 0x0006 },
    { 0, 0, 0 }
};
static parse_entry WindowsFormatKeywords[] = {
    { "MEMory", ProcMemory, 0x0008 },
    { "FOnt", ProcFont, 0x0008 },
    { 0, 0, 0 }
};

unsigned char ProcOS2( void )
{
    Extension = 4;
    while( ProcOne( SubFormats, SEP_NO, 0 ) ) {}
    if( FmtData.type & 8 ) {
        if( ProcOne( WindowsFormatKeywords, SEP_NO, 0 ) )
            ProcOne( WindowsFormatKeywords, SEP_NO, 0 );
    } else {
        ProcOne( OS2FormatKeywords, SEP_NO, 0 );
        if( (FmtData.type & 4) && (FmtData.flags & 1) )
            LinkFlags |= 0x08000000UL;
    }
    return 1;
}

static parse_entry NTFormatKeywords[] = {
    { "TNT", ProcTNT, 0x10 },
    { 0, 0, 0 }
};

unsigned char ProcPE( void )
{
    ProcOne( NTFormatKeywords, SEP_NO, 0 );
    FmtData.u.pe.os2.heapsize = 8192;
    FmtData.u.pe.stackcommit = 0xffffffffUL;
    FmtData.u.pe.objalign = 0x10000UL;
    FmtData.u.pe.heapcommit = 0x1000UL;
    return 1;
}

unsigned char ProcRunConsole( void )
{
    FmtData.u.pe.subsystem = 3;
    return 1;
}

unsigned char ProcOS2Export( void )
{
    if( GetToken( SEP_EQUALS, TOK_INCLUDE_DOT ) ) return GetWlibImports();
    return ProcArgList( getexport, 0 );
}

static parse_entry Exp_Keywords[] = {
    { "Resident", ProcExpResident, 0x000f },
    { 0, 0, 0 }
};

unsigned char getexport( void )
{
    entry_export *exp;
    unsigned short value;

    /* Unlike the released version, 1994 AllocExport owns a copied name. */
    exp = AllocExport( tostring() );
    if( GetToken( SEP_PERIOD, TOK_INCLUDE_DOT ) ) {
        if( getatoi( &exp->ordinal ) != 0 ) {
            LnkMsg( 0x7b29, 0 );
            _LnkFree( exp->name );
            _LnkFree( exp );
            GetToken( SEP_EQUALS, TOK_INCLUDE_DOT );
            return 1;
        }
    }
    if( GetToken( SEP_EQUALS, TOK_INCLUDE_DOT ) )
        exp->sym = SymXOp( 0x12, Token.this, Token.len );
    else exp->sym = RefISymbol( exp->name );
    exp->sym->info |= 0x4000;
    if( exp->ordinal == 0 ) exp->flags |= 1;
    exp->next = FmtData.u.os2.exports;
    FmtData.u.os2.exports = exp;
    ProcOne( Exp_Keywords, SEP_NO, 0 );
    FmtData.u.os2.exports = exp->next;
    exp->iopl_words = 0;
    if( !(FmtData.type & 0x18) && GetToken( SEP_NO, TOK_INCLUDE_DOT ) ) {
        if( getatoi( &value ) == 0 ) {
            if( value > 63 ) LnkMsg( 0x7b2a, 0 );
            else exp->iopl_words = value;
        } else Token.thumb = 1;
    }
    AddToExportList( exp );
    return 1;
}

entry_export *AllocExport( char *name )
{
    entry_export *exp;

    exp = ChkLAlloc( sizeof(*exp) );
    /* The original bit-field stores leave unused flag bits uninitialized.
     * Give those undefined bits zero; the four reached flags retain their
     * original exported/resident/movable/anonymous values. */
    exp->flags = 2;
    if( CmdFlags & 0x4000 ) exp->flags |= 8;
    exp->iopl_words = 0;
    exp->ordinal = 0;
    exp->name = name;
    return exp;
}

extern void FreeImpNameTab( void );
extern void FreeExportList( void );
extern void FreeSegFlags( seg_flags * );

void FreeOS2Fmt( void )
{
    _LnkFree( FmtData.u.os2.stub_file_name );
    _LnkFree( FmtData.u.os2.res_module_name );
    _LnkFree( FmtData.u.os2.old_lib_name );
    _LnkFree( FmtData.u.os2.description );
    FreeImpNameTab();
    FreeExportList();
    FreeSegFlags( FmtData.u.os2.os2_seg_flags );
}
