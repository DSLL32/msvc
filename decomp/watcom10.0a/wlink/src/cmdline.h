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
#ifndef WLINK_CMDLINE_H
#define WLINK_CMDLINE_H
#include "link.h"
struct pe_hdr_table_entry;

/* The old SYSTEM block owns a next/name header and inline command text. */
typedef struct sysblock {
    struct sysblock *next;
    char *name;
    char commands[1];
} sysblock;

#pragma pack(1)
/* Old format union/common record: only encountered fields are identified. */
typedef struct fmt_os2_data {
    entry_export *exports;
    char *stub_file_name;
    char *res_module_name;
    char *old_lib_name;
    void *os2_seg_flags;
    char *description;
    unsigned long heapsize;
    unsigned short segment_shift;
    unsigned short flags;
} fmt_os2_data;
/* Old Novell union: explicit start name, byte module type and flags. */
typedef struct fmt_nov_data {
    snamelist *exports, *modules;
    char *screenname, *description, *checkfn, *startfn, *exitfn;
    char *customdata, *threadname, *copyright, *messages, *help;
    char *rpcdata, *sharednlm;
    unsigned char moduletype, flags;
} fmt_nov_data;
typedef char old_nov_format_is_58[(sizeof(fmt_nov_data) == 58) ? 1 : -1];

typedef struct rtpblock {
    unsigned short signature, minreal, maxreal, minibuf, maxibuf;
    unsigned short nistack, istksize;
    unsigned long realbreak;
    unsigned short callbufs, extender_flags, unpriv;
} rtpblock;
typedef char old_rtp_is_24[(sizeof(rtpblock) == 24) ? 1 : -1];
typedef struct fmt_phar_data {
    unsigned long mindata, maxdata;
    char *breaksym;
    rtpblock *params;
    char *stub;
} fmt_phar_data;

typedef struct fmt_elf_data {
    snamelist *exports, *modules;
    unsigned long alignment;
} fmt_elf_data;

typedef struct fmt_qnx_data {
    seg_flags *seg_flags;
    unsigned long heapsize;
    char *resource;
    unsigned char flags, priv_level, reloc_flags;
} fmt_qnx_data;
typedef char old_qnx_format_is_15[(sizeof(fmt_qnx_data) == 15) ? 1 : -1];

typedef struct fmt_data {
    union {
        unsigned char platform_data[58];
        fmt_os2_data os2;
        fmt_nov_data nov;
        fmt_qnx_data qnx;
        fmt_phar_data phar;
        fmt_elf_data elf;
        struct {
            fmt_os2_data os2;
            unsigned long objalign;
            unsigned long heapcommit;
            unsigned long stackcommit;
            seg_leader *dgroupsplitseg;
            unsigned long bsspad;
            unsigned short subsystem;
            unsigned char flags;
            unsigned char unknown_tail[3];
        } pe;
    } u;
    unsigned short type;
    unsigned short def_seg_flags;
    int cpu_type;
    unsigned long base;
    unsigned short major, minor;
    unsigned char revision;
    unsigned char flags;          /* bit0 dll, original4500b3 */
    char *osname;              /* old final common pointer, original4500b4 */
} fmt_data;

typedef struct tok {
    char *buff;
    unsigned len;
    char *next;
    char *this;
    unsigned short line;
    unsigned short where : 2;
    unsigned short how : 3;
    unsigned short thumb : 1;
    unsigned short locked : 1;
    unsigned short quoted : 1;
    unsigned short reserved : 8; /* complete the original16-bit flag unit */
} tok;

typedef struct cmdfilelist {
    struct cmdfilelist *prev;
    struct cmdfilelist *next;
    int file;
    char *name;
    tok token;
} cmdfilelist;

