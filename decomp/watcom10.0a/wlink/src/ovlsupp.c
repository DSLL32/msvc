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
#include "fixup.h"
#include "diagnostic.h"
#include <recon.h>

/* These preexisting globals still have provisional role names. */
extern list_of_names *OvlClasses;              /* original4500f8 */
extern vecnode *OvlVectors;          /* original450114 */
seg_leader *OvlRefs;        /* original450158 */

void FreeOvlStruct( void )
{
    OvlClasses = 0;
    OvlVectors = 0;
    FreeList( (path_entry *)OvlRefs );
    FreeDistStuff();
}

group_entry *OvlGroup;            /* original450128 */
address OvltabAddr;                /* original450168 */
unsigned OvltabSize;               /* original4500dc */

extern void GetVecAddr( unsigned, address * );

void OvlForceVect( fixup_ref *target, unsigned char indirect )
{
    symbol *sym;
    unsigned short state;

    if( !(FmtData.type & 0x0080) || !(target->flags & 0x10) ) return;
    sym = target->u.sym;
    if( (sym->info & 15) <= 3 ) return;
    state = (unsigned short)(sym->u.value >> 16);
    if( ((state >> 12) & 3) != 2 ) return;
    if( !indirect && !(state & 0x4000) ) return;
    GetVecAddr( state & 0x0fff, &target->addr );
}

#ifdef __WATCOMC__
#pragma intrinsic(strlen,memcpy)
#endif
static address OvlvecAddr;       /* original4501e0 */
static address Stash;            /* original4501d8 */
extern unsigned SectionNum;

static symbol *OverlayTable;       /* original4501c8 */
static symbol *OverlayTableEnd;    /* original4501d4 */
static symbol *OvlVecStart;        /* original4501cc */
static symbol *OvlVecEnd;          /* original4501d0 */

void OvlPass1( void )
{
    symbol *sym;

    ProcAllOvl( LoadObjFiles );
    OverlayTable = DefISymbol( "__OVLTAB__" );
    OverlayTableEnd = DefISymbol( "__OVLTABEND__" );
    OvlVecStart = DefISymbol( "__OVLSTARTVEC__" );
    OvlVecEnd = DefISymbol( "__OVLENDVEC__" );
    OvlRefs = ChkLAlloc( sizeof(*OvlRefs) );
    OvlRefs->info = 8;
    OvlRefs->next_seg = 0;
    OvlRefs->section = Root;
    OvlRefs->u.p1.segflags = FmtData.def_seg_flags;
    OverlayTable->p.leader = OvlRefs;
    OverlayTableEnd->p.leader = OvlRefs;
    OvlVecStart->p.leader = OvlRefs;
    OvlVecEnd->p.leader = OvlRefs;
    if( LinkFlags & 0x0400 ) {
        RefISymbol( "__NOVLINIT__" );
        sym = RefISymbol( "__NOVLLDR__" );
    } else if( LinkFlags & 0x1000 ) {
        RefISymbol( "__SOVLINIT__" );
        sym = RefISymbol( "__SOVLLDR__" );
    } else {
        RefISymbol( "__LOVLINIT__" );
        sym = RefISymbol( "__LOVLLDR__" );
    }
    sym->u.value = (sym->u.value & 0xf000ffffUL) | 0x80000000UL;
}

void CalcOvl( void )
{
    unsigned size;
    outfilelist *file;

    CurrSect = Root;
    Align( 2 );
    XDefSymAddr( OverlayTable, CurrLoc.off, CurrLoc.seg );
    OvltabAddr = CurrLoc;
    size = 12 + (SectionNum - 1) * 16;
    XDefSymAddr( OverlayTableEnd, CurrLoc.off + size - 2, CurrLoc.seg );
    for( file = OutFiles; file != 0; file = file->next ) {
        file->ovlfnoff = size;
        size += strlen( file->fname ) + 1;
    }
    Root->outfile->ovlfnoff |= 0x8000;
    OvltabSize = size;
    CurrentSeg = 0;
    AddSize( size );
    XDefSymAddr( OvlVecStart, CurrLoc.off, CurrLoc.seg );
    OvlvecAddr = CurrLoc;
    size = VecNum * ((LinkFlags & 0x1000) ? 8 : 10);
    AddSize( size );
    OvltabSize += size;
    XDefSymAddr( OvlVecEnd, CurrLoc.off, CurrLoc.seg );
    AddSize( 2 );
    Align( 3 );
    NormalizeAddr();
    Stash = CurrLoc;
    AllocAreas( Root->areas );
    MAlign( 3 );
    NormalizeAddr();
}

void AllocAreas( ovl_area *area )
{
    while( area != 0 ) {
        AllocSections( area->sections );
        area = area->next;
    }
}

