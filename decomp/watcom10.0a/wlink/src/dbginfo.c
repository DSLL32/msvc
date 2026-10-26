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

#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include "cmdline.h"
#include <recon.h>

#ifdef __WATCOMC__
#pragma intrinsic(memcpy,strlen)
#endif

#pragma pack(1)
/* The old section debug workspace is154bytes; fields are identified as used. */
struct debug_info {
    dbi_section locallinks, local, typelinks, type, linelinks, line;
    dbi_section mod, global, addr;
    unsigned long DBIWrite;
    unsigned long line_link_start;
    short modnum;
};
#pragma pack()

/* Pass-one sizing shares the24-byte workspace with pass-two demand state. */
#pragma pack(1)
struct dbi_demand {
    unsigned long offset;
    unsigned short count, used;
};
struct odbimodinfo {
    union {
        unsigned long linelinksize;
        struct { dbi_demand type, local, line; } p2;
    } u;
};
#pragma pack()
typedef char old_odbimodinfo_is_24[(sizeof(odbimodinfo) == 24) ? 1 : -1];

#pragma pack(1)
typedef struct dbgheader {
    unsigned short signature;
    unsigned char exe_major_ver, exe_minor_ver, obj_major_ver, obj_minor_ver;
    unsigned short lang_size, segment_size;
    unsigned long debug_size;
} dbgheader;
#pragma pack()
typedef char dbgheader_is_14[(sizeof(dbgheader) == 14) ? 1 : -1];
static unsigned long DBISize = sizeof( dbgheader ); /* original4422b8 */
static dbgheader Master = { 0x8386, 3, 0, 0, 0, 0, 0, 0 }; /* original4422bc */

static snamelist *DBISourceLang;   /* original4422cc */

void ODBIInit( section *sect )
{
    if( DBISourceLang == 0 ) {
        DBISourceLang = LangAlloc( 1, "C" );
        DBISourceLang->next = 0;
    }
    sect->dbg_info = PermAlloc( sizeof(debug_info) );
    memset( sect->dbg_info, 0, sizeof(debug_info) );
}

snamelist *LangAlloc( unsigned char len, void *buff )
{
    snamelist *node;

    node = PermAlloc( sizeof(*node) + len );
    node->len = len;
    memcpy( node->name, buff, len );
    node->name[len] = '\0';
    return node;
}

void ODBIInitModule( mod_entry *mod )
{
    if( CurrSect->dbg_info == 0 ) return;
    mod->d.o = PermAlloc( sizeof( odbimodinfo ) );
    mod->d.o->u.linelinksize = 0;
}

void ODBIP1Source( unsigned char major, unsigned char minor,
                   unsigned char *name, int len )
{
    snamelist *node;

    if( Master.obj_major_ver == 0 ) Master.obj_major_ver = major;
    if( major != Master.obj_major_ver ) {
        LnkMsg( 0x7236, 0 );
        CurrMod->modinfo &= ~6;
    }
    if( minor > Master.obj_minor_ver ) Master.obj_minor_ver = minor;
    if( FindMatch( len, name ) == 0xffff ) {
        node = LangAlloc( len, name );
        node->next = DBISourceLang->next;
        DBISourceLang->next = node;
    }
}

unsigned short FindMatch( unsigned char len, void *buff )
{
    snamelist *node;
    unsigned offset;

    offset = 0;
    for( node = DBISourceLang; node != 0; node = node->next ) {
        if( node->len == len && memicmp( buff, node->name, len ) == 0 ) {
            return offset;
        }
        offset += node->len + 1;
    }
    return 0xffff;
}