typedef struct parse_entry {
    char *keyword;
    unsigned char (*rtn)( void );
    unsigned short format;
} parse_entry;
#pragma pack()
typedef char old_format_is_80[(sizeof(fmt_data) == 80) ? 1 : -1];
typedef char old_token_is_20[(sizeof(tok) == 20) ? 1 : -1];
typedef char old_command_source_is_36[(sizeof(cmdfilelist) == 36) ? 1 : -1];
extern cmdfilelist *CmdFile;
extern fmt_data FmtData;
extern tok Token;
extern char *Name;
extern unsigned short CmdFlags;
extern void ExecSystem( char * );
extern unsigned char Extension, DBIFlag, MapFlags;
extern char *SymFileName;
extern section *CurrSect;
extern file_list **CurrFList;
extern path_entry *Path;
extern unsigned long LinkFlags;

extern void ResetCmdFile( void );
extern void GetCmdLine( char * );
extern void NewCommandSource( char *, char *, unsigned );
extern int QIsADevice( int );
extern unsigned char QSysHelp( char ** );
extern void Help( void );
extern void EatWhite( void );
extern void Crash( unsigned char );
extern void DisplayOptions( void );
extern void WriteGenHelp( void );
extern void WriteHelp( unsigned, unsigned, unsigned char );
extern void PressKey( void );
extern unsigned char WaitForKey( void );
extern void WriteMsg( char * );
extern void WLPrtBanner( void );
extern void WriteStdOut( char *, unsigned );
extern void QWriteNL( int, char * );
extern void Msg_Get( unsigned, char * );
extern int WResLoadString( HANDLE_INFO *, unsigned, char __far *, int );
extern int GetString( WResLangInfo *, HANDLE_INFO *, unsigned, char __far *, int );
extern unsigned QRead( int, void *, unsigned, char * );
extern unsigned char ProcOne( parse_entry *, unsigned, unsigned char );
extern unsigned char ProcCom( void );
extern unsigned char ProcSection( void );
extern unsigned char ProcAutoSection( void );
extern unsigned char ProcEnd( void );
extern void NewArea( section *sect );
extern void DBISectInit( section *sect );
extern unsigned char ProcInto( void );
extern void SetOvlClasses( void );
extern void MakeNonArea( void );
extern void WalkSections( section *sect, void (*rtn)(section *) );
extern void OvlRefVector( symbol *sym );
extern void RefDistribSym( symbol *sym );
extern unsigned short LowestAncestor( unsigned short index, section *sect );
extern unsigned char IsAncestor( unsigned index, section *sect );
extern void Vectorize( symbol *sym );
extern void IndirectCall( symbol *sym );
extern void DistIndCall( symbol *sym );
extern symbol *DefISymbol( char *name );
extern unsigned char NewRefVector( symbol *sym, unsigned first, unsigned second );
extern void ScanArcs( mod_entry *mod );
struct arcdata;
extern void DoRefGraph( unsigned num, mod_entry *mod );
extern unsigned char NotAnArc( unsigned long arc );
extern void DeleteArc( struct arcdata *arcs, unsigned index );
extern void AddArc( unsigned long arc );
extern unsigned char SortGroupSections( section *sect );
extern void AllocAreas( ovl_area *area );
extern void MAlign( unsigned char align );
extern void AllocSections( section *sect );
extern void WriteVectors( void );
extern void DoSecPubs( section *sect );
extern void ShortVectors( symbol *sym );
extern void LongVectors( symbol *sym );
extern void PutOvlInfo( unsigned off, void *data, unsigned len );
extern unsigned long WriteDOSData( void );
extern void WriteCOMFile( void );
extern void PadOvlFiles( void );
extern unsigned char CompareDosSegments( address *left, address *right );
extern void OrderGroups( unsigned char (*cmp)(address *,address *) );
extern unsigned long WriteDOSRootRelocs( void );
extern void AssignFileLocs( section *sect );
extern void EmitOvlTable( void );
extern void OpenOvlFile( outfilelist *file );
extern unsigned char WriteDOSGroup( group_entry *group );
extern void WriteDOSSectRelocs( section *sect, unsigned char repos );
extern unsigned EmitOvlAreaEntry( unsigned off, ovl_area *area );
extern unsigned EmitOvlEntry( unsigned off, section *sect );
extern void WriteMapBox12B( void );
extern unsigned char ProcNLM( void );
extern unsigned char ProcLAN( void );
extern unsigned char ProcDSK( void );
extern unsigned char ProcNAM( void );
extern unsigned char ProcNovDBIExports( void );
extern unsigned char ProcNovDBIReferenced( void );
extern unsigned char GetNovImport( void );
extern void SetNovImportSymbol( symbol *sym );
extern unsigned char GetNovExport( void );
extern unsigned char GetNovModule( void );
extern unsigned char CompareProtSegments( address *left, address *right );
extern int QOpenR( char *name );
extern unsigned long CopyToLoad( int handle, char *name );
extern void ChkBase4K( void );
extern unsigned char ProcQNXFlat( void );
extern unsigned char GetQNXSegFlags( void );
extern unsigned char ProcQNXExecuteonly( void );
extern unsigned char ProcQNXExecuteread( void );
extern unsigned char ProcQNXReadOnly( void );
extern unsigned char ProcQNXReadWrite( void );
extern void SetQNXGroupFlags( void );
extern void WriteQNXGroup( group_entry *group, unsigned long *segments );
extern unsigned char ProcPharFlat( void );
extern unsigned char ProcRex( void );
extern unsigned char ProcPharSegmented( void );
extern void WritePharSimple( unsigned long start );
extern void WritePharExtended( unsigned long start );
extern unsigned long WriteSIT( void );
extern unsigned long WritePharRelocs( void );
extern unsigned long WriteRTPBlock( void );
extern unsigned long WritePharSegData( void );
extern unsigned long WritePharData( unsigned long file_pos );
extern void WriteDescriptor( unsigned long base, unsigned long limit, unsigned flags );
extern unsigned char ProcELFDLL( void );
extern unsigned char ProcOS2Alignment( void );
extern unsigned char ProcELFAlignment( void );
extern unsigned char GetELFImport( void );
extern void SetELFImportSymbol( symbol *sym );
extern char *DW_LEB128( char *buffer, long value );
extern unsigned long PadToAlign( unsigned long offset, unsigned long alignment );
extern void AssignELFExportIndices( void );
extern unsigned long WriteELFHash( symbol **indices );
struct stringtable;
extern unsigned long WriteELFSymbols( symbol **indices, struct stringtable *strings );
extern unsigned ELFPrimeBuckets( unsigned count );
extern unsigned long ELFHash( char *name );
extern unsigned char CompareOffsets( address *left, address *right );
extern sysblock *FindSysBlock( char * );
extern sysblock *FindSystemBlock( char * );
extern unsigned char ProcSysBegin( void );
extern unsigned char ProcSysEnd( void );
extern void SetSymCase( void );
extern unsigned char AddModTrace( void );
extern unsigned char AddSymTrace( void );
extern unsigned char AddLibFile( void );
extern char *totext( void );
extern void ReadNameTable( int file );
extern void ReadPEExportTable( int file, struct pe_hdr_table_entry *base );
extern void CheckExport( char *name, unsigned short ordinal, int (*compare)(const char *, const char *) );
extern entry_export *FindPlace( entry_export *entry );
extern void DefineLazyExtdef( symbol *sym, symbol *def, unsigned char isweak );
extern void ReadExports( unsigned long namestart, unsigned long nameend, unsigned long ordstart, unsigned numords, unsigned ord_base, int file, char *fname );
extern unsigned char ProcAlphabetical( void );
extern unsigned char ProcGlobal( void );
extern int SymAlphaCompare( const void *a, const void *b );
extern int SymAddrCompare( const void *a, const void *b );
extern int __unmangled_name( const char *name, unsigned len, const char **base, unsigned *size );
extern unsigned char_to_digit( char c );
extern unsigned demangle_recursive( const char *input, char *output, unsigned size );
extern unsigned char AddVector( void );
extern unsigned char AddForceVector( void );
extern unsigned char AddNoVector( void );
extern unsigned char ProcQNXNoRelocs( void );
extern unsigned char ProcPENoRelocs( void );
extern int ParseNumber( char *str, int radix );
extern unsigned char AddDisable( void );
extern unsigned short blog_32( unsigned long value );
extern unsigned char ProcDosHelp( void );
extern unsigned char ProcOS2Help( void );
extern unsigned char ProcWindowsHelp( void );
extern unsigned char ProcNTHelp( void );
extern unsigned char ProcPharHelp( void );
extern unsigned char ProcNovellHelp( void );
extern unsigned char ProcQNXHelp( void );
extern unsigned char ProcELFHelp( void );
extern int SearchPath( char * );
extern int QObjOpen( char * );
extern int NSOpen( char *, unsigned );
extern int DoOpen( char *, unsigned, unsigned char );
extern unsigned char CleanCachedHandles( void );
extern int QMakeFileName( char **, char *, char * );
extern void SetCommandFile( int, char * );
extern void DoCmdParse( void );
extern void Ignite( void );
extern void Suicide( void );
extern unsigned char HintFormat( unsigned short );
extern void CmdNovFini( void );
extern void CmdOvlFini( void );
extern file_list *AddObjLib( char *, unsigned char );
extern char *FileName( char *, unsigned, unsigned, int );
extern void CheckTraces( void );
extern void BurnUtils( void );
extern void PruneSystemList( void );
extern void CleanSystemList( unsigned char );
extern void NumberSections( void );
extern void DBIInit( void );
extern void SetQNXFmt( void );
extern void FreeQNXFmt( void );
extern void SetELFFmt( void );
extern void FreeELFFmt( void );
extern void SetOS2Fmt( void );
extern void FreeOS2Fmt( void );
extern void SetPharFmt( void );
extern void FreePharFmt( void );
extern void SetNovFmt( void );
extern void FreeNovFmt( void );

