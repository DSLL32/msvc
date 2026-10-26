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

#ifdef __WATCOMC__
#pragma intrinsic(strlen)
#endif

void SetOS2Fmt( void )
{
    if( LinkState & 0x400 ) return;
    LinkState |= 0x400;
    FmtData.u.os2.flags = 2;
    FmtData.u.os2.heapsize = 0;
    FmtData.u.os2.res_module_name = 0;
    FmtData.u.os2.stub_file_name = 0;
    FmtData.u.os2.os2_seg_flags = 0;
    FmtData.u.os2.description = 0;
    FmtData.u.os2.exports = 0;
    FmtData.u.os2.old_lib_name = 0;
    FmtData.u.os2.segment_shift = 0;
    if( FmtData.type & 8 ) FmtData.def_seg_flags |= 0x40;
    Extension = 4;
    ChkBase();
}

void ChkBase( void )
{
    if( FmtData.base != 0xffffffffUL && (FmtData.base & 0xffffUL) )
        LnkMsg( 0x7a7b, 0 );
}

static parse_entry SubFormats[] = {
    { "DLl", ProcOS2DLL, 0x001f },
    { "FLat", ProcLE, 0x0004 },
    { "LE", ProcLE, 0x0002 },
    { "LX", ProcLE, 0x0004 },
    { "NT", ProcPE, 0x0010 },
    { "PE", ProcPE, 0x0010 },
    { "OLDPE", ProcOldPE, 0x0010 },
    { 0, 0, 0 }
};
static parse_entry OS2FormatKeywords[] = {
    { "PM", ProcPM, 0x0007 },
    { "PMCompatible", ProcPMCompatible, 0x0007 },
    { "FULLscreen", ProcFullScreen, 0x0007 },
    { "PHYSdevice", ProcPhysDevice, 0x0006 },
    { "VIRTdevice", ProcVirtDevice, 0x0006 },
    { 0, 0, 0 }
};
static parse_entry WindowsFormatKeywords[] = {
    { "MEMory", ProcMemory, 0x0008 },
    { "FOnt", ProcFont, 0x0008 },
    { 0, 0, 0 }
};

unsigned char ProcOS2( void )
{
    Extension = 4;
    while( ProcOne( SubFormats, SEP_NO, 0 ) ) {}
    if( FmtData.type & 8 ) {
        if( ProcOne( WindowsFormatKeywords, SEP_NO, 0 ) )
            ProcOne( WindowsFormatKeywords, SEP_NO, 0 );
    } else {
        ProcOne( OS2FormatKeywords, SEP_NO, 0 );
        if( (FmtData.type & 4) && (FmtData.flags & 1) )
            LinkFlags |= 0x08000000UL;
    }
    return 1;
}

static parse_entry NTFormatKeywords[] = {
    { "TNT", ProcTNT, 0x10 },
    { 0, 0, 0 }
};

unsigned char ProcPE( void )
{
    ProcOne( NTFormatKeywords, SEP_NO, 0 );
    FmtData.u.pe.os2.heapsize = 8192;
    FmtData.u.pe.stackcommit = 0xffffffffUL;
    FmtData.u.pe.objalign = 0x10000UL;
    FmtData.u.pe.heapcommit = 0x1000UL;
    return 1;
}

unsigned char ProcRunConsole( void )
{
    FmtData.u.pe.subsystem = 3;
    return 1;
}

unsigned char ProcOS2Export( void )
{
    if( GetToken( SEP_EQUALS, TOK_INCLUDE_DOT ) ) return GetWlibImports();
    return ProcArgList( getexport, 0 );
}

static parse_entry Exp_Keywords[] = {
    { "Resident", ProcExpResident, 0x000f },
    { 0, 0, 0 }
};

