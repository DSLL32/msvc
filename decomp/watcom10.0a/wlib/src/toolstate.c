/* State initialized by original InitToolState,0x412af5..0x412c40.
   These declarations cover the globals encountered in that function rather
   than reproducing the binary's surrounding data or later nwlib subsystems. */
#include <stddef.h>
#include "toolstate.h"
#include <recon.h>

MemoryPoolBlock *NamePool;          /* 0x4301a0 */
MemoryPoolBlock *PathPool;          /* 0x43019c */
unsigned long AllocationCount;     /* 0x4302a8 */
unsigned long FreeCount;           /* 0x4302a4 */
int CommandArgumentCount;          /* 0x430304 */
char *CommandCurrent;              /* 0x430004 */
unsigned char ParseStage;          /* 0x430008 */
char **CommandArguments;           /* 0x4302e8 */
FileName *LibraryName;             /* 0x4302d4 */
FileName *OutputName;              /* 0x4302dc */
FileName *BackupName;              /* 0x4302d0 */
FileName *TemporaryName;           /* 0x4302e0 */
FileName *CurrentName;             /* 0x4302cc */
void *CurrentMember;               /* 0x4302d8 */
int ListingHandle;                 /* 0x4301b8 */
unsigned short InputPageSize;      /* 0x43030a */
unsigned short OutputPageSize;     /* 0x43030e */
struct WorkItem *CommandWork;       /* 0x4302f0 */
struct WorkItem *MemberWork;        /* 0x4302f4 */
struct WorkItem *ExtractWork;       /* 0x4302f8 */
unsigned long MemberCount;         /* 0x4302b0 */
unsigned long DictionarySize;      /* 0x430300 */
unsigned char UpperCaseMap[256];   /* 0x43004c */
int (*NamesEqual)( const unsigned char *, const unsigned char *,
                  unsigned, unsigned ); /* 0x4301c4 */
unsigned char FirstCommand;        /* 0x43031b */
unsigned char InCommand;           /* 0x43031a */
WorkItem *IndexBlocks;              /* 0x4301a4 */

void InitToolState( int argc, char **argv )
{
    int ch;

    ToolFlags = 0;
    CommandArgumentCount = argc - 1;
    CommandArguments = argv + 1;
    AllocationCount = 0;
    FreeCount = 0;
    NamePool = NULL;
    PathPool = NULL;
    LibraryName = AllocPoolItem( &NamePool, sizeof(FileName) );
    OutputName = AllocPoolItem( &NamePool, sizeof(FileName) );
    OutputName->File = NULL;
    BackupName = AllocPoolItem( &NamePool, sizeof(FileName) );
    BackupName->File = NULL;
    TemporaryName = AllocPoolItem( &NamePool, sizeof(FileName) );
    CurrentName = LibraryName;
    CurrentMember = NULL;
    ListingHandle = 1;              /* Initial descriptor at original0x4206be. */
    OutputPageSize = 0;
    InputPageSize = 0;
    CommandWork = NULL;
    MemberWork = NULL;
    ExtractWork = NULL;
    MemberCount = 0;
    DictionarySize = 0;
    for( ch = 0; ch < 256; ++ch ) {
        UpperCaseMap[ch] = ch > '`' && ch < '{' ? ch & 0xdf : ch;
    }
    NamesEqual = NamesEqualIgnoreCase;
    FirstCommand = 1;
    InCommand = 0;
    IndexBlocks = NULL;
}

WorkItem *NameHash[27];            /* Original0x430234 */
WorkItem *SymbolHash[27];          /* Original0x4301c8 */
WorkItem *LargeBlocks;             /* Original0x4302a0 */

/* Original0x413278..0x4132b4. Release each allocator/list family in
   its original order; the final call is the original tail transfer. */
void FiniToolState( void )
{
    FreeWorkList( MemberWork );
    FreePool( NamePool );
    FreePool( PathPool );
    FreeHashTable( NameHash );
    FreeHashTable( SymbolHash );
    FreeLargeBlocks();
    FreeIndexList();
}

/* Original0x4132b4..0x4132c6; preserve the next pointer before freeing. */
void FreeWorkList( WorkItem *head )
{
    WorkItem *next;

    while( head != NULL ) {
        next = head->Next;
        FreeMemory( head );
        head = next;
    }
}

/* Original0x4132c6..0x4132ea. Both original hash tables have27 buckets;
   the list heads are intentionally not rewritten during final cleanup. */
void FreeHashTable( WorkItem **buckets )
{
    WorkItem **end, *head, *next;

    end = buckets + 27;
    do {
        head = *buckets;
        while( head != NULL ) {
            next = head->Next;
            FreeMemory( head );
            head = next;
        }
        ++buckets;
    } while( buckets != end );
}

/* Original0x4132ea..0x413301. LargeBlocks remains a stale finalization
   head in the original; this function walks a local pointer. */
void FreeLargeBlocks( void )
{
    WorkItem *head, *next;

    head = LargeBlocks;
    while( head != NULL ) {
        next = head->Next;
        FreeMemory( head );
        head = next;
    }
}

/* Original entry0x4119f4 shares the preceding function's POPEDX/RET
   epilogue at0x4119f2. Its full encountered code union is32B ending411a12. */
void FreeIndexList( void )
{
    WorkItem *next;

    while( IndexBlocks != NULL ) {
        next = IndexBlocks->Next;
        FreeMemory( IndexBlocks );
        IndexBlocks = next;
    }
}
