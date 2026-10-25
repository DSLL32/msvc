#ifndef WLIB_TOOLSTATE_H
#define WLIB_TOOLSTATE_H
#include "wlib.h"
#include <recon.h>

/* The older filename cache predates the released nwlib architecture.
   Names are descriptive; packed offsets follow its path/name consumers. */
struct PathName;
#pragma pack(1)
typedef struct FileName {
    struct FileName *Next;
    unsigned char NameLength;
    char *Name;
    struct PathName *Path;
    struct OpenFile *File;
    unsigned char Flags;
    unsigned long FileOffset;
} FileName;
#pragma pack()
typedef char WlibFileNameSize[(sizeof(FileName) == 22) ? 1 : -1];

typedef struct MemoryPoolBlock {
    struct MemoryPoolBlock *Next;
    int UsedIndex;
    unsigned char Items[1];
} MemoryPoolBlock;
typedef struct WorkItem {
    struct WorkItem *Next;
} WorkItem;
extern MemoryPoolBlock *NamePool, *PathPool;
extern unsigned long AllocationCount, FreeCount;
extern int CommandArgumentCount;
extern char **CommandArguments;
extern char *CommandCurrent;
extern unsigned char ParseStage;
extern FileName *LibraryName, *OutputName, *BackupName, *TemporaryName;
extern FileName *CurrentName;
extern void *CurrentMember;
extern int ListingHandle;
extern unsigned short InputPageSize, OutputPageSize;
extern struct WorkItem *CommandWork, *MemberWork, *ExtractWork;
extern unsigned long MemberCount, DictionarySize;
extern unsigned char UpperCaseMap[256];
extern int (*NamesEqual)( const unsigned char *, const unsigned char *,
                          unsigned, unsigned );
extern unsigned char FirstCommand, InCommand;
extern WorkItem *IndexBlocks;
extern WorkItem *NameHash[27], *SymbolHash[27];
extern WorkItem *LargeBlocks;
void FreeWorkList( WorkItem *head );
void FreePool( MemoryPoolBlock *head );
void FreeHashTable( WorkItem **buckets );
void FreeLargeBlocks( void );
void FreeIndexList( void );
void *AllocPoolItem( MemoryPoolBlock **pool, unsigned item_size );
void *AllocMemory( unsigned size );
void __cdecl ReportError( unsigned destination, unsigned message, ... );
void AbortCommand( void );
#if defined(__WATCOMC__)
/* The original failure path transfers to longjmp and never returns. */
#pragma aux AbortCommand aborts;
#endif
void FreeMemory( void *memory );
void PrintUsage( void );
void PrintTextLine( const char *text );
void CheckBreak( void );
extern int BreakRequested;
void Usage( void );
void PauseOnConsole( void );
void __cdecl PrintMessage( unsigned message, ... );
void ParseArguments( void );
int NamesEqualIgnoreCase( const unsigned char *first, const unsigned char *second,
                          unsigned first_length, unsigned second_length );
unsigned char ParseCommandLine( void );
void PrintBanner( void );
unsigned char ValidateCommands( void );
void PrepareLibrary( void );
void PrepareOutput( void );
void BuildDictionary( void );
void FinishOutput( void );
void ReportUnprocessedMembers( void );
void ListLibrary( void );
void CloseLibraryFile( FileName *name );
void CompleteLibrary( void );
#endif
