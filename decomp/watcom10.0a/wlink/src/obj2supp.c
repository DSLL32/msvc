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

#include "cmdline.h"
#include "fixup.h"
#include <recon.h>

extern unsigned char CheckSpecials( fixup_record *, fixup_ref * );
extern void PatchData( fixup_record *, fixup_ref * );
extern unsigned char FarCallOpt( fixup_record *, fixup_ref * );
extern void FmtReloc( fixup_record *, fixup_ref * );

void Relocate( fixup_record *fix, fixup_ref *target )
{
    fix->flags &= ~0x30;
    if( (FmtData.type & 0x0010) && target->import != 0 ) {
        target->addr.off = target->u.sym->addr.off;
        target->addr.seg = target->u.sym->addr.seg;
        target->import = 0;
    }
    if( !CheckSpecials( fix, target ) ) {
        PatchData( fix, target );
        if( !FarCallOpt( fix, target ) ) FmtReloc( fix, target );
    }
}

/* Encountered old fixup widths, indexed by the seven1994 relocation kinds. */
const unsigned char FixupSizeTable[] = { 1, 2, 2, 4, 1, 4, 6 };
unsigned char FloatOptions;             /* original BSS450076 */

extern pending_reloc *NewReloc( void );
extern unsigned short FindGroupIdx( unsigned short );
extern void AddNovImpReloc( symbol *, unsigned long, unsigned char, unsigned char );
extern void PatchOffset( fixup_record *, long );
extern group_entry *GetGroupBySeg( unsigned short );

unsigned char CheckSpecials( fixup_record *fix, fixup_ref *target )
{
    unsigned kind, size;
    long off, end;
    pending_reloc *rel;
    group_entry *group;

    kind = target->flags >> 24;
    if( (FmtData.type & 0x4200) && kind != 0 ) {
        if( (FloatOptions & 1) && kind != 7 ) {
            rel = NewReloc();
            rel->flags |= 1;
            *(unsigned long *)(rel->data + 2) = fix->addr.off | ((unsigned long)kind << 28);
            *(unsigned short *)rel->data = FindGroupIdx( fix->addr.seg );
        }
        return 1;
    }
    if( !(fix->flags & 2) ) return 0;
    if( FmtData.type & 6 ) {
        if( target->import != 0 || fix->addr.seg != target->addr.seg ) return 0;
    }
    if( (fix->flags & 1) && !(FmtData.type & 0x4200) ) {
        LnkMsg( 0xf310, "a", fix->addr );
        return 1;
    }
    if( fix->type & 2 ) {
        LnkMsg( 0xf311, "a", fix->addr );
        return 1;
    }
    if( target->import != 0 ) {
        if( FmtData.type & 9 ) LnkMsg( 0xf335, "a", fix->addr );
        else if( FmtData.type & 0x8000 ) return 0;
        else if( FmtData.type & 0x0100 ) {
            if( fix->type != 5 ) {
                LnkMsg( 0xf342, "a", fix->addr );
                return 1;
            }
            AddNovImpReloc( target->u.sym, fix->addr.off, 1, fix->addr.seg == 2 );
            PatchOffset( fix, -4 );
        }
        return 1;
    }
    if( (FmtData.type & 0xff09) && fix->addr.seg != target->addr.seg && !(fix->flags & 1) ) {
        LnkMsg( (FmtData.type & 0x0100) ? 0xf343 : 0xf334, "a", fix->addr );
        return 1;
    }
    if( FmtData.type & 0x0010 ) {
        group = GetGroupBySeg( target->addr.seg );
        off = group->linear;
        group = GetGroupBySeg( fix->addr.seg );
        off -= group->linear;
        off += target->addr.off - fix->addr.off;
    } else if( FmtData.type & 0xff16 ) {
        off = target->addr.off - fix->addr.off;
        if( fix->flags & 1 ) off -= GetGroupBySeg( fix->addr.seg )->linear;
    } else {
        off = ((unsigned long)target->addr.seg - fix->addr.seg) * 16
                + target->addr.off - fix->addr.off;
    }
    size = FixupSizeTable[fix->type];
    off -= size;
    end = fix->addr.off + size + off;
    if( fix->type != 5 && (end < 0 || end >= 0x10000) )
        LnkMsg( 0xf312, "a", fix->addr );
    PatchOffset( fix, off );
    if( (FmtData.type & 4) && (LinkFlags & 0x20000000UL) ) fix->flags |= 0x20;
    else fix->flags &= ~0x20;
    return !(fix->flags & 0x21);
}

