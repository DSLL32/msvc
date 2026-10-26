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
#include <recon.h>

void DBISectCleanup( section *sect )
{
    if( LinkFlags & 0x4000 ) ODBISectCleanup( sect );
}

/* Old debug-group workspace is one allocation, unlike the later list. */
/* DBIModName (original442248) is shared by module work and cleanup. */

void DBICleanup( void )
{
    if( DBIModName != 0 ) {
        _LnkFree( DBIModName );
        DBIModName = 0;
    }
}

void DBIInit( void )
{
    if( LinkFlags & 0x4000 ) ODBIInit( Root );
    else if( LinkFlags & 0x20 ) CVInit();
}

void DBIInitModule( mod_entry *mod )
{
    if( LinkFlags & 0x4000 ) ODBIInitModule( mod );
    else if( LinkFlags & 1 ) DwarfInitModule( mod );
    else if( LinkFlags & 0x20 ) CVInitModule( mod );
}

void DBIP1Source( unsigned char *buff, unsigned char *endbuff )
{
    int len;
    unsigned char major, minor;

    major = *buff++;
    minor = *buff++;
    len = endbuff - buff;
    if( len <= 0 ) {
        BadObject();
        return;
    }
    ObjFormat |= 0x80;
    if( LinkFlags & 0x4000 ) ODBIP1Source( major, minor, buff, len );
}

section *DBIGetSect( char *name )
{
    if( stricmp( name, "DEBTYP" ) == 0 || stricmp( name, "DEBSYM" ) == 0 ) {
        return CurrSect;
    }
    if( stricmp( name, "DWARF" ) == 0 ) return Root;
    return 0;
}

void DBIColClass( class_entry *class )
{
    unsigned char isdbi;

    isdbi = 1;
    if( stricmp( class->name, "DWARF" ) == 0 ) class->flags |= 0x0c;
    else if( stricmp( class->name, "DEBTYP" ) == 0 ) class->flags |= 4;
    else if( stricmp( class->name, "DEBSYM" ) == 0 ) class->flags |= 8;
    else isdbi = 0;
    if( isdbi ) {
        if( class->flags & 1 ) LinkState |= 0x0800;
        else LinkState |= 0x4000;
    }
    if( (LinkState & 0x0800) && (LinkState & 0x4000) ) LnkMsg( 0x7460, 0 );
}

unsigned short DBIColSeg( class_entry *class )
{
    switch( class->flags & 0x0c ) {
    case 0x0c:
        CurrMod->modinfo |= 0x0800;
        return 0x5000;
    case 4:
        return 0x6000;
    case 8:
        return 0x7000;
    }
    return 0;
}

unsigned char DBISkip( unsigned short info )
{
    switch( info & 0x7000 ) {
    case 0x6000:
        return !((CurrMod->modinfo & 2) && (ObjFormat & 0x80)
                  && (LinkFlags & 0x4000)) && !(LinkFlags & 0x20);
    case 0x7000:
        return !((CurrMod->modinfo & 4) && (ObjFormat & 0x80)
                  && (LinkFlags & 0x4000)) && !(LinkFlags & 0x20);
    case 0:
        return 0;
    default:
        return !(LinkFlags & 1);
    }
}

void DBIAddAddrInfo( seg_leader *seg, unsigned long start )
{
    if( LinkFlags & 0x4000 ) ODBIAddAddrInfo( seg, start );
    else if( LinkFlags & 1 ) DwarfAddAddrInfo( seg, start );
    else if( LinkFlags & 0x20 ) CVAddAddrInfo( seg, start );
}

void DBIAddLocal( unsigned short info, unsigned long size )
{
    if( LinkFlags & 0x4000 ) ODBIAddLocal( info, size );
    else if( LinkFlags & 0x20 ) CVAddLocal( info, size );
}

void DBIAddGlobal( symbol *sym, unsigned len )
{
    if( LinkFlags & 0x4000 ) ODBIAddGlobal( sym, len );
    else if( LinkFlags & 1 ) DwarfAddGlobal( sym, len );
    else if( LinkFlags & 0x20 ) CVAddGlobal( sym, len );
    if( !(sym->info & 0x0100) && (LinkFlags & 0x0800) ) {
        NovDBIAddGlobal( len );
    }
}

void DBIAddLines( seg_leader *seg, unsigned char is32, unsigned char isdata )
{
    if( LinkFlags & 0x4000 ) ODBIAddLines( seg, is32, isdata );
    else if( LinkFlags & 1 ) DwarfAddLines( seg, is32, isdata );
    else if( LinkFlags & 0x20 ) CVAddLines( seg, is32, isdata );
}

void DBIP1ModuleScanned( void )
{
    if( LinkFlags & 0x4000 ) ODBIP1ModuleScanned();
    else if( LinkFlags & 1 ) DwarfP1ModuleScanned();
    else if( LinkFlags & 0x20 ) CVP1ModuleScanned();
}

char *DBIModName;                    /* original442248 */
unsigned DBIModNameLen;               /* original450358 */

void DBIGenModule( mod_entry *mod )
{
    if( !(LinkFlags & 0x4021) ) return;
    if( LinkFlags & 0x4000 ) ODBIGenModule( mod );
    else if( LinkFlags & 1 ) DwarfGenModule();
    else CVGenModule();
    _LnkFree( DBIModName );
    DBIModName = 0;
}

void DBIDefClass( class_entry *class, unsigned long size )
{
    if( LinkFlags & 0x4000 ) ODBIDefClass( class, size );
    else if( LinkFlags & 1 ) DwarfDefClass( class, size );
}

