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
#include <recon.h>

#ifdef __WATCOMC__
#pragma intrinsic(memcpy)
#endif

#pragma pack(1)
typedef struct sym_block {
    struct sym_block *next;
    unsigned size;
    char block[1];
} sym_block;
struct block_data {
    sym_block *list;
    unsigned currbrk;
};
#pragma pack()

static block_data Pass1Blocks;       /* original450208/45020c */

void ReleasePass1( void )
{
    FreeList( (path_entry *)Pass1Blocks.list );
    Pass1Blocks.list = 0;
}

static block_data PermBlocks;        /* original450200/450204 */

void RelSymBlock( void )
{
    FreeList( (path_entry *)PermBlocks.list );
    PermBlocks.list = 0;
}

void *PermAlloc( unsigned size )
{
    return AllocBlock( size, &PermBlocks );
}

void *AllocBlock( unsigned size, block_data *block )
{
    void *ptr;
    unsigned long newbrk;

    size = (size + 3U) & ~3U;
    newbrk = (unsigned long)block->currbrk + size;
    if( block->list == 0 ) {
        GetNewBlock( block, size );
    } else if( newbrk > block->list->size ) {
        ptr = 0;
        if( newbrk < 0xfffffff7UL )
            ptr = LnkExpand( block->list, 8U + newbrk );
        if( ptr != 0 ) block->list->size = newbrk;
        else GetNewBlock( block, size );
    }
    ptr = block->list->block + block->currbrk;
    block->currbrk += size;
    return ptr;
}

void GetNewBlock( block_data *block, unsigned size )
{
    unsigned try;
    sym_block *new;

    ShrinkBlock( block );
    try = 16384U;
    if( try < size ) try = size;
    for( ;; ) {
        new = LAlloc( try + 8U );
        if( new != 0 ) break;
        try /= 2;
        if( try < size || try < 32U ) LnkMsg( 0x3409, 0 );
    }
    new->next = block->list;
    block->list = new;
    new->size = try;
    block->currbrk = 0;
}

unsigned char ShrinkBlock( block_data *block )
{
    sym_block *new;

    if( block->list == 0 ) return 0;
    if( block->currbrk >= block->list->size ) return 0;
    new = LnkReAlloc( block->list, block->currbrk + 8U );
    new->size = block->currbrk;
    return 1;
}

symbol *LastSym;     /* original441f7c */

symbol *AddSym( char *name, unsigned length )
{
    symbol *sym;

    sym = PermAlloc( 33U + length );
    sym->hash = 0;
    sym->link = 0;
    sym->publink = 0;
    sym->addr.off = 0;
    sym->addr.seg = 0xffff;
    sym->p.seg = 0;
    sym->info = 0x0804;
    sym->u.value = 0;
    sym->e.def = 0;
    memcpy( sym->name, name, length + 1 );
    if( LastSym != 0 ) LastSym->link = sym;
    LastSym = sym;
    if( HeadSym == 0 ) HeadSym = sym;
    return sym;
}

void *Pass1Alloc( unsigned size )
{
    return AllocBlock( size, &Pass1Blocks );
}

unsigned char PermShrink( void )
{
    unsigned char freed;
    freed = ShrinkBlock( &PermBlocks );
    if( !freed ) freed = ShrinkBlock( &Pass1Blocks );
    return freed;
}
