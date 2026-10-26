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
/* Preserve the original compiler's dword-fill runtime edge and register ABI. */
void *__STOSD( void *, unsigned, unsigned );
#if defined(__WATCOMC__)
#pragma aux __STOSD "__STOSD" parm [eax] [edx] [ecx] value [eax] modify [ecx];
#endif
#include <recon.h>

void SetSegFlags( seg_flags *flag_list )
{
    seg_flags *start, *next_one;
    seg_leader *leader;
    class_entry *class;

    for( class = Root->classlist; class != 0; class = class->next_class ) {
        for( leader = class->segs; leader != 0; leader = leader->next_seg ) {
            if( class->flags & 0x40 ) leader->u.p1.segflags |= 0x80;
        }
    }
    start = flag_list;
    for( ; flag_list != 0; flag_list = flag_list->next ) {
        if( flag_list->type == 2 || flag_list->type == 3 ) {
            FillTypeFlags( flag_list->flags, flag_list->type );
        }
    }
    for( flag_list = start; flag_list != 0; flag_list = flag_list->next ) {
        if( flag_list->type == 1 ) {
            FillClassFlags( flag_list->name, flag_list->flags );
        }
    }
    for( flag_list = start; flag_list != 0; flag_list = next_one ) {
        if( flag_list->type == 0 ) {
            leader = FindASeg( Root->classlist, flag_list->name );
            if( leader == 0 ) LnkMsg( 0x322e, "s", flag_list->name );
            else leader->u.p1.segflags = flag_list->flags;
        }
        next_one = flag_list->next;
        _LnkFree( flag_list->name );
        _LnkFree( flag_list );
    }
}

void CheckClassOrder( void )
{
    if( LinkFlags & 0x10 ) ReOrderClasses( Root );
}

void ReOrderClasses( section *sec )
{
    class_entry *currcl, *nextcl, **owner;
    seg_leader *seg, **segowner;
    char *name;
    int i, ord;

    while( sec != 0 ) {
        class_entry *rings[11];

        __STOSD( rings, 0, 11 );

        currcl = sec->classlist;
        while( currcl != 0 ) {
            nextcl = currcl->next_class;
            currcl->next_class = 0;
            CheckClassUninitialized( currcl );
            if( (FmtData.type & 0x3d04) && !(currcl->flags & 1) ) {
                ord = 0;
            } else {
                name = currcl->name;
                if( currcl->flags & 0x10 ) {
                    if( stricmp( name, "_OVLCODE" ) == 0 ) ord = 3;
                    else if( stricmp( name, "BEGCODE" ) == 0 ) ord = 1;
                    else ord = 2;
                } else if( currcl->segs == 0 || currcl->segs->group == 0
                    || stricmp( currcl->segs->group->name, "DGROUP" ) != 0 ) {
                    ord = 4;
                } else {
                    if( stricmp( name, "BSS" ) == 0 ) ord = 9;
                    else if( stricmp( name, "STACK" ) == 0 ) ord = 10;
                    else if( stricmp( name, "BEGDATA" ) == 0 ) ord = 5;
                    else if( stricmp( name, "DATA" ) == 0 ) ord = 6;
                    else if( currcl->flags & 0x20 ) ord = 7;
                    else ord = 8;
                }
            }
            if( rings[ord] == 0 ) currcl->next_class = currcl;
            else {
                currcl->next_class = rings[ord]->next_class;
                rings[ord]->next_class = currcl;
            }
            rings[ord] = currcl;
            currcl = nextcl;
        }
        currcl = rings[2];
        if( currcl != 0 ) {
            do {
                segowner = &currcl->segs;
                for( seg = *segowner; seg != 0; seg = *segowner ) {
                    if( stricmp( seg->segname->name, "BEGTEXT" ) == 0 ) {
                        *segowner = seg->next_seg;
                        seg->next_seg = currcl->segs;
                        currcl->segs = seg;
                        break;
                    }
                    segowner = &seg->next_seg;
                }
                currcl = currcl->next_class;
            } while( currcl != rings[2] );
        }
        owner = &sec->classlist;
        for( i = 0; i <= 10; ++i ) {
            if( rings[i] != 0 ) {
                *owner = rings[i]->next_class;
                owner = &rings[i]->next_class;
            }
        }
        *owner = 0;
        ReOrderAreas( sec->areas );
        sec = sec->next_sect;
    }
}

