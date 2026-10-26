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
#include "cmdline.h"
#include <recon.h>

#ifdef __WATCOMC__
#pragma intrinsic(memcpy,strlen)
#endif

unsigned long PackDataLimit;    /* original4500fc */

static seg_leader *LastCodeSeg;         /* original4501c0 */

void P1Start( void )
{
    LastCodeSeg = 0;
}

list_of_names *AddName( char *name, unsigned len )
{
    list_of_names **entry;

    entry = AllocNode( NameNodes );
    *entry = ChkLAlloc( sizeof( list_of_names ) + len );
    (*entry)->flags = 0;
    (*entry)->next_name = 0;
    memcpy( (*entry)->name, name, len );
    (*entry)->name[len] = '\0';
    return *entry;
}

section *NonSect;                 /* original4500d0 */
unsigned NextSegIndex;    /* original441f5c */
void *SavedSegMask;        /* original441f60 */

void AllocateSegment( segdata *seg )
{
    segnode *node;
    section *sect;
    unsigned char isovlclass, iscode;
    class_entry *class;

    node = AllocNode( SegNodes );
    node->entry = 0;
    node->offset = 0;
    node->info = (seg->acbp >> 2) & 0x38;
    if( FmtData.type & 0x80 ) {
        iscode = CheckOvlClass( seg->cl.name->name, &isovlclass );
        seg->flags = (seg->flags & ~4) | ((iscode & 1) << 2);
        if( isovlclass ) node->info |= 0x40;
    } else {
        iscode = IsCodeClass( seg->cl.name->name, strlen( seg->cl.name->name ) );
        seg->flags = (seg->flags & ~4) | ((iscode & 1) << 2);
        isovlclass = 0;
    }
    if( seg->flags & 4 ) {
        node->info |= 0x0200;
        if( !(seg->acbp & 1) ) LinkState |= 0x2000;
    }
    if( (LinkFlags & 0x10000UL) && (seg->flags & 4) ) {
        if( LinkState & 0x1000 ) {
            if( !TestBit( SavedSegMask, NextSegIndex ) ) {
                node->entry = seg;
                node->info |= 0x80;
                ++NextSegIndex;
                return;
            }
        } else {
            isovlclass = 1;
        }
        ++NextSegIndex;
    }
    sect = DBIGetSect( seg->cl.name->name );
    if( (ObjFormat & 0x8000) && !(seg->flags & 8)
        && (isovlclass || (sect != 0 && (LinkFlags & 4))) ) {
        node->entry = seg;
        seg->u.name->flags = 1;
        seg->cl.name->flags = 1;
        seg->index = NextSegIndex - 1;
        seg->owner.normal.sect_id = CurrSect->sect_addr.seg;
        LinkList( CurrMod->arcs, seg );
        node->info |= 0x0100;
        return;
    }
    if( sect == 0 ) {
        if( !(FmtData.type & 0x80) || isovlclass ) sect = CurrSect;
        else sect = NonSect;
    }
    node->length = seg->length;
    class = FindClass( sect, seg->cl.name->name, (seg->acbp & 1) != 0 );
    seg->cl.entry = class;
    node->entry = AddSegment( seg, node, sect, &node->offset );
    FreeSegData( seg );
}

unsigned char IsCodeClass( char *name, unsigned len )
{
    return len >= 4 && memicmp( name + len - 4, "CODE", 4 ) == 0;
}

class_entry *FindClass( section *sect, char *name, unsigned char use32 )
{
    class_entry *class, *last;
    unsigned len;

    if( use32 ) use32 = 1;
    class = sect->classlist;
    last = class;
    while( class != 0 ) {
        if( stricmp( class->name, name ) == 0 && !((class->flags & 1) ^ use32) ) {
            return class;
        }
        last = class;
        class = class->next_class;
    }
    len = strlen( name );
    class = PermAlloc( sizeof( class_entry ) + len );
    memcpy( class->name, name, len + 1 );
    class->segs = 0;
    class->flags = use32;
    class->next_class = 0;
    /* The old target sets old_segs only when a first leader is appended. */
    if( last == 0 ) sect->classlist = class;
    else last->next_class = class;
    DBIColClass( class );
    if( IsCodeClass( name, len ) ) class->flags |= 0x10;
    if( IsStackClass( name, len ) ) class->flags |= 0x40;
    return class;
}

