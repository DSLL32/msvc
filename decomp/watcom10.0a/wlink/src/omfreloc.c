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
#include <recon.h>
#ifdef __WATCOMC__
#pragma intrinsic(memcpy)
#endif

/* The1994 pass-one parser records strip-code and overlay references only;
 * relocation application is a separate pass-two routine. */
void DoRelocs( void )
{
    unsigned char vectors, seen, no_code, no_base;
    unsigned char type, omftype, spec, frame, relative;
    unsigned target;
    segnode *seg;

    vectors = (FmtData.type & 0x0080) && !(LinkFlags & 0x80);
    if( !(LinkFlags & 0x10000UL) && !vectors ) return;
    seen = CurrNode != 0;
    if( seen && CurrNode->entry != 0 ) {
        no_base = !(CurrNode->info & 0x40);
        no_code = !(CurrNode->info & 0x0200);
    } else {
        no_code = 1;
        no_base = 1;
    }
    if( ObjFormat & 0x40 ) no_base = 1;
    do {
        type = *ObjBuff++;
        omftype = (type >> 2) & 7;
        if( !(type & 0x80) ) {
            if( type & 0x40 ) {
                if( omftype < 4 ) SkipIdx();
            } else {
                if( (omftype & 3) == 2 ) TargThreads[type & 3] = GetIdx();
                else SkipIdx();
            }
        } else {
            ++ObjBuff;
            relative = (type & 0x40) != 0;
            spec = *ObjBuff++;
            frame = (spec >> 4) & 7;
            if( !(spec & 0x80) && frame < 4 ) SkipIdx();
            if( spec & 8 ) target = TargThreads[spec & 3];
            else target = GetIdx();
            if( !(spec & 4) ) ObjBuff += (ObjFormat & 3) ? 4 : 2;
            if( target != 0 && seen ) {
                if( (spec & 3) == 0 ) {
                    if( LinkFlags & 0x10000UL ) {
                        seg = FindNode( SegNodes, target );
                        if( seg->info & 0x0100 ) RefSeg( seg->entry );
                    }
                } else if( (spec & 3) == 2 ) {
                    if( LinkFlags & 0x10000UL ) DoStripReference( target, no_code );
                    if( relative && vectors ) {
                        switch( omftype ) {
                        case 1:
                            if( !(LinkFlags & 0x1000) ) break;
                            VectorReference( target );
                            break;
                        case 2:
                            VectorReference( target );
                            break;
                        case 3:
                            if( no_base ) VectorReference( target );
                            break;
                        }
                    }
                }
            }
        }
    } while( ObjBuff < EOObjRec );
}

extern path_entry *ObjInfoList, *AuxObjInfoList;
extern void ParseFrame( unsigned char, fixup_ref * );
extern void ParseTarget( unsigned char, fixup_ref * );
extern void QueueFixupThread( fixup_ref **, fixup_ref * );
extern void ParseFixupRefs( fixup_ref *, fixup_ref *, unsigned, unsigned );
extern unsigned char CanUseOvlVector( unsigned char, unsigned char );
extern void OvlForceVect( fixup_ref *, unsigned char );
extern void Relocate( fixup_record *, fixup_ref * );