unsigned char getexport( void )
{
    entry_export *exp;
    unsigned short value;

    /* Unlike the released version, 1994 AllocExport owns a copied name. */
    exp = AllocExport( tostring() );
    if( GetToken( SEP_PERIOD, TOK_INCLUDE_DOT ) ) {
        if( getatoi( &exp->ordinal ) != 0 ) {
            LnkMsg( 0x7b29, 0 );
            _LnkFree( exp->name );
            _LnkFree( exp );
            GetToken( SEP_EQUALS, TOK_INCLUDE_DOT );
            return 1;
        }
    }
    if( GetToken( SEP_EQUALS, TOK_INCLUDE_DOT ) )
        exp->sym = SymXOp( 0x12, Token.this, Token.len );
    else exp->sym = RefISymbol( exp->name );
    exp->sym->info |= 0x4000;
    if( exp->ordinal == 0 ) exp->flags |= 1;
    exp->next = FmtData.u.os2.exports;
    FmtData.u.os2.exports = exp;
    ProcOne( Exp_Keywords, SEP_NO, 0 );
    FmtData.u.os2.exports = exp->next;
    exp->iopl_words = 0;
    if( !(FmtData.type & 0x18) && GetToken( SEP_NO, TOK_INCLUDE_DOT ) ) {
        if( getatoi( &value ) == 0 ) {
            if( value > 63 ) LnkMsg( 0x7b2a, 0 );
            else exp->iopl_words = value;
        } else Token.thumb = 1;
    }
    AddToExportList( exp );
    return 1;
}

entry_export *AllocExport( char *name )
{
    entry_export *exp;

    exp = ChkLAlloc( sizeof(*exp) );
    /* The original bit-field stores leave unused flag bits uninitialized.
     * Give those undefined bits zero; the four reached flags retain their
     * original exported/resident/movable/anonymous values. */
    exp->flags = 2;
    if( CmdFlags & 0x4000 ) exp->flags |= 8;
    exp->iopl_words = 0;
    exp->ordinal = 0;
    exp->name = name;
    return exp;
}

extern void FreeImpNameTab( void );
extern void FreeExportList( void );
extern void FreeSegFlags( seg_flags * );

void FreeOS2Fmt( void )
{
    _LnkFree( FmtData.u.os2.stub_file_name );
    _LnkFree( FmtData.u.os2.res_module_name );
    _LnkFree( FmtData.u.os2.old_lib_name );
    _LnkFree( FmtData.u.os2.description );
    FreeImpNameTab();
    FreeExportList();
    FreeSegFlags( FmtData.u.os2.os2_seg_flags );
}

extern unsigned char ProcInitGlobal( void );
extern unsigned char ProcInitInstance( void );
extern unsigned char ProcTermGlobal( void );
extern unsigned char ProcTermInstance( void );

/* The old thread options share the exact existing Memory/Font setter
 * entries; they are aliases, not extra calls or newly separate functions. */
static parse_entry Init_Keywords[] = {
    { "INITGlobal", ProcInitGlobal, 0x001f },
    { "INITInstance", ProcInitInstance, 0x001f },
    { "INITThread", ProcMemory, 0x0010 },
    { 0, 0, 0 }
};
static parse_entry Term_Keywords[] = {
    { "TERMGlobal", ProcTermGlobal, 0x0016 },
    { "TERMInstance", ProcTermInstance, 0x0016 },
    { "TERMThread", ProcFont, 0x0010 },
    { 0, 0, 0 }
};

unsigned char ProcOS2DLL( void )
{
    FmtData.flags |= 1;
    Extension = 6;
    FmtData.u.os2.flags = (FmtData.u.os2.flags & ~3) | 1;
    if( FmtData.type & 8 ) FmtData.def_seg_flags |= 0x20;
    if( ProcOne( Init_Keywords, SEP_NO, 0 ) ) {
        if( !ProcOne( Term_Keywords, SEP_NO, 0 ) ) {
            if( FmtData.u.os2.flags & 4 ) FmtData.u.os2.flags |= 0x8000;
        }
    }
    return 1;
}

unsigned char ProcDescription( void )
{
    if( !GetToken( SEP_NO, TOK_INCLUDE_DOT ) ) return 0;
    FmtData.u.os2.description = tostring();
    return 1;
}

unsigned char ProcObjAlign( void )
{
    unsigned long value;

    if( !HaveEquals( 0 ) ) return 0;
    if( getatol( &value ) != 0 || value == 0 ) return 0;
    if( value < 512 || value > 0x10000000UL || (value & (value - 1)) ) {
        LnkMsg( 0x7a3c, "s", "objalign" );
        value = 65536;
    }
    FmtData.u.pe.objalign = value;
    return 1;
}

extern unsigned char AddCommit( void );
unsigned char ProcCommit( void )
{
    return ProcArgList( AddCommit, TOK_INCLUDE_DOT );
}

