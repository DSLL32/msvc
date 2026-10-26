/* Named application callees remain trapped until reached and reconstructed. */
#include "diagnostic.h"
#include "demangle.h"
#include "fixup.h"
#include "peheader.h"
#include <recon.h>

#if defined(__WATCOMC__)
void reconstruction_trap( void );
#pragma aux reconstruction_trap = 0xcc;
#define TRAP() reconstruction_trap()
#else
#define TRAP() __asm__ __volatile__( "int3" )
#endif

void QDelete( char *name ) { name = name; TRAP(); }

void TrapBreak( int value ) { value = value; TRAP(); }
void ExecSystem( char *name ) { TRAP(); }
void AddLibPaths( char *name, unsigned len, unsigned char front ) { TRAP(); }
void InitModTable( void ) { TRAP(); }
unsigned char CheckVFList( symbol *sym ) { TRAP(); return 0; }

unsigned short OMFCompName( char *name, char *buff, unsigned short index ) { TRAP(); return 0; }
void AddModTable( mod_entry *mod, unsigned ref ) { TRAP(); }
void FinishArcs( mod_entry *mod ) { TRAP(); }
unsigned char ModNameCompare( char *name, char *member ) { TRAP(); return 0; }
void FillOutPtr( section *sect ) { TRAP(); }
void CmdQNXFini( void ) { TRAP(); }
void WeldSyms( symbol *sym, symbol *defsym ) { TRAP(); }
void BSSPass1( symbol *sym ) { TRAP(); }
void BSSPass2( symbol *sym ) { TRAP(); }
void FillTypeFlags( unsigned short flags, unsigned type ) { TRAP(); }
void FillClassFlags( char *name, unsigned short flags ) { TRAP(); }
seg_leader *FindASeg( class_entry *class, char *name ) { TRAP(); return 0; }
void FindExportedSyms( void ) { TRAP(); }
unsigned char SortGroupAreas( ovl_area *area ) { TRAP(); return 0; }
unsigned short ToQNXSel( unsigned short number ) { TRAP(); return 0; }
void NormalizeAddr( void ) { TRAP(); }
void DwarfDefClass( class_entry *class, unsigned long size ) { TRAP(); }
void CalcOvl( void ) { TRAP(); }
void CalcGrpAddr( group_entry *group ) { TRAP(); }
void DwarfAddrSectStart( section *sec ) { TRAP(); }
void NovDBIAddrStart( void ) { TRAP(); }
void CheckBSSInStart( symbol *sym, char *name ) { TRAP(); }
void PhoneyStack( void ) { TRAP(); }
void ReadOldLib( void ) { TRAP(); }
void SetQNXSegFlags( void ) { TRAP(); }
void FiniDOSLoadFile( void ) { TRAP(); }
void FiniOS2FlatLoadFile( void ) { TRAP(); }

void SpillRead( unsigned long loc, unsigned long off, void *buffer, unsigned size ) { TRAP(); }


void DwarfWriteDBI( void ) { TRAP(); }

void FiniOS2LoadFile( void ) { TRAP(); }
void FiniPharLapLoadFile( void ) { TRAP(); }
void FiniNovellLoadFile( void ) { TRAP(); }
void FiniQNXLoadFile( void ) { TRAP(); }
void FiniELFLoadFile( void ) { TRAP(); }


void Locator( char *name, char *member, unsigned rec )
{ name=name; member=member; rec=rec; TRAP(); }
unsigned long FindLinearAddr( msg_address *addr ) { addr=addr; TRAP(); return 0; }
void FileOrder( char *buff, int which ) { buff=buff; which=which; TRAP(); }
void Msg_Do_Put_Args( char *buff, MSG_ARG_LIST *info, char *types, ... )
{ buff=buff; info=info; types=types; TRAP(); }

unsigned char FreeUpMemory( void ) { TRAP(); return 0; }