void ODBIAddAddrInfo( seg_leader *seg, unsigned long oldsize )
{
    if( !(CurrMod->modinfo & 0x17) || (seg->info & 0x40) ) return;
    if( seg->info & 0x7000 ) {
        if( (long)seg->saved_size > 0xffffL ) {
            if( (seg->info & 0x7000) == 0x6000 ) {
                LnkMsg( 0x725f, "s", "type" );
                CurrMod->modinfo &= ~2;
            } else {
                LnkMsg( 0x725f, "s", "local" );
                CurrMod->modinfo &= ~4;
            }
        }
    } else if( (long)(seg->saved_size - oldsize) > 0 ) {
        ++seg->num;
        seg->info |= 0x40;
    }
}

void ODBIAddLocal( unsigned short info, unsigned long size )
{
    debug_info *dinfo;

    dinfo = CurrSect->dbg_info;
    if( dinfo == 0 ) return;
    if( (info & 0x7000) == 0x6000 ) DoAddLocal( &dinfo->typelinks, size );
    else if( (info & 0x7000) == 0x7000 ) DoAddLocal( &dinfo->locallinks, size );
}

void DoAddLocal( dbi_section *dbi, unsigned long size )
{
    if( dbi->size == 0 || dbi->size + size > 0x4000UL ) {
        dbi->size = 0;
        dbi->curr += sizeof( unsigned long );
    }
    dbi->size += size;
}

void ODBIAddGlobal( symbol *sym, unsigned len )
{
    debug_info *dinfo;

    dinfo = CurrSect->dbg_info;
    if( dinfo == 0 || (CurrMod->modinfo & 8) || !(CurrMod->modinfo & 0x17) ) return;
    if( (sym->info & 0x0100) && !(CurrMod->modinfo & 0x10) ) return;
    if( len > 255 ) {
        LnkMsg( 0x3263, "s", sym->name );
        len = 255;
    }
    dinfo->global.curr += len + 10;
}

void ODBIAddLines( seg_leader *seg, unsigned char is32, unsigned char isdata )
{
    debug_info *dinfo;
    unsigned long qty, size;

    dinfo = CurrSect->dbg_info;
    if( dinfo == 0 || !(CurrMod->modinfo & 1) ) return;
    qty = (unsigned)(EOObjRec - ObjBuff);
    if( is32 ) qty /= 6;
    else qty /= 4;
    size = qty * 6 + 6;
    if( isdata ) ((segdata *)seg)->line_size += size;
    else dinfo->line.curr += size;
    DoAddLocal( &dinfo->linelinks, size );
}

void ODBIP1ModuleScanned( void )
{
    debug_info *dinfo;

    dinfo = CurrSect->dbg_info;
    if( dinfo == 0 ) return;
    CurrMod->d.o->u.linelinksize += dinfo->linelinks.curr - dinfo->line_link_start;
    dinfo->typelinks.size = 0;
    dinfo->locallinks.size = 0;
    dinfo->linelinks.size = 0;
    dinfo->linelinks.curr = dinfo->line_link_start;
    ResetAddrInfo();
}

void ODBIGenModule( mod_entry *mod )
{
    debug_info *dinfo;
    unsigned len;

    dinfo = CurrSect->dbg_info;
    if( dinfo == 0 || !(mod->modinfo & 0x17) ) return;
    if( DBIModName == 0 ) len = strlen( mod->name );
    else len = DBIModNameLen;
    dinfo->mod.curr += len + 21;
    dinfo->linelinks.curr += CurrMod->d.o->u.linelinksize;
    dinfo->line_link_start = dinfo->linelinks.curr;
}

void ODBIDefClass( class_entry *class, unsigned long size )
{
    debug_info *dinfo;

    dinfo = CurrSect->dbg_info;
    if( dinfo == 0 ) return;
    if( class->flags & 4 ) dinfo->type.curr += size;
    else if( class->flags & 8 ) dinfo->local.curr += size;
}

