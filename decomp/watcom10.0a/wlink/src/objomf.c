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
#include "omf.h"
#include "fixup.h"
#include <recon.h>

#ifdef __WATCOMC__
#pragma intrinsic(memcpy,memcmp)
#endif

unsigned char IsOMF( file_list *list, unsigned long loc )
{
    unsigned char *rec;

    rec = CacheRead( list, loc, 1 );
    return rec != 0 && *rec == 0x80;
}

unsigned short TargThreads[4];       /* original4501b4 */

unsigned long OMFPass1( void )
{
    unsigned long loc;

    memset( TargThreads, 0, sizeof( TargThreads ) );
    P1Start();
    loc = ProcObj( CurrMod->f.source, CurrMod->location, Pass1Cmd );
    ObjFormat &= 0xf000;
    ResolveComdats();
    CollapseLazyExtdefs();
    return loc;
}

unsigned char *ObjBuff;           /* original450130 */
unsigned char *EOObjRec;          /* original45014c */
#pragma pack(1)
typedef struct obj_record {
    unsigned char command;
    unsigned short length;
} obj_record;
#pragma pack()

unsigned long ProcObj( file_list *file, unsigned long loc,
                       void (*procrtn)(unsigned char) )
{
    obj_record *rec;
    unsigned char cmd;
    unsigned short len;

    RecNum = 0;
    do {
        ObjFormat &= ~2;
        rec = CacheRead( file, loc, sizeof( obj_record ) );
        if( rec == 0 ) {
            EarlyEOF();
            break;
        }
        loc += sizeof( obj_record );
        len = rec->length;
        cmd = rec->command;
        if( procrtn != 0 ) {
            ++RecNum;
            ObjBuff = CacheRead( file, loc, len );
            if( ObjBuff == 0 ) {
                EarlyEOF();
                break;
            }
            EOObjRec = ObjBuff + len - 1;
            (*procrtn)( cmd );
            if( ObjBuff > EOObjRec ) {
                BadObject();
                break;
            }
        }
        loc += len;
    } while( cmd != 0x8a && cmd != 0x8b );
    return loc;
}

void Pass1Cmd( unsigned char cmd )
/******************************/
/* Process an object record for pass 1 */
{
    unsigned char isstatic;

    isstatic = 0;
    switch( cmd ) {
    case CMD_THEADR:
        ProcTHEADR();
        break;
    case CMD_COMENT:
        Comment( 1 );
        break;
    case CMD_LLNAME:
    case CMD_LNAMES:
        AddNames();
        break;
    case CMD_SEGD32:
        ObjFormat |= 6;
    case CMD_SEGDEF:
        CurrMod->modinfo |= 0x4000;
        ProcSegDef();
        break;
    case CMD_STATIC_PUBD32:
        ObjFormat |= 6;
    case CMD_STATIC_PUBDEF:
        ProcPubdef( 1 );
        break;
    case CMD_PUBD32:
        ObjFormat |= 6;
    case CMD_PUBDEF:
        ProcPubdef( 0 );
        break;
    case CMD_STATIC_EXTDEF:
    case CMD_STATIC_EXTD32:
        isstatic = 1;
    case CMD_EXTDEF:
        CurrMod->modinfo |= 0x4000;
        UseSymbols( isstatic, 0 );
        break;
    case CMD_CEXTDF:
        CurrMod->modinfo |= 0x4000;
        UseSymbols( 0, 1 );
        break;
    case CMD_GRPDEF:
        DefineGroup( 1 );
        break;
    case CMD_LINN32:
        ObjFormat |= 6;
    case CMD_LINNUM:
        ProcLinnum( 1 );
        break;
    case CMD_LINS32:
        ObjFormat |= 6;
    case CMD_LINSYM:
        ProcLinsym( 1 );
        break;
    case CMD_STATIC_COMDEF:
        isstatic = 1;
    case CMD_COMDEF:
        CurrMod->modinfo |= 0x4000;
        ProcComdef( 1, isstatic );
        break;
    case CMD_COMD32:
        ObjFormat |= 6;
    case CMD_COMDAT:
        CurrMod->modinfo |= 0x4000;
        ProcComdat( 1 );
        break;
    case CMD_LEDA32:
    case CMD_LIDA32:
        ObjFormat |= 6;
    case CMD_LEDATA:
    case CMD_LIDATA:
        CurrMod->modinfo |= 0x4000;
        ProcLxdata();
        break;
    case CMD_FIXU32:
        ObjFormat |= 6;
    case CMD_FIXUP:         /* count the fixups for each seg_leader */
        CurrMod->modinfo |= 0x4000;
        DoRelocs();
        ObjFormat &= ~0x40;
        break;
    case CMD_MODE32:
        ObjFormat |= 6;
    case CMD_MODEND:
        ProcModuleEnd();
        break;
    case CMD_ALIAS:
        OMFProcAlias();
        break;
    case CMD_VERNUM:
    case CMD_VENDEXT:
    case CMD_BAKPAT:
    case CMD_BAKP32:
    case CMD_NBKPAT:
    case CMD_NBKP32:    /* ignore bakpats in pass 1 */
    case CMD_LOCSYM:
    case CMD_TYPDEF:
    case CMD_DEBSYM:
    case CMD_BLKDEF:
    case CMD_BLKD32:
    case CMD_BLKEND:
    case CMD_BLKE32:
        /* ignore the Intel debugging records */
        break;
    case CMD_RHEADR:
    case CMD_REGINT:
    case CMD_REDATA:
    case CMD_RIDATA:
    case CMD_OVLDEF:
    case CMD_ENDREC:
    case CMD_LHEADR:
    case CMD_PEDATA:
    case CMD_PIDATA:
    case CMD_LIBHED:
    case CMD_LIBNAM:
    case CMD_LIBLOC:
    case CMD_LIBDIC:
        LnkMsg( 0xf220, "x", cmd );
        break;
    default:
        CurrMod->f.source->file->flags |= 4;
        LnkMsg( 0xf352, "x", cmd );
        break;
    }
}


