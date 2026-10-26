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
#pragma intrinsic(strlen)
#endif
extern list_of_names *OvlClasses;

section **SectOvlTab;   /* original441dfc */
extern unsigned SectionNum;    /* original45010c */

void NumberSections( void )
{
    if( LinkFlags & 4 ) {
        SectOvlTab = ChkLAlloc( sizeof(section *) * (SectionNum + 1) );
        SectOvlTab[0] = Root;
    }
    SectionNum = 1;
    ProcAllOvl( NumASect );
}

void ProcAllOvl( void (*rtn)(section *) )
{
    if( FmtData.type & 0x80 ) WalkAreas( Root->areas, rtn );
}

void TryDefVector( symbol *sym )
{
    if( FmtData.type & 0x0080 ) {
        if( sym->info & 0x2000 ) DefDistribSym( sym );
        else OvlDefVector( sym );
    }
}

void FillOutFilePtrs( void )
{
    ProcAllOvl( FillOutPtr );
}

void DBIAddrOvlStart( void )
{
    ProcAllOvl( DBIAddrSectStart );
}

extern void DBIFini( section * );

void DBIOvlFini( void )
{
    ProcAllOvl( DBIFini );
    if( NonSect != 0 ) DBIFini( NonSect );
}

extern void DBIP2Start( section * );

void DBIOvlPass2( void )
{
    ProcAllOvl( DBIP2Start );
}

extern void OvlUseVector( symbol *, extnode * );

void TryUseVector( symbol *sym, extnode *node )
{
    /* This older entry requires a real node; it has no later NULL check. */
    node->flags &= 0xf000;
    if( FmtData.type & 0x0080 ) OvlUseVector( sym, node );
}

unsigned char *GetObjAddr( segnode *seg )
{
    seg_leader *leader;

    leader = seg->entry;
    if( (LinkFlags & 0x20) && (leader->info & 0x7000) )
        return (unsigned char *)leader->data;
    return (unsigned char *)(leader->data + seg->offset + leader->addr_adjustment);
}

extern void GetVecAddr2( unsigned, fixup_ref * );
extern void GetSymAddr( symbol *, fixup_ref * );

void ResolveExtRef( extnode *ext, fixup_ref *ref )
{
    if( ext->flags & 0x0fff ) {
        GetVecAddr2( (ext->entry->u.value >> 16) & 0x0fff, ref );
        ref->flags |= 1;
    } else {
        GetSymAddr( ext->entry, ref );
    }
}

extern void EmitOvlVectors( void );
extern void PSection( section * );

void OvlPass2( void )
{
    if( FmtData.type & 0x80 ) {
        EmitOvlVectors();
        ProcAllOvl( PSection );
    }
}

extern void WriteDBISecs( section * );

void WriteOvlSecs( void )
{
    ProcAllOvl( WriteDBISecs );
}

unsigned short AreaSize = 0xffff; /* original44128c; overlay paragraph count */

extern section *CheckOvlSect( char * );
section *GetOvlSect( char *name )
{
    if( FmtData.type & 0x80 ) return CheckOvlSect( name );
    return Root;
}

void WalkSections( section *sect, void (*rtn)(section *) )
{
    for( ; sect != 0; sect = sect->next_sect ) {
        rtn( sect );
        WalkAreas( sect->areas, rtn );
    }
}
void WalkAreas( ovl_area *ovl, void (*rtn)(section *) )
{
    for( ; ovl != 0; ovl = ovl->next ) WalkSections( ovl->sections, rtn );
}

void NumASect( section *sect )
{
    if( LinkFlags & 4 ) SectOvlTab[SectionNum] = sect;
    sect->ovl_num = SectionNum++;
}

unsigned char CheckOvlClass( char *name, unsigned char *isovl )
{
    list_of_names *cl;
    unsigned char iscode;

    cl = OvlClasses;
    if( cl == 0 ) {
        iscode = IsCodeClass( name, strlen(name) );
        *isovl = iscode;
        return iscode;
    }
    do {
        if( stricmp( name, cl->name ) == 0 ) {
            *isovl = 1;
            return cl->flags;
        }
        cl = cl->next_name;
    } while( cl != 0 );
    *isovl = 0;
    return 0;
}

void TryRefVector( symbol *sym )
{
    if( FmtData.type & 0x80 ) {
        if( (LinkState & 2) && (LinkFlags & 4) ) RefDistribSym( sym );
        else OvlRefVector( sym );
    }
}

