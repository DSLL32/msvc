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
#include <malloc.h>
#include "cmdline.h"
#include <recon.h>

#ifdef __WATCOMC__
#pragma intrinsic(strlen,strcpy,memcpy)
#endif

/* Actual old option table: no later per-entry flags member. */
static parse_entry MainOptions[] = {
    { "Map", ProcMap, 0xffff },
    { "STack", ProcStack, 0xffff },
    { "NODefaultlibs", ProcNoDefLibs, 0xffff },
    { "Quiet", ProcQuiet, 0xffff },
    { "Dosseg", ProcDosSeg, 0xffff },
    { "Caseexact", ProcCase, 0xffff },
    { "Verbose", ProcVerbose, 0xffff },
    { "Undefsok", ProcUndefsOK, 0xffff },
    { "NAMELen", ProcNameLen, 0xffff },
    { "SYMFile", ProcSymFile, 0xffff },
    { "OSName", ProcOSName, 0xffff },
    { "ELiminate", ProcEliminate, 0xffff },
    { "MAXErrors", ProcMaxErrors, 0xffff },
    { "NOCASEexact", ProcNoCaseExact, 0xffff },
    { "NOCAChe", ProcNoCache, 0xffff },
    { "CAChe", ProcNoCache, 0xffff },
    { "MANGlednames", ProcMangledNames, 0xffff },
    { "STATics", ProcStatics, 0xffff },
    { "ARTificial", ProcArtificial, 0xffff },
    { "SHOwdead", ProcShowDead, 0xffff },
    { "VFRemoval", ProcVFRemoval, 0xffff },
    { "REDefsok", ProcRedefsOK, 0xffff },
    { "NOREDefsok", ProcNoRedefs, 0xffff },
    { "SMall", ProcSmall, 0x0080 },
    { "DIStribute", ProcDistribute, 0x0080 },
    { "DYNamic", ProcDynamic, 0x0080 },
    { "STANdard", ProcStandard, 0x0080 },
    { "NOIndirect", ProcNoIndirect, 0x0080 },
    { "ARea", ProcArea, 0x0080 },
    { "PADSections", ProcPadSections, 0x0080 },
    { "PACKCode", ProcPackcode, 0x42e9 },
    { "PACKData", ProcPackdata, 0x42e9 },
    { "Alignment", ProcAlignment, 0x801d },
    { "STUB", ProcStub, 0x3c1f },
    { "ONEautodata", ProcSingle, 0x000f },
    { "MANYautodata", ProcMultiple, 0x000f },
    { "NOAutodata", ProcNone, 0x000f },
    { "OLDlibrary", ProcOldLibrary, 0x001f },
    { "MODName", ProcModName, 0x001f },
    { "NEWFiles", ProcNewFiles, 0x0007 },
    { "PROTmode", ProcProtMode, 0x000f },
    { "DEscription", ProcDescription, 0x001f },
    { "OBJAlign", ProcObjAlign, 0x0010 },
    { "RWReloccheck", ProcRWRelocCheck, 0x0008 },
    { "SELFrelative", ProcSelfRelative, 0x0004 },
    { "INTernalrelocs", ProcInternalRelocs, 0x0004 },
    { "TOGglerelocsflag", ProcToggleRelocsFlag, 0x0004 },
    { "Heapsize", ProcHeapsize, 0x421f },
    { "MINData", ProcMinData, 0x3c00 },
    { "MAXData", ProcMaxData, 0x3c00 },
    { "OFFset", ProcOffset, 0x4816 },
    { "SCReenname", ProcScreenName, 0x0100 },
    { "CHeck", ProcCheck, 0x0100 },
    { "MULTILoad", ProcMultiLoad, 0x0100 },
    { "REentrant", ProcReentrant, 0x0100 },
    { "SYnchronize", ProcSynch, 0x0100 },
    { "CUSTom", ProcCustom, 0x0100 },
    { "START", ProcStart, 0x0100 },
    { "EXit", ProcExit, 0x0100 },
    { "THReadname", ProcThreadName, 0x0100 },
    { "PSeudopreemption", ProcPseudoPreemption, 0x0100 },
    { "COPYRight", ProcCopyright, 0x0100 },
    { "MESsages", ProcMessages, 0x0100 },
    { "HElp", ProcHelp, 0x0100 },
    { "XDCdata", ProcXDCData, 0x0100 },
    { "SHArelib", ProcSharelib, 0x0100 },
    { "VERsion", ProcVersion, 0x011e },
    { "NORelocs", ProcNoRelocs, 0x4210 },
    { "LOnglived", ProcLonglived, 0x4200 },
    { "RESource", ProcQNXResource, 0x4200 },
    { "PRIVilege", ProcPrivilege, 0x4200 },
    { "LInearrelocs", ProcLinearRelocs, 0x4200 },
    { 0, 0, 0 }
};