void ODBIAddrSectStart( section *sect )
{
    debug_info *dptr;

    dptr = sect->dbg_info;
    if( dptr == 0 ) return;
    if( dptr->local.curr > 0 || dptr->type.curr > 0 || dptr->line.curr > 0 ) {
        dptr->linelinks.curr += 4;
    }
    dptr->locallinks.start = 18;
    dptr->locallinks.size = dptr->locallinks.curr;
    dptr->locallinks.curr = DBIAlloc( dptr->locallinks.curr );
    dptr->locallinks.init = dptr->locallinks.curr;
    dptr->typelinks.start = dptr->locallinks.start + dptr->locallinks.size;
    dptr->typelinks.size = dptr->typelinks.curr;
    dptr->typelinks.curr = DBIAlloc( dptr->typelinks.curr );
    dptr->typelinks.init = dptr->typelinks.curr;
    dptr->linelinks.start = dptr->typelinks.start + dptr->typelinks.size;
    dptr->linelinks.size = dptr->linelinks.curr;
    dptr->linelinks.curr = DBIAlloc( dptr->linelinks.curr );
    dptr->linelinks.init = dptr->linelinks.curr;
    dptr->local.start = dptr->linelinks.start + dptr->linelinks.size;
    dptr->local.size = dptr->local.curr;
    dptr->local.curr = DBIAlloc( dptr->local.curr );
    dptr->local.init = dptr->local.curr;
    dptr->type.start = dptr->local.start + dptr->local.size;
    dptr->type.size = dptr->type.curr;
    dptr->type.curr = DBIAlloc( dptr->type.curr );
    dptr->type.init = dptr->type.curr;
    dptr->line.start = dptr->type.start + dptr->type.size;
    dptr->line.size = dptr->line.curr;
    dptr->line.curr = DBIAlloc( dptr->line.curr );
    dptr->line.init = dptr->line.curr;
    dptr->mod.start = dptr->line.start + dptr->line.size;
    dptr->mod.size = dptr->mod.curr;
    dptr->mod.curr = DBIAlloc( dptr->mod.curr );
    dptr->mod.init = dptr->mod.curr;
    dptr->global.start = dptr->mod.start + dptr->mod.size;
    dptr->addr.start = dptr->global.start + dptr->global.curr;
    dptr->global.size = 0;
    dptr->global.curr = DBIAlloc( dptr->global.curr );
    dptr->global.init = dptr->global.curr;
    dptr->addr.curr = DBIAlloc( dptr->addr.size );
    dptr->addr.init = dptr->addr.curr;
    dptr->DBIWrite = dptr->global.curr;
    dptr->modnum = -1;
    AllocDBIClasses( sect->classlist, dptr );
}

void AllocDBIClasses( class_entry *class, debug_info *dinfo )
{
    for( ; class != 0; class = class->next_class ) {
        if( class->flags & 4 ) AllocDBISegs( class->segs, dinfo->type.curr );
        else if( class->flags & 8 ) AllocDBISegs( class->segs, dinfo->local.curr );
    }
}

void AllocDBISegs( seg_leader *seg, unsigned long loc )
{
    for( ; seg != 0; seg = seg->next_seg ) {
        seg->data = loc;
        loc += CAlign( seg->saved_size, (seg->info & 0x38) >> 3 );
    }
}

void ODBIAddModule( mod_entry *mod, section *sect )
{
    debug_info *dptr;

    dptr = sect->dbg_info;
    if( dptr == 0 || !(mod->modinfo & 0x17) ) return;
    memset( mod->d.o, 0, sizeof( odbimodinfo ) );
    ++dptr->modnum;
}

#pragma pack(1)
typedef struct gblinfo {
    unsigned long off;
    unsigned short seg, mod_idx;
    unsigned char flags;
    char name[1];
} gblinfo;
#pragma pack()

extern void DoName( char *, char *, int );
extern void DumpInfo( debug_info *, void *, unsigned );

