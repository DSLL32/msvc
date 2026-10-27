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




unsigned char ProcRunOS2( void ) { TRAP(); return 0; }




































unsigned char ProcNone( void ) { TRAP(); return 0; }













































void PrintIOError( unsigned error, const char *types, char *name ) { TRAP(); }


unsigned char IsORL( file_list *list, unsigned long loc ) { TRAP(); return 0; }



unsigned long ORLPass1( void ) { TRAP(); return 0; }
















void EarlyEOF( void ) { TRAP(); }
void BadObject( void ) { TRAP(); }





























































void forceSuppression( output_desc *data ) { TRAP(); }










void ZeroAddrInfo( debug_info *dptr ) { TRAP(); }



void ORLPass2( void ) { TRAP(); }














































void SpillWrite( unsigned long spill, unsigned long off, void *data, unsigned size ) { TRAP(); }

unsigned long SpillAlloc( unsigned size ) { TRAP(); return 0; }



int CmpLn386( const void *a, const void *b ) { TRAP(); return 0; }
int CmpLn286( const void *a, const void *b ) { TRAP(); return 0; }













































unsigned char CacheRelease( void ) { TRAP(); return 0; }
unsigned char SwapOutVirt( void ) { TRAP(); return 0; }
unsigned char SwapOutRelocs( void ) { TRAP(); return 0; }


void DwarfAddModLines( unsigned count ) { TRAP(); }

























































































































void OpenOvlFile( outfilelist *file ) { TRAP(); }




































unsigned char ProcELFDLL( void ) { TRAP(); return 0; }




unsigned char ProcPENoRelocs( void ) { TRAP(); return 0; }