void CheckClassUninitialized( class_entry *class )
{
    seg_leader *seg;

    for( seg = class->segs; seg != 0; seg = seg->next_seg ) {
        if( seg->info & 0x8000 ) {
            class->flags |= 0x20;
            return;
        }
    }
}

void ReOrderAreas( ovl_area *area )
{
    for( ; area != 0; area = area->next ) {
        ReOrderClasses( area->sections );
    }
}

void CalcAddresses( void )
{
    unsigned long flat, size, align;
    group_entry *group;
    unsigned char calcgroups;

    if( FmtData.base == 0xffffffffUL ) {
        if( FmtData.type & 0x0010 ) FmtData.base = 0x400000;
        else if( FmtData.type & 0x4000 ) FmtData.base = 0x1000;
        else if( FmtData.type & 0x0006 ) FmtData.base = 0x10000;
        else FmtData.base = 0;
    }
    StartMemMap();
    CurrSect = Root;
    if( FmtData.type & 0xfe1f ) {
        AllocFileSegs();
        if( FmtData.type & 0x0010 ) FindUninitDataStart();
    }
    AllocClasses( Root->classlist );
    calcgroups = 0;
    if( FmtData.type & 0x00e0 ) {
        if( FmtData.type & 0x0080 ) CalcOvl();
        calcgroups = 1;
    } else if( FmtData.type & 0x4016 ) {
        if( FmtData.type & 0x0010 ) flat = GetPEHeaderSize();
        else flat = FmtData.base;
        for( group = Groups; group != 0; group = group->next_group ) {
            group->linear = flat;
            size = group->totalsize;
            if( FmtData.type & 0x0010 ) align = FmtData.u.pe.objalign;
            else if( FmtData.type & 0x4200 ) align = 0x1000;
            else align = 0x10000;
            flat += (size + align - 1) & ~(align - 1);
        }
        if( FmtData.type & 0x4010 ) ReallocFileSegs();
    } else if( FmtData.type & 0x4200 ) {
        ReallocFileSegs();
    } else if( !(FmtData.type & 0x200f) ) {
        calcgroups = 1;
    }
    if( calcgroups ) {
        CalcGrpAddr( Groups );
        CalcGrpAddr( AbsGroups );
    }
    DBIAddrStart();
    DBIAddrOvlStart();
    DefinePublics();
}

void AllocFileSegs( void )
{
    group_entry *group;
    unsigned short number;

    number = 1;
    for( group = Groups; group != 0; group = group->next_group ) {
        if( FmtData.type & 0x9c00 ) group->grp_addr.seg = 1;
        else if( FmtData.type & 0x4200 ) group->grp_addr.seg = ToQNXSel( number++ );
        else if( FmtData.type & 0x2000 ) group->grp_addr.seg = (number++ << 3) | 4;
        else group->grp_addr.seg = number++;
        group->grp_addr.off = 0;
    }
}

void FindUninitDataStart( void )
{
    class_entry *class;
    unsigned char setnext;

    setnext = 1;
    FmtData.u.pe.dgroupsplitseg = 0;
    FmtData.u.pe.bsspad = 0;
    if( !(LinkFlags & 0x10) ) return;
    for( class = Root->classlist; class != 0; class = class->next_class ) {
        if( !(class->flags & 0x0c) ) {
            if( class->flags & 0x20 ) setnext = 1;
            else if( setnext ) {
                FmtData.u.pe.dgroupsplitseg = class->segs;
                setnext = 0;
            }
        }
    }
    if( setnext ) FmtData.u.pe.dgroupsplitseg = 0;
}

