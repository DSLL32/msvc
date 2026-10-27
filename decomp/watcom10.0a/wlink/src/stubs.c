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
















unsigned char ProcLanguage( void ) { TRAP(); return 0; }























unsigned char QReadStr( int file, char *buff, unsigned len, char *name )
{ file=file; buff=buff; len=len; name=name; TRAP(); return 0; }
void OutPutPrompt( char *text ) { text=text; TRAP(); }










unsigned char ProcFullScreen( void ) { TRAP(); return 0; }








unsigned char ProcVirtDevice( void ) { TRAP(); return 0; }







unsigned char CheckFence( void ) { TRAP(); return 0; }




unsigned char ProcRunPosix( void ) { TRAP(); return 0; }
unsigned char ProcRunOS2( void ) { TRAP(); return 0; }






unsigned char ProcExportsDBI( void ) { TRAP(); return 0; }





























unsigned char ProcNoRedefs( void ) { TRAP(); return 0; }

unsigned char ProcNone( void ) { TRAP(); return 0; }




unsigned char ProcQuiet( void ) { TRAP(); return 0; }





































unsigned long CalcAlign( unsigned long loc, unsigned short align ) { TRAP(); return 0; }
member_list *GetObjMember( file_list *list, unsigned long *loc ) { TRAP(); return 0; }




void PrintIOError( unsigned error, const char *types, char *name ) { TRAP(); }


unsigned char IsORL( file_list *list, unsigned long loc ) { TRAP(); return 0; }



unsigned long ORLPass1( void ) { TRAP(); return 0; }
















void EarlyEOF( void ) { TRAP(); }
void BadObject( void ) { TRAP(); }

























void MSExportKeyword( void ) { TRAP(); }
void ProcExportKeyword( void ) { TRAP(); }
void ProcImportKeyword( void ) { TRAP(); }


void DBIComment( void ) { TRAP(); }

void ProcVFTableRecord( unsigned char ispure ) { TRAP(); }
void ProcVFReference( void ) { TRAP(); }























void WriteAbsSegs( class_entry *class ) { TRAP(); }










void forceSuppression( output_desc *data ) { TRAP(); }










void ZeroAddrInfo( debug_info *dptr ) { TRAP(); }



void ORLPass2( void ) { TRAP(); }



void ProcBackpatch( void ) { TRAP(); }
void ProcNamedBackpatch( void ) { TRAP(); }






void FloatReloc( void *data ) { TRAP(); }





































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


void DwarfAddModLines( unsigned count ) { TRAP(); }

























































































































void OpenOvlFile( outfilelist *file ) { TRAP(); }














unsigned char ProcDSK( void ) { TRAP(); return 0; }

unsigned char ProcNAM( void ) { TRAP(); return 0; }








unsigned char ProcQNXExecuteonly( void ) { TRAP(); return 0; }













unsigned char ProcELFDLL( void ) { TRAP(); return 0; }




unsigned char ProcPENoRelocs( void ) { TRAP(); return 0; }
