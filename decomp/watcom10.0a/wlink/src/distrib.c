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
#pragma intrinsic(strlen)
#endif

static unsigned short CurrModHandle;     /* original450066 */
static mod_entry **ModTable;             /* original441df8 */
static void *ArcBuffer;                  /* original441df4 */
/* SectOvlTab is shared with the overlay numbering owner. */

void FreeDistStuff( void )
{
    unsigned index;

    for( index = 1; index <= CurrModHandle; ++index ) {
        FreeAMod( ModTable[index] );
    }
    _LnkFree( ModTable );
    _LnkFree( ArcBuffer );
    _LnkFree( SectOvlTab );
    ReleasePass1();
}

/* The 1994 per-module segment list also holds its distribution reference. */
#pragma pack(1)
typedef struct module_segments {
    void *segments;
    unsigned short ovlref;
} module_segments;
#pragma pack()

/* Released SetSegments retains the phase role; this older build also
 * redistributes modules and replays saved segment definitions here. */
void SetSegments( void )
{
    unsigned bytes, index;
    unsigned short ovlref;
    mod_entry *mod, **owner;
    section *sect;

    if( !(LinkFlags & 0x10004UL) ) return;
    ObjFormat = (ObjFormat & 0x7f7f) | 0x80;
    if( LinkFlags & 4 ) {
        _LnkFree( ArcBuffer );
        ArcBuffer = 0;
    }
    sect = CurrSect;
    if( LinkFlags & 0x10000UL ) {
        bytes = (NextSegIndex + 7) / 8;
        SavedSegMask = PermAlloc( bytes );
        memset( SavedSegMask, 0xff, bytes );
        NextSegIndex = 0;
        SetSectSegments( Root );
        ProcAllOvl( SetSectSegments );
        CurrSect = Root;
        SetModSegments( LibModules );
        sect = CurrSect;
    }
    CurrSect = sect;
    if( LinkFlags & 4 ) {
        for( index = 1; index <= CurrModHandle; ++index ) {
            mod = ModTable[index];
            CurrMod = mod;
            ovlref = ((module_segments *)mod->arcs)->ovlref;
            if( ovlref == 0xffff ) sect = Root;
            else sect = SectOvlTab[ovlref];
            CurrSect = sect;
            SetModSegments( mod );
            sect = CurrSect;
            mod->arcs = 0;
            owner = &sect->u.dist_mods;
            while( *owner != 0 ) owner = (mod_entry **)&(*owner)->arcs;
            *owner = mod;
            mod->next_mod = (mod_entry *)sect;
        }
    }
    FixGroupProblems();
    FindRedefs();
    if( LinkFlags & 4 ) {
        _LnkFree( SectOvlTab );
        SectOvlTab = 0;
    }
    ReleasePass1();
}

void InitArcBuffer( mod_entry *mod )
{
    if( !(LinkFlags & 4) || !(LinkState & 2) ) {
        mod->arcs = PermAlloc( sizeof(segdata *) );
        *(segdata **)mod->arcs = 0;
    }
}

/* This older DefModSegments receives the saved segment list and publist
 * directly, before the later source merged those arguments into mod_entry. */
extern void DefModSegments( segdata *, symbol * );
void SetModSegments( mod_entry *mod )
{
    for( ; mod != 0; mod = mod->next_mod ) {
        CurrMod = mod;
        DefModSegments( *(segdata **)mod->arcs, mod->publist );
        DBIP1ModuleScanned();
        if( mod->modinfo & 0x4000 ) DBIGenModule( mod );
        FreeSavedSegs();
    }
}

void SetSectSegments( section *sect )
{
    CurrSect = sect;
    SetModSegments( sect->mods );
}

extern void SetSavedSegs( segdata * );
extern void LinkSavedSeg( segdata * );
extern void ClearBit( void *, unsigned );
extern void RestoreComdat( symbol *, segdata * );
extern void DBIAddModLines( unsigned );
extern void FinishComdat( saved_seg * );
void DefModSegments( segdata *segs, symbol *syms )
{
    segdata *seg;
    symbol *sym;
    unsigned type;
    unsigned char removed;

    SetSavedSegs( segs );
    for( seg = segs; seg != 0; seg = seg->next ) {
        if( (seg->flags & 1) || !(LinkFlags & 0x10000UL) ) {
            seg->cl.entry = FindClass( CurrSect, seg->cl.name->name, seg->acbp & 1 );
            seg->u.leader = AddSegment( seg, 0, CurrSect, &seg->a.saved_addr );
            DBIAddModLines( seg->line_size );
        } else {
            seg->flags |= 0x10;
            ClearBit( SavedSegMask, seg->index );
        }
    }
    for( sym = syms; sym != 0; sym = sym->publink ) {
        if( sym->info & 0x10 ) {
            sym->info &= ~0x10;
            removed = 0;
            if( LinkFlags & 0x10000UL ) {
                seg = sym->p.seg;
                if( !(seg->flags & 1) ) {
                    if( sym->e.def != 0 ) WeldSyms( sym, sym->e.def );
                    else sym->info |= 0x10;
                    removed = 1;
                    if( LinkFlags & 0x02000000UL ) LnkMsg( 0x2090, "S", sym );
                } else if( (sym->info & 0x0e) == 2 ) {
                    seg->flags &= ~4;
                    if( ((segdata *)seg->u.leader)->flags & 0x10 ) {
                        RestoreComdat( sym, seg );
                    } else {
                        seg->u.leader = ((segdata *)seg->u.leader)->u.leader;
                        FinishComdat( seg );
                    }
                    LinkSavedSeg( seg );
                    removed = 1;
                }
            }
            if( !removed ) {
                seg = sym->p.seg;
                sym->addr.off += seg->a.saved_addr;
                sym->p.leader = seg->u.leader;
            }
        }
        type = sym->info & 0x0f;
        if( type < 7 && type != 5 && sym->p.leader != 0
            && !(sym->info & 0x10) && (sym->p.leader->info & 0x38) != 0 ) {
            DBIAddGlobal( sym, strlen( sym->name ) );
        }
    }
}

void FixGroupProblems( void )
{
    group_entry *group;
    seg_leader **first, **cursor, **prev, *leader;
    short index, limit;

    for( group = Groups; group != 0; group = group->next_group ) {
        if( group->autogroup == -1 ) continue;
        first = group->leaders + group->autogroup;
        cursor = first;
        limit = group->num;
        for( index = group->autogroup; index < limit; ++index ) {
            leader = ((segdata *)*cursor)->u.leader;
            for( prev = first; prev < cursor; ++prev ) {
                if( *prev == leader ) {
                    memmove( cursor, cursor + 1, (limit - index - 1) * sizeof(*cursor) );
                    --group->num;
                    ++group->free_slots;
                    leader = 0;
                    break;
                }
            }
            if( leader != 0 ) {
                leader->group = group;
                *cursor++ = leader;
            }
        }
    }
}
