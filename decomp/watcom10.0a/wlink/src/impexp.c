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
#include <ctype.h>
#include "cmdline.h"
#include "peimports.h"
#include <recon.h>

#ifdef __WATCOMC__
#pragma intrinsic(strlen,memcmp)
#endif

void AddToExportList( entry_export *exp )
{
    entry_export **owner, **place;
    entry_export *curr;
    unsigned len, currlen;

    place = 0;
    owner = &FmtData.u.os2.exports;
    len = strlen( exp->name );
    for( ;; ) {
        curr = *owner;
        if( curr == 0 ) break;
        currlen = strlen( curr->name );
        if( currlen == len && CmpSym( curr->name, exp->name, len ) == 0 ) {
            LnkMsg( 0x7279, "s", curr->name );
            FreeAnExport( exp );
            return;
        }
        if( place == 0 ) {
            if( exp->ordinal == 0 ) {
                if( curr->ordinal != 0 ) place = owner;
            } else if( curr->ordinal == exp->ordinal ) {
                LnkMsg( 0x322b, 0 );
                exp->ordinal = 0;
                place = &FmtData.u.os2.exports;
            } else if( curr->ordinal > exp->ordinal ) place = owner;
        }
        owner = &curr->next;
    }
    /* This version predates the later symbol e.export assignment. */
    if( place == 0 ) place = owner;
    exp->next = *place;
    *place = exp;
}

void MSImportKeyword( void )
{
    unsigned char isordinal;
    unsigned len;
    char *name;
    symbol *sym;
    dll_sym_info *dll;

    isordinal = *ObjBuff++;
    len = *ObjBuff++;
    name = (char *)ObjBuff;
    ObjBuff += len;
    sym = SymXOp( 0x26, name, len );
    if( sym == 0 ) return;
    sym->info = (sym->info & 0xfff0) | 6;
    dll = ChkLAlloc( sizeof( dll_sym_info ) );
    sym->u.import = dll;
    dll->modnum = ReadImpName( 1 );
    dll->isordinal = isordinal;
    if( !isordinal ) {
        if( *ObjBuff == 0 ) dll->entry = AddImpName( name, len, 0 );
        else dll->entry = ReadImpName( 0 );
    } else {
        dll->entry = *(unsigned short *)ObjBuff;
    }
}

unsigned short ReadImpName( unsigned char module )
{
    unsigned len;
    char *name;

    len = *ObjBuff++;
    name = (char *)ObjBuff;
    ObjBuff += len;
    return AddImpName( name, len, module );
}

static snamelist *ImportNames;  /* original4502d8 */
static snamelist *ModuleNames;  /* original4502dc */

unsigned short AddImpName( char *name, unsigned len, unsigned char module )
{
    return AddNameTable( name, len, module,
        module ? &ModuleNames : &ImportNames );
}

extern void ReadOldLib( void );

void AssignOrdinals( void )
{
    entry_export *exp, *prev, *place;
    unsigned char isspace;

    if( FmtData.u.os2.exports != 0 ) {
        if( FmtData.u.os2.old_lib_name != 0 ) ReadOldLib();
        exp = prev = FmtData.u.os2.exports;
        place = prev->next;
        while( exp->ordinal == 0 ) {
            for( ;; ) {
                isspace = 0;
                if( place != 0 ) isspace = place->ordinal - prev->ordinal > 1;
                if( place == 0 || isspace ) {
                    if( FmtData.u.os2.exports != prev ) {
                        FmtData.u.os2.exports = exp->next;
                        prev->next = exp;
                        exp->next = place;
                    }
                    exp->ordinal = prev->ordinal + 1;
                    prev = exp;
                    break;
                }
                prev = place;
                place = place->next;
            }
            exp = FmtData.u.os2.exports;
        }
    }
}

module_import *PEImpList;               /* original4502ec */
unsigned NumMods, NumImports;          /* original4502f0,4502e8 */