void DoPass2Relocs( void )
{
    unsigned char type, code, overlay, iscode;
    unsigned offset;
    fixup_ref *thread;
    fixup_ref target, frame;
    fixup_record fix;

    if( ObjFormat & 0x20 ) return;
    overlay = (LinkFlags & 8) && CurrRec != 0 && (FmtData.type & 0x00e0);
    iscode = overlay && (CurrRec->seg->info & 0x0200);
    do {
        type = *ObjBuff++;
        code = (type >> 2) & 7;
        if( !(type & 0x80) ) {
            thread = ChkLAlloc( sizeof(*thread) );
            /* Unused bits in the old allocated thread flags are undefined. */
            thread->flags = (unsigned long)(type & 3) << 16;
            if( type & 0x40 ) {
                ParseFrame( code, thread );
                QueueFixupThread( (fixup_ref **)&ObjInfoList, thread );
            } else {
                ParseTarget( code, thread );
                QueueFixupThread( (fixup_ref **)&AuxObjInfoList, thread );
            }
        } else {
            offset = ((type & 3) << 8) + *ObjBuff++;
            target.flags = 0;
            frame.flags = 0;
            ParseFixupRefs( &frame, &target, offset, code );
            fix.type = code;
            fix.flags = (type & 0x40) ? 0 : 2;
            if( type & 0x20 ) {
                if( fix.type == 1 || fix.type == 5 ) fix.type = 5;
                else if( fix.type == 3 ) fix.type = 6;
            } else if( fix.type == 5 && !(ObjFormat & 3) ) {
                fix.type = 1;
                fix.flags |= 4;
            }
            if( overlay && !(fix.flags & 2) && (target.flags & 0x10)
                && (target.u.sym->info & 0x0400)
                && (fix.type == 1 || fix.type == 2 || (fix.type == 3 && !iscode)) )
                LnkMsg( 0x2068, "Ss", target.u.sym, CurrMod->name );
            if( (frame.flags & 1) && (target.flags & 1) && CurrRec != 0 ) {
                if( target.flags & 2 ) fix.flags |= 1;
                else fix.flags &= ~1;
                fix.loc = CurrRec->buffer + offset;
                fix.addr.off = CurrRec->addr.off + offset;
                fix.addr.seg = CurrRec->addr.seg;
                if( frame.flags & 0x0100 ) fix.flags |= 0x40;
                else fix.flags &= ~0x40;
                if( !(fix.flags & 2)
                    && !(((seg_leader *)CurrRec->seg->entry)->info & 0x7000) )
                    OvlForceVect( &target, CanUseOvlVector( fix.type, 0 ) );
                Relocate( &fix, &target );
            }
        }
    } while( ObjBuff < EOObjRec );
}

extern fixup_ref *FindFixupThread( fixup_ref *, unsigned char );

void ParseFixupRefs( fixup_ref *frame, fixup_ref *target,
                      unsigned offset, unsigned code )
{
    address loc;
    unsigned char spec, method;
    unsigned long value, adjusted;
    group_entry *group;

    if( CurrRec != 0 ) {
        loc = CurrRec->addr;
        loc.off += offset;
    } else {
        loc.off = offset;
        loc.seg = 0;
    }
    spec = *ObjBuff++;
    if( spec & 0x80 )
        *frame = *FindFixupThread( (fixup_ref *)ObjInfoList, (spec >> 4) & 7 );
    else
        ParseFrame( (spec >> 4) & 7, frame );
    method = spec & 7;
    if( spec & 8 )
        *target = *FindFixupThread( (fixup_ref *)AuxObjInfoList, method & 3 );
    else
        ParseTarget( method, target );
    if( method <= 3 ) {
        if( ObjFormat & 3 ) {
            value = *(unsigned long *)ObjBuff;
            ObjBuff += 4;
        } else {
            value = *(unsigned short *)ObjBuff;
            ObjBuff += 2;
            if( (short)value < 0 && (unsigned long)(-(long)(short)value) <= target->addr.off )
                value = (long)(short)value;
        }
        adjusted = value;
        if( target->import != 0 ) {
            if( !(FmtData.type & 0x8016) ) LnkMsg( 0xf322, "a", loc );
        } else {
            adjusted = target->addr.off + value;
            if( !(FmtData.type & 0xff16) && adjusted > 0x10000 )
                LnkMsg( 0xf354, "a", loc );
        }
        target->addr.off = adjusted;
    } else if( target->import != 0 ) {
        target->addr.off = 0;
    }
    if( !(target->flags & 1) || !(frame->flags & 1) ) return;
    if( (frame->flags & 4) || (target->flags & 0xff000000UL) ) {
        frame->addr.seg = target->addr.seg;
    } else if( frame->flags & 0x40 ) {
        frame->addr.seg = loc.seg;
    } else if( target->import != 0 ) {
        if( FmtData.type & 0x0116 ) target->addr.seg = frame->addr.seg;
        else LnkMsg( 0xf337, "a", loc );
    }
    if( code == 2 || code == 3 || code == 6 ) {
        if( (FmtData.type & 0xff09) && target->addr.seg != frame->addr.seg )
            LnkMsg( (FmtData.type & 0x0100) ? 0x7343 : 0xf337, "a", loc );
        if( (LinkFlags & 0x00020000UL) && (target->flags & 8) && CurrRec != 0 ) {
            group = ((seg_leader *)CurrRec->seg->entry)->group;
            if( (CurrRec->seg->info & 0x0200) || (group != 0 && (group->segflags & 0x80)) )
                LnkMsg( 0xf288, "a", loc );
        }
    }
    if( target->import == 0 ) ConvertToFrame( &target->addr, frame->addr.seg );
}

