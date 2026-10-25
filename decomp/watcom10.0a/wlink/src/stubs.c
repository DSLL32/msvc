/* Named application callees remain trapped until reached and reconstructed. */
#include "link.h"
#include <recon.h>

#if defined(__WATCOMC__)
void reconstruction_trap( void );
#pragma aux reconstruction_trap = 0xcc;
#define TRAP() reconstruction_trap()
#else
#define TRAP() __asm__ __volatile__( "int3" )
#endif

void QClose( int handle, char *name ) { handle = handle; name = name; TRAP(); }
void QDelete( char *name ) { name = name; TRAP(); }
void _LnkFree( void *memory ) { memory = memory; TRAP(); }
void BurnSystemList( void ) { TRAP(); }
void FreeList( path_entry *list ) { list = list; TRAP(); }
void CloseSpillFile( void ) { TRAP(); }
void CleanTraces( void ) { TRAP(); }
void FreePaths( void ) { TRAP(); }
void FreeUndefs( void ) { TRAP(); }
void CleanLoadFile( void ) { TRAP(); }
void CleanLinkStruct( void ) { TRAP(); }
void FreeFormatStuff( void ) { TRAP(); }
void FreeObjInfo( void ) { TRAP(); }
void FreeVirtMem( void ) { TRAP(); }
void FiniMsg( void ) { TRAP(); }

void TrapBreak( int value ) { value = value; TRAP(); }
void StartTime( void ) { TRAP(); }
void DoCmdFile( char *text ) { text = text; TRAP(); }
void CheckErr( void ) { TRAP(); }
void MapInit( void ) { TRAP(); }
void ProcObjFiles( void ) { TRAP(); }
void DoDefaultSystem( void ) { TRAP(); }
void FindLibPaths( void ) { TRAP(); }
void ResolveUndefined( void ) { TRAP(); }
void DecideFormat( void ) { TRAP(); }
void SetFormat( void ) { TRAP(); }
void ConvertLazyRefs( void ) { TRAP(); }
void PrepareSegments( void ) { TRAP(); }
void CheckSymbolPass( int value ) { value = value; TRAP(); }
void DefBSSSyms( void ) { TRAP(); }
void ReportUndefined( void ) { TRAP(); }
void PreAddrCalcFormatSpec( void ) { TRAP(); }
void CheckClassOrder( void ) { TRAP(); }
void SetStkSize( void ) { TRAP(); }
void AutoGroup( void ) { TRAP(); }
void CalcAddresses( void ) { TRAP(); }
void GetBSSSize( void ) { TRAP(); }
void GetStkAddr( void ) { TRAP(); }
void PostAddrCalcFormatSpec( void ) { TRAP(); }
void ReportLinkMap( void ) { TRAP(); }
void InitLoadFile( void ) { TRAP(); }
void ObjPass2( void ) { TRAP(); }
void FiniLoadFile( void ) { TRAP(); }
void EndTime( void ) { TRAP(); }
void LnkMsg( unsigned message, const char *types, ... ) { message = message; types = types; TRAP(); }
void LnkFilesInit( void ) { TRAP(); }
void InitNodes( void ) { TRAP(); }
void InitTokBuff( void ) { TRAP(); }
void InitSpillFile( void ) { TRAP(); }
void InitSym( void ) { TRAP(); }
void * NewSection( void ) { TRAP(); return 0; }
void InitObjIO( void ) { TRAP(); }

unsigned char FreeUpMemory( void ) { TRAP(); return 0; }
long QPos( int file ) { file = file; TRAP(); return 0; }
void __FreeTypeList( WResDir dir ) { dir = dir; TRAP(); }
int WResIsWResFile( int file ) { file = file; TRAP(); return 0; }
int readWResDir( int file, WResDir dir ) { file = file; dir = dir; TRAP(); return 0; }
int readMResDir( int file, WResDir dir, int *duplicates )
{ file = file; dir = dir; duplicates = duplicates; TRAP(); return 0; }
unsigned QWrite( int file, void *buffer, unsigned length, char *name )
{
    file = file; buffer = buffer; length = length; name = name;
    TRAP();
    return 0;
}


void WresRecordError( int status ) { status = status; TRAP(); }