void RegisterImport( dll_sym_info *sym )
{
    module_import *mod;
    import_name *imp, *chk, **owner;
    snamelist *modname, *os2_imp;
    int index, cmp;
    unsigned off, len;

    modname = ModuleNames;
    for( index = (int)sym->modnum - 1; index > 0; --index ) {
        if( modname == 0 ) return;
        modname = modname->next;
    }
    for( mod = PEImpList; ; mod = mod->next ) {
        if( mod == 0 ) {
            ++NumMods;
            mod = PermAlloc( sizeof(module_import) );
            mod->next = PEImpList;
            PEImpList = mod;
            mod->mod = modname;
            mod->imports = 0;
            mod->num_entries = 0;
            break;
        }
        if( mod->mod == modname ) break;
    }
    os2_imp = 0;
    if( !sym->isordinal ) {
        os2_imp = ImportNames;
        off = 1;
        while( off != sym->entry ) {
            if( os2_imp == 0 ) return;
            off += os2_imp->len + 1;
            os2_imp = os2_imp->next;
        }
    }
    ++mod->num_entries;
    imp = PermAlloc( sizeof(import_name) );
    imp->dll = sym;
    imp->imp = os2_imp;
    owner = &mod->imports;
    if( os2_imp != 0 ) {
        for( ;; ) {
            chk = *owner;
            if( chk == 0 ) break;
            if( chk->imp != 0 ) {
                len = chk->imp->len;
                if( len > os2_imp->len ) len = os2_imp->len;
                cmp = memcmp( chk->imp->name, os2_imp->name, len );
                if( cmp > 0 ) break;
                /* Preserve the old equal-prefix comparison as emitted. */
                if( cmp == 0 && len > chk->imp->len ) break;
            }
            owner = &chk->next;
        }
    } else chk = *owner;
    imp->next = chk;
    *owner = imp;
    ++NumImports;
}

unsigned long CalcImpOff( dll_sym_info *dll )
{
    module_import *mod;
    import_name *imp;
    unsigned long off;

    off = (NumMods + 1) * 20;
    for( mod = PEImpList; mod != 0; mod = mod->next ) {
        off += (mod->num_entries + 1) * 4;
        for( imp = mod->imports; imp != 0; imp = imp->next ) {
            if( imp->dll == dll ) return off;
            off += 4;
        }
        off += 4;
    }
    return 0;
}

extern snamelist *IndexSName( snamelist *, unsigned );

char *ImpModuleName( dll_sym_info *dll )
{
    return IndexSName( ModuleNames, dll->modnum )->name;
}

void FreeImpNameTab( void )
{
    FreeList( (path_entry *)ModuleNames );
    FreeList( (path_entry *)ImportNames );
    ModuleNames = 0;
    ImportNames = 0;
}

void FreeExportList( void )
{
    entry_export *exp;

    exp = FmtData.u.os2.exports;
    while( exp != 0 ) exp = FreeAnExport( exp );
}

entry_export *FreeAnExport( entry_export *exp )
{
    entry_export *next;

    _LnkFree( exp->name );
    next = exp->next;
    _LnkFree( exp );
    return next;
}

extern unsigned long WriteTabList( snamelist *, unsigned long *, unsigned char );
unsigned long ImportModTable( unsigned long *count )
{
    return WriteTabList( ModuleNames, count, 0 );
}

extern void WriteLoad( void *, unsigned );
unsigned long WriteTabList( snamelist *list, unsigned long *count, unsigned char upper )
{
    snamelist *node;
    unsigned long off, i;
    int j;

    off = i = 0;
    for( node = list; node != NULL; node = node->next ) {
        WriteLoad( &node->len, 1 );
        ++i;
        if( upper ) {
            for( j = (int)node->len - 1; j >= 0; --j ) {
                node->name[j] = toupper( (unsigned char)node->name[j] );
            }
        }
        WriteLoad( node->name, node->len );
        off += node->len + 1;
    }
    *count = i;
    return off;
}

unsigned long ImportProcTable( unsigned long *count )
{
    return WriteTabList( ImportNames, count, !(LinkFlags & 0x2000) );
}

unsigned long ModRefTable( void )
{
    snamelist *node;
    unsigned long count, off;

    off = 0;
    if( FmtData.type & 9 ) {
        off = 1;
        for( node = ImportNames; node != NULL; node = node->next ) {
            off += node->len + 1;
        }
    }
    count = 0;
    for( node = ModuleNames; node != NULL; node = node->next ) {
        WriteLoad( &off, 2 );
        off += node->len + 1;
        ++count;
    }
    return count;
}