int WResReadExtraWResID( WResID *id, int file ) { id = id; file = file; TRAP(); return 0; }
int WResReadFixedResRecord( WResResInfo *info, int file )
{ info = info; file = file; TRAP(); return 0; }
int readLangInfoList( int file, WResResNode *node )
{ file = file; node = node; TRAP(); return 0; }
int readMResDir( int file, WResDir dir, int *duplicates )
{ file = file; dir = dir; duplicates = duplicates; TRAP(); return 0; }
int WResIDNameCmp( const WResIDName *first, const WResIDName *second )
{ first = first; second = second; TRAP(); return 0; }



void WresRecordError( int status ) { status = status; TRAP(); }





unsigned char ProcDosHelp( void ) { TRAP(); return 0; }
unsigned char ProcOS2Help( void ) { TRAP(); return 0; }
unsigned char ProcWindowsHelp( void ) { TRAP(); return 0; }
unsigned char ProcNTHelp( void ) { TRAP(); return 0; }
unsigned char ProcPharHelp( void ) { TRAP(); return 0; }
unsigned char ProcNovellHelp( void ) { TRAP(); return 0; }
unsigned char ProcQNXHelp( void ) { TRAP(); return 0; }
unsigned char ProcELFHelp( void ) { TRAP(); return 0; }

unsigned char CleanCachedHandles( void ) { TRAP(); return 0; }









void CmdNovFini( void ) { TRAP(); }

void CmdOvlFini( void ) { TRAP(); }




void CheckFileTrace( section *sect ) { sect=sect; TRAP(); }



void NumASect( section *sect ) { sect=sect; TRAP(); }









void SetQNXFmt( void ) { TRAP(); }
void FreeQNXFmt( void ) { TRAP(); }
void SetELFFmt( void ) { TRAP(); }
void FreeELFFmt( void ) { TRAP(); }


void SetPharFmt( void ) { TRAP(); }
void FreePharFmt( void ) { TRAP(); }
void SetNovFmt( void ) { TRAP(); }
void FreeNovFmt( void ) { TRAP(); }






void WalkAreas( ovl_area *areas, void (*fn)(section *) ) { areas=areas; fn=fn; TRAP(); }







void Syntax( void ) { TRAP(); }





unsigned char ProcSystem( void ) { TRAP(); return 0; }
unsigned char ProcLibPath( void ) { TRAP(); return 0; }
unsigned char ProcLibFile( void ) { TRAP(); return 0; }
unsigned char ProcPath( void ) { TRAP(); return 0; }

unsigned char ProcModTrace( void ) { TRAP(); return 0; }
unsigned char ProcSymTrace( void ) { TRAP(); return 0; }


unsigned char ProcDisable( void ) { TRAP(); return 0; }
unsigned char ProcSort( void ) { TRAP(); return 0; }
unsigned char ProcLanguage( void ) { TRAP(); return 0; }

unsigned char ProcAnonExport( void ) { TRAP(); return 0; }

unsigned char GetWlibImports( void ) { TRAP(); return 0; }

unsigned char ProcExpResident( void ) { TRAP(); return 0; }
unsigned char ProcNovExport( void ) { TRAP(); return 0; }
unsigned char ProcSegment( void ) { TRAP(); return 0; }
unsigned char ProcOverlay( void ) { TRAP(); return 0; }
unsigned char ProcBegin( void ) { TRAP(); return 0; }
unsigned char ProcFixedLib( void ) { TRAP(); return 0; }
unsigned char ProcNoVector( void ) { TRAP(); return 0; }
unsigned char ProcVector( void ) { TRAP(); return 0; }
unsigned char ProcForceVector( void ) { TRAP(); return 0; }

unsigned char ProcModule( void ) { TRAP(); return 0; }
unsigned char ProcNewSegment( void ) { TRAP(); return 0; }



void ExpandEnvVariable( void ) { TRAP(); }
void BackupParser( void ) { TRAP(); }



void MapEscapeChar( void ) { TRAP(); }





