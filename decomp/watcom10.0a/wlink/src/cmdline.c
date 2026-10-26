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

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "cmdline.h"
#include "strtab.h"
#include <recon.h>

/* The old parser table has no later command-flags member. */
static parse_entry FormatHelp[] = {
    { "Dos", &ProcDosHelp, 0xffff },
    { "OS2", &ProcOS2Help, 0xffff },
    { "WINdows", &ProcWindowsHelp, 0xffff },
    { "NT", &ProcNTHelp, 0xffff },
    { "PHARlap", &ProcPharHelp, 0xffff },
    { "NOVell", &ProcNovellHelp, 0xffff },
    { "QNX", &ProcQNXHelp, 0xffff },
    { "ELF", &ProcELFHelp, 0xffff },
    { 0, 0, 0 }
};
#if defined(__WATCOMC__)
#pragma intrinsic(strlen,memcpy,memcmp)
#endif

fmt_data FmtData;                 /* original450068,80B */
tok Token;                       /* original450040,20B */
char *Name;                      /* original450058 */
unsigned short CmdFlags;         /* original45005c */
unsigned char Extension;         /* original45005e */
unsigned char DBIFlag;           /* original450188 */
unsigned char MapFlags;           /* original450189 */
char *SymFileName;               /* original44127c */
section *CurrSect;               /* original4500d4 */
file_list **CurrFList;                 /* original450054 */
path_entry *Path;                /* original4500ec */

void ResetCmdFile( void )
{
    Extension = 4;                 /* E_LOAD */
    Name = 0;
    Path = 0;
    CmdFlags = 0x10;               /* CF_UNNAMED */
    memset( &FmtData, 0, sizeof(FmtData) );
    FmtData.base = 0xffffffffUL;
    FmtData.type = 0xffff;
    FmtData.def_seg_flags = 0x0c00;
    CurrSect = Root;
    CurrFList = &Root->files;
    DBIFlag = 0;
}

void Help( void )
{
    EatWhite();
    if( *Token.next == '?' ) {
        Crash( 0 );
    } else if( *Token.next == '\0' || !ProcOne( FormatHelp, 0, 0 ) ) {
        Crash( 1 );
    } else {
        Ignite();
        Suicide();
    }
}

void Crash( unsigned char check_file )
{
    char buff[80];
    unsigned len;
    int file;

    if( check_file ) {
        file = SearchPath( "wlink.hlp" );
        if( file != -1 ) {
            WLPrtBanner();
            for( ;; ) {
                len = QRead( file, buff, sizeof(buff), "wlink.hlp" );
                if( len == 0 ) break;
                QWrite( fileno(stdout), buff, len, 0 );
            }
            QClose( file, "wlink.hlp" );
            Ignite();
            Suicide();
        }
    }
    DisplayOptions();
    Ignite();
    Suicide();
}

void DisplayOptions( void )
{
    unsigned char isout;

    isout = (CmdFlags & 1) != 0;
    WriteGenHelp();
    WriteHelp( 0x174, 0x183, isout ); /* DOS */
    WriteHelp( 0x184, 0x1a3, isout ); /* OS/2 */
    WriteHelp( 0x1a4, 0x1c3, isout ); /* Windows */
    WriteHelp( 0x1c4, 0x1e3, isout ); /* NT */
    WriteHelp( 0x1e4, 0x1f3, isout ); /* Phar Lap */
    WriteHelp( 0x1f4, 0x213, isout ); /* NetWare */
    WriteHelp( 0x224, 0x233, isout ); /* QNX */
    WriteHelp( 0x234, 0x243, isout ); /* ELF */
}

void WriteGenHelp( void )
{
    WLPrtBanner();
    WriteHelp( 0x154, 0x173, 0 );
}

void WriteHelp( unsigned first, unsigned last, unsigned char prompt )
{
    char msg_buffer[128];
    int previous_null;

    previous_null = 0;
    if( prompt ) PressKey();
    for( ; first <= last; ++first ) {
        Msg_Get( first, msg_buffer );
        if( previous_null ) {
            if( msg_buffer[0] != '\0' ) {
                PressKey();
                WriteMsg( msg_buffer );
                previous_null = 0;
            } else break;
        } else if( msg_buffer[0] == '\0' ) {
            previous_null = 1;
        } else {
            WriteMsg( msg_buffer );
        }
    }
}