unsigned char AddOption( void )
{
    Token.thumb = REJECT;
    return ProcOne( MainOptions, SEP_NO, 0 ) != 0;
}

unsigned char ProcMap( void )
{
    MapFlags |= 1;
    if( GetToken( SEP_EQUALS, TOK_INCLUDE_DOT | TOK_IS_FILENAME ) ) {
        if( MapFName != 0 ) _LnkFree( MapFName );
        MapFName = FileName( Token.this, Token.len, 1, 0 );
    }
    return 1;
}

unsigned char ProcVerbose( void )
{
    if( LinkFlags & 8 ) {
        LnkMsg( 0x7a65, 0 );
        return 1;
    }
    MapFlags |= 0x40;
    /* The genuine successful prefix falls into the shared true callback. */
    return ProcSysEnd();
}

unsigned char ProcStack( void )
{
    LinkFlags |= 2;
    return GetLong( &StackSize );
}

unsigned char ProcName( void )
{
    if( !GetToken( SEP_NO, TOK_INCLUDE_DOT | TOK_IS_FILENAME ) ) return 0;
    CmdFlags &= ~0x10;
    if( Name != 0 ) _LnkFree( Name );
    Name = tostring();
    return 1;
}

unsigned char ProcFiles( void )
{
    if( !(LinkFlags & 0x4801) ) CmdFlags |= 8;
    return ProcArgList( AddFile, TOK_INCLUDE_DOT | TOK_IS_FILENAME );
}

static void *LastFile;          /* original441284: file or member node */

unsigned char AddFile( void )
{
    char *ptr, *membname;
    file_list **temp;

    CmdFlags &= ~0x440;
    CmdFlags |= 0x400;
    if( CmdFlags & 0x100 ) {
        if( CmdFlags & 0x200 ) CmdFlags &= ~0x200;
        else MakeNewSection();
    }
    ptr = GetFileName( &membname, 1 );
    temp = CurrFList;
    if( *CurrFList != 0 ) CurrFList = &(*CurrFList)->next_file;
    LastFile = AddObjFile( ptr, membname, CurrFList );
    if( CmdFlags & 0x40 ) CurrFList = temp;
    else if( membname != 0 ) {
        LastFile = ((file_list *)LastFile)->u.member;
        CmdFlags |= 0x40;
    }
    _LnkFree( ptr );
    return 1;
}

void *AddObjFile( char *name, char *member, file_list **filelist )
{
    file_list *new_entry;
    member_list *new_member;

    new_member = 0;
    if( member != 0 ) {
        new_member = ChkLAlloc( sizeof(member_list) + strlen(member) );
        new_member->flags = DBIFlag;
        strcpy( new_member->name, member );
        new_member->next = 0;
        _LnkFree( member );
        new_entry = CurrSect->files;
        while( new_entry != 0 ) {
            if( stricmp( new_entry->file->name, name ) == 0 ) {
                CmdFlags |= 0x40;
                if( new_entry->u.member != 0 ) {
                    LinkList( &new_entry->u.member, new_member );
                    return new_member;
                }
                _LnkFree( new_member );
                return new_entry->u.member;
            }
            new_entry = new_entry->next_file;
        }
    }
    new_entry = AllocNewFile( new_member );
    if( new_member != 0 ) {
        new_entry->file = AllocUniqueFileEntry( name, LibPath );
        new_entry->file->flags |= 2;
    } else {
        new_entry->file = AllocFileEntry( name, Path );
    }
    *filelist = new_entry;
    return new_entry;
}