extern unsigned char ProcCommitStack( void );
extern unsigned char ProcCommitHeap( void );
static parse_entry CommitKeywords[] = {
    { "STack", ProcCommitStack, 0x0010 },
    { "Heap", ProcCommitHeap, 0x0010 },
    { 0, 0, 0 }
};

unsigned char AddCommit( void )
{
    Token.thumb = REJECT;
    return ProcOne( CommitKeywords, SEP_NO, 0 ) != 0;
}

unsigned char ProcCommitStack( void )
{
    return GetLong( &FmtData.u.pe.stackcommit );
}

unsigned char ProcCommitHeap( void )
{
    return GetLong( &FmtData.u.pe.heapcommit );
}

unsigned char ProcOS2HeapSize( void )
{
    unsigned long value;

    if( !HaveEquals( 0 ) ) return 0;
    if( getatol( &value ) != 0 || value == 0 ) {
        LnkMsg( 0x7a3c, "s", "heapsize" );
    } else {
        FmtData.u.os2.heapsize = value;
    }
    return 1;
}

extern unsigned char getimport( void );
unsigned char ProcOS2Import( void )
{
    return ProcArgList( getimport, 0 );
}

unsigned char getimport( void )
{
    char *intname, *modname, *extname;
    unsigned char state;
    unsigned short ordinal;
    symbol *sym;
    dll_sym_info *dll;

    intname = tostring();
    if( !GetToken( SEP_NO, 0 ) ) {
        _LnkFree( intname );
        return 0;
    }
    modname = tostring();
    state = 2;
    if( GetToken( SEP_PERIOD, TOK_INCLUDE_DOT ) ) {
        state = getatoi( &ordinal );
        if( state == 1 ) {
            extname = tostring();
        } else if( state == 2 ) {
            LnkMsg( 0x7b28, 0 );
            _LnkFree( intname );
            _LnkFree( modname );
            return 1;
        }
    }
    sym = SymOp( 0x26, intname, strlen( intname ) );
    if( sym != 0 && sym->u.import == 0 ) {
        sym->info = (sym->info & 0xfff0) | 0x4006;
        dll = ChkLAlloc( sizeof(dll_sym_info) );
        sym->u.import = dll;
        dll->modnum = AddImpName( modname, strlen( modname ), 1 );
        if( state == 0 ) {
            dll->isordinal = 1;
            dll->entry = ordinal;
        } else {
            dll->isordinal = 0;
            if( state == 1 ) {
                dll->entry = AddImpName( extname, strlen( extname ), 0 );
                _LnkFree( extname );
            } else {
                dll->entry = AddImpName( intname, strlen( intname ), 0 );
            }
        }
    }
    _LnkFree( intname );
    _LnkFree( modname );
    return 1;
}

unsigned char ProcLE( void )
{
    return 1;
}

unsigned char ProcInitInstance( void )
{
    FmtData.u.os2.flags |= 4;
    return 1;
}

unsigned char ProcTermGlobal( void )
{
    FmtData.u.os2.flags &= ~0x8000;
    return 1;
}

unsigned char ProcExpResident( void )
{
    FmtData.u.os2.exports->flags |= 1;
    return 1;
}

extern unsigned char getsegflags( void );
unsigned char ProcOS2Segment( void )
{
    return ProcArgList( getsegflags, TOK_INCLUDE_DOT );
}

extern unsigned char ProcOS2Class( void );
extern unsigned char ProcSegType( void );
extern unsigned char ProcSegCode( void );
extern unsigned char ProcSegData( void );
extern unsigned char ProcPreload( void );
extern unsigned char ProcLoadoncall( void );
extern unsigned char ProcIopl( void );
extern unsigned char ProcNoIopl( void );
extern unsigned char ProcExecuteonly( void );
extern unsigned char ProcExecuteread( void );
extern unsigned char ProcReadOnly( void );
extern unsigned char ProcReadWrite( void );
extern unsigned char ProcShared( void );
extern unsigned char ProcNonShared( void );
extern unsigned char ProcConforming( void );
extern unsigned char ProcNonConforming( void );
extern unsigned char ProcMovable( void );
extern unsigned char ProcFixed( void );
extern unsigned char ProcNonPageable( void );
extern unsigned char ProcPageable( void );
extern unsigned char ProcDiscardable( void );
extern unsigned char ProcOS2Dynamic( void );
extern unsigned char ProcInvalid( void );
extern unsigned char ProcPermanent( void );
extern unsigned char ProcNonPermanent( void );
extern unsigned char ProcContiguous( void );