/* Old command parser separators/control match the encountered call sites. */
extern unsigned char GetToken( unsigned, unsigned );
extern void RestoreParser( void );
extern void Syntax( void );
extern unsigned char ProcFiles( void );
extern unsigned char ProcLibrary( void );
extern unsigned char AddLib( void );
extern unsigned char IsSystemBlock( void );
extern unsigned char ProcName( void );
extern unsigned char ProcOptions( void );
extern unsigned char ProcDebug( void );
extern unsigned char ProcSystem( void );
extern unsigned char ProcLibPath( void );
extern unsigned char ProcLibFile( void );
extern unsigned char ProcPath( void );
extern unsigned char ProcFormat( void );
extern unsigned char ProcModTrace( void );
extern unsigned char ProcSymTrace( void );
extern unsigned char ProcAlias( void );
extern unsigned char ProcReference( void );
extern unsigned char ProcDisable( void );
extern unsigned char ProcSort( void );
extern unsigned char ProcLanguage( void );
extern unsigned char ProcCommit( void );
extern unsigned char ProcAnonExport( void );
extern unsigned char ProcImport( void );
extern unsigned char ProcExport( void );
extern unsigned char ProcOS2Export( void );
extern unsigned char GetWlibImports( void );
extern unsigned char getexport( void );
extern unsigned char ProcNovExport( void );
extern unsigned char ProcSegment( void );
extern unsigned char ProcOverlay( void );
extern unsigned char ProcBegin( void );
extern unsigned char ProcFixedLib( void );
extern unsigned char ProcNoVector( void );
extern unsigned char ProcVector( void );
extern unsigned char ProcForceVector( void );
extern unsigned char ProcRuntime( void );
extern unsigned char ProcModule( void );
extern unsigned char ProcNewSegment( void );
enum { SEP_NO, SEP_COMMA, SEP_EQUALS, SEP_PERIOD, SEP_END, SEP_QUOTE,
       SEP_PAREN, SEP_SPACE, SEP_PERCENT, SEP_DOT_EXT, SEP_LCURLY, SEP_RCURLY };