extern unsigned char DBINoReloc( unsigned short );
extern unsigned long FindRealAddr( fixup_record *, fixup_ref * );
extern unsigned short GetCVSegment( unsigned short );

void PatchData( fixup_record *fix, fixup_ref *target )
{
    unsigned char *loc;
    unsigned char isdbi;
    unsigned width;

    loc = fix->loc;
    fix->flags &= ~8;
    fix->value = (target->import != 0 && (FmtData.type & 6)) ? target->addr.off : 0;
    switch( fix->type ) {
    case 0:
    case 4:
        fix->value += *loc;
        break;
    case 1:
    case 3:
        fix->value += *(unsigned short *)loc;
        break;
    case 5:
    case 6:
        fix->value += *(unsigned long *)loc;
        break;
    }
    if( fix->value != 0 ) fix->flags |= 8;
    if( fix->flags & 2 ) return;
    isdbi = DBINoReloc( CurrRec->seg->info );
    if( target->import != 0 ) {
        if( !isdbi ) return;
        target->addr.off = 0;
        target->addr.seg = 0;
    }
    if( ((fix->flags & 1) && !(FmtData.type & 0x4200)) || isdbi ) fix->flags |= 0x10;
    width = 2;
    switch( fix->type ) {
    case 0:
    case 1:
    case 4:
    case 5:
        if( (FmtData.type & 8) && (fix->flags & 4) ) return;
        PatchOffset( fix, FindRealAddr( fix, target ) );
        if( FmtData.type & 0x5316 ) {
            if( !(FmtData.type & 4) || (LinkFlags & 0x08000000UL) ) return;
        }
        fix->flags |= 0x10;
        return;
    case 6:
        width = 4;
        /* fall through */
    case 3:
        if( !(fix->flags & 0x10) && (FmtData.type & 15) ) return;
        PatchOffset( fix, FindRealAddr( fix, target ) );
        loc += width;
        break;
    }
    if( FmtData.type & 0xff1f ) *(unsigned short *)loc = 0;
    if( FmtData.type & 0x2000 ) {
        *(unsigned short *)loc = target->addr.seg;
    } else {
        if( !(fix->flags & 0x10) && !(FmtData.type & 0x42e0) ) return;
        if( isdbi && (LinkFlags & 0x20) ) *(unsigned short *)loc = GetCVSegment( target->addr.seg );
        else *(unsigned short *)loc += target->addr.seg;
    }
}

unsigned long FindRealAddr( fixup_record *fix, fixup_ref *target )
{
    unsigned char isdbi;
    unsigned long value;
    group_entry *group;

    value = target->addr.off;
    isdbi = DBINoReloc( CurrRec->seg->info );
    if( fix->flags & 1 ) return value;
    if( isdbi && !(ObjFormat & 0x0400) ) return value;
    if( !(FmtData.type & 0x4014) ) return value;
    group = GetGroupBySeg( target->addr.seg );
    if( group == 0 ) return value;
    value += group->linear - group->grp_addr.off;
    if( fix->flags & 2 ) {
        group = ((seg_leader *)CurrRec->seg->entry)->group;
        value -= group->linear - group->grp_addr.off;
        value -= FixupSizeTable[fix->type];
    } else if( FmtData.type & 0x0010 ) {
        value += FmtData.base;
        if( isdbi ) value -= Groups->linear;
    }
    if( isdbi ) value -= FmtData.base;
    return value;
}

group_entry *GetGroupBySeg( unsigned short seg )
{
    group_entry *group;

    for( group = Groups; group != 0; group = group->next_group )
        if( group->grp_addr.seg == seg ) break;
    return group;
}