extern void ResolveExtRef( extnode *, fixup_ref * );

void ParseFrame( unsigned char code, fixup_ref *ref )
{
    unsigned index;
    segnode *seg;
    group_entry *group;

    index = 0;
    if( code < 4 ) index = GetIdx();
    ref->addr.off = 0;
    ref->flags = (ref->flags & 0x00ff0000UL) | 1;
    ref->import = 0;
    ref->u.ptr = 0;
    switch( code ) {
    case 0:
        seg = FindNode( SegNodes, index );
        ref->addr.seg = seg->addr.seg;
        break;
    case 1:
        group = *(group_entry **)FindNode( GrpNodes, index );
        if( group == 0 ) ref->flags |= 0x0104;
        else ref->addr.seg = group->grp_addr.seg;
        break;
    case 2:
        ResolveExtRef( FindNode( ExtNodes, index ), ref );
        if( ref->import != 0 ) ref->flags |= 4;
        break;
    case 4:
        ref->flags |= 0x40;
        break;
    case 5:
        ref->flags |= 4;
        break;
    default:
        BadObject();
        ref->flags &= ~1;
        break;
    }
}

extern void SetGroupRefFlags( group_entry *, fixup_ref * );

void ParseTarget( unsigned char code, fixup_ref *ref )
{
    segnode *seg;
    seg_leader *leader;
    group_entry *group;
    extnode *ext;
    symbol *sym;

    ref->flags = (ref->flags & 0x00ff0000UL) | 1;
    ref->addr.off = 0;
    ref->import = 0;
    ref->u.ptr = 0;
    if( code > 7 ) return;
    switch( code & 3 ) {
    case 0:
        seg = FindNode( SegNodes, GetIdx() );
        ref->addr.off = seg->addr.off;
        ref->addr.seg = seg->addr.seg;
        if( !(seg->info & 0x0038) ) ref->flags |= 2;
        if( seg->info & 0x0400 ) ref->flags |= 0x20;
        leader = seg->entry;
        SetGroupRefFlags( leader->group, ref );
        break;
    case 1:
        group = *(group_entry **)FindNode( GrpNodes, GetIdx() );
        ref->addr.off = group->grp_addr.off;
        ref->addr.seg = group->grp_addr.seg;
        if( group->num == 0 || !(group->leaders[0]->info & 0x0400) )
            ref->flags |= 0x20;
        SetGroupRefFlags( group, ref );
        break;
    case 2:
        ext = FindNode( ExtNodes, GetIdx() );
        ResolveExtRef( ext, ref );
        sym = ext->entry;
        ref->u.sym = sym;
        ref->flags |= 0x10;
        if( (sym->info & 0x1000) && sym->p.seg != 0 && (sym->info & 15) != 6 ) {
            leader = sym->p.seg;
            if( leader->info & 0x0400 ) ref->flags |= 0x20;
            SetGroupRefFlags( leader->group, ref );
        }
        break;
    case 3:
        ref->addr.seg = *(unsigned short *)ObjBuff;
        ObjBuff += 2;
        ref->addr.off = 0;
        ref->flags |= 2;
        break;
    }
}

