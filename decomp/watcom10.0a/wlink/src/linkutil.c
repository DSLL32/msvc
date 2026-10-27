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
/* Spawn flow independently matched to original VA416d3c..416d6f.
 * The released source supplies the original names and C structure.
 * Original Watcom _setjmp ABI comes from shipped SETJMP.H. */
#include <setjmp.h>
#include <stdlib.h>
#include <string.h>
#include "cmdline.h"
#include "diagnostic.h"
#include <recon.h>

#if defined(__WATCOMC__)
#pragma intrinsic(memcpy,memcmp)
#endif

int SearchPath( char *name )
{
    char buffer[80];
    char *path;
    int file;

    file = QObjOpen( name );
    if( file != -1 ) return file;
    path = getenv( "PATH" );
    if( path != 0 ) {
        while( QMakeFileName( &path, name, buffer ) ) {
            file = QObjOpen( buffer );
            if( file != -1 ) return file;
        }
    }
    return -1;
}

static void *SpawnStack;

int Spawn( void (*fn)(void) )
{
    void *save_env;
    jmp_buf env;
    int status;

    save_env = SpawnStack;
    SpawnStack = env;
    status = setjmp( env );
    if( status == 0 ) {
        (*fn)();
    }
    SpawnStack = save_env;
    return status;
}

void FreeList( path_entry *curr )
{
    path_entry *next;

    while( curr != 0 ) {
        next = curr->next;
        _LnkFree( curr );
        curr = next;
    }
}

void Suicide( void )
{
    if( SpawnStack != 0 ) {
        longjmp( SpawnStack, 1 );
    }
}

void LinkList( void *in_head, void *newnode )
{
    path_entry **owner;

    owner = in_head;
    ((path_entry *)newnode)->next = 0;
    while( *owner != 0 ) owner = &(*owner)->next;
    *owner = newnode;
}

void CheckErr( void )
{
    if( LinkState & 8 ) {
        WriteLibsUsed();
        Suicide();
    }
}

unsigned char TestBit( void *bits, unsigned bit )
{
    return ((unsigned char *)bits)[bit >> 3] & (1U << (bit & 7));
}

unsigned short AddNameTable( char *name, unsigned len, unsigned char is_mod,
    snamelist **owner )
{
    snamelist *imp;
    unsigned short off, index;

    off = 1;
    index = 1;
    for( ;; ) {
        imp = *owner;
        if( imp == 0 ) {
            imp = ChkLAlloc( len + (is_mod ? 6 : 5) );
            imp->next = 0;
            imp->len = len;
            *owner = imp;
            memcpy( imp->name, name, len );
            if( is_mod ) imp->name[len] = 0;
            break;
        }
        if( len == imp->len && memcmp( imp->name, name, len ) == 0 ) break;
        off += imp->len + 1;
        owner = &imp->next;
        ++index;
    }
    return is_mod ? index : off;
}

unsigned short binary_log( unsigned short value )
{
    unsigned short log;

    if( value == 0 ) return 0;
    log = 15;
    while( !(value & 0x8000) ) {
        value <<= 1;
        --log;
    }
    return log;
}

void ClearBit( void *bits, unsigned bit )
{
    ((unsigned char *)bits)[bit >> 3] &= ~(1U << (bit & 7));
}

extern group_entry *GetGroupBySeg( unsigned short );
unsigned long FindLinearAddr( msg_address *addr )
{
    group_entry *group;
    group = GetGroupBySeg( addr->seg );
    if( group != 0 ) return addr->off + group->linear - group->grp_addr.off;
    return addr->off;
}

unsigned short blog_32( unsigned long value )
{
    unsigned short log;

    if( value == 0 ) return 0;
    log = 31;
    while( !(value & 0x80000000UL) ) {
        value <<= 1;
        --log;
    }
    return log;
}