void ODBIGenGlobal( symbol *sym, section *sect )
{
    debug_info *dptr;
    gblinfo *data;
    seg_leader *seg;
    char *name;
    int len, entrylen;

    dptr = sect->dbg_info;
    if( dptr == 0 || !(CurrMod->modinfo & 0x17) ) return;
    if( (CurrMod->modinfo & 8) && !(sym->info & 0x2000) ) return;
    if( (sym->info & 0x0100) && !(CurrMod->modinfo & 0x10)
        && (sym->info & 15) != 2 ) return;
    name = sym->name;
    len = strlen( name );
    if( len > 255 ) len = 255;
    entrylen = sizeof(gblinfo) + len;
    data = alloca( entrylen );
    data->off = sym->addr.off;
    data->seg = sym->addr.seg;
    data->mod_idx = dptr->modnum;
    data->flags = 0;
    if( sym->info & 0x0100 ) data->flags |= 1;
    seg = sym->p.seg;
    if( seg != 0 ) {
        if( seg->info & 0x0200 ) data->flags |= 4;
        else data->flags |= 2;
    }
    DoName( name, data->name, len );
    DumpInfo( dptr, data, entrylen );
    dptr->global.size += entrylen;
}

void DoName( char *name, char *dest, int len )
{
    dest[0] = len;
    memcpy( dest + 1, name, len );
}

void DumpInfo( debug_info *dinfo, void *data, unsigned len )
{
    memcpy( (void *)dinfo->DBIWrite, data, len );
    dinfo->DBIWrite += len;
}

extern void GenAddrInfo( section * );

void ODBIP2Start( section *sect )
{
    debug_info *dptr;

    dptr = sect->dbg_info;
    if( (LinkFlags & 0x4000) && dptr != 0 ) {
        GenAddrInfo( sect );
        dptr->DBIWrite = dptr->line.curr;
        dptr->modnum = 0;
    }
}

extern void BuildAddrInfoList( section *, dbi_addr_range ** );
extern void FixAddrInfo( dbi_addr_range *, section * );
extern void ZeroAddrInfo( debug_info * );
extern void GenAddrHeaders( dbi_addr_range *, section * );
extern void GenAddrEntries( dbi_addr_range *, section * );

void GenAddrInfo( section *sect )
{
    dbi_addr_range *list;
    debug_info *dptr;
    unsigned char extra;

    list = 0;
    BuildAddrInfoList( sect, &list );
    extra = sect == Root && NonSect != 0;
    if( extra ) BuildAddrInfoList( NonSect, &list );
    FixAddrInfo( list, sect );
    dptr = sect->dbg_info;
    if( dptr->addr.curr == 0 ) {
        ZeroAddrInfo( dptr );
    } else {
        dptr->addr.size = dptr->addr.curr;
        dptr->addr.curr = DBIAlloc( dptr->addr.curr );
        dptr->addr.init = dptr->addr.curr;
        dptr->DBIWrite = dptr->addr.curr;
        GenAddrHeaders( list, sect );
        GenAddrEntries( list, sect );
        if( extra ) GenAddrEntries( list, NonSect );
        FreeList( (path_entry *)list );
    }
}

extern dbi_addr_range *AddAddrInfo( seg_leader *, dbi_addr_range ** );

void BuildAddrInfoList( section *sect, dbi_addr_range **list )
{
    class_entry *class;
    seg_leader *seg;
    dbi_addr_range *range;

    for( class = sect->classlist; class != 0; class = class->next_class ) {
        if( class->flags & 0x0c ) continue;
        for( seg = class->segs; seg != 0; seg = seg->next_seg ) {
            if( (seg->info & 0x0038) && seg->num > 0 ) {
                range = AddAddrInfo( seg, list );
                range->flags = (range->flags & ~1) | (sect == NonSect);
            }
        }
    }
}