void WriteMsg( char *text )
{
    QWrite( fileno(stdout), text, strlen(text), 0 );
    QWriteNL( fileno(stdout), 0 );
}

void PressKey( void )
{
    char msg_buffer[128];
    unsigned char result;

    Msg_Get( 0xfa, msg_buffer );
    QWrite( fileno(stdout), msg_buffer, strlen(msg_buffer), 0 );
    result = WaitForKey();
    QWriteNL( fileno(stdout), 0 );
    if( tolower(result) == 'q' ) {
        Ignite();
        Suicide();
    }
}

void DoCmdFile( char *fname )
{
    unsigned short possible;
    int file;
    unsigned namelen;

    ResetCmdFile();
    if( fname == 0 ) {
        fname = ChkLAlloc( 10 * 1024 );
        GetCmdLine( fname );
    }
    NewCommandSource( 0, fname, 1 ); /* COMMANDLINE in this1994 build */
    if( QIsADevice( fileno( stdout ) ) ) CmdFlags |= 1;
    while( *fname == ' ' ) ++fname;
    if( QSysHelp( &Token.next ) ) Help();
    if( *fname == '?' ) {
        Token.next = fname + 1;
        Help();
    } else if( (*fname == '-' || *fname == '/') && fname[1] == '?' ) {
        Token.next = fname + 2;
        Help();
    }
    if( *fname == 0 ) {
        Token.how = 2;            /* INTERACTIVE */
        Token.where = 1;          /* ENDOFLINE */
        LnkMsg( 0x1055, 0 );
    }
    file = SearchPath( "wlink.lnk" );
    if( file != -1 ) {
        fname = ChkLAlloc( sizeof("wlink.lnk") );
        memcpy( fname, "wlink.lnk", sizeof("wlink.lnk") );
        SetCommandFile( file, fname );
    }
    if( Spawn( DoCmdParse ) ) {
        Ignite();
        Suicide();
    }
    if( !(LinkState & 0x40) ) {
        HintFormat( 0x18 );
    } else {
        for( possible = 1; possible != 0; possible <<= 1 ) {
            if( FmtData.type & possible ) {
                FmtData.type = possible;
                break;
            }
        }
    }
    if( FmtData.type & 0x100 ) {
        CmdNovFini();
    } else if( FmtData.type & 0x80 ) {
        CmdOvlFini();
        AddObjLib( "wovl.lib", 0 );
    }
    if( Name == 0 || !(CmdFlags & 0x400) ) {
        Ignite();
        LnkMsg( 0x347a, 0 );
    }
    namelen = strlen( Name );
    if( MapFlags & 1 ) {
        if( MapFName == 0 ) MapFName = FileName( Name, namelen, 1, 1 );
    } else {
        MapFlags = 0;
    }
    if( SymFileName == 0 && ((CmdFlags & 0x80) ||
                      ((LinkFlags & 0x4000) && (FmtData.type & 0x40))) ) {
        SymFileName = FileName( Name, namelen, 15, 1 );
    }
    CheckTraces();
    BurnUtils();
    PruneSystemList();
    NumberSections();
    DBIInit();
}

/* In 10.0a the local cleanup remains in Ignite; the later source splits
 * it into a separate Burn function for workframe options. */
void Ignite( void )
{
    outfilelist *fnode;

    BurnSystemList();
    FreePaths();
    if( MapFName != 0 ) {
        _LnkFree( MapFName );
        MapFName = 0;
    }
    fnode = OutFiles;
    while( fnode != 0 ) {
        _LnkFree( fnode->fname );
        OutFiles = fnode->next;
        _LnkFree( fnode );
        fnode = OutFiles;
    }
    BurnUtils();
}

static sysblock *SysBlocks;       /* original441288 */