extern unsigned short AreaSize;
void AllocSections( section *first )
{
    address save, max;
    section *sect;
    unsigned size, minimum;
    unsigned long result;
    unsigned char specified;

    CurrSect = first;
    MAlign( 3 );
    NormalizeAddr();
    if( first == NonSect ) {
        if( LinkFlags & 0x0400 ) {
            size = CurrLoc.seg - Stash.seg;
            minimum = size + SectionNum + 1;
            if( minimum < 64 ) minimum = 64;
            specified = 1;
            if( AreaSize == 0xffff ) {
                AreaSize = size * 2;
                specified = 0;
            } else if( AreaSize == 0 ) specified = 0;
            if( AreaSize < minimum ) {
                AreaSize = minimum;
                if( specified ) LnkMsg( 0x3274, "l", (unsigned long)minimum * 16 );
            }
            result = CurrLoc.seg + (unsigned long)(AreaSize - size);
            if( result > 0xffff ) LnkMsg( 0x3269, "l", (unsigned long)AreaSize << 4 );
            else CurrLoc.seg = result;
        }
        AreaSize = CurrLoc.seg - Stash.seg;
    }
    save = CurrLoc;
    max.off = max.seg = 0;
    for( sect = first; sect != 0; sect = sect->next_sect ) {
        CurrSect = sect;
        sect->sect_addr.off = save.off;
        sect->sect_addr.seg = save.seg;
        AllocClasses( sect->classlist );
        if( sect->areas != 0 ) AllocAreas( sect->areas );
        NormalizeAddr();
        AddSize( 2 );
        NormalizeAddr();
        if( CurrLoc.seg > max.seg || (CurrLoc.seg == max.seg && CurrLoc.off > max.off) )
            max = CurrLoc;
        CurrLoc = save;
    }
    CurrLoc = max;
}

void ProcOvlPubs( void )
{
    WriteVectors();
    ProcAllOvl( DoSecPubs );
}

extern void XReportSymAddr( symbol * );
extern void WriteOvlHead( void );
void WriteVectors( void )
{
    vecnode *vec;
    unsigned num;
    address addr;
    symbol *sym;

    WriteMapNL( 2 );
    XReportSymAddr( OverlayTable );
    XReportSymAddr( OverlayTableEnd );
    XReportSymAddr( OvlVecStart );
    XReportSymAddr( OvlVecEnd );
    WriteOvlHead();
    num = 0;
    for( vec = OvlVectors; vec != 0; vec = vec->next ) {
        GetVecAddr( ++num, &addr );
        sym = vec->entry;
        WriteMap( "%a section %d : %S", addr,
                  (unsigned)sym->p.leader->section->ovl_num, sym );
    }
}

void GetVecAddr( unsigned num, address *addr )
{
    unsigned off;

    *addr = OvlvecAddr;
    --num;
    off = num * ((LinkFlags & 0x1000) ? 8 : 10);
    addr->off += off;
    if( LinkFlags & 0x1000 ) {
        addr->off += (unsigned long)addr->seg << 4;
        addr->seg = 0;
    }
}

extern void WriteSegs( class_entry * );
extern void StartMapSort( void );
extern void WritePubHead( void );
extern void ProcPubs( mod_entry *, section * );
extern void ProcOvlSectPubs( section * );
extern void FinishMapSort( void );
void DoSecPubs( section *sect )
{
    address addr;

    WriteMapNL( 2 );
    addr.off = sect->sect_addr.off;
    addr.seg = sect->sect_addr.seg;
    WriteMap( "Overlay section %d address %a", (unsigned)sect->ovl_num, addr );
    WriteMap( "====================================" );
    WriteSegs( sect->classlist );
    StartMapSort();
    if( MapFlags & 1 ) WritePubHead();
    ProcPubs( sect->mods, sect );
    ProcOvlSectPubs( sect );
    FinishMapSort();
}

void GetVecAddr2( unsigned num, fixup_ref *ref )
{
    ref->addr = OvlvecAddr;
    ref->addr.off += (num - 1) * ((LinkFlags & 0x1000) ? 8 : 10);
}

#pragma pack(1)
typedef struct dos_addr {
    unsigned short off, seg;
} dos_addr;
#pragma pack()
extern void WriteReloc( group_entry *, unsigned long, void *, unsigned short );
void EmitOvlVectors( void )
{
    dos_addr addr;
    symbol *sym;
    char *name;
    unsigned char isshort;

    addr.seg = OvlGroup->grp_addr.seg;
    addr.off = OvltabAddr.off + 4;
    WriteReloc( OvlGroup, addr.off, &addr, sizeof(addr) );
    addr.off = OvltabAddr.off + 6;
    WriteReloc( OvlGroup, addr.off, &addr, sizeof(addr) );
    isshort = 0;
    if( LinkFlags & 0x0400 ) name = "__NOVLLDR__";
    else if( LinkFlags & 0x1000 ) { name = "__SOVLLDR__"; isshort = 1; }
    else name = "__LOVLLDR__";
    sym = RefISymbol( name );
    if( !(sym->info & 0x1000) ) LnkMsg( 0x3318, 0 );
    else if( isshort ) ShortVectors( sym );
    else LongVectors( sym );
}