dbi_addr_range *AddAddrInfo( seg_leader *seg, dbi_addr_range **list )
{
    dbi_addr_range *range, *prev;
    dbi_addr_range **link;
    unsigned short selector;

    if( seg->num > 0x2a00 ) LnkMsg( 0x3481, 0 );
    selector = seg->group != 0 ? seg->group->grp_addr.seg : seg->seg_addr.seg;
    link = list;
    prev = 0;
    for( range = *link; range != 0; range = *link ) {
        if( selector < range->seg ) break;
        if( selector == range->seg && !(range->flags & 2) ) {
            if( (unsigned)range->num + seg->num <= 0x2a00 ) {
                range->last = seg;
                range->num += seg->num;
                return range;
            }
            range->flags |= 2;
        }
        prev = range;
        link = &range->next;
    }
    range = ChkLAlloc( sizeof(*range) );
    range->seg = selector;
    range->num = seg->num;
    /* Original defect: link can be the stack-owned head, not a range node.
     * Its byte+28 controls this base selection.  Preserve that Win32 read;
     * native stack layout differs.  The NT-link first-node packet observed
     * flag0: use that deterministic head-only choice, not an initialized
     * sentinel invariant.  Other predecessor nodes retain their real flags. */
#ifdef __linux__
    if( seg->group != 0 && (prev == 0 || !(prev->flags & 2)) )
#else
    if( seg->group != 0 && !(((dbi_addr_range *)link)->flags & 2) )
#endif
        range->start = seg->group->grp_addr.off;
    else
        range->start = seg->seg_addr.off;
    range->prev = 0;
    range->last = seg;
    range->next = *link;
    range->curr = range->start;
    /* Original allocation preserves unused flag bits; none have meaning. */
    range->flags = 0;
    *link = range;
    return range;
}

void FixAddrInfo( dbi_addr_range *list, section *sect )
{
    debug_info *dptr;

    dptr = sect->dbg_info;
    for( ; list != 0; list = list->next )
        dptr->addr.curr += (unsigned long)list->num * 6 + 8;
}

void GenAddrHeaders( dbi_addr_range *list, section *sect )
{
    debug_info *dptr;

    dptr = sect->dbg_info;
    for( ; list != 0; list = list->next ) {
        if( list->flags & 1 ) list->num |= 0x8000;
        DumpInfo( dptr, &list->start, 8 );
        list->num &= 0x7fff;
        list->write_pos = dptr->DBIWrite;
        dptr->DBIWrite += (unsigned long)list->num * 6;
    }
}

void GenAddrEntries( dbi_addr_range *list, section *sect )
{
    class_entry *class;
    seg_leader *seg;
    dbi_addr_range *range;
    unsigned short selector;
    unsigned long offset, size;

    for( class = sect->classlist; class != 0; class = class->next_class ) {
        if( class->flags & 0x0c ) continue;
        for( seg = class->segs; seg != 0; seg = seg->next_seg ) {
            if( !(seg->info & 0x0038) || seg->num == 0 ) continue;
            selector = seg->group != 0 ? seg->group->grp_addr.seg : seg->seg_addr.seg;
            offset = seg->seg_addr.off + ((unsigned long)seg->seg_addr.seg - selector) * 16;
            for( range = list; range->seg != selector || (range->flags & 4); range = range->next )
                ;
            if( range->prev != 0 )
                range->prev->num = (unsigned short)(offset - range->curr);
            size = seg->saved_size;
            seg->saved_size = range->write_pos;
            seg->u.p2.prev_offset = 0;
            seg->u.p2.prev_mod = 0;
            range->write_pos += (unsigned long)seg->num * 6;
            seg->num = 0;
            if( size != 0 ) {
                range->prev = seg;
                range->curr = offset + size;
            }
            if( seg == range->last ) range->flags |= 4;
        }
    }
}

static unsigned short ObjSourceLang;   /* original4422d0 */

void ODBIP2Source( unsigned char *buff, int len )
{
    ObjSourceLang = FindMatch( (unsigned char)len, buff );
    if( ObjSourceLang == 0xffff ) ObjSourceLang = 0;
}

extern void DumpAddrInfo( seg_leader *, debug_info * );

