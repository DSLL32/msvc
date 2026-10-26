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


/*
 *  DBGCV -- routines for producing codeview information in the load file
 *
*/

#include <string.h>
#include "cmdline.h"
#include <recon.h>
#ifdef __WATCOMC__
#pragma intrinsic(strlen,memcpy)
#endif

/* The old writer uses direct pointers rather than later virtual-memory IDs. */
enum { CVSECT_MODULE, CVSECT_MISC, CVSECT_MODDIR, CVSECT_DIRECTORY, NUM_CV_SECTS };
static unsigned char *SectAddrs[NUM_CV_SECTS]; /* original45035c */
static unsigned TempIndex;          /* original45036c */

void CVInit( void )
{
    memset( SectAddrs, 0, sizeof(SectAddrs) );
    TempIndex = 0;
}

#pragma pack(1)
typedef struct cvmodinfo {
    unsigned long pubsize;
    unsigned short numlines, modidx, numsegs;
} cvmodinfo;
#pragma pack()
typedef char old_cvmodinfo_is_10[(sizeof(cvmodinfo) == 10) ? 1 : -1];

void CVInitModule( mod_entry *mod )
{
    cvmodinfo *info;

    info = PermAlloc( sizeof(cvmodinfo) );
    mod->d.cv = info;
    memset( info, 0, sizeof(cvmodinfo) );
}

void CVAddAddrInfo( seg_leader *seg, unsigned long start )
{
    cvmodinfo *info;

    if( !(seg->info & 0x7040) && (seg->info & 0x0200)
        && seg->saved_size - start > 0 ) {
        info = CurrMod->d.cv;
        ++info->numsegs;
        seg->info |= 0x40;
    }
}

extern void AddSubSection( unsigned char );
void CVAddLocal( unsigned short info, unsigned long length )
{
    if( info & 0x7000 ) {
        SectAddrs[CVSECT_MISC] += length;
        AddSubSection( 0 );
    }
}

#pragma pack(1)
typedef struct cv_directory_entry {
    unsigned short subsection, iMod;
    unsigned long lfo, cb;
} cv_directory_entry;
#pragma pack()
typedef char cv_directory_entry_is_12[(sizeof(cv_directory_entry) == 12) ? 1 : -1];

void AddSubSection( unsigned char ismodule )
{
    unsigned sect;

    sect = ismodule ? CVSECT_MODDIR : CVSECT_DIRECTORY;
    SectAddrs[sect] += sizeof(cv_directory_entry);
}

void CVAddLines( seg_leader *seg, unsigned char is32, unsigned char isdata )
{
    unsigned count;
    cvmodinfo *info;

    count = EOObjRec - ObjBuff;
    if( is32 ) count /= 6;
    else count /= 4;
    if( isdata ) {
        ((segdata *)seg)->line_size += count;
    } else {
        info = CurrMod->d.cv;
        info->numlines += count;
    }
}

extern void ResetAddrInfo( void );
void CVP1ModuleScanned( void )
{
    ResetAddrInfo();
}

void CVAddModLines( unsigned count )
{
    cvmodinfo *info;

    info = CurrMod->d.cv;
    info->numlines += count;
}

void CVAddGlobal( symbol *sym, unsigned length )
{
    unsigned size;
    unsigned char is32;
    cvmodinfo *info;

    if( sym->info & 0x0100 ) return;
    if( sym->p.seg == 0 ) is32 = 1;
    /* Imported symbols may still own 9B elimination edges. The original reads
     * a leader USE32 bit at edge+35, outside that owner. For this observed
     * defect choose zero/16bit; this does not recover valid segment storage. */
    else if( (sym->info & 15) == 6 ) is32 = 0;
    else if( sym->info & 0x10 ) is32 = ((segdata *)sym->p.seg)->acbp & 1;
    else is32 = (sym->p.leader->info & 0x0400) != 0;
    size = (length + (is32 ? 12 : 10) + 4) & ~3U;
    info = CurrMod->d.cv;
    info->pubsize += size;
    SectAddrs[CVSECT_MISC] += size;
}