void AllocClasses( class_entry *class )
{
    address save;
    unsigned long size;

    for( ; class != 0; class = class->next_class ) {
        if( class->flags & 0x0c ) {
            save = CurrLoc;
            size = CurrSect->size;
            AllocSegs( class );
            DBIDefClass( class, CurrSect->size - size );
            CurrSect->size = size;
            CurrLoc = save;
        } else {
            AllocSegs( class );
        }
    }
}

/* This old class walker predates the later one-leader AllocSeg callback. */
void AllocSegs( class_entry *class )
{
    seg_leader *seg;

    for( seg = class->segs; seg != 0; seg = seg->next_seg ) {
        if( seg->info & 0x38 ) {
            if( seg->info & 0x7000 ) {
                CurrLoc.off = 0;
                CurrLoc.seg = 0;
            }
            CurrentSeg = seg;
            NewSegment( seg );
        }
        seg->data_start = 0;
        seg->size = 0;
    }
}

void ReallocFileSegs( void )
{
    group_entry *group;
    class_entry *class;
    seg_leader *seg;
    unsigned seg_num;

    seg_num = 1;
    for( group = Groups; group != 0; group = group->next_group ) {
        if( group->totalsize != 0 ) {
            if( FmtData.type & 0x4200 ) group->grp_addr.seg = ToQNXSel( seg_num++ );
            else group->grp_addr.seg = seg_num++;
        } else --NumGroups;
    }
    for( class = Root->classlist; class != 0; class = class->next_class ) {
        if( !(class->flags & 0x0c) ) {
            for( seg = class->segs; seg != 0; seg = seg->next_seg ) {
                if( seg->info & 0x38 ) seg->seg_addr.seg = seg->group->grp_addr.seg;
            }
        }
    }
}

extern void WriteGroups( void );
extern void WriteSegs( class_entry * );
extern void WritePubHead( void );
extern void StartMapSort( void );
extern void ProcPubs( mod_entry *, section * );
extern void ProcOvlSectPubs( section * );
extern void FinishMapSort( void );
extern void ProcOvlPubs( void );
extern void WriteCommunals( void );
extern void WriteModSegHead( void );
extern void WriteMapBox12B( void );
extern void FindFloatSyms( void );

void DefinePublics( void )
{
    CheckSymbolPass( 2 );
    if( MapFlags & 1 ) {
        StartMapBuffering();
        LnkMsg( 0x1005, 0 );
        WriteGroups();
        WriteMapNL( 1 );
        WriteSegs( Root->classlist );
        WritePubHead();
    }
    StartMapSort();
    ProcPubs( Root->mods, Root );
    if( FmtData.type & 0x00e0 ) ProcOvlSectPubs( Root );
    ProcPubs( LibModules, Root );
    FinishMapSort();
    if( FmtData.type & 0x0080 ) ProcOvlPubs();
    if( MapFlags & 1 ) {
        WriteCommunals();
        if( LinkFlags & 8 ) WriteMapBox12B();
        else if( MapFlags & 0x40 ) WriteModSegHead();
        StopMapBuffering();
    }
    FindFloatSyms();
}

static unsigned NumMapSyms;          /* original4501ac */

void StartMapSort( void )
{
    NumMapSyms = 0;
}

extern void DoPubs( section * );

void ProcPubs( mod_entry *head, section *sect )
{
    for( CurrMod = head; CurrMod != 0; CurrMod = CurrMod->next_mod ) {
        DoPubs( sect );
    }
}

extern void DBIAddModule( mod_entry *, section * );
extern void DBIGenGlobal( symbol *, section * );
extern unsigned char SkipSymbol( symbol * );
extern void WritePubModHead( void );
extern void XReportSymAddr( symbol * );
extern void WriteSymArray( symbol **, unsigned );

