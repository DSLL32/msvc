/* Named application callees remain trapped until reached and reconstructed. */
#include "diagnostic.h"
#include "demangle.h"
#include "fixup.h"
#include "peheader.h"
#include "exeflat.h"
#include "exene.h"
#include "dbgdwarf.h"
#include "strtab.h"
#include "exenov.h"
#include "exeqnx.h"
#include "dbgdwarf.h"
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


unsigned char CheckVFList( symbol *sym ) { TRAP(); return 0; }















void CheckBSSInStart( symbol *sym, char *name ) { TRAP(); }
void PhoneyStack( void ) { TRAP(); }





void SpillRead( unsigned long loc, unsigned long off, void *buffer, unsigned size ) { TRAP(); }





void Locator( char *name, char *member, unsigned rec )
{ name=name; member=member; rec=rec; TRAP(); }
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















void CheckFileTrace( section *sect ) { sect=sect; TRAP(); }





























void Syntax( void ) { TRAP(); }














unsigned char ProcDisable( void ) { TRAP(); return 0; }
unsigned char ProcSort( void ) { TRAP(); return 0; }
unsigned char ProcLanguage( void ) { TRAP(); return 0; }

unsigned char ProcAnonExport( void ) { TRAP(); return 0; }

unsigned char GetWlibImports( void ) { TRAP(); return 0; }


unsigned char ProcFixedLib( void ) { TRAP(); return 0; }
unsigned char ProcNoVector( void ) { TRAP(); return 0; }
unsigned char ProcVector( void ) { TRAP(); return 0; }
unsigned char ProcForceVector( void ) { TRAP(); return 0; }

unsigned char ProcNewSegment( void ) { TRAP(); return 0; }




void BackupParser( void ) { TRAP(); }



void MapEscapeChar( void ) { TRAP(); }





unsigned char QReadStr( int file, char *buff, unsigned len, char *name )
{ file=file; buff=buff; len=len; name=name; TRAP(); return 0; }
void OutPutPrompt( char *text ) { text=text; TRAP(); }









unsigned char ProcFont( void ) { TRAP(); return 0; }
unsigned char ProcFullScreen( void ) { TRAP(); return 0; }

unsigned char ProcMemory( void ) { TRAP(); return 0; }





unsigned char ProcPhysDevice( void ) { TRAP(); return 0; }
unsigned char ProcVirtDevice( void ) { TRAP(); return 0; }

unsigned char ProcTNT( void ) { TRAP(); return 0; }





unsigned char CheckFence( void ) { TRAP(); return 0; }




unsigned char ProcRunPosix( void ) { TRAP(); return 0; }
unsigned char ProcRunOS2( void ) { TRAP(); return 0; }
unsigned char ProcRunDosstyle( void ) { TRAP(); return 0; }





unsigned char ProcExportsDBI( void ) { TRAP(); return 0; }



unsigned char ProcLine( void ) { TRAP(); return 0; }
unsigned char ProcType( void ) { TRAP(); return 0; }
unsigned char ProcLocal( void ) { TRAP(); return 0; }
unsigned char ProcDBIStatic( void ) { TRAP(); return 0; }



unsigned char ProcArea( void ) { TRAP(); return 0; }
unsigned char ProcArtificial( void ) { TRAP(); return 0; }


unsigned char ProcDynamic( void ) { TRAP(); return 0; }


unsigned char ProcInternalRelocs( void ) { TRAP(); return 0; }
unsigned char ProcMangledNames( void ) { TRAP(); return 0; }

unsigned char ProcMaxErrors( void ) { TRAP(); return 0; }


unsigned char ProcNameLen( void ) { TRAP(); return 0; }

unsigned char ProcNoCache( void ) { TRAP(); return 0; }
unsigned char ProcNoCaseExact( void ) { TRAP(); return 0; }

unsigned char ProcNoIndirect( void ) { TRAP(); return 0; }
unsigned char ProcNoRedefs( void ) { TRAP(); return 0; }
unsigned char ProcNoRelocs( void ) { TRAP(); return 0; }
unsigned char ProcNone( void ) { TRAP(); return 0; }




unsigned char ProcQuiet( void ) { TRAP(); return 0; }
unsigned char ProcRWRelocCheck( void ) { TRAP(); return 0; }
unsigned char ProcRedefsOK( void ) { TRAP(); return 0; }
unsigned char ProcSelfRelative( void ) { TRAP(); return 0; }
unsigned char ProcSharelib( void ) { TRAP(); return 0; }
unsigned char ProcShowDead( void ) { TRAP(); return 0; }
unsigned char ProcSingle( void ) { TRAP(); return 0; }
unsigned char ProcSmall( void ) { TRAP(); return 0; }