void CleanSystemList( unsigned char check )
{
    sysblock **sys;
    sysblock *next;
    char *name;

    sys = &SysBlocks;
    while( *sys != 0 ) {
        name = (*sys)->name;
        if( !check || (memcmp( "286", name, 4 ) != 0 &&
                       memcmp( "386", name, 4 ) != 0) ) {
            next = (*sys)->next;
            _LnkFree( name );
            _LnkFree( *sys );
            *sys = next;
        } else sys = &(*sys)->next;
    }
}

void BurnSystemList( void )
{
    CleanSystemList( 0 );
}

void PruneSystemList( void )
{
    CleanSystemList( 1 );
}

void FreePaths( void )
{
    FreeList( Path );
    Path = 0;
    if( Name != 0 ) {
        _LnkFree( Name );
        Name = 0;
    }
}

#pragma pack(1)
static struct select_format {
    unsigned short bits;
    char *lib_var_name;
    void (*set_func)( void );
    void (*free_func)( void );
} PossibleFmt[] = {
    { 0x00e0, "LIBDOS", 0, 0 },
    { 0x4200, "LIBQNX", SetQNXFmt, FreeQNXFmt },
    { 0x8000, "LIBELF", SetELFFmt, FreeELFFmt },
    { 0x0008, "LIBWIN", SetOS2Fmt, FreeOS2Fmt },
    { 0x0001, "LIBOS2", SetOS2Fmt, FreeOS2Fmt },
    { 0x0002, "LIBOS2FLAT", SetOS2Fmt, FreeOS2Fmt },
    { 0x0004, "LIBOS2FLAT", SetOS2Fmt, FreeOS2Fmt },
    { 0x0010, "LIBPE", SetOS2Fmt, FreeOS2Fmt },
    { 0x3c00, "LIBPHAR", SetPharFmt, FreePharFmt },
    { 0x0100, "LIBNOV", SetNovFmt, FreeNovFmt },
    { 0, 0, 0, 0 }
};
#pragma pack()

void FreeFormatStuff( void )
{
    struct select_format *check;
    unsigned short possible;

    if( !(LinkState & 0x40) ) return;
    check = PossibleFmt;
    for( ;; ) {
        possible = check->bits;
        if( possible == 0 ) return;
        if( (~possible & FmtData.type) == 0 ) break;
        ++check;
    }
    if( check->free_func != 0 ) check->free_func();
}

unsigned char HintFormat( unsigned short hint )
{
    struct select_format *check;
    unsigned short possible;

    if( !(hint & FmtData.type) ) return 0;
    FmtData.type &= hint;
    if( LinkState & 0x40 ) return 1;
    check = PossibleFmt;
    for( ;; ) {
        possible = check->bits;
        if( possible == 0 ) {
            if( (~0x1fU & FmtData.type) == 0 ) InitFmt( SetOS2Fmt );
            return 1;
        }
        if( (~possible & FmtData.type) == 0 ) break;
        ++check;
    }
    InitFmt( check->set_func );
    LinkState |= 0x40;
    if( LinkState & 2 ) AddFmtLibPaths();
    return 1;
}

void InitFmt( void (*set)(void) )
{
    if( !(LinkState & 0x400) ) {
        if( set != 0 ) set();
        LinkState |= 0x400;
    }
}

unsigned char ProcExport( void )
{
    if( HintFormat( 0x001f ) ) return ProcOS2Export();
    return ProcNovExport();
}

void AddCommentLib( char *name, unsigned len, unsigned char priority )
{
    file_list *list;
    char *ptr;

    if( CmdFlags & 4 ) return;
    ptr = FileName( name, len, 2, 0 );
    list = AddObjLib( ptr, priority );
    CheckLibTrace( list );
    _LnkFree( ptr );
}

void AddFmtLibPaths( void )
{
    struct select_format *check;
    unsigned short possible;

    if( !(LinkState & 0x40) ) return;
    check = PossibleFmt;
    for( ;; ) {
        possible = check->bits;
        if( possible == 0 ) return;
        if( (~possible & FmtData.type) == 0 ) break;
        ++check;
    }
    AddEnvPaths( check->lib_var_name );
}