void PatchOffset( fixup_record *fix, long value )
{
    switch( fix->type ) {
    case 0:
    case 4:
        *fix->loc += value;
        break;
    case 1:
    case 2:
    case 3:
        *(unsigned short *)fix->loc += value;
        break;
    case 5:
    case 6:
        *(unsigned long *)fix->loc += value;
        break;
    default:
        LnkMsg( 0xf30f, 0 );
        break;
    }
}

extern unsigned long LastOptimized;       /* original441f18 */
static unsigned char LastOptType;          /* original4501a8 */

/* Turn a same-segment far transfer into the old aligned near sequence. */
unsigned char FarCallOpt( fixup_record *fix, fixup_ref *target )
{
    unsigned char *code;
    unsigned char instruction, is32;
    unsigned short temp16;
    unsigned long temp32;

    if( (ObjFormat & 0x40) || target->import != 0 || (fix->flags & 1) ) return 0;
    if( fix->type == 3 ) is32 = 0;
    else if( fix->type == 6 ) is32 = 1;
    else return 0;
    if( fix->loc == CurrRec->buffer || fix->addr.seg != target->addr.seg ) return 0;
    if( !(CurrRec->seg->info & 0x8000) ) return 0;
    code = fix->loc - 1;
    instruction = *code;
    if( instruction != 0x9a && instruction != 0xea ) return 0;
    if( is32 ) temp32 = *(unsigned long *)(code + 1);
    else temp16 = *(unsigned short *)(code + 1);
    if( FmtData.type & 15 ) {
        if( is32 ) temp32 += target->addr.off;
        else temp16 += target->addr.off;
    }
    if( instruction == 0xea ) {
        *code = 0xe9;
        if( is32 ) {
            temp32 -= fix->addr.off + 4;
            *(unsigned long *)(code + 1) = temp32;
            *(unsigned short *)(code + 5) = 0xc089;
        } else {
            temp16 -= fix->addr.off + 2;
            *(unsigned short *)(code + 1) = temp16;
            *(unsigned short *)(code + 3) = 0xc089;
        }
    } else {
        *code = 0x0e;
        if( is32 ) temp32 -= fix->addr.off + 6;
        else temp16 -= fix->addr.off + 4;
        if( fix->addr.off & 1 ) {
            code[1] = 0xe8;
            if( is32 ) {
                code[6] = 0x90;
                ++temp32;
            } else {
                code[4] = 0x90;
                ++temp16;
            }
            LastOptimized = fix->addr.off;
            LastOptType = fix->type;
            --code;
        } else {
            if( (LastOptType == 3 && fix->addr.off >= 5 && LastOptimized == fix->addr.off - 5)
                || (LastOptType == 6 && fix->addr.off >= 7 && LastOptimized == fix->addr.off - 7) )
                code[-1] = 0x36;
            /* The old binary writes DS override3e here, unlike later source. */
            code[1] = 0x3e;
            code[2] = 0xe8;
        }
        if( is32 ) *(unsigned long *)(code + 3) = temp32;
        else *(unsigned short *)(code + 3) = temp16;
    }
    return 1;
}

/* The encountered OS/2 source kinds match the old seven fixup kinds. */
const unsigned char OS2FixTypeMap[] = { 0, 5, 2, 3, 0, 7, 6 };
#pragma pack(1)
typedef struct os2_reloc_item {
    unsigned char addr_type, reloc_type;
    unsigned short reloc_offset, module, target;
} os2_reloc_item;
#pragma pack()
extern unsigned short __cdecl FindEntryOrdinal( address, group_entry * );
extern unsigned char FindOS2ExportSym( symbol *, dll_sym_info ** );
extern void RemoveReloc( pending_reloc * );
extern void MakeBase( fixup_record * );
extern unsigned long GetElfImport( symbol * );