void CVP1ModuleFinished( mod_entry *mod )
{
    unsigned char namelen;
    cvmodinfo *info;

    namelen = strlen( mod->name );
    info = mod->d.cv;
    SectAddrs[CVSECT_MODULE] += namelen + 21 + (info->numsegs - 1) * 12;
    AddSubSection( 1 );
    if( info->pubsize > 0 ) {
        AddSubSection( 0 );
        SectAddrs[CVSECT_MISC] += 4;
        info->pubsize += 4;
    }
    if( info->numlines > 0 ) {
        AddSubSection( 0 );
        SectAddrs[CVSECT_MISC] += ((namelen + 20) & ~3U) + 24
                              + ((info->numlines * 6U + 3) & ~3U);
    }
}

#pragma pack(1)
typedef struct cv_trailer {
    char sig[4];
    unsigned long offset;
} cv_trailer;
typedef struct cv_subsection_directory {
    unsigned short cbDirHeader, cbDirEntry;
    unsigned long cDir, lfoNextDir, flags;
} cv_subsection_directory;
#pragma pack()
unsigned long CVSize;          /* original450370 */
static unsigned char *CVBase;         /* original450374 */
extern void CVDumpInfo( unsigned, const void *, unsigned );

void CVAddrStart( void )
{
    cv_subsection_directory dir;
    cv_trailer start;
    unsigned char *curr;
    unsigned long size, numentries;
    unsigned index;

    AddSubSection( 0 );
    numentries = ((unsigned long)SectAddrs[CVSECT_MODDIR]
               + (unsigned long)SectAddrs[CVSECT_DIRECTORY]) / 12;
    SectAddrs[CVSECT_MISC] += 24 + (NumGroups - 1) * 20;
    SectAddrs[CVSECT_MODDIR] += sizeof(dir);
    CVSize = 2 * sizeof(start);
    for( index = 0; index < NUM_CV_SECTS; ++index )
        CVSize += (unsigned long)SectAddrs[index];
    CVBase = (unsigned char *)DBIAlloc( CVSize );
    curr = CVBase + sizeof(start);
    for( index = 0; index < NUM_CV_SECTS; ++index ) {
        size = (unsigned long)SectAddrs[index];
        SectAddrs[index] = curr;
        curr += size;
    }
    memcpy( start.sig, "NB05", 4 );
    start.offset = SectAddrs[CVSECT_MODDIR] - CVBase;
    memcpy( CVBase, &start, sizeof(start) );
    start.offset = CVSize;
    memcpy( CVBase + CVSize - sizeof(start), &start, sizeof(start) );
    dir.cbDirHeader = sizeof(dir);
    dir.cbDirEntry = sizeof(cv_directory_entry);
    dir.cDir = numentries;
    dir.lfoNextDir = 0;
    dir.flags = 0;
    CVDumpInfo( CVSECT_MODDIR, &dir, sizeof(dir) );
}

/* File-local DumpInfo in the released CodeView writer, distinct from
 * the Watcom debug writer's DumpInfo. Use the scope prefix for ELF lookup. */
void CVDumpInfo( unsigned sect, const void *data, unsigned length )
{
    memcpy( SectAddrs[sect], data, length );
    SectAddrs[sect] += length;
}

extern void GenSubSection( unsigned short, unsigned long );
void CVAddModule( mod_entry *mod, section *sect )
{
    cvmodinfo *info;
    unsigned long sig;

    sect = sect;
    info = mod->d.cv;
    info->modidx = ++TempIndex;
    if( info->pubsize > 0 ) {
        GenSubSection( 0x123, info->pubsize );
        sig = 1;
        CVDumpInfo( CVSECT_MISC, &sig, sizeof(sig) );
    }
}