void DoPubs( section *sect )
{
    symbol *sym, *next, *head;
    symbol **symarray;
    seg_leader *leader;
    group_entry *group;
    unsigned count, num;
    unsigned char first;
    unsigned long offset;

    if( CurrMod->modinfo & 0x4000 ) DBIAddModule( CurrMod, sect );
    head = 0;
    count = 0;
    for( sym = CurrMod->publist; sym != 0; sym = next ) {
        ++count;
        next = sym->publink;
        sym->publink = head;
        head = sym;
    }
    symarray = 0;
    if( (MapFlags & 8) && !(MapFlags & 0x20) && count > 0 ) {
        symarray = ChkLAlloc( count * sizeof(symbol *) );
    }
    CurrMod->publist = head;
    num = 0;
    first = 1;
    for( sym = head; sym != 0; sym = sym->publink ) {
        if( sym->info & 0x10 ) continue;
        if( (sym->info & 15) <= 1 || (sym->info & 15) == 5 ) continue;
        leader = sym->p.seg;
        if( leader != 0 ) {
            if( !(leader->info & 0x38) ) {
                XDefSymAddr( sym, sym->addr.off + leader->seg_addr.off,
                             leader->seg_addr.seg );
            } else {
                group = leader->group;
                offset = sym->addr.off;
                offset += (leader->seg_addr.seg - group->grp_addr.seg) * 16UL;
                offset += leader->seg_addr.off - group->grp_addr.off;
                offset += leader->addr_adjustment;
                offset += group->grp_addr.off;
                XDefSymAddr( sym, offset, group->grp_addr.seg );
                DBIGenGlobal( sym, sect );
            }
        }
        if( (MapFlags & 1) && !SkipSymbol( sym ) ) {
            if( first && !(MapFlags & 0x20) ) {
                WritePubModHead();
                first = 0;
            }
            if( MapFlags & 8 ) {
                if( MapFlags & 0x20 ) {
                    sym->info |= 0x80;
                    ++NumMapSyms;
                } else symarray[num++] = sym;
            } else XReportSymAddr( sym );
        }
    }
    if( num > 0 ) WriteSymArray( symarray, num );
    if( symarray != 0 ) _LnkFree( symarray );
}

void FinishMapSort( void )
{
    symbol **array, **curr;
    symbol *sym;
    unsigned char ok;

    if( (MapFlags & 0x20) && NumMapSyms > 0 ) {
        array = 0;
        if( NumMapSyms < 0x3ffffffeUL ) array = LAlloc( NumMapSyms * sizeof(symbol *) );
        curr = array;
        ok = array != 0;
        for( sym = HeadSym; sym != 0; sym = sym->link ) {
            if( sym->info & 0x80 ) {
                sym->info &= ~0x80;
                if( ok ) *curr++ = sym;
                else XReportSymAddr( sym );
            }
        }
        if( !ok ) LnkMsg( 0x328f, 0 );
        else {
            WriteSymArray( array, NumMapSyms );
            _LnkFree( array );
        }
    }
}

extern symbol *FindISymbol( char * );

void FindFloatSyms( void )
{
    static char *FloatNames[] = { "FIWRQQ", "FIDRQQ", "FIERQQ",
                                 "FICRQQ", "FISRQQ", "FIARQQ" };
    static char *IgnoreNames[] = { "FJCRQQ", "FJSRQQ", "FJARQQ" };
    unsigned index;
    symbol *sym;

    for( index = 0; index < 6; ++index ) {
        sym = FindISymbol( FloatNames[index] );
        if( sym != 0 ) {
            if( FmtData.type & 0x4200 ) {
                sym->info = (sym->info & 0x1fff) | ((index + 1) << 13);
            } else sym->info |= 0x8000;
        }
    }
    for( index = 0; index < 3; ++index ) {
        sym = FindISymbol( IgnoreNames[index] );
        if( sym != 0 ) {
            if( FmtData.type & 0x4200 ) sym->info = (sym->info & 0x1fff) | 0xe000;
            else sym->info |= 0x8000;
        }
    }
}

