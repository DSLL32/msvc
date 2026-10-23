/* Named unreconstructed application callees. Addresses and tentative name
 * correlations are recorded in cases/help/main-groundwork.json. */
#include "link.h"
#include <recon.h>

#if defined(__WATCOMC__)
void reconstruction_trap( void );
#pragma aux reconstruction_trap = 0xcc;
#define TRAP() reconstruction_trap()
#else
#define TRAP() __asm__ __volatile__( "int3" )
#endif

int Spawn( void (*callback)(void) )
{
    callback = callback;
    TRAP();
    return 0;
}

void DoLinkFromCommandLine( void ) { TRAP(); }
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
void LnkMemFini( void ) { TRAP(); }
