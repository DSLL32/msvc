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
#include "link.h"
#include "strtab.h"
#include <recon.h>
#ifdef __WATCOMC__
#pragma intrinsic(memcpy)
#endif

void InitStringTable( stringtable *table )
{
    table->used = 0;
    table->data = ChkLAlloc( 4096 );
    table->size = 4096;
}

void CharStringTable( stringtable *table, char data )
{
    AddStringTable( table, &data, 1 );
}

unsigned char AddStringTable( stringtable *table, char *data, unsigned len )
{
    unsigned size;

    if( table->used + len > table->size ) {
        size = table->size + 1024;
        if( size < table->size ) return 0;
        table->size = size;
        table->data = LnkReAlloc( table->data, size );
    }
    memcpy( table->data + table->used, data, len );
    table->used += len;
    return 1;
}

unsigned GetStringTableSize( stringtable *table )
{
    return table->used;
}

char *GetStringTable( stringtable *table )
{
    return table->data;
}
