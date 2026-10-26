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

extern void ResetSegDataList( void );
extern void DBIP2Start( section * );
extern void DBIOvlPass2( void );
extern void PModList( mod_entry * );
extern void OvlPass2( void );
extern void ProcDistMods( void );
extern void WriteUndefined( void );
extern void SetOvlStartAddr( void );
extern void WritePETransferTable( void );
extern void DBIFini( section * );
extern void DBIOvlFini( void );

void ObjPass2( void )
{
    LinkState |= 0x1000;
    ResetSegDataList();
    DBIP2Start( Root );
    DBIOvlPass2();
    CurrSect = Root;
    PModList( Root->mods );
    OvlPass2();
    if( LinkFlags & 4 ) ProcDistMods();
    else {
        CurrSect = Root;
        PModList( LibModules );
    }
    WriteUndefined();
    if( FmtData.type & 0x0080 ) SetOvlStartAddr();
    if( FmtData.type & 0x0010 ) WritePETransferTable();
    DBIFini( Root );
    DBIOvlFini();
}

extern void PModule( mod_entry * );

void PModList( mod_entry *head )
{
    mod_entry *obj;

    for( obj = head; obj != 0; obj = obj->next_mod ) PModule( obj );
}

extern void SetPubCurr( symbol *, unsigned char );
extern void OMFPass2( void );
extern void ORLPass2( void );
extern void WriteModSegs( void );
extern void TraceModRelocs( void );
extern void DBIP2Module( void );

void PModule( mod_entry *obj )
{
    if( !(obj->modinfo & 0x4000) ) return;
    if( !CacheOpen( obj->f.source ) ) return;
    CurrMod = obj;
    SetPubCurr( obj->publist, 1 );
    IdentifyObject( obj->f.source, obj->location );
    if( ObjFormat & 0x0100 ) OMFPass2();
    else ORLPass2();
    SetPubCurr( obj->publist, 0 );
    if( MapFlags & 0x40 ) {
        WriteModSegs();
        TraceModRelocs();
    }
    DBIP2Module();
    ReleaseNames();
    FreeObjInfo();
    CacheClose( obj->f.source, 2 );
}

extern unsigned char *ExpandIteratedData( unsigned char *, unsigned long );
extern void ReleaseIteratedRecord( void );

void FlushObjectData( void )
{
    unsigned char *curr, *end;
    unsigned long repeat;

    if( CurrIterated == 0 ) return;
    curr = CurrIterated->buffer;
    end = curr + CurrIterated->length;
    CurrRec = 0;
    while( curr < end ) {
        if( CurrIterated->is32 ) {
            repeat = *(unsigned long *)curr;
            curr += 4;
        } else {
            repeat = *(unsigned short *)curr;
            curr += 2;
        }
        curr = ExpandIteratedData( curr, repeat );
    }
    ReleaseIteratedRecord();
}

extern void ApplyObjectFixups( void );

void FlushObjectRelocs( void )
{
    ApplyObjectFixups();
    if( CurrRec != 0 ) {
        _LnkFree( CurrRec );
        CurrRec = 0;
    }
}


unsigned long LastOptimized;       /* original441f18 */
extern void WriteObjectData( void );
extern void FloatReloc( void * );
extern void QNXLinearReloc( group_entry *, void * );
extern void WriteReloc( group_entry *, unsigned long, void *, unsigned short );