unsigned char QReadStr( int file, char *buff, unsigned len, char *name )
{ file=file; buff=buff; len=len; name=name; TRAP(); return 0; }
void OutPutPrompt( char *text ) { text=text; TRAP(); }




unsigned char ProcDos( void ) { TRAP(); return 0; }
unsigned char ProcELF( void ) { TRAP(); return 0; }
unsigned char ProcNovell( void ) { TRAP(); return 0; }

unsigned char ProcPharlap( void ) { TRAP(); return 0; }
unsigned char ProcQNX( void ) { TRAP(); return 0; }



unsigned char ProcFont( void ) { TRAP(); return 0; }
unsigned char ProcFullScreen( void ) { TRAP(); return 0; }
unsigned char ProcLE( void ) { TRAP(); return 0; }
unsigned char ProcMemory( void ) { TRAP(); return 0; }

unsigned char ProcOldPE( void ) { TRAP(); return 0; }

unsigned char ProcPM( void ) { TRAP(); return 0; }
unsigned char ProcPMCompatible( void ) { TRAP(); return 0; }
unsigned char ProcPhysDevice( void ) { TRAP(); return 0; }
unsigned char ProcVirtDevice( void ) { TRAP(); return 0; }

unsigned char ProcTNT( void ) { TRAP(); return 0; }





unsigned char CheckFence( void ) { TRAP(); return 0; }

unsigned char ProcMinReal( void ) { TRAP(); return 0; }
unsigned char ProcMaxReal( void ) { TRAP(); return 0; }
unsigned char ProcRealBreak( void ) { TRAP(); return 0; }
unsigned char ProcCallBufs( void ) { TRAP(); return 0; }
unsigned char ProcMiniBuf( void ) { TRAP(); return 0; }
unsigned char ProcMaxiBuf( void ) { TRAP(); return 0; }
unsigned char ProcNIStack( void ) { TRAP(); return 0; }
unsigned char ProcIStkSize( void ) { TRAP(); return 0; }
unsigned char ProcUnpriv( void ) { TRAP(); return 0; }
unsigned char ProcPriv( void ) { TRAP(); return 0; }
unsigned char ProcFlags( void ) { TRAP(); return 0; }
unsigned char ProcRunNative( void ) { TRAP(); return 0; }
unsigned char ProcRunWindows( void ) { TRAP(); return 0; }

unsigned char ProcRunPosix( void ) { TRAP(); return 0; }
unsigned char ProcRunOS2( void ) { TRAP(); return 0; }
unsigned char ProcRunDosstyle( void ) { TRAP(); return 0; }



unsigned char ProcDwarfDBI( void ) { TRAP(); return 0; }

unsigned char ProcNovDBI( void ) { TRAP(); return 0; }
unsigned char ProcExportsDBI( void ) { TRAP(); return 0; }



unsigned char ProcLine( void ) { TRAP(); return 0; }
unsigned char ProcType( void ) { TRAP(); return 0; }
unsigned char ProcLocal( void ) { TRAP(); return 0; }
unsigned char ProcDBIStatic( void ) { TRAP(); return 0; }



unsigned char ProcAlignment( void ) { TRAP(); return 0; }
unsigned char ProcArea( void ) { TRAP(); return 0; }
unsigned char ProcArtificial( void ) { TRAP(); return 0; }
unsigned char ProcCase( void ) { TRAP(); return 0; }
unsigned char ProcCheck( void ) { TRAP(); return 0; }
unsigned char ProcCopyright( void ) { TRAP(); return 0; }
unsigned char ProcCustom( void ) { TRAP(); return 0; }

unsigned char ProcDistribute( void ) { TRAP(); return 0; }
unsigned char ProcDosSeg( void ) { TRAP(); return 0; }
unsigned char ProcDynamic( void ) { TRAP(); return 0; }

unsigned char ProcExit( void ) { TRAP(); return 0; }