void GenSubSection( unsigned short sect, unsigned long size )
{
    cv_directory_entry entry;

    entry.subsection = sect;
    if( CurrMod == 0 ) entry.iMod = 0xffff;
    else entry.iMod = ((cvmodinfo *)CurrMod->d.cv)->modidx;
    entry.cb = size;
    if( sect == 0x120 ) {
        entry.lfo = SectAddrs[CVSECT_MODULE] - CVBase;
        CVDumpInfo( CVSECT_MODDIR, &entry, sizeof(entry) );
    } else {
        entry.lfo = SectAddrs[CVSECT_MISC] - CVBase;
        CVDumpInfo( CVSECT_DIRECTORY, &entry, sizeof(entry) );
    }
}

#pragma pack(1)
typedef struct cv_pub16 {
    unsigned short length, code, offset, segment, type;
} cv_pub16;
typedef struct cv_pub32 {
    unsigned short length, code;
    unsigned long offset;
    unsigned short segment, type;
} cv_pub32;
#pragma pack()
extern unsigned short CVGetSegment( seg_leader * );
void CVGenGlobal( symbol *sym, section *sect )
{
    cv_pub16 pub16;
    cv_pub32 pub32;
    unsigned size, pad;
    unsigned long zero;
    unsigned char namelen, invalid_import;
    seg_leader *leader;

    sect = sect;
    if( sym->info & 0x0100 ) return;
    namelen = strlen( sym->name );
    size = namelen + 1;
    leader = sym->p.leader;
    invalid_import = 0;
    /* The 1994 transfer writer leaves nonnull p.edges on imported symbols.
     * Its leader flag/group/segment reads are outside the 9B owner. Preserve
     * the call to CVGetSegment, but pass NULL and emit segment0 for this
     * observed invalid-owner domain in both builds. The fixture masks only
     * those invalid segment words, not record shape. */
    invalid_import = (sym->info & 15) == 6 && leader != 0;
    if( invalid_import ) leader = 0;
    if( sym->p.seg == 0 || (!invalid_import && (sym->p.leader->info & 0x0400)) ) {
        size += sizeof(pub32);
        pub32.length = (size + 3) & ~3U;
        pad = pub32.length - size;
        pub32.length -= 2;
        pub32.code = 0x0203;
        pub32.offset = sym->addr.off;
        pub32.segment = CVGetSegment( leader );
        pub32.type = 0;
        CVDumpInfo( CVSECT_MISC, &pub32, sizeof(pub32) );
    } else {
        size += sizeof(pub16);
        pub16.length = (size + 3) & ~3U;
        pad = pub16.length - size;
        pub16.length -= 2;
        pub16.code = 0x0103;
        pub16.offset = sym->addr.off;
        pub16.segment = CVGetSegment( leader );
        pub16.type = 0;
        CVDumpInfo( CVSECT_MISC, &pub16, sizeof(pub16) );
    }
    CVDumpInfo( CVSECT_MISC, &namelen, 1 );
    CVDumpInfo( CVSECT_MISC, sym->name, namelen );
    if( pad > 0 ) {
        zero = 0;
        CVDumpInfo( CVSECT_MISC, &zero, pad );
    }
}

extern unsigned short FindGroupIdx( unsigned short );
/* File-local GetCVSegment in dbgcv.c takes a leader, unlike the older
 * selector-index helper in obj2supp.c. Keep their ELF identities distinct. */
unsigned short CVGetSegment( seg_leader *seg )
{
    group_entry *group;
    unsigned index;

    if( seg == 0 || seg->group == 0 ) return 0;
    if( FmtData.type & 0x9de0 ) {
        index = 1;
        for( group = Groups; group != 0; group = group->next_group, ++index ) {
            if( group == seg->group ) return index;
        }
        return 0;
    }
    if( FmtData.type & 0x4200 ) return FindGroupIdx( seg->seg_addr.seg );
    return seg->seg_addr.seg;
}

void CVGenAddrInfo( seg_leader *seg, unsigned long start, unsigned long padding )
{
    unsigned long size;

    if( (seg->info & 0x7000) || !(seg->info & 0x0200) ) return;
    size = seg->size - start;
    if( size + padding > 0 ) {
        if( seg->info & 0x40 ) seg->saved_size += size + padding;
        else if( size > 0 ) {
            seg->saved_size = size;
            seg->info |= 0x40;
        }
    }
}