unsigned char IsStackClass( char *name, unsigned len )
{
    return len >= 5 && memicmp( name + len - 5, "STACK", 5 ) == 0;
}

seg_leader *AddSegment( segdata *seg, segnode *node, section *sect,
                        unsigned long *offset )
{
    unsigned short info, dbiflags;
    unsigned char combine;
    seg_leader *leader;

    combine = (seg->acbp >> 2) & 7;
    info = (((seg->acbp >> 5) & 7) << 3) | combine;
    dbiflags = DBIColSeg( seg->cl.entry );
    info |= dbiflags;
    if( seg->acbp & 1 ) {
        info |= 0x0400;
        if( dbiflags == 0 ) {
            Set32BitMode();
            if( (FmtData.type & 0x4200) && (LinkState & 0x2000) ) {
                LnkMsg( 0x748b, 0 );
            }
        }
    } else if( dbiflags == 0 ) {
        if( (FmtData.type & 0x4200) && (LinkState & 0x80) ) LnkMsg( 0x748b, 0 );
        if( !HintFormat( 0x4aff ) && !(ObjFormat & 0x10) ) {
            ObjFormat |= 0x10;
            LnkMsg( 0x3250, "sd", CurrMod->f.source->file->name, 16 );
        }
    }
    if( seg->cl.entry->flags & 0x10 ) info |= 0x0200;
    if( LinkState & 0x1000 ) node->info = info;
    if( seg->flags & 8 ) {
        if( DBISkip( info ) ) return 0;
        if( LinkState & 0x1000 ) {
            node->addr.off = 0;
            node->addr.seg = seg->a.absolute.frame;
        }
        leader = MakeNewLeader( seg->cl.entry, seg->u.name, seg->length, info );
        leader->seg_addr.off = 0;
        leader->seg_addr.seg = seg->a.absolute.frame;
        leader->info |= 0x4000;
        leader->section = sect;
        return leader;
    }
    if( combine == 0 ) {
        if( DBISkip( info ) ) return 0;
        leader = MakeNewLeader( seg->cl.entry, seg->u.name, seg->length, info );
    } else {
        leader = FindALeader( seg->cl.entry, seg->u.name, info );
        /* Preserve the old precedence defect: the mismatch diagnostic's
         * condition cannot select the USE32 bit from a Boolean value. */
        if( (leader->info & (0x0400 != info)) & 0x0400 ) LnkMsg( 0x3340, 0 );
    }
    leader->section = sect;
    seg->u.leader = leader;
    AddToSegment( seg, offset );
    if( LinkState & 0x1000 ) {
        node->addr.seg = leader->seg_addr.seg;
        node->addr.off = leader->seg_addr.off + leader->addr_adjustment + *offset;
        DBIGenLocal( leader, seg->length );
    } else {
        DBIAddLocal( info, seg->length );
    }
    return leader;
}

void Set32BitMode( void )
{
    LinkState |= 0x80;
    ObjFormat |= 4;
    if( !HintFormat( 0xff16 ) && !(ObjFormat & 0x10) ) {
        ObjFormat |= 0x10;
        LnkMsg( 0x3250, "sd", CurrMod->f.source->file->name, 32 );
    }
}

seg_leader *FindALeader( class_entry *class, list_of_names *name,
                         unsigned short info )
{
    seg_leader **link, *stop, *leader;

    link = &class->segs;
    if( (LinkState & 0x1000) && !(LinkFlags & 0x10000UL) ) stop = class->old_segs;
    else stop = 0;
    while( *link != stop ) {
        leader = *link;
        if( stricmp( leader->segname->name, name->name ) == 0 && (leader->info & 7) ) {
            return leader;
        }
        link = &leader->next_seg;
    }
    if( LinkState & 0x1000 ) {
        leader = class->old_segs;
        class->old_segs = leader->next_seg;
    } else {
        leader = InitLeader( name, info );
        CheckForLast( leader, class );
        *link = leader;
        class->old_segs = class->segs;
    }
    return leader;
}