void ProcTHEADR( void )
{
    unsigned len;

    if( ObjFormat & 8 ) return;
    len = *ObjBuff++;
    CurrMod->name = PermAlloc( len + 1 );
    memcpy( CurrMod->name, ObjBuff, len );
    CurrMod->name[len] = '\0';
    ObjFormat |= 8;
}

void Comment( unsigned pass )
{
    unsigned char attribute, class, which;
    int proc;

    attribute = *ObjBuff++;
    class = *ObjBuff++;
    switch( class ) {
    case 0xa0:                       /* DLL entry */
        which = *ObjBuff++;
        if( which == 1 || which == 2 ) {
            if( pass != 1 ) break;
            LinkState |= 0x100;
            if( !HintFormat( 0x811f ) ) {
                LnkMsg( 0x723e, 0 );
                break;
            }
            if( which == 2 ) {
                if( FmtData.type & 0x1f ) MSExportKeyword();
                else ProcExportKeyword();
            } else {
                if( FmtData.type & 0x1f ) MSImportKeyword();
                else ProcImportKeyword();
            }
        } else if( which == 4 && (FmtData.type & 8) ) {
            FmtData.u.os2.flags |= 0x2000;
        }
        break;
    case 0x9b:                      /* Watcom processor model */
    case 0x9d:                      /* Microsoft processor model */
        proc = (unsigned char)*ObjBuff - '0';
        if( proc > FmtData.cpu_type ) FmtData.cpu_type = proc;
        break;
    case 0x9e:
        LinkFlags |= 0x10;
        break;
    case 0x9f:
        AddCommentLib( ObjBuff, EOObjRec - ObjBuff, 0xfe );
        break;
    case 0xa8:
        DoLazyExtdef( 1 );
        break;
    case 0xa9:
        DoLazyExtdef( 0 );
        break;
    case 0xaa:
        if( memcmp( ObjBuff, "80386", 5 ) == 0 ) ObjFormat |= 5;
        break;
    case 0xff:
        DBIComment();
        break;
    case 0xfe:
        LinkDirective( pass );
        break;
    case 0x80:                      /* old code generator's byte order */
        if( attribute == 0xff ) DBIComment();
        else if( attribute == 0xfe ) LinkDirective( pass );
        break;
    }
}