void CVGenLocal( seg_leader *seg, unsigned long size )
{
    if( seg->info & 0x7000 ) {
        GenSubSection( (seg->info & 0x7000) == 0x6000 ? 0x121 : 0x124, size );
        seg->data = (unsigned long)SectAddrs[CVSECT_MISC];
        SectAddrs[CVSECT_MISC] += size;
    }
}

#pragma pack(1)
typedef struct cvlineinfo {
    unsigned char *linestart, *offbase, *numbase;
    struct { unsigned long start, end; } range;
    unsigned long prevaddr;
    unsigned short seg;
    unsigned char needsort;
} cvlineinfo;
typedef struct cv_line386 { unsigned short num; unsigned long off; } cv_line386;
typedef struct cv_line286 { unsigned short num, off; } cv_line286;
#pragma pack()
static cvlineinfo LineInfo;            /* original450378..450392 */

void CVGenLines( seg_leader *seg, unsigned long adjust, unsigned char is32 )
{
    unsigned char *pair;
    unsigned long size, count, offset;
    unsigned short number;

    if( LineInfo.offbase == 0 ) {
        LineInfo.seg = CVGetSegment( seg );
        LineInfo.linestart = SectAddrs[CVSECT_MISC];
        size = ((strlen(CurrMod->name) + 20) & ~3U) + 24;
        count = ((cvmodinfo *)CurrMod->d.cv)->numlines;
        LineInfo.offbase = SectAddrs[CVSECT_MISC] + size;
        LineInfo.numbase = LineInfo.offbase + count * 4;
        size += count * 4 + ((count * 2 + 3) & ~3U);
        GenSubSection( 0x127, size );
        SectAddrs[CVSECT_MISC] += size;
        LineInfo.range.start = adjust;
        LineInfo.range.end = adjust + seg->size;
    } else {
        if( adjust < LineInfo.range.start ) LineInfo.range.start = adjust;
        if( adjust + seg->size > LineInfo.range.end )
            LineInfo.range.end = adjust + seg->size;
    }
    pair = ObjBuff;
    while( pair < EOObjRec ) {
        if( is32 ) {
            ((cv_line386 *)pair)->off += adjust;
            offset = ((cv_line386 *)pair)->off;
            number = ((cv_line386 *)pair)->num;
            pair += sizeof(cv_line386);
        } else {
            ((cv_line286 *)pair)->off += adjust;
            offset = ((cv_line286 *)pair)->off;
            number = ((cv_line286 *)pair)->num;
            pair += sizeof(cv_line286);
        }
        if( offset < LineInfo.prevaddr ) LineInfo.needsort |= 1;
        LineInfo.prevaddr = offset;
        memcpy( LineInfo.offbase, &offset, sizeof(offset) );
        memcpy( LineInfo.numbase, &number, sizeof(number) );
        LineInfo.offbase += sizeof(offset);
        LineInfo.numbase += sizeof(number);
    }
}

extern void SortRelocs( void );
extern void GenSrcModHeader( void );
extern void CVGenSegInfo( seg_leader * );
extern void ProcModSegments( void (*)(seg_leader *) );
#pragma pack(1)
typedef struct cvmoduleheader {
    unsigned short overlay, library, segments, style;
} cvmoduleheader;
#pragma pack()
void CVP2Module( void )
{
    cvmoduleheader header;
    unsigned char namelen;
    unsigned long size;

    if( LineInfo.needsort & 1 ) SortRelocs();
    GenSrcModHeader();
    namelen = strlen( CurrMod->name );
    size = namelen + 21 + (((cvmodinfo *)CurrMod->d.cv)->numsegs - 1) * 12;
    GenSubSection( 0x120, size );
    header.overlay = 0;
    header.library = 0;
    header.segments = ((cvmodinfo *)CurrMod->d.cv)->numsegs;
    header.style = 0x5643;
    CVDumpInfo( CVSECT_MODULE, &header, sizeof(header) );
    ProcModSegments( CVGenSegInfo );
    CVDumpInfo( CVSECT_MODULE, &namelen, 1 );
    CVDumpInfo( CVSECT_MODULE, CurrMod->name, namelen );
}