file_list *AllocNewFile( member_list *member )
{
    file_list *entry;
    entry = PermAlloc( sizeof(file_list) );
    entry->next_file = 0;
    entry->status = DBIFlag;
    entry->u.member = member;
    if( member != 0 ) entry->status |= 0x8000;
    return entry;
}

unsigned char ProcLibrary( void )
{
    if( !(LinkFlags & 0x4801) && !IsSystemBlock() ) CmdFlags |= 8;
    return ProcArgList( AddLib, TOK_INCLUDE_DOT | TOK_IS_FILENAME );
}

unsigned char OvlLevel;       /* original450030 */
extern unsigned SectionNum;  /* original45010c */

unsigned char AddLib( void )
{
    char *ptr;
    file_list *result;

    ptr = FileName( Token.this, Token.len, 2, 0 );
    result = AddObjLib( ptr, 255 );
    if( CmdFlags & 2 ) {
        result->status |= 0x0800;
        if( OvlLevel == 0 ) result->ovlref = 0;
        else result->ovlref = SectionNum - 1;
    }
    _LnkFree( ptr );
    return 1;
}

file_list *AddObjLib( char *name, unsigned char priority )
{
    file_list **next_owner, **proc_owner;
    file_list *proc_curr, *newproc;
    unsigned char added;

    proc_owner = &ObjLibFiles;
    for( ;; ) {
        proc_curr = *proc_owner;
        if( proc_curr == 0 || proc_curr->priority < priority ) break;
        if( stricmp( proc_curr->file->name, name ) == 0 ) return proc_curr;
        proc_owner = &proc_curr->next_file;
    }
    added = 1;
    next_owner = proc_owner;
    for( ;; ) {
        if( proc_curr == 0 ) break;
        if( stricmp( proc_curr->file->name, name ) == 0 ) {
            *next_owner = proc_curr->next_file;
            proc_curr->next_file = *proc_owner;
            *proc_owner = proc_curr;
            proc_curr->priority = priority;
            newproc = proc_curr;
            added = 0;
            break;
        }
        next_owner = &proc_curr->next_file;
        proc_curr = *next_owner;
    }
    if( added ) {
        newproc = AllocNewFile( 0 );
        newproc->file = AllocUniqueFileEntry( name, LibPath );
        newproc->file->flags |= 0x12;
        newproc->priority = priority;
        newproc->next_file = *proc_owner;
        *proc_owner = newproc;
        LinkState |= 4;
    }
    return newproc;
}

unsigned char ProcCodeviewDBI( void )
{
    if( LinkFlags & 0x4001 ) LnkMsg( 0x7a95, 0 );
    else LinkFlags |= 0x20;
    return 1;
}

unsigned char ProcVersion( void )
{
    unsigned char result;
    unsigned short value;

    if( !GetToken( SEP_EQUALS, 0 ) ) return 0;
    FmtData.minor = 0;
    FmtData.revision = 0;
    if( getatoi( &FmtData.major ) != 0 ) {
        LnkMsg( 0x7a3c, "s", "version" );
        return 1;
    }
    FmtData.flags |= 2;
    if( !GetToken( SEP_PERIOD, 0 ) ) return 1;
    if( getatoi( &value ) != 0 || value >= 100 ) {
        LnkMsg( 0x7a3c, "s", "version" );
        return 1;
    }
    FmtData.minor = value;
    if( !GetToken( SEP_PERIOD, 0 ) ) return 1;
    result = getatoi( &value );
    if( result == 1 && Token.len == 1 ) {
        FmtData.revision = tolower( (unsigned char)*Token.this ) - 'a' + 1;
    } else if( result == 0 && value < 27 ) {
        FmtData.revision = value;
    } else {
        LnkMsg( 0x7a3c, "s", "version" );
    }
    return 1;
}

extern unsigned char ProcOS2HeapSize( void );
extern unsigned char ProcQNXHeapSize( void );
unsigned char ProcHeapsize( void )
{
    if( HintFormat( 0x001f ) ) return ProcOS2HeapSize();
    if( HintFormat( 0x4200 ) ) return ProcQNXHeapSize();
    return 1;
}

unsigned char ProcEliminate( void )
{
    LinkFlags |= 0x10000UL;
    return 1;
}