void ApplyObjectFixups( void )
{
    pending_reloc *rel;
    group_entry *group;
    unsigned long left;

    LastOptimized = 0xffffffffUL;
    if( CurrRec == 0 ) return;
    WriteObjectData();
    if( !(LinkState & 1) ) return;
    group = ((seg_leader *)CurrRec->seg->entry)->group;
    while( CurrRec->fixups != 0 ) {
        rel = (pending_reloc *)CurrRec->fixups;
        if( rel->flags & 1 ) FloatReloc( rel->data );
        else if( rel->flags & 2 ) QNXLinearReloc( group, rel->data );
        else {
            WriteReloc( group, rel->offset, rel->data, rel->length );
            if( FmtData.type & 6 ) {
                left = 4096 - (rel->offset & 4095);
                if( left < rel->span ) {
                    *(unsigned short *)(rel->data + 2) -= 4096;
                    WriteReloc( group, rel->offset + 4096, rel->data, rel->length );
                }
            }
        }
        CurrRec->fixups = (path_entry *)rel->next;
        _LnkFree( rel );
    }
}

extern unsigned char *GetObjAddr( segnode * );
extern void AllocObjectData( unsigned char *, segnode *, unsigned char *, unsigned short );

void LoadObj( segnode *seg, unsigned long offset, unsigned char iterated )
{
    seg_leader *leader;
    group_entry *group;
    unsigned char *dest;
    unsigned short len;

    FlushObjectData();
    FlushObjectRelocs();
    ObjFormat &= ~0x20;
    if( DBISkip( seg->info ) || !(seg->info & 0x0038) ) return;
    leader = seg->entry;
    dest = GetObjAddr( seg );
    if( EOObjRec == ObjBuff ) return;
    len = (unsigned short)((unsigned long)EOObjRec - (unsigned long)ObjBuff);
    if( offset + len > seg->length && !iterated ) LnkMsg( 0xf40b, 0 );
    AllocObjectData( dest + offset, seg, ObjBuff, len );
    group = leader->group;
    if( group == 0 ) {
        CurrRec->addr.off = seg->addr.off + offset;
        CurrRec->addr.seg = seg->addr.seg;
    } else {
        CurrRec->addr.off = group->grp_addr.off;
        CurrRec->addr.seg = group->grp_addr.seg;
        CurrRec->addr.off += ((unsigned long)seg->addr.seg - group->grp_addr.seg) * 16
                            + seg->addr.off - group->grp_addr.off + offset;
    }
    if( iterated ) {
        CurrIterated = CurrRec;
        CurrIterated->is32 = (ObjFormat & 2) != 0;
    }
}

void AllocObjectData( unsigned char *dest, segnode *seg,
                       unsigned char *data, unsigned short len )
{
    lobject_data *rec;

    rec = ChkLAlloc( sizeof(*rec) - 1 + len );
    rec->fixups = 0;
    rec->offset = 0;
    rec->data = dest;
    CurrRec = rec;
    rec->seg = seg;
    rec->length = len;
    memcpy( rec->buffer, data, len );
}

extern unsigned long TrimTrailingZeros( unsigned char *, unsigned long, unsigned long );

void WriteObjectData( void )
{
    lobject_data *rec;
    seg_leader *leader;
    group_entry *group;
    unsigned long len, total, min, oldlen;
    long gap;

    rec = CurrRec;
    leader = rec->seg->entry;
    len = rec->length - rec->offset;
    group = leader->group;
    if( group != 0 ) {
        total = (unsigned long)rec->data - group->data + len;
        if( ((FmtData.type & 0x801f) && (group->segflags & 1))
            || ((FmtData.type & 0x4200) && (leader->u.p1.segflags & 3) <= 1) ) {
            if( total > group->size + len ) min = 0;
            else min = len - (total - group->size);
            oldlen = len;
            len = TrimTrailingZeros( rec->buffer, len, min );
            if( len == 0 ) return;
            total -= oldlen - len;
        }
        if( total > group->size ) {
            gap = (unsigned long)rec->data - (group->data + group->size);
            if( gap > 0 ) memset( (void *)(group->data + group->size), 0, gap );
            group->size = total;
        }
    }
    memcpy( rec->data, rec->buffer, len );
}

unsigned long TrimTrailingZeros( unsigned char *data, unsigned long len, unsigned long min )
{
    while( len > min && data[len - 1] == 0 ) --len;
    return len;
}