seg_leader *InitLeader( list_of_names *name, unsigned short info )
{
    seg_leader *seg;

    name->flags = 1;
    seg = PermAlloc( sizeof( seg_leader ) );
    seg->next_seg = 0;
    seg->seg_addr.off = 0;
    seg->seg_addr.seg = 0xffff;
    seg->group = 0;
    seg->data_start = 0;
    seg->size = 0;
    seg->addr_adjustment = 0;
    seg->num = 0;
    seg->mod = 0;
    seg->recnum = 0;
    seg->saved_size = 0;
    seg->info = info;
    seg->segname = name;
    seg->u.p1.segflags = FmtData.def_seg_flags;
    /* Section, the field at14 and final four bytes are initialized later. */
    return seg;
}

void CheckForLast( seg_leader *seg, class_entry *class )
{
    if( (CurrMod->modinfo & 0x8000) && (class->flags & 0x10) ) {
        if( LastCodeSeg != 0 ) LastCodeSeg->info &= ~0x0800;
        LastCodeSeg = seg;
        seg->info |= 0x0800;
    }
}

unsigned long AddToSegment( segdata *seg, unsigned long *offset )
{
    seg_leader *leader;
    unsigned long oldsize, padding;
    unsigned char combine;

    *offset = 0;
    leader = seg->u.leader;
    combine = (seg->acbp >> 2) & 7;
    padding = 0;
    if( combine == 0 ) {
        oldsize = leader->size - seg->length;
    } else {
        oldsize = leader->size;
        if( !DBISkip( leader->info ) ) {
            ResizeSegment( leader, offset, seg->length,
                           (seg->acbp >> 5) & 7, combine );
        }
        padding = leader->size - oldsize - seg->length;
        if( oldsize != leader->size ) oldsize += padding;
    }
    if( !(LinkState & 0x1000) ) {
        leader->saved_size = leader->size;
        if( leader->data_start > leader->addr_adjustment ) {
            leader->addr_adjustment = leader->data_start;
        }
        if( !(leader->info & 0x40) ) DBIAddAddrInfo( leader, oldsize );
    } else {
        DBIGenAddrInfo( leader, oldsize, padding, leader->section );
    }
    return padding;
}

seg_leader *MakeNewLeader( class_entry *class, list_of_names *name,
                           unsigned long size, unsigned short info )
{
    mod_entry *save;
    seg_leader **link, *stop, *leader;

    save = CurrMod;
    if( (LinkState & 0x1000) && !(LinkFlags & 0x10000UL) ) stop = class->old_segs;
    else stop = 0;
    link = &class->segs;
    while( *link != stop ) {
        leader = *link;
        if( leader->recnum == RecNum && leader->mod == save ) {
            CurrMod = save;
            return leader;
        }
        link = &leader->next_seg;
    }
    CurrMod = save;
    if( LinkState & 0x1000 ) {
        leader = class->old_segs;
        class->old_segs = leader->next_seg;
    } else {
        leader = InitLeader( name, info );
        CheckForLast( leader, class );
        *link = leader;
        class->old_segs = class->segs;
        leader->size = size;
        if( LinkFlags & 0x10000UL ) {
            leader->mod = CurrMod;
            leader->recnum = RecNum;
        }
    }
    return leader;
}

group_entry *FindGroup( list_of_names *name )
{
    group_entry *group, *last;

    last = 0;
    for( group = Groups; group != 0; group = group->next_group ) {
        if( stricmp( group->name, name->name ) == 0 ) return group;
        last = group;
    }
    group = PermAlloc( sizeof( group_entry ) );
    name->flags = 1;
    group->grp_addr.seg = 0xffff;
    group->grp_addr.off = 0;
    group->totalsize = 0;
    group->next_group = 0;
    group->grp_relocs = 0;
    group->data = 0;
    group->leaders = 0;
    group->autogroup = -1;
    group->info = 0;
    group->segflags = 0x0c80;
    group->name = name->name;
    if( last == 0 ) Groups = group;
    else last->next_group = group;
    return group;
}

void AddToGroup( group_entry *group, seg_leader *seg, unsigned char isdead )
{
    int i;
    seg_leader **newlist;

    for( i = 0; i < group->num; ++i ) {
        if( group->leaders[i] == seg ) return;
    }
    if( isdead ) {
        if( group->autogroup == -1 ) group->autogroup = group->num;
    } else {
        if( seg->group != 0 && seg->group != group ) {
            LnkMsg( 0x731f, "123", seg->segname->name,
                    seg->group->name, group->name );
        }
        seg->group = group;
    }
    if( group->free_slots == 0 ) {
        newlist = ChkLAlloc( group->num * 2 * sizeof( seg_leader * ) );
        memcpy( newlist, group->leaders, group->num * sizeof( seg_leader * ) );
        _LnkFree( group->leaders );
        group->leaders = newlist;
        group->free_slots = group->num;
    }
    group->leaders[group->num++] = seg;
    --group->free_slots;
}

