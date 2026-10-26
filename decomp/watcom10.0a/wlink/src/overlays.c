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
        GetVecAddr2( ext->entry->u.value & 0x0fff, ref );
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