void LinkDirective( unsigned pass )
{
    unsigned char directive, priority;
    segnode *seg;

    directive = *ObjBuff++;
    if( pass == 1 ) {
        switch( directive ) {
        case 'L':
            if( ObjBuff + 1 < EOObjRec ) {
                priority = *ObjBuff++;
                AddCommentLib( ObjBuff, EOObjRec - ObjBuff, priority );
            }
            break;
        case 'D':
            DBIP1Source( ObjBuff, EOObjRec );
            break;
        case 'P':
            ProcVFTableRecord( 1 );
            break;
        case 'V':
            ProcVFTableRecord( 0 );
            break;
        case 'R':
            ProcVFReference();
            break;
        case '7':
            if( !(LinkFlags & 0x04000000UL) ) {
                PackDataLimit = *(unsigned long *)ObjBuff;
                LinkFlags |= 0x04000000UL;
            }
            break;
        }
    } else {
        switch( directive ) {
        case 'D':
            DBIP2Source( ObjBuff, EOObjRec );
            break;
        case 'O':
            seg = FindNode( SegNodes, GetIdx() );
            seg->info |= 0x8000;
            break;
        case 'F':
            ObjFormat |= 0x0400;
            break;
        }
    }
    if( directive == 'U' ) ObjFormat |= 0x40;
}

void AddNames( void )
{
    unsigned len;

    while( ObjBuff < EOObjRec ) {
        len = *ObjBuff;
        AddName( (char *)ObjBuff + 1, len );
        ObjBuff += len + 1;
    }
}

void ProcSegDef( void )
{
    segdata *seg;
    unsigned align;

    seg = AllocSegData();
    seg->acbp = *ObjBuff++;
    if( ObjFormat & 1 ) seg->acbp |= 1;
    align = (seg->acbp >> 5) & 7;
    if( align == 0 ) {
        seg->a.absolute.frame = *(unsigned short *)ObjBuff;
        ObjBuff += 3;
        seg->flags |= 8;
    } else if( align == 6 && !(ObjFormat & 3) ) {
        ObjBuff += 5;
    }
    if( ObjFormat & 3 ) {
        if( seg->acbp & 2 ) {
            BadObject();
            return;
        }
        seg->length = *(unsigned long *)ObjBuff;
        ObjBuff += 4;
    } else {
        if( seg->acbp & 2 ) seg->length = 65536UL;
        else seg->length = *(unsigned short *)ObjBuff;
        ObjBuff += 2;
    }
    seg->u.name = FindName( GetIdx() );
    seg->cl.name = FindName( GetIdx() );
    if( ObjFormat & 1 ) {
        SkipIdx();
        if( ObjBuff < EOObjRec && !(*ObjBuff & 4) ) seg->acbp &= ~1;
    }
    AllocateSegment( seg );
}

unsigned short GetIdx( void )
{
    unsigned short index;

    index = *ObjBuff++;
    if( index & 0x80 ) {
        index = ((index & 0x7f) << 8) + *ObjBuff++;
    }
    return index;
}

list_of_names *FindName( unsigned short index )
{
    return *(list_of_names **)FindNode( NameNodes, index );
}

void DefineGroup( unsigned char pass )
{
    list_of_names *name;
    unsigned char *anchor;
    unsigned count;
    group_entry **newnode, *group;
    segnode *seg;

    name = FindName( GetIdx() );
    anchor = ObjBuff;
    count = 0;
    while( ObjBuff < EOObjRec ) {
        if( *ObjBuff != 0xff ) {
            BadObject();
            return;
        }
        ++ObjBuff;
        SkipIdx();
        ++count;
    }
    newnode = AllocNode( GrpNodes );
    if( count == 0 ) {
        *newnode = 0;
        return;
    }
    group = FindGroup( name );
    *newnode = group;
    if( group->leaders == 0 ) {
        group->leaders = ChkLAlloc( count * 2 * sizeof( seg_leader * ) );
        group->num = 0;
        group->free_slots = count * 2;
    }
    ObjBuff = anchor;
    if( pass == 2 ) return;
    while( ObjBuff < EOObjRec ) {
        ++ObjBuff;
        seg = FindNode( SegNodes, GetIdx() );
        AddToGroup( group, seg->entry, (seg->info & 0x0100) != 0 );
        --count;
        if( pass == 2 ) break;
    }
}

void SkipIdx( void )
{
    unsigned char first;

    first = *ObjBuff++;
    if( first & 0x80 ) ++ObjBuff;
}