unsigned char ProcHelp( void ) { TRAP(); return 0; }
unsigned char ProcInternalRelocs( void ) { TRAP(); return 0; }
unsigned char ProcLinearRelocs( void ) { TRAP(); return 0; }
unsigned char ProcLonglived( void ) { TRAP(); return 0; }
unsigned char ProcMangledNames( void ) { TRAP(); return 0; }

unsigned char ProcMaxData( void ) { TRAP(); return 0; }
unsigned char ProcMaxErrors( void ) { TRAP(); return 0; }
unsigned char ProcMessages( void ) { TRAP(); return 0; }
unsigned char ProcMinData( void ) { TRAP(); return 0; }
unsigned char ProcModName( void ) { TRAP(); return 0; }
unsigned char ProcMultiLoad( void ) { TRAP(); return 0; }
unsigned char ProcMultiple( void ) { TRAP(); return 0; }
unsigned char ProcNameLen( void ) { TRAP(); return 0; }
unsigned char ProcNewFiles( void ) { TRAP(); return 0; }
unsigned char ProcNoCache( void ) { TRAP(); return 0; }
unsigned char ProcNoCaseExact( void ) { TRAP(); return 0; }
unsigned char ProcNoDefLibs( void ) { TRAP(); return 0; }
unsigned char ProcNoIndirect( void ) { TRAP(); return 0; }
unsigned char ProcNoRedefs( void ) { TRAP(); return 0; }
unsigned char ProcNoRelocs( void ) { TRAP(); return 0; }
unsigned char ProcNone( void ) { TRAP(); return 0; }
unsigned char ProcOSName( void ) { TRAP(); return 0; }

unsigned char ProcOffset( void ) { TRAP(); return 0; }
unsigned char ProcOldLibrary( void ) { TRAP(); return 0; }
unsigned char ProcPackcode( void ) { TRAP(); return 0; }
unsigned char ProcPackdata( void ) { TRAP(); return 0; }
unsigned char ProcPadSections( void ) { TRAP(); return 0; }
unsigned char ProcPrivilege( void ) { TRAP(); return 0; }
unsigned char ProcProtMode( void ) { TRAP(); return 0; }
unsigned char ProcPseudoPreemption( void ) { TRAP(); return 0; }
unsigned char ProcQNXResource( void ) { TRAP(); return 0; }
unsigned char ProcQuiet( void ) { TRAP(); return 0; }
unsigned char ProcRWRelocCheck( void ) { TRAP(); return 0; }
unsigned char ProcRedefsOK( void ) { TRAP(); return 0; }
unsigned char ProcReentrant( void ) { TRAP(); return 0; }
unsigned char ProcScreenName( void ) { TRAP(); return 0; }
unsigned char ProcSelfRelative( void ) { TRAP(); return 0; }
unsigned char ProcSharelib( void ) { TRAP(); return 0; }
unsigned char ProcShowDead( void ) { TRAP(); return 0; }
unsigned char ProcSingle( void ) { TRAP(); return 0; }
unsigned char ProcSmall( void ) { TRAP(); return 0; }

unsigned char ProcStandard( void ) { TRAP(); return 0; }
unsigned char ProcStart( void ) { TRAP(); return 0; }
unsigned char ProcStatics( void ) { TRAP(); return 0; }
unsigned char ProcStub( void ) { TRAP(); return 0; }
unsigned char ProcSymFile( void ) { TRAP(); return 0; }
unsigned char ProcSynch( void ) { TRAP(); return 0; }
unsigned char ProcThreadName( void ) { TRAP(); return 0; }
unsigned char ProcToggleRelocsFlag( void ) { TRAP(); return 0; }
unsigned char ProcUndefsOK( void ) { TRAP(); return 0; }
unsigned char ProcVFRemoval( void ) { TRAP(); return 0; }


unsigned char ProcXDCData( void ) { TRAP(); return 0; }








void MakeNewSection( void ) { TRAP(); }










void OvlPass1( void ) { TRAP(); }