unsigned char ProcToggleRelocsFlag( void ) { TRAP(); return 0; }
unsigned char ProcUndefsOK( void ) { TRAP(); return 0; }
unsigned char ProcVFRemoval( void ) { TRAP(); return 0; }

























unsigned long CalcAlign( unsigned long loc, unsigned short align ) { TRAP(); return 0; }
member_list *GetObjMember( file_list *list, unsigned long *loc ) { TRAP(); return 0; }




void PrintIOError( unsigned error, const char *types, char *name ) { TRAP(); }


unsigned char IsORL( file_list *list, unsigned long loc ) { TRAP(); return 0; }



unsigned long ORLPass1( void ) { TRAP(); return 0; }
















void EarlyEOF( void ) { TRAP(); }
void BadObject( void ) { TRAP(); }

void ReportMultiple( symbol *sym, char *name, int len ) { TRAP(); }








void UndefinedSym( symbol *sym ) { TRAP(); }












void OMFProcAlias( void ) { TRAP(); }

void MSExportKeyword( void ) { TRAP(); }
void ProcExportKeyword( void ) { TRAP(); }
void ProcImportKeyword( void ) { TRAP(); }


void DBIComment( void ) { TRAP(); }

void ProcVFTableRecord( unsigned char ispure ) { TRAP(); }
void ProcVFReference( void ) { TRAP(); }























void WriteAbsSegs( class_entry *class ) { TRAP(); }







void WriteSymArray( symbol **syms, unsigned num ) { TRAP(); }


void forceSuppression( output_desc *data ) { TRAP(); }










void ZeroAddrInfo( debug_info *dptr ) { TRAP(); }



void ORLPass2( void ) { TRAP(); }



void ProcBackpatch( void ) { TRAP(); }
void ProcNamedBackpatch( void ) { TRAP(); }






void FloatReloc( void *data ) { TRAP(); }






























unsigned char FindOS2ExportSym( symbol *sym, dll_sym_info **dll ) { TRAP(); return 0; }






void SpillWrite( unsigned long spill, unsigned long off, void *data, unsigned size ) { TRAP(); }

unsigned long SpillAlloc( unsigned size ) { TRAP(); return 0; }



int CmpLn386( const void *a, const void *b ) { TRAP(); return 0; }
int CmpLn286( const void *a, const void *b ) { TRAP(); return 0; }


void RestoreLinkerComdat( symbol *sym ) { TRAP(); }


































unsigned long CalcLIDataLength( void ) { TRAP(); return 0; }







seg_leader *AddToLinkerComdat( symbol *sym ) { sym=sym;TRAP();return 0; }


unsigned char CacheRelease( void ) { TRAP(); return 0; }
unsigned char SwapOutVirt( void ) { TRAP(); return 0; }
unsigned char SwapOutRelocs( void ) { TRAP(); return 0; }
void RestoreComdat( symbol *sym, segdata *seg ) { TRAP(); }

void DwarfAddModLines( unsigned count ) { TRAP(); }
int based_encoding( output_desc *data, state_desc *state ) { TRAP(); return 0; }
int array( output_desc *data, state_desc *state ) { TRAP(); return 0; }
char prevChar( output_desc *data ) { TRAP(); return 0; }





int special_type_names( output_desc *data, state_desc *state ) { TRAP(); return 0; }
int watcom_object( output_desc *data, state_desc *state ) { TRAP(); return 0; }
int check_recurse( void ) { TRAP(); return 0; }
int recursive_mangled_name( output_desc *data, state_desc *state ) { TRAP(); return 0; }
int base_32_num( output_desc *data, int *value ) { TRAP(); return 0; }











unsigned char ProcReadOnly( void ) { TRAP(); return 0; }





unsigned char ProcFixed( void ) { TRAP(); return 0; }














unsigned char ProcAutoSection( void ) { TRAP(); return 0; }







unsigned char ProcInto( void ) { TRAP(); return 0; }















































void ShortVectors( symbol *sym ) { TRAP(); }





















void OpenOvlFile( outfilelist *file ) { TRAP(); }












unsigned char ProcLAN( void ) { TRAP(); return 0; }

unsigned char ProcDSK( void ) { TRAP(); return 0; }

unsigned char ProcNAM( void ) { TRAP(); return 0; }

unsigned char ProcNovDBIExports( void ) { TRAP(); return 0; }

unsigned char ProcNovDBIReferenced( void ) { TRAP(); return 0; }




unsigned char ProcQNXExecuteonly( void ) { TRAP(); return 0; }


unsigned char ProcRex( void ) { TRAP(); return 0; }


void WritePharSimple( unsigned long start ) { TRAP(); }







unsigned char ProcELFDLL( void ) { TRAP(); return 0; }

unsigned char ProcOS2Alignment( void ) { TRAP(); return 0; }
