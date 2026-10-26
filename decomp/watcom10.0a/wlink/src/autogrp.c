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

void AutoGroup( void )
{
    AutoGroupSect( Root );
    ProcAllOvl( AutoGroupSect );
    SortGroupList();
    FindSplitGroups();
}

static group_entry *CurrGroup;       /* original450004 */

void AutoGroupSect( section *sec )
{
    class_entry *class;

    CurrGroup = 0;
    for( class = sec->classlist; class != 0; class = class->next_class ) {
        if( !(class->flags & 0x0c) ) PackClass( class, sec );
    }
}

void PackClass( class_entry *class, section *sec )
{
    seg_leader *seg, *packstart;
    unsigned long size, align_size, new_size, limit;
    unsigned num_segs;
    unsigned char lastseg, isdata, isreadwrite;

    seg = class->segs;
    isreadwrite = !(class->flags & 0x40);
    packstart = seg;
    size = 0;
    num_segs = 0;
    lastseg = 0;
    /* Empty classes pass an ignored undefined data flag to PackSegs(0).
     * Define it as zero; no original output depends on that stack byte. */
    isdata = 0;
    limit = 0;
    if( seg != 0 ) {
        limit = SetSegType( seg );
        isdata = !(seg->info & 0x0200);
    }
    while( seg != 0 ) {
        if( seg->group == 0 ) {
            CurrentSeg = 0;
            align_size = CAlign( size, (seg->info & 0x38) >> 3 );
            new_size = align_size + seg->saved_size;
            if( seg->saved_size != 0 && (new_size < size || new_size >= limit
                || lastseg || !CanPack( packstart, seg )) ) {
                PackSegs( packstart, num_segs, size, sec, isdata, isreadwrite );
                packstart = seg;
                num_segs = 1;
                if( FmtData.type & 0x00e0 ) {
                    size = (align_size & 15) + seg->saved_size;
                } else {
                    size = seg->saved_size;
                }
                lastseg = 0;
                limit = SetSegType( seg );
            } else {
                size = new_size;
                ++num_segs;
            }
            if( seg->info & 0x0800 ) lastseg = 1;
        } else {
            seg->group->section = sec;
            if( !(seg->info & 0x0200) ) seg->group->segflags |= 1;
            if( isreadwrite ) seg->group->segflags &= ~0x80;
            PackSegs( packstart, num_segs, size, sec, isdata, isreadwrite );
            packstart = seg->next_seg;
            num_segs = 0;
            if( FmtData.type & 0x00e0 ) {
                size = (CAlign( size, (seg->info & 0x38) >> 3 )
                    + seg->saved_size) & 15;
            } else {
                size = 0;
            }
            lastseg = 0;
            limit = SetSegType( packstart );
        }
        seg = seg->next_seg;
    }
    PackSegs( packstart, num_segs, size, sec, isdata, isreadwrite );
}

unsigned long PackCodeLimit;        /* original450100 */

unsigned long SetSegType( seg_leader *seg )
{
    if( seg == 0 ) return 0xffff;
    if( seg->info & 0x0200 ) {
        if( LinkFlags & 0x0200 ) return PackCodeLimit;
    } else {
        if( LinkFlags & 0x04000000UL ) return PackDataLimit;
    }
    if( seg->info & 0x0400 ) return 0xffffffffUL;
    if( FmtData.type & 0x0008 ) return 0xfff0;
    return 0xffff;
}

unsigned char CanPack( seg_leader *one, seg_leader *two )
{
    if( one->info & 0x0200 ) {
        if( !(two->info & 7) ) return 0;
    } else {
        if( (one->info & 0x38) != (two->info & 0x38) ) return 0;
    }
    if( (one->info & 0x0400) != (two->info & 0x0400) ) return 0;
    if( one->u.p1.segflags != two->u.p1.segflags ) return 0;
    return 1;
}

void PackSegs( seg_leader *seg, unsigned num_segs, unsigned long size,
    section *sec, unsigned char isdata, unsigned char isrdwr )
{
    group_entry *group;
    seg_leader **place;
    unsigned char fakegroup;

    if( num_segs == 0 ) return;
    fakegroup = size == 0 && CurrGroup != 0;
    if( fakegroup ) {
        group = CurrGroup;
    } else {
        group = GetAutoGroup( num_segs, (seg->info & 0x38) == 0 );
        if( isdata ) group->segflags |= 1;
        if( isrdwr ) group->segflags &= ~0x80;
        place = group->leaders;
        group->section = sec;
    }
    while( num_segs != 0 ) {
        if( seg->group == 0 ) {
            seg->group = group;
            if( !fakegroup ) *place++ = seg;
            --num_segs;
        }
        seg = seg->next_seg;
    }
}

group_entry *GetAutoGroup( unsigned num_segs, unsigned char absolute )
{
    group_entry *group;

    group = PermAlloc( sizeof( group_entry ) );
    group->grp_addr.seg = 0xffff;
    group->grp_addr.off = 0;
    group->next_group = 0;
    group->totalsize = 0;
    group->leaders = 0;
    group->grp_relocs = 0;
    group->info = 0;
    group->segflags = 0x0c80;
    group->num = num_segs;
    if( num_segs != 0 ) group->leaders = ChkLAlloc( num_segs * sizeof( seg_leader * ) );
    group->free_slots = 0;
    if( absolute ) {
        LinkList( &AbsGroups, group );
    } else {
        LinkList( &Groups, group );
        CurrGroup = group;
    }
    group->name = "";
    return group;
}

int NumGroups;                      /* original450000 */
group_entry *DataGroup;              /* original450124 */

void SortGroupList( void )
{
    group_entry *group;
    int number;

    if( Groups == 0 ) return;
    NumGroups = 0;
    group = Groups;
    while( Groups != 0 ) {
        if( stricmp( Groups->name, "DGROUP" ) == 0 ) DataGroup = Groups;
        Groups = Groups->next_group;
        group->next_group = 0;
        group = Groups;
        ++NumGroups;
    }
    number = NumGroups;
    CurrGroup = 0;
    if( !SortGroupSect( Root ) ) SortGroupAreas( Root->areas );
    CurrGroup->next_group = 0;
    NumGroups = number;
}

unsigned char SortGroupSect( section *sec )
{
    class_entry *class;
    seg_leader *seg;
    group_entry *group, *head, *tail;
    int remaining;
    unsigned char result;

    tail = CurrGroup;
    remaining = NumGroups;
    head = Groups;
    for( class = sec->classlist; class != 0; class = class->next_class ) {
        for( seg = class->segs; seg != 0; seg = seg->next_seg ) {
            group = seg->group;
            if( group == 0 || (seg->info & 0x38) == 0 ) continue;
            if( group->next_group != 0 ) continue;
            if( tail == 0 ) head = tail = group;
            else {
                tail->next_group = group;
                tail = group;
            }
            --remaining;
            tail->next_group = head;
            if( remaining <= 0 ) {
                result = 1;
                goto finished;
            }
        }
    }
    result = 0;
finished:
    Groups = head;
    NumGroups = remaining;
    CurrGroup = tail;
    return result;
}

void FindSplitGroups( void )
{
    group_entry *group;
    int index;

    if( !(FmtData.type & 0x0080) ) return;
    for( group = Groups; group != 0; group = group->next_group ) {
        for( index = 0; index < group->num; ++index ) {
            if( group->section != group->leaders[index]->section ) {
                LnkMsg( 0x335b, "s", group->name );
                return;
            }
        }
    }
}
