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
* Description:  ELF format options and imported-symbol marking.
*
****************************************************************************/

#include "cmdline.h"
#include <recon.h>

void SetELFFmt( void )
{
    Extension = 17;
    FmtData.u.elf.exports = 0;
    FmtData.u.elf.modules = 0;
    FmtData.u.elf.alignment = 4;
}

static parse_entry ELFFormatKeywords[] = {
    { "DLl", ProcELFDLL, 0x8000 },
    { 0, 0, 0 }
};

unsigned char ProcELF( void )
{
    ProcOne( ELFFormatKeywords, SEP_NO, 0 );
    return 1;
}

unsigned char ProcELFAlignment( void )
{
    unsigned char ret;
    unsigned long value, lessone;
    if( !HaveEquals( 0 ) ) return 0;
    ret = getatol( &value );
    if( ret != 0 || value == 0 ) return 0;
    for( ;; ) {
        lessone = value & (value - 1);
        if( lessone == 0 ) break;
        value = lessone;
    }
    FmtData.u.elf.alignment = value;
    return 1;
}

void SetELFImportSymbol( symbol *sym )
{
    sym->u.import = (dll_sym_info *)0xffffffffUL;
}

unsigned char GetELFImport( void )
{
    symbol *sym;
    sym = SymXOp( 0x26, Token.this, Token.len );
    if( sym != 0 && sym->u.import == 0 ) {
        sym->info = (sym->info & 0xfff0) | 0x4006;
        SetELFImportSymbol( sym );
    }
    return 1;
}

unsigned char ProcELFImport( void )
{
    return ProcArgList( GetELFImport, 1 );
}

void FreeELFFmt( void )
{
    FreeList( (path_entry *)FmtData.u.elf.exports );
    FreeList( (path_entry *)FmtData.u.elf.modules );
}

unsigned char ProcELFDLL( void )
{
    FmtData.flags |= 1;
    return 1;
}