void ConvertToFrame( address *addr, unsigned short frame )
{
    unsigned long off;

    if( FmtData.type & 0x00e0 ) {
        off = (addr->seg - frame) * 16UL + addr->off;
        if( off > 0x10000UL ) LnkMsg( 0xf353, "ax", *addr, frame );
        addr->off = off;
    }
    addr->seg = frame;
}

void AllocGroupBuffers( void )
{
    group_entry *group;
    seg_leader *seg;
    unsigned long group_addr, seg_addr;
    int i;

    for( group = Groups; group != 0; group = group->next_group ) {
        if( group->totalsize != 0 ) group->data = AllocStg( group->totalsize );
        else group->data = 0;
        group->size = 0;
        group_addr = group->grp_addr.off;
        if( FmtData.type & 0x00e0 ) group_addr += group->grp_addr.seg * 16UL;
        for( i = 0; i < group->num; ++i ) {
            seg = group->leaders[i];
            seg_addr = seg->seg_addr.off;
            if( FmtData.type & 0x00e0 ) seg_addr += seg->seg_addr.seg * 16UL;
            seg->data = group->data + seg_addr - group_addr;
        }
    }
    if( FmtData.type & 0x0080 ) {
        OvlGroup = PermAlloc( sizeof(group_entry) );
        OvlGroup->data = AllocStg( OvltabSize );
        OvlGroup->num = 0;
        OvlGroup->free_slots = 0;
        OvlGroup->name = "";
        OvlGroup->leaders = 0;
        OvlGroup->autogroup = -1;
        OvlGroup->grp_relocs = 0;
        OvlGroup->size = OvltabSize;
        OvlGroup->totalsize = OvltabSize;
        OvlGroup->section = Root;
        OvlGroup->grp_addr.off = OvltabAddr.off;
        OvlGroup->grp_addr.seg = OvltabAddr.seg;
        OvlGroup->next_group = Groups;
        OvlGroup->u.info = 0;
        Groups = OvlGroup;
        OvlRefs->group = OvlGroup;
    }
}

void FillClassFlags( char *name, unsigned short flags )
{
    class_entry *class;
    seg_leader *seg;

    for( class = Root->classlist; class != NULL; class = class->next_class ) {
        if( stricmp( class->name, name ) == 0 ) {
            for( seg = class->segs; seg != NULL; seg = seg->next_seg ) {
                seg->u.p1.segflags = flags;
            }
            return;
        }
    }
    LnkMsg( 0x322f, "s", name );
}

void CalcGrpAddr( group_entry *group )
{
    unsigned short index;
    unsigned char real, first;
    unsigned long start, low, high;
    seg_leader *seg;

    real = (FmtData.type & 0x00e0) != 0;
    for( ; group != 0; group = group->next_group ) {
        first = 1;
        index = group->num;
        while( index != 0 ) {
            seg = group->leaders[--index];
            start = seg->seg_addr.off;
            if( real ) start += (unsigned long)seg->seg_addr.seg << 4;
            if( first ) {
                group->grp_addr.off = seg->seg_addr.off;
                group->grp_addr.seg = seg->seg_addr.seg;
                first = 0;
                low = start;
                high = start + seg->saved_size;
            } else {
                if( start < low ) {
                    group->grp_addr.off = seg->seg_addr.off;
                    group->grp_addr.seg = seg->seg_addr.seg;
                    low = start;
                }
                if( start + seg->saved_size > high ) high = start + seg->saved_size;
            }
        }
        if( real && high - low > 0x10000UL ) {
            LnkMsg( 0x3314, "sl", group->name, high - low - 0x10000UL );
            low = high - 0x10001UL;
        }
        group->totalsize = high - low;
    }
}