void ODBIGenAddrInfo( seg_leader *seg, unsigned long start,
                      unsigned long padding, section *sect )
{
    long size;

    if( sect == NonSect ) sect = Root;
    if( sect->dbg_info != 0 ) {
        seg->u.p2.prev_offset += padding;
        size = seg->size - start;
        if( size > 0 ) {
            DumpAddrInfo( seg, sect->dbg_info );
            seg->u.p2.prev_offset += size;
        }
    }
}

#pragma pack(1)
typedef struct addrinfo {
    unsigned long size;
    unsigned short mod_idx;
} addrinfo;
#pragma pack()

void DumpAddrInfo( seg_leader *seg, debug_info *dptr )
{
    addrinfo rec;

    if( (seg->info & 0x40) || !(CurrMod->modinfo & 0x17) ) return;
    if( seg->info & 0x80 ) {
        rec.size = seg->u.p2.prev_offset;
        rec.mod_idx = seg->u.p2.prev_mod;
        memcpy( (void *)seg->saved_size, &rec, sizeof(rec) );
        seg->u.p2.prev_offset = 0;
        seg->saved_size += sizeof(rec);
        seg->info &= ~0x80;
    }
    seg->u.p2.prev_mod = dptr->modnum;
    if( CurrMod->modinfo & 0x17 ) seg->info |= 0x80;
    seg->info |= 0x40;
}

extern void GenLocalBlocks( dbi_section *, dbi_section *, dbi_demand *, unsigned long );

void ODBIGenLocal( seg_leader *seg, unsigned long size )
{
    debug_info *dptr;
    odbimodinfo *mod;

    dptr = CurrSect->dbg_info;
    if( dptr == 0 ) return;
    mod = CurrMod->d.o;
    if( (seg->info & 0x7000) == 0x6000 )
        GenLocalBlocks( &dptr->type, &dptr->typelinks, &mod->u.p2.type, size );
    else if( (seg->info & 0x7000) == 0x7000 )
        GenLocalBlocks( &dptr->local, &dptr->locallinks, &mod->u.p2.local, size );
}

void GenLocalBlocks( dbi_section *data, dbi_section *links,
                      dbi_demand *mod, unsigned long size )
{
    unsigned long offset;

    if( mod->used == 0 || (unsigned long)mod->used + size > 0x4000 ) {
        offset = data->start + data->curr - data->init;
        memcpy( (void *)links->curr, &offset, sizeof(offset) );
        ++mod->count;
        if( mod->used == 0 ) mod->offset = links->start + links->curr - links->init;
        else mod->used = 0;
        links->curr += sizeof(offset);
    }
    data->curr += size;
    mod->used += size;
}

#pragma pack(1)
typedef struct ln_off_386 {
    unsigned short linnum;
    unsigned long off;
} ln_off_386;
typedef struct ln_off_286 {
    unsigned short linnum, off;
} ln_off_286;
typedef struct lineseg {
    unsigned long segment;
    unsigned short num;
} lineseg;
#pragma pack()
extern segnode *FindSegdata( seg_leader * );
extern int CmpLn386( const void *, const void * );
extern int CmpLn286( const void *, const void * );