void UseSymbols( unsigned char isstatic, unsigned char comdat )
{
    unsigned char flags;
    unsigned len;
    char *name;
    list_of_names *lname;
    symbol *sym;
    extnode *node;

    flags = isstatic ? 0x13 : 0x12;
    while( ObjBuff < EOObjRec ) {
        if( comdat ) {
            lname = FindName( GetIdx() );
            sym = RefISymbol( lname->name );
        } else {
            len = *ObjBuff++;
            name = (char *)ObjBuff;
            ObjBuff += len;
            sym = SymXOp( flags, name, len );
        }
        node = AllocNode( ExtNodes );
        node->entry = sym;
        node->flags &= ~0x1000;
        DefineReference( sym, node );
        SkipIdx();
    }
}

void ProcPubdef( unsigned char isstatic )
{
    unsigned short index, frame;
    segnode *seg;
    unsigned len;
    char *name;
    unsigned long off;
    symbol *sym;

    SkipIdx();
    index = GetIdx();
    /* The original leaves frame uninitialized for nonabsolute symbols;
     * DefineSymbol ignores that argument when seg is present. */
    frame = 0;
    if( index != 0 ) {
        seg = FindNode( SegNodes, index );
    } else {
        seg = 0;
        frame = *(unsigned short *)ObjBuff;
        ObjBuff += 2;
    }
    while( ObjBuff < EOObjRec ) {
        len = *ObjBuff++;
        name = (char *)ObjBuff;
        ObjBuff += len;
        if( ObjFormat & 3 ) {
            off = *(unsigned long *)ObjBuff;
            ObjBuff += 4;
        } else {
            off = *(unsigned short *)ObjBuff;
            ObjBuff += 2;
        }
        sym = SymXOp( isstatic ? 0x27 : 0x26, name, len );
        DefineSymbol( sym, seg, off, frame );
        SkipIdx();
    }
}

segnode *CurrNode;                 /* original4501bc */

void ProcLxdata( void )
{
    seg_leader *leader;

    CurrNode = FindNode( SegNodes, GetIdx() );
    leader = CurrNode->entry;
    if( leader != 0 && !(CurrNode->info & 0x0100) ) leader->info |= 0x8000;
}

void ProcLinnum( unsigned char pass )
{
    segnode *seg;
    unsigned char is32;

    SkipIdx();
    seg = FindNode( SegNodes, GetIdx() );
    if( seg->info & 0x80 ) return;
    is32 = (ObjFormat & 3) != 0;
    if( pass == 1 ) {
        DBIAddLines( seg->entry, is32, (ObjFormat & 0x8000) != 0 );
    } else {
        DBIGenLines( seg->entry, seg->entry, seg->addr.off, is32 );
    }
}

void ProcModuleEnd( void )
{
    unsigned char type;
    unsigned index;
    segnode *seg;
    extnode *ext;
    group_entry **group;

    if( !(*ObjBuff & 0x40) ) return;
    ++ObjBuff;
    if( ObjBuff == EOObjRec ) return;
    if( StartMod != 0 ) LnkMsg( 0x731e, 0 );
    else StartMod = CurrMod;
    if( !(LinkFlags & 0x10000UL) ) return;
    type = *ObjBuff++;
    if( ((type >> 4) & 7) <= 2 ) SkipIdx();
    index = GetIdx();
    switch( type & 3 ) {
    case 0:
        seg = FindNode( SegNodes, index );
        if( seg->info & 0x0100 ) RefSeg( seg->entry );
        break;
    case 1:
        group = FindNode( GrpNodes, index );
        RefGroup( *group );
        break;
    case 2:
        ext = FindNode( ExtNodes, index );
        DataRef( ext->entry );
        break;
    }
}

extern void Pass2Cmd( unsigned char );
extern void ExecBakPats( void );
extern void FinishSavedSegs( void );

void OMFPass2( void )
{
    ProcObj( CurrMod->f.source, CurrMod->location, Pass2Cmd );
    ExecBakPats();
    ObjFormat &= 0xf000;
    FinishSavedSegs();
}

extern void FlushObjectData( void );
extern void FlushObjectRelocs( void );
extern void ProcData( unsigned char );
extern void DoPass2Relocs( void );
extern void ProcBackpatch( void );
extern void ProcNamedBackpatch( void );
extern void P2ModEnd( void );