#pragma pack(1)
typedef struct lvec {
    unsigned char call;
    unsigned short loader, sec_num;
    unsigned char jmpto;
    dos_addr target;
} lvec;
#pragma pack()
void LongVectors( symbol *loadsym )
{
    vecnode *vec;
    lvec entry;
    dos_addr addr;
    unsigned off, num;
    unsigned long loader;
    long diff;

    entry.call = 0xe8;
    entry.jmpto = 0xea;
    loader = loadsym->addr.off;
    off = OvlvecAddr.off;
    addr.seg = OvlGroup->grp_addr.seg;
    num = 1;
    for( vec = OvlVectors; vec != 0; vec = vec->next ) {
        diff = (long)(loader - off - 3);
        if( diff < -32768 || diff > 32767 ) LnkMsg( 0x3319, "d", num );
        entry.loader = diff;
        loadsym = vec->entry;
        entry.sec_num = loadsym->p.leader->section->ovl_num;
        entry.target.off = loadsym->addr.off;
        if( LinkFlags & 0x0400 ) {
            entry.target.seg = loadsym->addr.seg - loadsym->p.leader->group->grp_addr.seg;
        } else {
            entry.target.seg = loadsym->addr.seg;
            addr.off = off + 8;
            WriteReloc( OvlGroup, addr.off, &addr, sizeof(addr) );
        }
        PutOvlInfo( off, &entry, sizeof(entry) );
        off += sizeof(entry);
        ++num;
    }
}

void PutOvlInfo( unsigned off, void *data, unsigned len )
{
    memcpy( (void *)(OvlGroup->data + off - OvlGroup->grp_addr.off), data, len );
}

extern address StartInfo;
extern symbol *FindISymbol( char * );
void SetOvlStartAddr( void )
{
    symbol *sym;

    Stash = StartInfo;
    if( LinkFlags & 0x0400 ) sym = FindISymbol( "__NOVLINIT__" );
    else if( LinkFlags & 0x1000 ) sym = FindISymbol( "__SOVLINIT__" );
    else sym = FindISymbol( "__LOVLINIT__" );
    if( sym != 0 && (sym->info & 0x1000) ) {
        StartInfo.off = sym->addr.off;
        StartInfo.seg = sym->addr.seg;
    }
}

#pragma pack(1)
typedef struct ovltab_prolog {
    unsigned char major, minor;
    dos_addr start;
    unsigned short delta, ovl_size;
} ovltab_prolog;
#pragma pack()
void EmitOvlTable( void )
{
    ovltab_prolog prolog;
    unsigned short end;
    unsigned off, len;
    outfilelist *file;

    off = OvltabAddr.off;
    prolog.major = 3;
    prolog.minor = 0;
    prolog.start.off = Stash.off;
    prolog.start.seg = Stash.seg;
    prolog.delta = 0;
    prolog.ovl_size = AreaSize;
    PutOvlInfo( off, &prolog, sizeof(prolog) );
    off += sizeof(prolog);
    off = EmitOvlAreaEntry( off, Root->areas );
    end = 0xffff;
    PutOvlInfo( off, &end, sizeof(end) );
    off += sizeof(end);
    for( file = OutFiles; file != 0; file = file->next ) {
        len = strlen( file->fname ) + 1;
        PutOvlInfo( off, file->fname, len );
        off += len;
    }
}

#pragma pack(1)
typedef struct ovltab_entry {
    unsigned short flags_anc, relocs, start_para, code_handle, num_paras, fname;
    unsigned long disk_addr;
} ovltab_entry;
#pragma pack()
unsigned EmitOvlAreaEntry( unsigned off, ovl_area *area )
{
    for( ; area != 0; area = area->next ) off = EmitOvlEntry( off, area->sections );
    return off;
}
unsigned EmitOvlEntry( unsigned off, section *sect )
{
    ovltab_entry entry;

    entry.code_handle = 0;
    for( ; sect != 0; sect = sect->next_sect ) {
        entry.flags_anc = sect->parent->ovl_num;
        if( sect == NonSect ) entry.flags_anc |= 0x8000;
        entry.fname = sect->outfile->ovlfnoff;
        entry.relocs = sect->relocs;
        entry.start_para = sect->sect_addr.seg + (sect->sect_addr.off >> 4);
        entry.num_paras = (sect->size + 15) >> 4;
        entry.disk_addr = sect->u.file_loc;
        PutOvlInfo( off, &entry, sizeof(entry) );
        off += sizeof(entry);
        off = EmitOvlAreaEntry( off, sect->areas );
    }
    return off;
}

extern void WriteNulls( int, unsigned long, char * );
void PadOvlFiles( void )
{
    outfilelist *file;
    unsigned pad;

    for( file = OutFiles; file != 0; file = file->next ) {
        pad = 16 - (file->file_loc & 15);
        if( pad != 16 ) {
            if( file->handle == -1 ) OpenOvlFile( file );
            WriteNulls( file->handle, pad, file->fname );
            file->file_loc += pad;
        }
    }
}