void ODBIGenLines( seg_leader *seg, seg_leader *seg2,
                    unsigned long adjust, unsigned char is32 )
{
    unsigned len, qty;
    unsigned long prev;
    unsigned char needsort;
    unsigned char *curr;
    debug_info *dptr;
    segnode *first;
    lineseg header;
    ln_off_386 tmp;

    dptr = CurrSect->dbg_info;
    if( dptr == 0 || !(CurrMod->modinfo & 1) ) return;
    len = EOObjRec - ObjBuff;
    qty = is32 ? len / sizeof(ln_off_386) : len / sizeof(ln_off_286);
    GenLocalBlocks( &dptr->line, &dptr->linelinks, &CurrMod->d.o->u.p2.line,
                    qty * sizeof(ln_off_386) + sizeof(lineseg) );
    DumpAddrInfo( seg, dptr );
    header.segment = seg->saved_size - dptr->addr.init;
    header.num = qty;
    DumpInfo( dptr, &header, sizeof(header) );
    first = FindSegdata( seg );
    if( first == 0 ) first = FindSegdata( seg2 );
    adjust -= first->addr.off;
    prev = 0;
    needsort = 0;
    if( is32 ) {
        for( curr = ObjBuff; curr < EOObjRec; curr += sizeof(ln_off_386) ) {
            ((ln_off_386 *)curr)->off += adjust;
            if( prev > ((ln_off_386 *)curr)->off ) needsort = 1;
            prev = ((ln_off_386 *)curr)->off;
        }
        if( needsort ) qsort( ObjBuff, qty, sizeof(ln_off_386), CmpLn386 );
        DumpInfo( dptr, ObjBuff, len );
    } else {
        for( curr = ObjBuff; curr < EOObjRec; curr += sizeof(ln_off_286) ) {
            if( prev > ((ln_off_286 *)curr)->off ) needsort = 1;
            prev = ((ln_off_286 *)curr)->off;
        }
        if( needsort ) qsort( ObjBuff, qty, sizeof(ln_off_286), CmpLn286 );
        for( curr = ObjBuff; curr < EOObjRec; curr += sizeof(ln_off_286) ) {
            tmp.linnum = ((ln_off_286 *)curr)->linnum;
            tmp.off = ((ln_off_286 *)curr)->off + adjust;
            DumpInfo( dptr, &tmp, sizeof(tmp) );
        }
    }
}

#pragma pack(1)
typedef struct old_modinfo {
    unsigned short language;
    unsigned long local_offset;
    unsigned short local_count;
    unsigned long type_offset;
    unsigned short type_count;
    unsigned long line_offset;
    unsigned short line_count;
    char name[1];
} old_modinfo;
#pragma pack()
typedef char old_modinfo_is_21[(sizeof(old_modinfo) == 21) ? 1 : -1];

void ODBIP2Module( void )
{
    debug_info *dptr;
    odbimodinfo *rec;
    old_modinfo *info;
    char *name;
    unsigned len;

    dptr = CurrSect->dbg_info;
    if( dptr == 0 || !(CurrMod->modinfo & 0x17) ) return;
    rec = CurrMod->d.o;
    if( DBIModName == 0 ) {
        name = CurrMod->name;
        len = strlen( name );
    } else {
        name = DBIModName;
        len = DBIModNameLen;
    }
    info = alloca( len + sizeof( old_modinfo ) );
    info->type_count = rec->u.p2.type.count;
    info->type_offset = rec->u.p2.type.offset;
    info->local_count = rec->u.p2.local.count;
    info->local_offset = rec->u.p2.local.offset;
    info->line_count = rec->u.p2.line.count;
    info->line_offset = rec->u.p2.line.offset;
    DoName( name, info->name, len );
    info->language = ObjSourceLang;
    ObjSourceLang = 0;
    memcpy( (void *)dptr->mod.curr, info, len + sizeof( old_modinfo ) );
    dptr->mod.curr += len + sizeof( old_modinfo );
    ++dptr->modnum;
    ResetAddrInfo();
}

/* Permanent debug storage needs no per-section release in this version. */
void ODBISectCleanup( section *sect )
{
    sect = sect;
}

void ODBIFini( section *sect )
{
    debug_info *dptr;
    class_entry *class;
    seg_leader *seg;
    addrinfo rec;
    unsigned long spot;

    dptr = (sect == NonSect) ? Root->dbg_info : sect->dbg_info;
    if( dptr == 0 ) return;
    for( class = sect->classlist; class != 0; class = class->next_class ) {
        if( class->flags & 0x0c ) continue;
        for( seg = class->segs; seg != 0; seg = seg->next_seg ) {
            if( seg->info & 0x80 ) {
                rec.size = seg->u.p2.prev_offset + seg->num;
                rec.mod_idx = seg->u.p2.prev_mod;
                memcpy( (void *)seg->saved_size, &rec, sizeof( rec ) );
            }
        }
    }
    if( dptr->local.size > 0 || dptr->type.size > 0 || dptr->line.size > 0 ) {
        spot = dptr->line.start + (dptr->line.curr - dptr->line.init);
        memcpy( (void *)dptr->linelinks.curr, &spot, sizeof( spot ) );
    }
}

