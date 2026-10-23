#ifndef WLINK_LINK_H
#define WLINK_LINK_H

/* Only the encountered outfilelist prefix is known at this frontier. */
typedef struct outfilelist {
    struct outfilelist *next;
    char *fname;
    int handle;
} outfilelist;
typedef struct path_entry path_entry;

typedef char link_pointer_is_32[(sizeof(void *) == 4) ? 1 : -1];
typedef char link_int_is_32[(sizeof(int) == 4) ? 1 : -1];
typedef char link_state_is_16[(sizeof(unsigned short) == 2) ? 1 : -1];

extern char **Argv;
extern int MapFile;
extern char *MapFName;
extern outfilelist *OutFiles;
extern path_entry *LibPath;
extern unsigned short LinkState;

extern int Spawn( void (*)(void) );
extern void DoLinkFromCommandLine( void );
extern void QClose( int, char * );
extern void QDelete( char * );
extern void _LnkFree( void * );
extern void BurnSystemList( void );
extern void FreeList( path_entry * );
extern void CloseSpillFile( void );
extern void CleanTraces( void );
extern void FreePaths( void );
extern void FreeUndefs( void );
extern void CleanLoadFile( void );
extern void CleanLinkStruct( void );
extern void FreeFormatStuff( void );
extern void FreeObjInfo( void );
extern void FreeVirtMem( void );
extern void FiniMsg( void );
extern void LnkMemFini( void );
extern void exit( int );
#if defined(__WATCOMC__)
/* Genuine CRT exit does not return. Watcom10 supports this declaration;
 * its shipped SETJMP.H uses the same annotation for longjmp. */
#pragma aux exit aborts;
#endif

#include <recon.h>
#endif