void OvlRefVector( symbol *sym )
{
    unsigned short state, num;

    state = sym->u.value >> 16;
    if( (sym->info & 15) <= 3 || (state & 0x3000) != 0 ) return;
    if( !(sym->info & 0x1000) ) {
        if( !(state & 0x8000) ) {
            state = (state & 0xf000) | (CurrSect->ovl_num & 0x0fff) | 0x8000;
            sym->u.value = (sym->u.value & 0xffffUL) | ((unsigned long)state << 16);
        } else if( LinkFlags & 4 ) {
            num = LowestAncestor( state & 0x0fff, CurrSect );
            state = (state & 0xf000) | (num & 0x0fff);
            sym->u.value = (sym->u.value & 0xffffUL) | ((unsigned long)state << 16);
        }
    } else {
        if( sym->p.seg == 0 ) return;
        if( sym->info & 0x10 ) num = ((segdata *)sym->p.seg)->owner.normal.sect_id;
        else num = sym->p.leader->section->ovl_num;
        if( !IsAncestor( num, CurrSect ) ) Vectorize( sym );
    }
}

void OvlDefVector( symbol *sym )
{
    unsigned short state, num;
    unsigned char code;

    state = sym->u.value >> 16;
    if( (sym->info & 15) <= 3 || (state & 0x2000) ) return;
    if( sym->p.seg == 0 ) {
        state |= 0x3000;
    } else {
        if( sym->info & 0x10 ) {
            num = ((segdata *)sym->p.seg)->owner.normal.sect_id;
            code = (((segdata *)sym->p.seg)->flags & 4) != 0;
        } else {
            num = sym->p.leader->section->ovl_num;
            code = (sym->p.leader->info & 0x0200) != 0;
        }
        if( !code || num == 0 ) state |= 0x3000;
        else if( sym->info & 0x40 ) {
            if( (state & 0x0fff) != num ) Vectorize( sym );
            return;
        } else {
            state = (state & 0xf000) | (num & 0x0fff) | 0x8000;
        }
    }
    sym->u.value = (sym->u.value & 0xffffUL) | ((unsigned long)state << 16);
}

void IndirectCall( symbol *sym )
{
    unsigned short state, num;
    unsigned char code;

    if( !(FmtData.type & 0x80) ) return;
    state = sym->u.value >> 16;
    if( (sym->info & 15) <= 3 || (state & 0x2000) ) return;
    if( sym->info & 0x1000 ) {
        if( sym->info & 0x2000 ) {
            DistIndCall( sym );
        } else if( sym->p.seg != 0 ) {
            if( sym->info & 0x10 ) {
                num = ((segdata *)sym->p.seg)->owner.normal.sect_id;
                code = (((segdata *)sym->p.seg)->flags & 4) != 0;
            } else {
                num = sym->p.leader->section->ovl_num;
                code = (sym->p.leader->info & 0x0200) != 0;
            }
            if( num != 0 && code ) Vectorize( sym );
        }
    } else {
        state = (state & 0xf000) | 0x8000;
        sym->u.value = (sym->u.value & 0xffffUL) | ((unsigned long)state << 16);
    }
}

void Vectorize( symbol *sym )
{
    vecnode *vec;
    unsigned short state;

    state = sym->u.value >> 16;
    state = (state & 0xf000) | (++VecNum & 0x0fff);
    state = (state & ~0x1000) | 0x2000;
    sym->u.value = (sym->u.value & 0xffffUL) | ((unsigned long)state << 16);
    vec = PermAlloc( sizeof(*vec) );
    vec->entry = sym;
    LinkList( &OvlVectors, vec );
}

void FillOutPtr( section *sect )
{
    if( sect->outfile == 0 ) {
        if( sect->parent != 0 ) sect->outfile = sect->parent->outfile;
        else sect->outfile = Root->outfile;
    }
}

section *CheckOvlSect( char *name )
{
    unsigned char isovl;
    section *sect;

    sect = DBIGetSect( name );
    if( sect == 0 ) {
        if( CheckOvlClass( name, &isovl ) ) sect = CurrSect;
        else sect = NonSect;
    }
    return sect;
}

extern void DoPubs( section * );
void ProcOvlSectPubs( section *sect )
{
    mod_entry *next;

    if( sect->u.dist_mods != 0 ) {
        for( CurrMod = sect->u.dist_mods; CurrMod != 0; CurrMod = next ) {
            next = CurrMod->arcs;
            DoPubs( sect );
        }
    }
}

void OvlUseVector( symbol *sym, extnode *node )
{
    unsigned short state;

    if( !(FmtData.type & 0x80) || (sym->info & 15) <= 3 ) return;
    state = sym->u.value >> 16;
    if( ((state >> 12) & 3) != 2 ) return;
    if( IsAncestor( sym->p.leader->section->ovl_num, CurrSect ) ) return;
    node->flags = (node->flags & 0xf000) | (state & 0x0fff);
}

unsigned char IsAncestor( unsigned index, section *sect )
{
    for( ; sect != 0; sect = sect->parent ) {
        if( sect->ovl_num == index ) return 1;
    }
    return 0;
}

extern void PModList( mod_entry * );
void PSection( section *sect )
{
    CurrSect = sect;
    PModList( sect->mods );
}
