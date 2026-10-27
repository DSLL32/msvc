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
#pragma intrinsic(strlen,memcpy)
#endif

#pragma pack(1)
typedef union dist_arc {
    symbol *sym;
    unsigned short mod;
    unsigned long test;
} dist_arc;
typedef struct arcdata {
    segdata *segments;
    unsigned short ovlref, numarcs;
    dist_arc arcs[1];
} arcdata;
#pragma pack()
static unsigned short CurrModThere;      /* original450064 */
static unsigned long ArcBufLen;           /* original450060 */
static unsigned short CurrModHandle;     /* original450066 */
static mod_entry **ModTable;             /* original441df8 */
static arcdata *ArcBuffer;                  /* original441df4 */
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

unsigned short LowestAncestor( unsigned short index, section *sect )
{
    section *list;

    for( list = sect; list != 0; list = list->parent ) list->ovl_num |= 0x8000;
    list = SectOvlTab[index];
    while( !(list->ovl_num & 0x8000) ) list = list->parent;
    while( sect != 0 ) {
        sect->ovl_num &= ~0x8000;
        sect = sect->parent;
    }
    return list->ovl_num;
}

void InitModTable( void )
{
    CurrModThere = 32;
    ModTable = ChkLAlloc( CurrModThere * sizeof(*ModTable) );
    CurrModHandle = 0;
    ArcBufLen = 32;
    ArcBuffer = ChkLAlloc( sizeof(*ArcBuffer) + (ArcBufLen - 1) * sizeof(dist_arc) );
    InitSymStorage();
}

void AddModTable( mod_entry *mod, unsigned ref )
{
    mod_entry **table;

    ++CurrModHandle;
    if( CurrModHandle == CurrModThere ) {
        if( CurrModThere > 8192 ) LnkMsg( 0x3461, 0 );
        table = ChkLAlloc( sizeof(*table) * CurrModThere * 2 );
        memcpy( table, ModTable, sizeof(*table) * CurrModThere );
        _LnkFree( ModTable );
        ModTable = table;
        CurrModThere *= 2;
    }
    ModTable[CurrModHandle] = mod;
    mod->arcs = ArcBuffer;
    ArcBuffer->segments = 0;
    ArcBuffer->numarcs = 0;
    ArcBuffer->ovlref = (mod->modinfo & 0x1000) ? (unsigned short)ref : 0xffff;
}

void DefDistribSym( symbol *sym )
{
    arcdata *arcs;
    unsigned short state;

    state = sym->u.value >> 16;
    if( sym->info & 0x40 ) {
        arcs = CurrMod->arcs;
        if( CurrMod->modinfo & 0x1000 ) {
            if( ((segdata *)sym->p.seg)->flags & 4 ) {
                NewRefVector( sym, state & 0x0fff, arcs->ovlref );
            } else if( !(state & 0x2000) ) {
                state |= 0x3000;
                sym->u.value = (sym->u.value & 0xffffUL) | ((unsigned long)state << 16);
            }
        } else {
            if( arcs->ovlref == 0xffff ) arcs->ovlref = state & 0x0fff;
            else arcs->ovlref = LowestAncestor( arcs->ovlref, SectOvlTab[state & 0x0fff] );
        }
    }
    sym->u.value = (sym->u.value & 0xffff0000UL) | CurrModHandle;
}

void FinishArcs( mod_entry *mod )
{
    arcdata *arcs, *copy;
    unsigned size;

    ScanArcs( mod );
    arcs = mod->arcs;
    if( mod->modinfo & 0x1000 ) arcs->numarcs = 0;
    size = sizeof(*arcs) + (unsigned)arcs->numarcs * sizeof(dist_arc) - sizeof(dist_arc);
    copy = Pass1Alloc( size );
    memcpy( copy, arcs, size );
    mod->arcs = copy;
}

void ScanArcs( mod_entry *mod )
{
    arcdata *arcs;
    symbol *sym;
    mod_entry *refmod;
    unsigned short index, num, state;
    dist_arc arc;

    mod->modinfo |= 0x2000;
    arcs = mod->arcs;
    num = arcs->ovlref;
    if( num != 0xffff ) {
        index = arcs->numarcs;
        while( index != 0 ) {
            --index;
            arc = arcs->arcs[index];
            if( arc.test <= 8192 ) {
                DoRefGraph( num, ModTable[arc.mod] );
            } else {
                sym = arc.sym;
                if( sym->info & 0x1000 ) {
                    if( sym->info & 0x2000 ) {
                        arc.test = (unsigned short)sym->u.value;
                        refmod = ModTable[arc.mod];
                        if( refmod->modinfo & 0x1000 ) {
                            if( NewRefVector( sym, num, ((arcdata *)refmod->arcs)->ovlref ) )
                                DeleteArc( arcs, index );
                        } else {
                            DoRefGraph( num, refmod );
                            if( !NotAnArc( arc.test ) ) DeleteArc( arcs, index );
                            else arcs->arcs[index] = arc;
                        }
                    } else if( sym->p.seg == 0 ||
                               NewRefVector( sym, num, sym->p.leader->section->ovl_num ) ) {
                        DeleteArc( arcs, index );
                    }
                } else {
                    state = sym->u.value >> 16;
                    if( !(state & 0x8000) ) state = (state & 0xf000) | (num & 0x0fff) | 0x8000;
                    else state = (state & 0xf000) | (LowestAncestor( num, SectOvlTab[state & 0x0fff] ) & 0x0fff);
                    sym->u.value = (sym->u.value & 0xffffUL) | ((unsigned long)state << 16);
                }
            }
        }
    }
    mod->modinfo &= ~0x2000;
}