void DecideFormat( void )
{
    unsigned short possible;
    char rc_buff[128];

    if( !(LinkState & 0x40) ) {
        possible = FmtData.type;
        if( possible & 0x10 ) possible = 0x10;
        HintFormat( possible );
        if( !(LinkState & 0x40) ) {
            Msg_Get( 0xf0, rc_buff );
            LnkMsg( 0x3402, "s", rc_buff );
        }
    }
}

void SetFormat( void )
{
    char *fname;

    fname = FileName( Name, strlen( Name ), Extension, CmdFlags & 0x10 );
    Root->outfile = NewOutFile( fname );
    _LnkFree( Name );
    Name = 0;
    FillOutFilePtrs();
    if( MapFlags & 1 ) {
        LnkMsg( 0x2023, "s", Root->outfile->fname );
        LnkMsg( 0x2006, 0 );
    }
    if( FmtData.type & 0x4200 ) CmdQNXFini();
    SetRelocSize();
}

extern unsigned char ProcOS2Import( void );
extern unsigned char ProcELFImport( void );
extern unsigned char ProcNovImport( void );
unsigned char ProcImport( void )
{
    if( HintFormat( 0x001f ) ) return ProcOS2Import();
    if( HintFormat( 0x8000 ) ) return ProcELFImport();
    return ProcNovImport();
}

static parse_entry SysBeginOptions[] = {
    { "Begin", ProcSysBegin, 0xffff },
    { 0, 0, 0 }
};
unsigned char ProcSystem( void )
{
    char *name;
    sysblock *sys;

    if( ProcOne( SysBeginOptions, SEP_NO, 0 ) ) return 1;
    Token.thumb = OK;
    name = tostring();
    sys = FindSystemBlock( name );
    if( sys == 0 ) {
        LnkMsg( 0x7a6b, "s", name );
        _LnkFree( name );
        return 1;
    }
    RestoreParser();
    NewCommandSource( name, sys->commands, SYSTEM );
    return 1;
}

static parse_entry SysEndOptions[] = {
    { "End", ProcSysEnd, 0xffff },
    { 0, 0, 0 }
};
unsigned char ProcSysBegin( void )
{
    stringtable table;
    char *name;
    sysblock *sys;
    unsigned char duplicate;

    if( !GetToken( SEP_NO, TOK_INCLUDE_DOT ) ) {
        LnkMsg( 0x7a3c, "s", "system begin" );
        return 1;
    }
    duplicate = 0;
    name = tostring();
    sys = FindSysBlock( name );
    if( sys != 0 ) {
        LnkMsg( 0x7a6c, "s", name );
        duplicate = 1;
    } else {
        InitStringTable( &table );
        AddStringTable( &table, (char *)&table, 8 );
    }
    while( !ProcOne( SysEndOptions, SEP_SPACE, 0 ) ) {
        Token.thumb = OK;
        if( !duplicate ) {
            if( !AddStringTable( &table, Token.this, Token.len ) )
                LnkMsg( 0x7c6d, "s", name );
            CharStringTable( &table, ' ' );
        }
        RestoreParser();
    }
    if( !duplicate ) {
        CharStringTable( &table, 0 );
        sys = (sysblock *)GetStringTable( &table );
        sys->name = name;
        sys->next = SysBlocks;
        SysBlocks = sys;
    }
    return 1;
}

sysblock *FindSysBlock( char *name )
{
    sysblock *sys;

    for( sys = SysBlocks; sys != 0; sys = sys->next ) {
        if( stricmp( sys->name, name ) == 0 ) return sys;
    }
    return 0;
}

unsigned char ProcSysEnd( void )
{
    return 1;
}

sysblock *FindSystemBlock( char *name )
{
    sysblock *found, *sys;
    unsigned len;

    found = FindSysBlock( name );
    if( found == 0 ) {
        len = strlen( name );
        for( sys = SysBlocks; sys != 0; sys = sys->next ) {
            if( strnicmp( sys->name, name, len ) == 0 ) {
                if( found != 0 ) return 0;
                found = sys;
            }
        }
    }
    return found;
}