void DBIAddrStart( void )
{
    if( LinkFlags & 0x0800 ) NovDBIAddrStart();
    if( LinkFlags & 0x20 ) CVAddrStart();
    DBIAddrSectStart( Root );
}

void DBIAddrSectStart( section *sect )
{
    if( LinkFlags & 0x4000 ) ODBIAddrSectStart( sect );
    else if( LinkFlags & 1 ) DwarfAddrSectStart( sect );
}

unsigned long DBIAlloc( unsigned long size )
{
    if( size == 0 ) return 0;
    return AllocStg( size );
}

extern void ODBIAddModule( mod_entry *, section * );
extern void DwarfAddModule( mod_entry *, section * );
extern void CVAddModule( mod_entry *, section * );

void DBIAddModule( mod_entry *mod, section *sect )
{
    if( LinkFlags & 0x4000 ) ODBIAddModule( mod, sect );
    else if( LinkFlags & 1 ) DwarfAddModule( mod, sect );
    else if( LinkFlags & 0x20 ) CVAddModule( mod, sect );
}

extern void ODBIGenGlobal( symbol *, section * );
extern void DwarfGenGlobal( symbol *, section * );
extern void CVGenGlobal( symbol *, section * );
extern void NovDBIGenGlobal( symbol * );

void DBIGenGlobal( symbol *sym, section *sect )
{
    if( LinkFlags & 0x4000 ) ODBIGenGlobal( sym, sect );
    else if( LinkFlags & 1 ) DwarfGenGlobal( sym, sect );
    else if( LinkFlags & 0x20 ) CVGenGlobal( sym, sect );
    if( !(sym->info & 0x0100) && (LinkFlags & 0x0800) ) NovDBIGenGlobal( sym );
}

extern void ODBIP2Start( section * );

void DBIP2Start( section *sect )
{
    if( LinkFlags & 0x4000 ) ODBIP2Start( sect );
}

extern void ODBIP2Source( unsigned char *, int );

void DBIP2Source( unsigned char *buff, unsigned char *end )
{
    int len;

    buff += 2;
    len = end - buff;
    if( len <= 0 ) {
        BadObject();
        return;
    }
    ObjFormat |= 0x80;
    if( LinkFlags & 0x4000 ) ODBIP2Source( buff, len );
}

extern void ODBIGenAddrInfo( seg_leader *, unsigned long, unsigned long, section * );
extern void DwarfGenAddrInfo( seg_leader *, unsigned long, unsigned long );
extern void CVGenAddrInfo( seg_leader *, unsigned long, unsigned long );

void DBIGenAddrInfo( seg_leader *seg, unsigned long start,
                     unsigned long padding, section *sect )
{
    if( LinkFlags & 0x4000 ) ODBIGenAddrInfo( seg, start, padding, sect );
    else if( LinkFlags & 1 ) DwarfGenAddrInfo( seg, start, padding );
    else if( LinkFlags & 0x20 ) CVGenAddrInfo( seg, start, padding );
}

extern void ODBIGenLocal( seg_leader *, unsigned long );
extern void CVGenLocal( seg_leader *, unsigned long );
extern void ODBIP2Module( void );
extern void DwarfP2Module( void );
extern void CVP2Module( void );

void DBIGenLocal( seg_leader *seg, unsigned long size )
{
    if( LinkFlags & 0x4000 ) ODBIGenLocal( seg, size );
    else if( LinkFlags & 0x20 ) CVGenLocal( seg, size );
}

void DBIP2Module( void )
{
    if( LinkFlags & 0x4021 ) {
        if( LinkFlags & 0x4000 ) ODBIP2Module();
        else if( LinkFlags & 1 ) DwarfP2Module();
        else CVP2Module();
        _LnkFree( DBIModName );
        DBIModName = 0;
    }
}

unsigned char DBINoReloc( unsigned short info )
{
    return (info & 0x7000) != 0;
}

extern void ODBIGenLines( seg_leader *, seg_leader *, unsigned long, unsigned char );
extern void DwarfGenLines( seg_leader *, unsigned long, unsigned char );
extern void CVGenLines( seg_leader *, unsigned long, unsigned char );

void DBIGenLines( seg_leader *seg, seg_leader *seg2,
                   unsigned long adjust, unsigned char is32 )
{
    if( LinkFlags & 0x4000 ) ODBIGenLines( seg, seg2, adjust, is32 );
    else if( LinkFlags & 1 ) DwarfGenLines( seg, adjust, is32 );
    else if( LinkFlags & 0x20 ) CVGenLines( seg, adjust, is32 );
}

extern void ODBIFini( section * );
extern void CVFini( section * );

void DBIFini( section *sect )
{
    if( LinkFlags & 0x4000 ) ODBIFini( sect );
    else if( LinkFlags & 0x20 ) CVFini( sect );
}

extern void OWriteDBI( void );
extern void DwarfWriteDBI( void );
extern void CVWriteDBI( void );

void WriteDBI( void )
{
    int handle, saved;

    if( !(LinkFlags & 0x4021) ) return;
    if( SymFileName != 0 ) {
        handle = QOpenRW( SymFileName );
        saved = Root->outfile->handle;
        Root->outfile->handle = handle;
    }
    if( LinkFlags & 0x4000 ) OWriteDBI();
    else if( LinkFlags & 1 ) DwarfWriteDBI();
    else if( LinkFlags & 0x20 ) CVWriteDBI();
    if( SymFileName != 0 ) {
        QClose( handle, SymFileName );
        Root->outfile->handle = saved;
    }
}