void Pass2Cmd( unsigned char cmd )
{
    unsigned char isstatic;

    isstatic = 0;
    switch( cmd ) {
    case CMD_STATIC_EXTDEF:
    case CMD_STATIC_EXTD32:
        isstatic = 1;
    case CMD_EXTDEF:
        UseSymbols( isstatic, 0 );
        break;
    case CMD_CEXTDF:
        UseSymbols( 0, 1 );
        break;
    case CMD_COMENT:
        Comment( 2 );
        break;
    case CMD_LNAMES:
    case CMD_LLNAME:
        AddNames();
        break;
    case CMD_SEGD32:
        ObjFormat |= 6;
    case CMD_SEGDEF:
        FlushObjectData();
        FlushObjectRelocs();
        ProcSegDef();
        break;
    case CMD_GRPDEF:
        DefineGroup( 2 );
        break;
    case CMD_STATIC_COMDEF:
        isstatic = 1;
    case CMD_COMDEF:
        ProcComdef( 2, isstatic );
        break;
    case CMD_COMD32:
        ObjFormat |= 6;
    case CMD_COMDAT:
        ProcComdat( 2 );
        break;
    case CMD_LEDA32:
        ObjFormat |= 6;
    case CMD_LEDATA:
        ProcData( 0 );
        break;
    case CMD_LIDA32:
        ObjFormat |= 6;
    case CMD_LIDATA:
        ProcData( 1 );
        break;
    case CMD_FIXU32:
        ObjFormat |= 6;
    case CMD_FIXUP:
        DoPass2Relocs();
        ObjFormat &= ~0x40;
        break;
    case CMD_LINN32:
        ObjFormat |= 6;
    case CMD_LINNUM:
        ProcLinnum( 2 );
        break;
    case CMD_LINS32:
        ObjFormat |= 6;
    case CMD_LINSYM:
        ProcLinsym( 2 );
        break;
    case CMD_BAKP32:
        ObjFormat |= 6;
    case CMD_BAKPAT:
        ProcBackpatch();
        break;
    case CMD_NBKP32:
        ObjFormat |= 6;
    case CMD_NBKPAT:
        ProcNamedBackpatch();
        break;
    case CMD_MODE32:
        ObjFormat |= 6;
    case CMD_MODEND:
        FlushObjectData();
        FlushObjectRelocs();
        P2ModEnd();
        break;
    default:
        break;
    }
}

extern void LoadObj( segnode *, unsigned long, unsigned char );

void ProcData( unsigned char iterated )
{
    segnode *seg;
    unsigned long offset;

    seg = FindNode( SegNodes, GetIdx() );
    if( seg->info & 0x80 ) {
        ObjFormat |= 0x20;
        return;
    }
    if( ObjFormat & 3 ) {
        offset = *(unsigned long *)ObjBuff;
        ObjBuff += 4;
    } else {
        offset = *(unsigned short *)ObjBuff;
        ObjBuff += 2;
    }
    LoadObj( seg, offset, iterated );
}

extern void ParseFixupRefs( fixup_ref *, fixup_ref *, unsigned, unsigned );
extern address StartInfo;

void P2ModEnd( void )
{
    unsigned char type;
    fixup_ref target, frame;

    type = *ObjBuff++;
    if( !(type & 0x40) || ObjBuff == EOObjRec ) return;
    if( !(type & 1) || (*ObjBuff >> 4) == 4 ) {
        BadObject();
        return;
    }
    target.flags = 0;
    frame.flags = 0;
    ParseFixupRefs( &frame, &target, 0, (type >> 2) & 7 );
    if( target.import != 0 ) LnkMsg( 0x3316, 0 );
    StartInfo = target.addr;
}

void DoLazyExtdef( unsigned char isweak )
{
    extnode *ext;
    symbol *sym;
    unsigned short index;

    while( ObjBuff < EOObjRec ) {
        ext = FindNode( ExtNodes, GetIdx() );
        sym = ext->entry;
        ext->flags |= 0x1000;
        index = GetIdx();
        ext = FindNode( ExtNodes, index );
        DefineLazyExtdef( sym, ext->entry, isweak );
    }
}