extern unsigned char AddAlias( void );
unsigned char ProcAlias( void )
{
    return ProcArgList( AddAlias, TOK_INCLUDE_DOT );
}

extern void MakeSymAlias( char *, int, char *, int );
unsigned char AddAlias( void )
{
    char *name;
    int namelen, targetlen;
    char *target;

    namelen = Token.len;
    name = alloca( namelen );
    memcpy( name, Token.this, namelen );
    if( !GetToken( SEP_EQUALS, TOK_INCLUDE_DOT ) ) return 0;
    targetlen = Token.len;
    target = tostring();
    MakeSymAlias( name, namelen, target, targetlen );
    return 1;
}

extern unsigned char AddReference( void );
unsigned char ProcReference( void )
{
    return ProcArgList( AddReference, TOK_INCLUDE_DOT );
}

unsigned char AddReference( void )
{
    symbol *sym;

    sym = SymXOp( 0x12, Token.this, Token.len );
    sym->info |= 0x4000;
    return 1;
}

extern unsigned char GetPackValue( unsigned long *, char * );
unsigned char ProcPackcode( void )
{
    unsigned long value;

    if( !GetPackValue( &value, "packcode" ) ) return 0;
    PackCodeLimit = value;
    LinkFlags |= 0x0200;
    return 1;
}

unsigned char GetPackValue( unsigned long *value, char *name )
{
    if( !HaveEquals( 0 ) ) return 0;
    if( getatol( value ) != 0 ) {
        LnkMsg( 0x7a3c, "s", name );
        return 1;
    }
    if( *value == 0 ) *value = 1;
    return 1;
}

unsigned char ProcPackdata( void )
{
    unsigned long value;

    if( !GetPackValue( &value, "packdata" ) ) return 0;
    PackDataLimit = value;
    LinkFlags |= 0x04000000UL;
    return 1;
}

extern unsigned char ProcOS2Segment( void );
extern unsigned char ProcQNXSegment( void );
unsigned char ProcSegment( void )
{
    if( HintFormat( 0x001f ) ) return ProcOS2Segment();
    if( HintFormat( 0x4200 ) ) return ProcQNXSegment();
    return 1;
}

unsigned char ProcStub( void )
{
    char *name;
    char **nameptr;

    if( HintFormat( 0x001f ) ) {
        nameptr = &FmtData.u.os2.stub_file_name;
    } else {
        nameptr = &FmtData.u.phar.stub;
        Extension = 4;
    }
    if( !HaveEquals( 3 ) ) return 0;
    name = FileName( Token.this, Token.len, 4, 0 );
    if( *nameptr == 0 ) {
        *nameptr = name;
    } else if( IsSystemBlock() ) {
        _LnkFree( name );
    } else {
        _LnkFree( *nameptr );
        *nameptr = name;
    }
    return 1;
}

unsigned char ProcAlignment( void )
{
    if( HintFormat( 0x001d ) ) return ProcOS2Alignment();
    if( HintFormat( 0x8000 ) ) return ProcELFAlignment();
    return 1;
}

unsigned char ProcOffset( void )
{
    if( !GetLong( &FmtData.base ) ) return 0;
    if( !(FmtData.type & 0x7c00) ) ChkBase();
    else if( !(FmtData.type & 0x0016) ) ChkBase4K();
    return 1;
}

unsigned char ProcSymFile( void )
{
    CmdFlags |= 0x80;
    if( GetToken( SEP_EQUALS, TOK_INCLUDE_DOT | TOK_IS_FILENAME ) ) {
        if( SymFileName != 0 ) _LnkFree( SymFileName );
        SymFileName = FileName( Token.this, Token.len, 15, 0 );
    }
    return 1;
}

unsigned char ProcCase( void )
{
    LinkFlags |= 0x2000;
    SetSymCase();
    return 1;
}

unsigned char ProcStatics( void )
{
    MapFlags |= 2;
    return 1;
}

unsigned char ProcPath( void )
{
    path_entry *entry;
    unsigned char ret;

    ret = GetToken( SEP_NO, TOK_INCLUDE_DOT | TOK_IS_FILENAME );
    if( ret ) {
        entry = ChkLAlloc( sizeof(*entry) + Token.len );
        memcpy( entry->name, Token.this, Token.len );
        entry->name[Token.len] = 0;
        entry->next = Path;
        Path = entry;
    }
    return ret;
}