#pragma pack(1)
typedef struct os2_seg_flags {
    seg_flags entry;
    unsigned short specified;
} os2_seg_flags;
#pragma pack()
typedef char old_os2_seg_flags_13[(sizeof(os2_seg_flags) == 13) ? 1 : -1];

static parse_entry SegDesc[] = {
    { "Class", ProcOS2Class, 0x000f },
    { "TYpe", ProcSegType, 0x000f },
    { 0, 0, 0 }
};
static parse_entry SegTypeDesc[] = {
    { "CODE", ProcSegCode, 0x000f },
    { "DATA", ProcSegData, 0x000f },
    { 0, 0, 0 }
};
static parse_entry SegModel[] = {
    { "PReload", ProcPreload, 0x000f },
    { "LOadoncall", ProcLoadoncall, 0x000f },
    { "Iopl", ProcIopl, 0x0007 },
    { "NOIopl", ProcNoIopl, 0x0007 },
    { "EXECUTEOnly", ProcExecuteonly, 0x000f },
    { "EXECUTERead", ProcExecuteread, 0x000f },
    { "SHared", ProcShared, 0x001f },
    { "NONShared", ProcNonShared, 0x001f },
    { "READOnly", ProcReadOnly, 0x000f },
    { "READWrite", ProcReadWrite, 0x000f },
    { "CONforming", ProcConforming, 0x0007 },
    { "NONCONforming", ProcNonConforming, 0x0007 },
    { "MOVeable", ProcMovable, 0x0008 },
    { "FIXed", ProcFixed, 0x0008 },
    { "DIScardable", ProcDiscardable, 0x0008 },
    { "INValid", ProcInvalid, 0x0006 },
    /* The original table shares this handler with export RESident. */
    { "RESident", ProcExpResident, 0x0006 },
    { "CONTiguous", ProcContiguous, 0x0006 },
    { "DYNamic", ProcOS2Dynamic, 0x0006 },
    { "NONPERManent", ProcNonPermanent, 0x0006 },
    { "PERManent", ProcPermanent, 0x0006 },
    { "PAGEable", ProcPageable, 0x0010 },
    { "NONPageable", ProcNonPageable, 0x0010 },
    { 0, 0, 0 }
};
unsigned char getsegflags( void )
{
    os2_seg_flags *flags;

    Token.thumb = REJECT;
    flags = ChkLAlloc( sizeof(*flags) );
    flags->specified = 0;
    flags->entry.name = 0;
    flags->entry.flags = FmtData.def_seg_flags;
    flags->entry.type = 0;
    flags->entry.next = FmtData.u.os2.os2_seg_flags;
    FmtData.u.os2.os2_seg_flags = flags;
    ProcOne( SegDesc, SEP_NO, 0 );
    if( flags->entry.type != 2 && flags->entry.type != 3 ) {
        if( !GetToken( SEP_NO, TOK_INCLUDE_DOT ) ) {
            FmtData.u.os2.os2_seg_flags = flags->entry.next;
            _LnkFree( flags );
            return 0;
        }
        flags->entry.name = tostring();
    }
    while( ProcOne( SegModel, SEP_NO, 0 ) ) {}
    return 1;
}

unsigned char ProcOS2Class( void )
{
    ((os2_seg_flags *)FmtData.u.os2.os2_seg_flags)->entry.type = 1;
    return 1;
}

unsigned char ProcPreload( void )
{
    os2_seg_flags *flags;

    flags = FmtData.u.os2.os2_seg_flags;
    if( flags->specified & 0x40 ) LnkMsg( 0x7a2d, 0 );
    flags->entry.flags |= 0x40;
    flags->specified |= 0x40;
    return 1;
}

unsigned char ProcMovable( void )
{
    os2_seg_flags *flags;

    flags = FmtData.u.os2.os2_seg_flags;
    if( flags->specified & 0x10 ) LnkMsg( 0x7a2d, 0 );
    flags->entry.flags |= 0x10;
    flags->specified |= 0x10;
    return 1;
}

unsigned char ProcDiscardable( void )
{
    ((os2_seg_flags *)FmtData.u.os2.os2_seg_flags)->entry.flags |= 0x1000;
    /* The old code falls through to the separately named common true tail. */
    return ProcOS2Dynamic();
}

unsigned char ProcOS2Dynamic( void )
{
    return 1;
}