void OMFProcAlias( void )
{
    char *alias, *target;
    unsigned aliaslen, targetlen;
    symbol *sym;

    while( ObjBuff < EOObjRec ) {
        aliaslen = *ObjBuff++;
        alias = (char *)ObjBuff;
        ObjBuff += aliaslen;
        targetlen = *ObjBuff++;
        sym = SymXOp( 0x0c, alias, aliaslen );
        if( !(sym->info & 0x1000) ) {
            target = ChkLAlloc( targetlen + 1 );
            memcpy( target, ObjBuff, targetlen );
            target[targetlen] = '\0';
            MakeSymAlias( alias, aliaslen, target, targetlen );
        }
        ObjBuff += targetlen;
    }
}

void ProcVFTableRecord( unsigned char ispure )
{
    extnode *ext;
    symbol *sym;

    if( !(LinkFlags & 0x10000000UL) ) return;
    ext = FindNode( ExtNodes, GetIdx() );
    sym = ext->entry;
    ext->flags |= 0x1000;
    ext = FindNode( ExtNodes, GetIdx() );
    /* The old reader uses the live ObjBuff list, before vflistrtns existed. */
    DefineVFTableRecord( sym, ext->entry, ispure );
}

unsigned char *GetVFListStart( void )
{
    return ObjBuff;
}

unsigned char EndOfVFList( void )
{
    return ObjBuff >= EOObjRec;
}

char *GetVFListName( void )
{
    list_of_names *lname;

    lname = FindName( GetIdx() );
    return lname->name;
}

extern symbol *FindISymbol( char * );
void ProcVFReference( void )
{
    extnode *ext;
    segnode *seg;
    symbol *sym;
    list_of_names *lname;

    if( !(LinkFlags & 0x10000000UL) ) return;
    ext = FindNode( ExtNodes, GetIdx() );
    if( LinkFlags & 0x10000UL ) {
        if( *ObjBuff == 0 ) {
            ++ObjBuff;
            lname = FindName( GetIdx() );
            sym = FindISymbol( lname->name );
            if( sym == 0 ) sym = MakeWeakExtdef( lname->name, 0 );
            DefineVFReference( sym, ext->entry, 1 );
        } else {
            seg = FindNode( SegNodes, GetIdx() );
            DefineVFReference( seg, ext->entry, 0 );
        }
    }
    if( !(ext->entry->info & 0x1000) ) ext->entry->info |= 0x2000;
}

void SetVFListStart( unsigned char *start )
{
    ObjBuff = start;
}

/* The1994 Novell/ELF comments use only the internal name.  Their other
 * length-prefixed DLL fields remain ignored, as in the original reader. */
void ProcExportKeyword( void )
{
    unsigned len;

    ++ObjBuff;
    len = *ObjBuff++;
    SymXOp( 0x12, (char *)ObjBuff, len );
    AddNameTable( (char *)ObjBuff, len, 1, &FmtData.u.nov.exports );
}

void ProcImportKeyword( void )
{
    unsigned len;
    char *name;
    symbol *sym;

    ++ObjBuff;
    len = *ObjBuff++;
    name = (char *)ObjBuff;
    ObjBuff += len;
    sym = SymXOp( 2, name, len );
    if( !(sym->info & 0x1000) ) {
        sym->info |= 0x5000;
        sym->info = (sym->info & 0xfff0) | 6;
        if( FmtData.type & 0x100 ) SetNovImportSymbol( sym );
        else SetELFImportSymbol( sym );
    }
}

/* Read the actual member header and unlink the matching requested name.
 * The cached checksum byte is used as a NUL terminator by the old reader. */
member_list *GetOMFMember( file_list *list, unsigned long *loc )
{
    unsigned long pos;
    unsigned length, namelen;
    unsigned char *header, *record;
    char *name;
    member_list **owner, *member;

    pos = *loc;
    header = CacheRead( list, pos, 3 );
    if( header == 0 ) return 0;
    pos += 3;
    length = *(unsigned short *)(header + 1);
    record = CacheRead( list, pos, length );
    pos += length;
    if( record == 0 ) return 0;
    namelen = record[0];
    name = (char *)record + 1;
    name[namelen] = 0;
    owner = &list->u.member;
    while( *owner != 0 ) {
        if( ModNameCompare( name, (*owner)->name ) ) break;
        owner = &(*owner)->next;
    }
    member = *owner;
    if( member == 0 ) *loc = ProcObj( list, pos, 0 );
    else *owner = member->next;
    return member;
}