unsigned char ProcLibPath( void )
{
    if( !GetToken( SEP_NO, TOK_INCLUDE_DOT | TOK_IS_FILENAME ) ) {
        LnkMsg( 0x7a3c, "s", "libpath" );
        return 1;
    }
    AddLibPaths( Token.this, Token.len, 1 );
    return 1;
}

unsigned char ProcLibFile( void )
{
    if( !(LinkFlags & 0x4801) ) CmdFlags |= 8;
    return ProcArgList( AddLibFile, TOK_INCLUDE_DOT | TOK_IS_FILENAME );
}

unsigned char AddLibFile( void )
{
    char *name, *member;
    file_list *file;

    CmdFlags &= ~0x440;
    CmdFlags |= 0x400;
    name = GetFileName( &member, 0 );
    if( member != 0 ) {
        LnkMsg( 0x7a6e, 0 );
        _LnkFree( member );
    } else {
        file = AllocNewFile( 0 );
        file->file = AllocFileEntry( name, LibPath );
        file->next_file = Root->files;
        Root->files = file;
        file->file->flags |= 1;
    }
    _LnkFree( name );
    return 1;
}

unsigned char ProcOSName( void )
{
    if( !GetToken( SEP_EQUALS, TOK_INCLUDE_DOT ) ) return 0;
    if( FmtData.osname != 0 ) _LnkFree( FmtData.osname );
    FmtData.osname = tostring();
    return 1;
}

unsigned char ProcNoDefLibs( void )
{
    CmdFlags |= 4;
    return 1;
}

unsigned char ProcVFRemoval( void )
{
    LinkFlags |= 0x10000000UL;
    return 1;
}

unsigned char ProcShowDead( void )
{
    LinkFlags |= 0x2000000UL;
    return 1;
}

unsigned char ProcArtificial( void )
{
    MapFlags |= 0x4;
    return 1;
}

unsigned char ProcAlphabetical( void )
{
    MapFlags |= 0x10;
    return 1;
}

unsigned char ProcGlobal( void )
{
    MapFlags |= 0x20;
    return 1;
}

static parse_entry SortOptions[] = {
    { "ALPhabetical", ProcAlphabetical, 0xffff },
    { "GLobal", ProcGlobal, 0xffff },
    { 0, 0, 0 }
};

unsigned char ProcSort( void )
{
    MapFlags |= 8;
    if( !ProcOne( SortOptions, SEP_NO, 0 ) ) return 1;
    ProcOne( SortOptions, SEP_NO, 0 );
    return 1;
}

extern unsigned long MaxErrors;
unsigned char ProcMaxErrors( void )
{
    if( !GetLong( &MaxErrors ) ) return 0;
    LinkFlags |= 0x00040000UL;
    return 1;
}

extern unsigned NameLen;
unsigned char ProcNameLen( void )
{
    unsigned long value;
    unsigned char ret;

    ret = GetLong( &value );
    if( ret ) {
        if( value == 0 ) LnkMsg( 0x7a3c, "s", "namelen" );
        else NameLen = value;
    }
    return ret;
}

unsigned char ProcNoCaseExact( void )
{
    LinkFlags &= ~0x2000UL;
    SetSymCase();
    return ProcNoCache();
}

unsigned char ProcNoCache( void )
{
    /* This 1994 entry is a no-op; it does not select a cache policy. */
    return 1;
}

unsigned char ProcRedefsOK( void )
{
    LinkFlags |= 0x40000000UL;
    return 1;
}

unsigned char ProcUndefsOK( void )
{
    LinkFlags |= 0x40UL;
    return 1;
}

unsigned char ProcMangledNames( void )
{
    LinkFlags |= 0x80000UL;
    return 1;
}

unsigned char ProcNoRelocs( void )
{
    /* The 1994 first hint also includes NT, so that format takes the
     * QNX setter rather than reaching the second PE-only branch. */
    if( HintFormat( 0x4210 ) ) return ProcQNXNoRelocs();
    if( HintFormat( 0x0010 ) ) return ProcPENoRelocs();
    return 1;
}