void RefDistribSym( symbol *sym )
{
    mod_entry *mod;
    unsigned long arc;

    arc = (unsigned long)sym;
    if( sym->info & 0x1000 ) {
        if( sym->info & 0x2000 ) {
            mod = ModTable[(unsigned short)sym->u.value];
            if( mod->modinfo & 0x1000 ) {
                if( ((segdata *)sym->p.seg)->flags & 4 ) AddArc( arc );
            } else {
                arc = (unsigned short)sym->u.value;
                if( NotAnArc( arc ) && (unsigned short)sym->u.value != CurrModHandle ) AddArc( arc );
            }
        } else if( ((sym->u.value >> 28) & 3) == 0 ) {
            if( NotAnArc( arc ) ) AddArc( arc );
        }
    } else {
        AddArc( arc );
    }
}

void AddArc( unsigned long arc )
{
    arcdata *arcs;

    arcs = CurrMod->arcs;
    if( arcs->numarcs >= ArcBufLen ) {
        arcs = ChkLAlloc( sizeof(*arcs) + (ArcBufLen * 2 - 1) * sizeof(dist_arc) );
        memcpy( arcs, ArcBuffer, sizeof(*arcs) + (ArcBufLen - 1) * sizeof(dist_arc) );
        _LnkFree( ArcBuffer );
        CurrMod->arcs = arcs;
        ArcBuffer = arcs;
        ArcBufLen *= 2;
    }
    arcs->arcs[arcs->numarcs].test = arc;
    ++arcs->numarcs;
}

unsigned char NotAnArc( unsigned long arc )
{
    arcdata *arcs;
    unsigned index;

    arcs = CurrMod->arcs;
    index = arcs->numarcs;
    while( index != 0 ) {
        if( arcs->arcs[--index].test == arc ) return 0;
    }
    return 1;
}

void DistIndCall( symbol *sym )
{
    arcdata *arcs;
    unsigned short save;

    arcs = CurrMod->arcs;
    save = arcs->ovlref;
    arcs->ovlref = 0;
    CurrMod->modinfo |= 0x2000;
    DoRefGraph( 0, ModTable[(unsigned short)sym->u.value] );
    CurrMod->modinfo &= ~0x2000;
    arcs->ovlref = save;
    sym->u.value = (sym->u.value & 0xf000ffffUL) | 0x80000000UL;
}

void DoRefGraph( unsigned num, mod_entry *mod )
{
    arcdata *arcs;
    unsigned short ancestor;

    arcs = mod->arcs;
    if( ((mod->modinfo & 0x2000) && (unsigned short)num == arcs->ovlref) ||
        (mod->modinfo & 0x1000) ) return;
    if( arcs->ovlref == 0xffff ) {
        arcs->ovlref = 0;
        ancestor = 0;
        ScanArcs( mod );
    } else {
        ancestor = LowestAncestor( (unsigned short)num, SectOvlTab[arcs->ovlref] );
        if( ancestor != arcs->ovlref ) {
            arcs->ovlref = ancestor;
            ScanArcs( mod );
        }
    }
    if( ancestor == 0 ) arcs->numarcs = 0;
}

unsigned char NewRefVector( symbol *sym, unsigned first, unsigned second )
{
    if( sym->p.seg == 0 || ((sym->u.value >> 28) & 3) != 0 ) return 1;
    if( LowestAncestor( (unsigned short)second, SectOvlTab[(unsigned short)first] ) != (unsigned short)second ) {
        Vectorize( sym );
        return 1;
    }
    return 0;
}

void DeleteArc( struct arcdata *arcs, unsigned index )
{
    --arcs->numarcs;
    if( arcs->numarcs != 0 ) arcs->arcs[(unsigned short)index] = arcs->arcs[arcs->numarcs];
}

extern void PModule( mod_entry * );
void ProcDistMods( void )
{
    unsigned short index;
    mod_entry *mod;

    for( index = 1; index <= CurrModHandle; ++index ) {
        mod = ModTable[index];
        CurrSect = (section *)mod->next_mod;
        PModule( mod );
    }
}

void ResetDistrib( void )
{
    ArcBuffer = 0;
    ModTable = 0;
    SectOvlTab = 0;
}