unsigned long CalcAlign( unsigned long loc, unsigned short align ) { TRAP(); return 0; }
member_list *GetObjMember( file_list *list, unsigned long *loc ) { TRAP(); return 0; }
void TraceSymList( symbol *list ) { TRAP(); }


int PathObjOpen( char *path, char *name, char *buffer, infilelist *list ) { TRAP(); return -1; }
void PrintIOError( unsigned error, const char *types, char *name ) { TRAP(); }


unsigned char IsORL( file_list *list, unsigned long loc ) { TRAP(); return 0; }



unsigned long ORLPass1( void ) { TRAP(); return 0; }



void DwarfP1ModuleScanned( void ) { TRAP(); }


void DwarfGenModule( void ) { TRAP(); }


void DwarfInitModule( mod_entry *mod ) { TRAP(); }






void EarlyEOF( void ) { TRAP(); }
void BadObject( void ) { TRAP(); }

void ReportMultiple( symbol *sym, char *name, int len ) { TRAP(); }




void DwarfAddGlobal( symbol *sym, unsigned len ) { TRAP(); }
void NovDBIAddGlobal( unsigned len ) { TRAP(); }
void OvlDefVector( symbol *sym ) { TRAP(); }
void DefDistribSym( symbol *sym ) { TRAP(); }
void TryRefVector( symbol *sym ) { TRAP(); }
void UndefinedSym( symbol *sym ) { TRAP(); }



void DwarfAddLines( seg_leader *seg, unsigned char is32, unsigned char isdata ) { TRAP(); }


void ProcComdef( unsigned char pass, unsigned char isstatic ) { TRAP(); }




void VectorReference( unsigned index ) { TRAP(); }


void OMFProcAlias( void ) { TRAP(); }

void MSExportKeyword( void ) { TRAP(); }
void ProcExportKeyword( void ) { TRAP(); }
void ProcImportKeyword( void ) { TRAP(); }

void DoLazyExtdef( unsigned char isweak ) { TRAP(); }
void DBIComment( void ) { TRAP(); }

void ProcVFTableRecord( unsigned char ispure ) { TRAP(); }
void ProcVFReference( void ) { TRAP(); }








unsigned char CheckOvlClass( char *name, unsigned char *isovl ) { TRAP(); return 0; }








void DwarfAddAddrInfo( seg_leader *seg, unsigned long start ) { TRAP(); }





void WriteAbsSegs( class_entry *class ) { TRAP(); }
void ProcOvlSectPubs( section *sect ) { TRAP(); }
void ProcOvlPubs( void ) { TRAP(); }
void WriteUndefHead( void ) { TRAP(); }
void WriteOvlHead( void ) { TRAP(); }


void DwarfAddModule( mod_entry *mod, section *sect ) { TRAP(); }
void DwarfGenGlobal( symbol *sym, section *sect ) { TRAP(); }
void NovDBIGenGlobal( symbol *sym ) { TRAP(); }
void WriteSymArray( symbol **syms, unsigned num ) { TRAP(); }


void forceSuppression( output_desc *data ) { TRAP(); }
void unforceSuppression( output_desc *data ) { TRAP(); }

void EmitOvlVectors( void ) { TRAP(); }
void PSection( section *sect ) { TRAP(); }
void ProcDistMods( void ) { TRAP(); }
void SetOvlStartAddr( void ) { TRAP(); }




void ZeroAddrInfo( debug_info *dptr ) { TRAP(); }



void ORLPass2( void ) { TRAP(); }



void ProcBackpatch( void ) { TRAP(); }
void ProcNamedBackpatch( void ) { TRAP(); }






void FloatReloc( void *data ) { TRAP(); }
void QNXLinearReloc( group_entry *group, void *data ) { TRAP(); }

void DwarfGenAddrInfo( seg_leader *seg, unsigned long start, unsigned long padding ) { TRAP(); }


void DwarfP2Module( void ) { TRAP(); }








void OvlUseVector( symbol *sym, extnode *node ) { TRAP(); }