enum { NONBUFFERED, COMMANDLINE, INTERACTIVE, BUFFERED, ENVIRONMENT, SYSTEM };
enum { MIDST, ENDOFLINE, ENDOFFILE, ENDOFCMD };
enum { OK, REJECT };
#define TRUE 1
#define FALSE 0
#define CTRLZ 26
#define TOK_INCLUDE_DOT 1
extern unsigned char MakeToken( unsigned, unsigned );
extern void StartNewFile( void );
extern void ExpandEnvVariable( void );
extern void BackupParser( void );
extern void GetNewLine( void );
extern void RestoreCmdLine( void );
extern int MapDoubleByteChar( unsigned char );
extern void MapEscapeChar( void );
#define TOK_IS_FILENAME 2
extern char *tostring( void );
extern unsigned long QFileSize( int );
extern unsigned char QReadStr( int, char *, unsigned, char * );
extern void OutPutPrompt( char * );
extern void InitFmt( void (*)(void) );
extern void AddFmtLibPaths( void );
extern void AddEnvPaths( char * );
extern void AddLibPaths( char *, unsigned, unsigned char );
extern unsigned char ProcDos( void );
extern unsigned char ProcELF( void );
extern unsigned char ProcNovell( void );
extern unsigned char ProcOS2( void );
extern unsigned char ProcPharlap( void );
extern unsigned char ProcQNX( void );
extern void ChkBase( void );
extern unsigned char ProcFont( void );
extern unsigned char ProcFullScreen( void );
extern unsigned char ProcLE( void );
extern unsigned char ProcMemory( void );
extern unsigned char ProcOS2DLL( void );
extern unsigned char ProcOldPE( void );
extern unsigned char ProcPE( void );
extern unsigned char ProcPM( void );
extern unsigned char ProcPMCompatible( void );
extern unsigned char ProcPhysDevice( void );
extern unsigned char ProcVirtDevice( void );
extern unsigned char ProcTNT( void );
extern unsigned char ProcArgList( unsigned char (*)(void), unsigned );
extern unsigned char AddRuntime( void );
extern unsigned char WildCard( unsigned char (*)(void), unsigned );
extern unsigned char CheckFence( void );
extern unsigned char ProcMinReal( void );
extern unsigned char ProcMaxReal( void );
extern unsigned char ProcRealBreak( void );
extern unsigned char ProcCallBufs( void );
extern unsigned char ProcMiniBuf( void );
extern unsigned char ProcMaxiBuf( void );
extern unsigned char ProcNIStack( void );
extern unsigned char ProcIStkSize( void );
extern unsigned char ProcUnpriv( void );
extern unsigned char ProcPriv( void );
extern unsigned char ProcFlags( void );
extern unsigned char ProcRunNative( void );
extern unsigned char ProcRunWindows( void );
extern unsigned char ProcRunConsole( void );
extern unsigned char ProcRunPosix( void );
extern unsigned char ProcRunOS2( void );
extern unsigned char ProcRunDosstyle( void );
extern unsigned char ProcAllDBI( void );
extern unsigned char ProcWatcomDBI( void );
extern unsigned char ProcDwarfDBI( void );
extern unsigned char ProcCodeviewDBI( void );
extern unsigned char ProcNovDBI( void );
extern unsigned char ProcExportsDBI( void );
extern unsigned char ProcAll( void );
extern unsigned char ProcLine( void );
extern unsigned char ProcType( void );
extern unsigned char ProcLocal( void );
extern unsigned char ProcDBIStatic( void );
extern unsigned char AddOption( void );
extern unsigned char ProcAlignment( void );
extern unsigned char ProcArea( void );
extern unsigned char ProcArtificial( void );
extern unsigned char ProcCase( void );
extern unsigned char ProcCheck( void );
extern unsigned char ProcCopyright( void );
extern unsigned char ProcCustom( void );
extern unsigned char ProcDescription( void );
extern unsigned char ProcDistribute( void );
extern unsigned char ProcDosSeg( void );
extern unsigned char ProcDynamic( void );
extern unsigned char ProcEliminate( void );
extern unsigned char ProcExit( void );
extern unsigned char ProcHeapsize( void );
extern unsigned char ProcHelp( void );
extern unsigned char ProcInternalRelocs( void );
extern unsigned char ProcLinearRelocs( void );
extern unsigned char ProcLonglived( void );
extern unsigned char ProcMangledNames( void );
extern unsigned char ProcMap( void );
extern unsigned char ProcMaxData( void );
extern unsigned char ProcMaxErrors( void );
extern unsigned char ProcMessages( void );
extern unsigned char ProcMinData( void );
extern unsigned char ProcModName( void );
extern unsigned char ProcMultiLoad( void );
extern unsigned char ProcMultiple( void );
extern unsigned char ProcNameLen( void );
extern unsigned char ProcNewFiles( void );
extern unsigned char ProcNoCache( void );
extern unsigned char ProcNoCaseExact( void );
extern unsigned char ProcNoDefLibs( void );
extern unsigned char ProcNoIndirect( void );
extern unsigned char ProcNoRedefs( void );
extern unsigned char ProcNoRelocs( void );
extern unsigned char ProcNone( void );
extern unsigned char ProcOSName( void );
extern unsigned char ProcObjAlign( void );
extern unsigned char ProcOffset( void );
extern unsigned char ProcOldLibrary( void );
extern unsigned char ProcPackcode( void );
extern unsigned char ProcPackdata( void );
extern unsigned char ProcPadSections( void );
extern unsigned char ProcPrivilege( void );
extern unsigned char ProcProtMode( void );
extern unsigned char ProcPseudoPreemption( void );
extern unsigned char ProcQNXResource( void );
extern unsigned char ProcQuiet( void );
extern unsigned char ProcRWRelocCheck( void );
extern unsigned char ProcRedefsOK( void );
extern unsigned char ProcReentrant( void );
extern unsigned char ProcScreenName( void );
extern unsigned char ProcSelfRelative( void );
extern unsigned char ProcSharelib( void );
extern unsigned char ProcShowDead( void );
extern unsigned char ProcSingle( void );
extern unsigned char ProcSmall( void );
extern unsigned char ProcStack( void );
extern unsigned char ProcStandard( void );
extern unsigned char ProcStart( void );
extern unsigned char ProcStatics( void );
extern unsigned char ProcStub( void );
extern unsigned char ProcSymFile( void );
extern unsigned char ProcSynch( void );
extern unsigned char ProcThreadName( void );
extern unsigned char ProcToggleRelocsFlag( void );
extern unsigned char ProcUndefsOK( void );
extern unsigned char ProcVFRemoval( void );
extern unsigned char ProcVerbose( void );
extern unsigned char ProcVersion( void );
extern unsigned char ProcXDCData( void );
extern unsigned long StackSize;
extern unsigned char GetLong( unsigned long * );
extern unsigned char HaveEquals( unsigned );
extern unsigned char getatol( unsigned long * );
extern unsigned char getatoi( unsigned short * );
extern unsigned char ProcExpResident( void );
extern unsigned char AddFile( void );
extern void MakeNewSection( void );
extern char *GetFileName( char **, unsigned char );
extern void *AddObjFile( char *, char *, file_list ** );
extern file_list *AllocNewFile( member_list * );
extern infilelist *AllocUniqueFileEntry( char *, path_entry * );
extern infilelist *AllocFileEntry( char *, path_entry * );
extern void LinkList( void *, void * );
extern void *PermAlloc( unsigned );
#include <recon.h>
#endif