extern int RelocCompare( const void *, const void * );
extern void SwapRelocs( void *, void * );
void SortRelocs( void )
{
    unsigned char *    p1;
    unsigned char *    p2;
    unsigned char *    mid;
    int         comparison;
    int         last_non_equal_count;
    unsigned    i;
    unsigned    count;
    unsigned    sp;
    auto unsigned char * base_stack[32];
    auto unsigned n_stack[32];

    unsigned char *base;
    unsigned n;

    n = ((cvmodinfo *)CurrMod->d.cv)->numlines;
    LineInfo.offbase -= n * 4;
    LineInfo.numbase -= n * 2;
    base = LineInfo.offbase;
    sp = 0;
    for(;;) {
        while( n > 1 ) {
            p1 = base + 4;
            if( n == 2 ) {
                if( RelocCompare( base, p1 ) > 0 ) {
                    SwapRelocs( base, p1 );
                }
                break;
            } else {
                /* store mid element at base for pivot */
                /* this will speed up sorting of a sorted list */
                mid = base + (n >> 1) * 4;
                SwapRelocs( base, mid );
                p2 = base;
                count = 0;
                last_non_equal_count = 0;
                for( i = 1; i < n; ++i ) {
                    comparison = RelocCompare( p1, base );
                    if( comparison <= 0 ) {
                        p2 += 4;
                        count++;
                        if( i != count ) {              /* p1 != p2 */
                            SwapRelocs( p1, p2 );
                        }
                    }
                    if( comparison != 0 ) last_non_equal_count = count;
                    p1 += 4;
                }
                /* special check to see if all values compared are equal */
                if( count == n-1  &&  last_non_equal_count == 0 ) break;
                if( count != 0 ) {  /* store pivot in right spot */
                    SwapRelocs( base, p2 );
                }
                n = n - count - 1;          /* calc. size of right part */

                /* The pivot is at p2. It is in its final position.
                   There are count items to the left of the pivot.
                   There are n items to the right of the pivot.
                */

                if( count != last_non_equal_count ) {   /* 18-jul-90 */
                    /*
                       There are last_non_equal_count+1 items to the left
                       of the pivot that still need to be checked.
                       There are (count - (last_non_equal_count+1)) items
                       immediately to the left of the pivot that are
                       equal to the pivot. They are in their final position.
                    */
                    count = last_non_equal_count + 1;
                }
                if( count < n ) {           /* if left part is shorter */
                    base_stack[sp] = p2 + 4;  /* - stack right part */
                    n_stack[sp] = n;
                    n = count;
                } else {                    /* right part is shorter */
                    base_stack[sp] = base;  /* - stack left part */
                    n_stack[sp] = count;
                    base = p2 + 4;
                }
                ++sp;
            }
        }
        if( sp == 0 ) break;
        --sp;
        base = base_stack[sp];
        n    = n_stack[sp];
    }
}

void SwapRelocs( void *a, void *b )
{
    unsigned long temp;
    unsigned short *anum, *bnum, number;

    temp = *(unsigned long *)a;
    *(unsigned long *)a = *(unsigned long *)b;
    *(unsigned long *)b = temp;
    anum = (unsigned short *)(LineInfo.numbase
                + (((unsigned char *)a - LineInfo.offbase) / 4) * 2);
    bnum = (unsigned short *)(LineInfo.numbase
                + (((unsigned char *)b - LineInfo.offbase) / 4) * 2);
    number = *anum;
    *anum = *bnum;
    *bnum = number;
}

int RelocCompare( const void *a, const void *b )
{
    return *(const unsigned long *)a - *(const unsigned long *)b;
}