void FmtReloc( fixup_record *fix, fixup_ref *target )
{
    pending_reloc *rec;
    group_entry *group;
    segnode *seg;
    address targ;
    unsigned long off;
    unsigned char flags, fixtype, freedll, saved;
    unsigned char *item, *cursor;
    dll_sym_info *dll;
    os2_reloc_item *os2;

    if( (fix->flags & 1) && !(FmtData.type & 0x4200) && target->import == 0 ) return;
    if( fix->flags & 0x10 ) return;
    if( (FmtData.type & 0x0c00)
        || ((FmtData.type & 0x8110) && fix->type != 5)
        || ((FmtData.type & 0x1000) && fix->type != 1 && fix->type != 5) ) {
        LnkMsg( 0xf33f, "a", fix->addr );
        return;
    }
    if( !(LinkState & 1) ) return;
    if( (FmtData.type & 0x8000) && target->import == 0 ) return;
    if( FmtData.type & 0x4200 ) {
        if( fix->type == 5 ) {
            if( !(FloatOptions & 2) ) return;
        } else if( fix->type == 2 || fix->type == 3 ) {
            if( !(FloatOptions & 1) ) return;
        } else if( fix->type != 6 ) return;
    }
    rec = NewReloc();
    rec->span = FixupSizeTable[fix->type];
    rec->offset = fix->addr.off;
    off = fix->addr.off;
    seg = CurrRec->seg;
    targ = target->addr;
    item = rec->data;
    if( FmtData.type & 0x0010 ) {
        off += ((seg_leader *)seg->entry)->group->linear;
        *(unsigned short *)item = (off & 4095) | 0x3000;
        return;
    }
    if( FmtData.type & 9 ) {
        os2 = (os2_reloc_item *)item;
        os2->addr_type = OS2FixTypeMap[fix->type];
        os2->reloc_offset = off - ((seg_leader *)seg->entry)->group->grp_addr.off;
        dll = target->import;
        if( dll == 0 ) {
            os2->reloc_type = 0;
            for( group = Groups; group != 0; group = group->next_group )
                if( group->grp_addr.seg == targ.seg ) break;
            if( group != 0 && (group->segflags & 0x10) ) {
                os2->module = 0x00ff;
                os2->target = FindEntryOrdinal( targ, group );
            } else {
                os2->module = (unsigned char)targ.seg;
                os2->target = targ.off;
            }
        } else {
            os2->reloc_type = dll->isordinal ? 1 : 2;
            os2->module = dll->modnum;
            os2->target = dll->entry;
        }
        if( fix->flags & 8 ) os2->reloc_type |= 4;
        else *(unsigned short *)fix->loc = 0xffff;
        return;
    }
    if( FmtData.type & 6 ) {
        dll = target->import;
        freedll = 0;
        if( fix->flags & 2 ) {
            fixtype = 8;
            if( (fix->flags & 0x20) && (target->flags & 0x10) )
                freedll = FindOS2ExportSym( target->u.sym, &dll );
        } else fixtype = OS2FixTypeMap[fix->type];
        if( !(target->flags & 0x20) ) {
            switch( fixtype ) {
            case 2:
            case 6:
                if( fix->flags & 0x40 ) break;
                /* fall through */
            case 3:
                fixtype |= 0x10;
                /* fall through */
            case 5:
                for( group = Groups; group != 0; group = group->next_group )
                    if( group->grp_addr.seg == targ.seg ) break;
                if( group != 0 ) group->info |= 1;
                break;
            }
        }
        item[0] = fixtype;
        *(unsigned short *)(item + 2) = (fix->addr.off - ((seg_leader *)seg->entry)->group->grp_addr.off) & 4095;
        cursor = item + 4;
        if( dll == 0 ) {
            if( !(fix->flags & 0x20) ) targ.off += fix->value;
            flags = 0;
            if( targ.seg > 255 ) {
                flags |= 0x40;
                *(unsigned short *)cursor = targ.seg;
                ++cursor;
            } else *cursor = targ.seg;
            ++cursor;
            targ.off -= GetGroupBySeg( targ.seg )->grp_addr.off;
            if( fix->type != 2 ) {
                if( targ.off > 65535 ) {
                    flags |= 0x10;
                    *(unsigned long *)cursor = targ.off;
                    cursor += 2;
                } else *(unsigned short *)cursor = targ.off;
                cursor += 2;
            }
        } else {
            flags = dll->isordinal ? 1 : 2;
            if( dll->modnum > 255 ) {
                flags |= 0x40;
                *(unsigned short *)cursor = dll->modnum;
                ++cursor;
            } else *cursor = dll->modnum;
            ++cursor;
            if( !dll->isordinal || dll->entry > 255 ) {
                *(unsigned short *)cursor = dll->entry;
                ++cursor;
            } else {
                flags |= 0x80;
                *cursor = dll->entry;
            }
            ++cursor;
            if( fix->flags & 8 ) {
                flags |= 4;
                switch( fix->type ) {
                case 0:
                case 4:
                    *fix->loc = 0;
                    break;
                case 1:
                case 3:
                    *(unsigned short *)fix->loc = 0;
                    break;
                case 5:
                case 6:
                    *(unsigned long *)fix->loc = 0;
                    break;
                }
                if( fix->value > 0x7fff ) {
                    flags |= 0x20;
                    *(unsigned long *)cursor = fix->value;
                    cursor += 2;
                } else *(unsigned short *)cursor = fix->value;
                cursor += 2;
            }
        }
        if( freedll ) _LnkFree( dll );
        item[1] = flags;
        rec->length = cursor - item;
        return;
    }
    if( FmtData.type & 0x0100 ) {
        if( target->import != 0 ) {
            RemoveReloc( rec );
            AddNovImpReloc( target->u.sym, off, (fix->flags & 2) != 0, fix->addr.seg == 2 );
        } else {
            if( fix->addr.seg != 2 ) off |= 0x40000000UL;
            if( targ.seg != 2 ) off |= 0x80000000UL;
            *(unsigned long *)item = off;
        }
        return;
    }
    if( FmtData.type & 0x4200 ) {
        saved = 0;
        if( (fix->type == 5 || fix->type == 6) && (FloatOptions & 2) ) {
            rec->flags |= 2;
            *(unsigned long *)item = fix->addr.off;
            if( !(target->flags & 0x80) ) item[3] |= 0x80;
            if( (fix->flags & 3) == 3 ) item[3] |= 0x40;
            if( fix->type == 6 ) {
                rec = NewReloc();
                rec->span = FixupSizeTable[fix->type];
                rec->offset = fix->addr.off;
                item = rec->data;
            } else saved = 1;
        }
        if( !saved ) {
            MakeBase( fix );
            *(unsigned long *)(item + 2) = fix->addr.off;
            *(unsigned short *)item = FindGroupIdx( fix->addr.seg );
        }
        return;
    }
    if( FmtData.type & 0x8000 ) {
        if( target->import == (dll_sym_info *)-1 )
            target->import = (dll_sym_info *)GetElfImport( target->u.sym );
        *(unsigned long *)item = off;
        *(unsigned long *)(item + 4) = ((fix->flags & 2) ? 2 : 1)
                                      | ((unsigned long)target->import << 8);
        return;
    }
    if( FmtData.type & 0x1000 ) {
        if( fix->type == 5 ) off |= 0x80000000UL;
        *(unsigned long *)item = off;
        return;
    }
    if( FmtData.type & 0x0040 ) {
        RemoveReloc( rec );
        LnkMsg( 0xf213, "a", fix->addr );
        return;
    }
    MakeBase( fix );
    group = ((seg_leader *)seg->entry)->group;
    if( FmtData.type & 0x2000 ) {
        *(unsigned long *)item = fix->addr.off;
        *(unsigned short *)(item + 4) = group->grp_addr.seg;
    } else {
        *(unsigned short *)item = fix->addr.off;
        *(unsigned short *)(item + 2) = group->grp_addr.seg;
        if( group->section != Root )
            *(unsigned short *)(item + 2) -= group->section->sect_addr.seg;
    }
}

pending_reloc *NewReloc( void )
{
    pending_reloc *rel;

    rel = ChkLAlloc( sizeof(*rel) );
    rel->length = FmtRelocSize;
    /* Original allocation leaves six unused flag bits undefined. */
    rel->flags = 0;
    rel->next = (pending_reloc *)CurrRec->fixups;
    CurrRec->fixups = (path_entry *)rel;
    return rel;
}