void DefineReference( symbol *sym, extnode *node )
{
    if( !(LinkState & 0x1000) ) {
        if( FmtData.type & 0x0080 ) TryRefVector( sym );
        return;
    }
    if( !(sym->info & 0x1000) ) UndefinedSym( sym );
    if( (sym->info & 0x0400) && !(sym->info & 0x0800) ) {
        LnkMsg( 0x2064, "Ss", sym, CurrMod->name );
    }
    TryUseVector( sym, node );
}

/* Deferred public definitions belonging to discarded strip-code segments. */
struct saved_public {
    struct saved_public *next;
    segnode *seg;
    unsigned long off;
    mod_entry *mod;
    symbol *sym;
};
static struct saved_public *SavedPublics;       /* original441f58 */

void DefineSymbol( symbol *sym, segnode *seg, unsigned long off,
                    unsigned short frame )
{
    unsigned len;
    unsigned short flags;
    unsigned char isdead, communal, frame_ok;
    struct saved_public *saved;

    isdead = 0;
    if( seg != 0 ) {
        isdead = (seg->info & 0x0100) != 0;
        frame = 0;
        off += seg->offset;
    }
    len = strlen( sym->name );
    if( sym->addr.seg != 0xffff && (sym->info & 15) > 3 ) {
        if( seg != 0 ) {
            if( isdead ) {
                saved = Pass1Alloc( sizeof( struct saved_public ) );
                saved->seg = seg;
                saved->sym = sym;
                saved->off = off;
                saved->mod = CurrMod;
                LinkList( &SavedPublics, saved );
                off = sym->addr.off;
                frame_ok = 1;
            } else {
                frame_ok = sym->p.seg == seg->entry;
            }
        } else {
            frame_ok = sym->p.seg == 0 && frame == sym->addr.seg;
        }
        if( !(frame_ok && off == sym->addr.off) ) {
            ReportMultiple( sym, sym->name, len );
        }
        return;
    }
    communal = (sym->info & 14) == 2;
    if( communal && !(sym->info & 0x0800) ) {
        sym->info |= 0x0100;
        flags = sym->info & 0x4040;
        sym->info = (sym->info & ~15) | 2;
        sym = SymOp( 0x26, sym->name, len );
        sym->info |= flags;
        communal = 0;
    }
    if( (sym->info & 15) > 3 || !(sym->info & 0x0800) ) {
        sym->publink = CurrMod->publist;
        CurrMod->publist = sym;
    }
    ClearSymUnion( sym );
    sym->info = (sym->info & ~15) | 4;
    sym->info &= ~0x2000;
    if( seg != 0 ) {
        if( isdead ) {
            sym->info |= 0x10;
            if( LinkFlags & 0x10000UL ) DefStripSym( sym, seg->entry );
            if( (LinkFlags & 4) && (LinkState & 2) ) sym->info |= 0x2000;
        } else if( LinkFlags & 0x10000UL ) {
            CleanStripInfo( sym );
        }
        sym->p.seg = seg->entry;
        if( !communal && !(ObjFormat & 0x8000) && (seg->info & 0x38) ) {
            DBIAddGlobal( sym, strlen( sym->name ) );
        }
        TryDefVector( sym );
        sym->addr.off = off;
        sym->addr.seg = frame;
    } else {
        if( LinkFlags & 0x10000UL ) CleanStripInfo( sym );
        XDefSymAddr( sym, off, frame );
        sym->p.seg = 0;
    }
}

void FindRedefs( void )
{
    struct saved_public *saved;
    symbol *sym;

    for( saved = SavedPublics; saved != 0; saved = saved->next ) {
        sym = saved->sym;
        if( sym->p.seg != saved->seg->entry || sym->addr.off != saved->off ) {
            CurrMod = saved->mod;
            ReportMultiple( sym, sym->name, strlen( sym->name ) );
        }
    }
}