#pragma pack(1)
typedef struct cvsourceheader {
    unsigned short files, segments;
    unsigned long filebase;
    struct { unsigned long start, end; } range;
    unsigned short segment, pad;
} cvsourceheader;
typedef struct cvfiletable {
    unsigned short segments, pad;
    unsigned long linebase;
    struct { unsigned long start, end; } range;
    unsigned char namelen;
} cvfiletable;
typedef struct cvmapping { unsigned short segment, pairs; } cvmapping;
#pragma pack()
void GenSrcModHeader( void )
{
    cvsourceheader header;
    cvfiletable file;
    cvmapping mapping;
    unsigned adjust;
    unsigned long zero;

    if( LineInfo.linestart == 0 ) return;
    memset( &header, 0, sizeof(header) );
    header.files = 1;
    header.segments = 1;
    header.range.start = LineInfo.range.start;
    header.range.end = LineInfo.range.end;
    header.filebase = sizeof(header);
    header.segment = LineInfo.seg;
    header.pad = 0;
    memcpy( LineInfo.linestart, &header, sizeof(header) );
    LineInfo.linestart += sizeof(header);
    file.segments = 1;
    file.pad = 0;
    file.range.start = LineInfo.range.start;
    file.range.end = LineInfo.range.end;
    file.namelen = strlen( CurrMod->name );
    file.linebase = sizeof(header) + ((sizeof(file) + file.namelen + 3) & ~3U);
    memcpy( LineInfo.linestart, &file, sizeof(file) );
    LineInfo.linestart += sizeof(file);
    memcpy( LineInfo.linestart, CurrMod->name, file.namelen );
    LineInfo.linestart += file.namelen;
    adjust = file.linebase - sizeof(header) - sizeof(file) - file.namelen;
    if( adjust != 0 ) {
        zero = 0;
        memcpy( LineInfo.linestart, &zero, adjust );
        LineInfo.linestart += adjust;
    }
    mapping.segment = header.segment;
    mapping.pairs = ((cvmodinfo *)CurrMod->d.cv)->numlines;
    memcpy( LineInfo.linestart, &mapping, sizeof(mapping) );
    memset( &LineInfo, 0, sizeof(LineInfo) );
}

#pragma pack(1)
typedef struct cvseginfo {
    unsigned short segment, pad;
    unsigned long offset, length;
} cvseginfo;
#pragma pack()
void CVGenSegInfo( seg_leader *seg )
{
    cvseginfo info;

    if( (seg->info & 0x40) && (seg->info & 0x0200) ) {
        info.segment = CVGetSegment( seg );
        info.pad = 0;
        info.offset = seg->seg_addr.off + seg->size - seg->saved_size;
        info.length = seg->saved_size;
        CVDumpInfo( CVSECT_MODULE, &info, sizeof(info) );
        seg->info &= ~0x40;
    }
}

#pragma pack(1)
typedef struct cvsegdesc {
    unsigned short flags, overlay, group, frame, segname, classname;
    unsigned long offset, size;
} cvsegdesc;
#pragma pack()
void CVFini( section *sect )
{
    unsigned short counts[2];
    cvsegdesc desc;
    group_entry *group;

    if( sect != Root ) return;
    CurrMod = 0;
    GenSubSection( 0x12d, 24 + (NumGroups - 1) * 20 );
    counts[0] = NumGroups;
    counts[1] = NumGroups;
    CVDumpInfo( CVSECT_MISC, counts, sizeof(counts) );
    memset( &desc, 0, sizeof(desc) );
    if( !(LinkFlags & 0xe0) ) desc.flags |= 0x0100;
    desc.flags |= 1;
    desc.segname = 0xffff;
    desc.classname = 0xffff;
    for( group = Groups; group != 0; group = group->next_group ) {
        desc.frame = group->grp_addr.seg;
        desc.offset = group->grp_addr.off;
        desc.size = group->totalsize;
        /* The 1994 writer accumulates these flags across groups. */
        if( !(group->segflags & 1) ) desc.flags |= 4;
        if( (group->segflags & 0x81) == 1 ) desc.flags |= 2;
        if( group->leaders[0]->info & 0x0400 ) desc.flags |= 8;
        CVDumpInfo( CVSECT_MISC, &desc, sizeof(desc) );
    }
}

void CVWriteDBI( void )
{
    QWrite( CurrSect->outfile->handle, CVBase, CVSize, CurrSect->outfile->fname );
}
