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

extern void LnkInit( void );
extern void TrapBreak( int );
extern void StartTime( void );
extern void DoCmdFile( char * );
extern void CheckErr( void );
extern void MapInit( void );
extern void ProcObjFiles( void );
extern void DoDefaultSystem( void );
extern void FindLibPaths( void );
extern void ResolveUndefined( void );
extern void DecideFormat( void );
extern void SetFormat( void );
extern void ConvertLazyRefs( void );
extern void PrepareSegments( void );
extern void CheckSymbolPass( int );
extern void DefBSSSyms( void );
extern void ReportUndefined( void );
extern void PreAddrCalcFormatSpec( void );
extern void CheckClassOrder( void );
extern void SetStkSize( void );
extern void AutoGroup( void );
extern void CalcAddresses( void );
extern void GetBSSSize( void );
extern void GetStkAddr( void );
extern void PostAddrCalcFormatSpec( void );
extern void ReportLinkMap( void );
extern void InitLoadFile( void );
extern void ObjPass2( void );
extern void FiniLoadFile( void );
extern void EndTime( void );
extern void LnkMsg( unsigned, const char *, ... );
extern int InitMsg( char * );
extern void LnkFilesInit( void );
extern void InitNodes( void );
extern void InitTokBuff( void );
extern void InitSpillFile( void );
extern void InitSym( void );
extern void * NewSection( void );
extern void InitObjIO( void );
typedef struct handle_info {
    int handle;
    char *filename;
} HANDLE_INFO;
extern int OpenResFile( HANDLE_INFO * );
extern int ResOpenFileRO( const char * );
extern void WresRecordError( int );
extern int FindResources( HANDLE_INFO * );
extern long ResSeek( int, long, int );
extern long QLSeek( int, long, int, char * );
extern int InitResources( HANDLE_INFO * );
extern unsigned QWrite( int, void *, unsigned, char * );
#include <recon.h>
#endif