extern void DBIWriteLocal( void *, unsigned short );
extern unsigned short WriteSegValues( void );
extern void WriteDBISecs( section * );
extern void WriteOvlSecs( void );
extern void WriteLoad( void *, unsigned short );

void OWriteDBI( void )
{
    snamelist *node;

    CurrSect = Root;
    Master.lang_size = 0;
    for( node = DBISourceLang; node != 0; node = node->next ) {
        Master.lang_size += node->len + 1;
        DBIWriteLocal( node->name, node->len + 1 );
    }
    DBISourceLang = 0;
    Master.segment_size = WriteSegValues();
    WriteDBISecs( Root );
    WriteOvlSecs();
    Master.debug_size = DBISize;
    if( Master.obj_major_ver == 0 ) Master.obj_major_ver = 1;
    WriteLoad( &Master, sizeof( Master ) );
}

void DBIWriteInfo( void *buffer, unsigned size )
{
    if( size != 0 ) {
        DBISize += size;
        QWrite( CurrSect->outfile->handle, buffer, size, CurrSect->outfile->fname );
    }
}

void DBIWriteLocal( void *buffer, unsigned short size )
{
    if( size != 0 ) {
        DBISize += size;
        WriteLoad( buffer, size );
    }
}

unsigned short WriteSegValues( void )
{
    unsigned short segments[2], *buffer;
    unsigned length;
    group_entry *group;

    if( FmtData.type & 0x9c00 ) {
        segments[0] = 1;
        DBIWriteLocal( segments, 2 );
        return 2;
    } else if( FmtData.type & 0x100 ) {
        segments[0] = 1;
        segments[1] = 2;
        DBIWriteLocal( segments, 4 );
        return 4;
    }
    buffer = (unsigned short *)TokBuff;
    length = 0;
    for( group = Groups; group != 0; group = group->next_group ) {
        *buffer++ = group->grp_addr.seg;
        length += 2;
    }
    DBIWriteLocal( TokBuff, length );
    return length;
}

#pragma pack(1)
typedef struct sectheader {
    unsigned long mod_offset, gbl_offset, addr_offset, section_size;
    unsigned short section_id;
} sectheader;
#pragma pack()
typedef char sectheader_is_18[(sizeof(sectheader) == 18) ? 1 : -1];

void WriteDBISecs( section *sect )
{
    debug_info *dptr;
    sectheader header;

    dptr = sect->dbg_info;
    if( dptr != 0 ) {
        header.section_size = dptr->addr.start + dptr->addr.size;
        header.mod_offset = dptr->mod.start;
        header.gbl_offset = dptr->global.start;
        header.addr_offset = dptr->addr.start;
        header.section_id = sect->ovl_num;
        DBIWriteLocal( &header, sizeof( header ) );
        DBIWriteInfo( (void *)dptr->locallinks.init, dptr->locallinks.size );
        DBIWriteInfo( (void *)dptr->typelinks.init, dptr->typelinks.size );
        DBIWriteInfo( (void *)dptr->linelinks.init, dptr->linelinks.size );
        DBIWriteInfo( (void *)dptr->local.init, dptr->local.size );
        DBIWriteInfo( (void *)dptr->type.init, dptr->type.size );
        DBIWriteInfo( (void *)dptr->line.init, dptr->line.size );
        DBIWriteInfo( (void *)dptr->mod.init, dptr->mod.size );
        DBIWriteInfo( (void *)dptr->global.init, dptr->global.size );
        DBIWriteInfo( (void *)dptr->addr.init, dptr->addr.size );
    }
}