void QueueFixupThread( fixup_ref **list, fixup_ref *ref ) { TRAP(); }

fixup_ref *FindFixupThread( fixup_ref *list, unsigned char index ) { TRAP(); return 0; }



void GetVecAddr2( unsigned index, fixup_ref *ref ) { TRAP(); }

void GetVecAddr( unsigned index, address *addr ) { TRAP(); }


unsigned short FindGroupIdx( unsigned short seg ) { TRAP(); return 0; }
void AddNovImpReloc( symbol *sym, unsigned long off, unsigned char relative, unsigned char isdata ) { TRAP(); }



unsigned short __cdecl FindEntryOrdinal( address addr, group_entry *group ) { TRAP(); return 0; }
unsigned char FindOS2ExportSym( symbol *sym, dll_sym_info **dll ) { TRAP(); return 0; }
void RemoveReloc( pending_reloc *rel ) { TRAP(); }
void MakeBase( fixup_record *fix ) { TRAP(); }
unsigned long GetElfImport( symbol *sym ) { TRAP(); return 0; }


struct os2_reloc_header **OS2FlatRelocInit( unsigned long size ) { TRAP(); return 0; }


void SpillWrite( unsigned long spill, unsigned long off, void *data, unsigned size ) { TRAP(); }

unsigned long SpillAlloc( unsigned size ) { TRAP(); return 0; }

void DwarfGenLines( seg_leader *seg, unsigned long adjust, unsigned char is32 ) { TRAP(); }

int CmpLn386( const void *a, const void *b ) { TRAP(); return 0; }
int CmpLn286( const void *a, const void *b ) { TRAP(); return 0; }


void RestoreLinkerComdat( symbol *sym ) { TRAP(); }

unsigned char ProcInitGlobal( void ) { TRAP(); return 0; }

unsigned char ProcInitInstance( void ) { TRAP(); return 0; }


unsigned char ProcTermGlobal( void ) { TRAP(); return 0; }

unsigned char ProcTermInstance( void ) { TRAP(); return 0; }








unsigned char ProcQNXHeapSize( void ) { TRAP(); return 0; }









unsigned char ProcELFImport( void ) { TRAP(); return 0; }

unsigned char ProcNovImport( void ) { TRAP(); return 0; }







unsigned long CalcLIDataLength( void ) { TRAP(); return 0; }







seg_leader *AddToLinkerComdat( symbol *sym ) { sym=sym;TRAP();return 0; }


unsigned char CacheRelease( void ) { TRAP(); return 0; }
unsigned char SwapOutVirt( void ) { TRAP(); return 0; }
unsigned char SwapOutRelocs( void ) { TRAP(); return 0; }
void RestoreComdat( symbol *sym, segdata *seg ) { TRAP(); }
void ODBIAddModLines( unsigned count ) { TRAP(); }
void DwarfAddModLines( unsigned count ) { TRAP(); }
int based_encoding( output_desc *data, state_desc *state ) { TRAP(); return 0; }
int array( output_desc *data, state_desc *state ) { TRAP(); return 0; }
char prevChar( output_desc *data ) { TRAP(); return 0; }
int dtor_or_delete( output_desc *data, state_desc *state ) { TRAP(); return 0; }
int op_new( output_desc *data, state_desc *state ) { TRAP(); return 0; }
int assignment_function( output_desc *data, state_desc *state ) { TRAP(); return 0; }
int relational_function( output_desc *data, state_desc *state ) { TRAP(); return 0; }
int operator_function( output_desc *data, state_desc *state ) { TRAP(); return 0; }
int special_type_names( output_desc *data, state_desc *state ) { TRAP(); return 0; }
int watcom_object( output_desc *data, state_desc *state ) { TRAP(); return 0; }
int check_recurse( void ) { TRAP(); return 0; }
int recursive_mangled_name( output_desc *data, state_desc *state ) { TRAP(); return 0; }
int base_32_num( output_desc *data, int *value ) { TRAP(); return 0; }