void SetGroupRefFlags( group_entry *group, fixup_ref *ref )
{
    if( group != 0 && (group->segflags & 1) ) {
        ref->flags |= 0x80;
        if( !(group->segflags & 0x80) ) ref->flags |= 8;
    }
}

unsigned char CanUseOvlVector( unsigned char type, unsigned char unused )
{
    seg_leader *seg;

    unused = unused;
    if( LinkFlags & 0x80 ) return 0;
    seg = CurrRec->seg->entry;
    if( !(seg->info & 0x0200) ) return 1;
    if( type == 1 || type == 2 || (type == 3 && (ObjFormat & 0x40)) ) return 1;
    return 0;
}

#pragma pack(1)
typedef struct bakpat {
    struct bakpat *next;
    unsigned char *addr;
    unsigned short size;
    unsigned char type, is32;
    unsigned char data[1];
} bakpat;
#pragma pack()
static bakpat *BakPats;                 /* original441f68 */

void ExecBakPats( void )
{
    bakpat *pat, *next;
    unsigned char *curr, *dest;
    unsigned long off, value;

    for( pat = BakPats; pat != 0; pat = next ) {
        next = pat->next;
        curr = pat->data;
        while( pat->size > 0 ) {
            if( pat->is32 ) {
                off = *(unsigned long *)curr;
                value = *(unsigned long *)(curr + 4);
                curr += 8;
                pat->size -= 8;
            } else {
                off = *(unsigned short *)curr;
                value = *(unsigned short *)(curr + 2);
                curr += 4;
                pat->size -= 4;
            }
            dest = pat->addr + off;
            switch( pat->type ) {
            case 0:
                *dest += value;
                break;
            case 1:
                *(unsigned short *)dest += value;
                break;
            case 2:
                *(unsigned long *)dest += value;
                break;
            }
        }
        _LnkFree( pat );
    }
    BakPats = 0;
}

void QueueFixupThread( fixup_ref **list, fixup_ref *ref )
{
    fixup_ref *old;

    while( (old = *list) != 0 ) {
        if( (unsigned char)(old->flags >> 16) == (unsigned char)(ref->flags >> 16) ) break;
        list = &old->next;
    }
    if( old != 0 ) {
        ref->next = old->next;
        *list = ref;
        _LnkFree( old );
    } else ref->next = 0;
    *list = ref;
}

fixup_ref *FindFixupThread( fixup_ref *list, unsigned char index )
{
    while( list != 0 && (unsigned char)(list->flags >> 16) != index ) list = list->next;
    return list;
}

void ProcBackpatch( void )
{
    segnode *seg;
    unsigned char type;

    seg = FindNode( SegNodes, GetIdx() );
    if( !(seg->info & 0x80) ) {
        type = *ObjBuff++;
        SaveBakPat( seg, type );
    }
}

extern unsigned char *GetObjAddr( segnode * );
void SaveBakPat( segnode *seg, unsigned char type )
{
    bakpat *pat;
    unsigned len;

    len = EOObjRec - ObjBuff;
    pat = ChkLAlloc( sizeof(*pat) - 1 + len );
    pat->size = len;
    pat->addr = GetObjAddr( seg );
    pat->type = type;
    pat->is32 = (ObjFormat & 3) != 0;
    memcpy( pat->data, ObjBuff, len );
    LinkList( &BakPats, pat );
}

void ProcNamedBackpatch( void )
{
    unsigned char type;
    list_of_names *lname;
    symbol *sym;
    segnode seg;

    type = *ObjBuff++;
    lname = FindName( GetIdx() );
    sym = RefISymbol( lname->name );
    if( (sym->info & 14) == 2 && !(sym->info & 0x10) ) {
        seg.entry = sym->p.leader;
        seg.offset = sym->u.comdat->a.saved_addr;
        SaveBakPat( &seg, type );
    }
}
