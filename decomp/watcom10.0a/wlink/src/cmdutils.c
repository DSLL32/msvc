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

#include <stdio.h>
#include <string.h>
#include "cmdline.h"
#include <recon.h>

#ifdef __WATCOMC__
#pragma intrinsic(memcpy)
#endif

cmdfilelist *CmdFile;                  /* original441da8 */

extern unsigned SectionNum;          /* original45010c, owned by LnkInit */

section *NewSection( void )
{
    section *sect;

    ++SectionNum;
    sect = ChkLAlloc( sizeof(*sect) );
    sect->next_sect = 0;
    sect->classlist = 0;
    sect->areas = 0;
    sect->files = 0;
    sect->mods = 0;
    sect->reloclist = 0;
    sect->sect_addr.off = 0;
    sect->sect_addr.seg = 0xffff;
    sect->ovl_num = 0;
    sect->parent = 0;
    sect->relocs = 0;
    sect->size = 0;
    sect->outfile = 0;
    sect->u.dist_mods = 0;
    sect->dbg_info = 0;
    return sect;
}

void NewCommandSource( char *name, char *buff, unsigned how )
{
    cmdfilelist *newfile;

    newfile = ChkLAlloc( sizeof(*newfile) );
    newfile->file = fileno( stdin );
    if( CmdFile != 0 ) {
        memcpy( &CmdFile->token, &Token, sizeof(tok) );
        newfile->next = CmdFile->next;
        if( newfile->next != 0 ) newfile->next->prev = newfile;
    } else {
        newfile->next = 0;
    }
    newfile->prev = CmdFile;
    if( newfile->prev != 0 ) newfile->prev->next = newfile;
    CmdFile = newfile;
    CmdFile->name = name;
    CmdFile->token.buff = buff;
    CmdFile->token.how = how;
    Token.buff = buff;
    Token.next = buff;
    Token.where = 0;
    Token.line = 1;
    Token.how = how;
    Token.thumb = 0;
    Token.locked = 0;
    Token.quoted = 0;
}

void EatWhite( void )
{
    while( *Token.next == ' ' || *Token.next == '\t' || *Token.next == '\r' ) {
        ++Token.next;
    }
}

void BurnUtils( void )
{
    cmdfilelist *temp;

    if( CmdFile->next != 0 ) {
        LnkMsg( 0x7b71, 0 );
    }
    while( CmdFile != 0 ) {
        if( CmdFile->file > 0 ) {
            QClose( CmdFile->file, CmdFile->name );
        }
        _LnkFree( CmdFile->name );
        switch( CmdFile->token.how ) {
        case 4:                   /* ENVIRONMENT */
        case 5:                   /* SYSTEM */
            break;
        default:
            _LnkFree( CmdFile->token.buff );
            break;
        }
        temp = CmdFile;
        CmdFile = CmdFile->prev;
        _LnkFree( temp );
    }
}
