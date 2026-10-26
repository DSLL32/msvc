#ifndef WLIB_TOOLSTATE_H
#define WLIB_TOOLSTATE_H
#include "wlib.h"
#include <recon.h>

/* The older filename cache predates the released nwlib architecture.
   Names are descriptive; packed offsets follow its path/name consumers. */
#pragma pack(1)
typedef struct PathName {
    unsigned char DirectoryLength;
    char *Directory;
    unsigned char ExtensionLength;
    char *Extension;
} PathName;
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
#pragma pack(1)
typedef struct ImportCommand {
    FileName *Next;
    unsigned char NameLength;
    char *Module;
    char *Symbol;
    unsigned long Ordinal;
    unsigned char Flags;
    char *Entry;
} ImportCommand;
typedef struct MemberCommand {
    struct MemberCommand *Next;
    char *Name;
    unsigned char NameLength;
    unsigned char Flags;
} MemberCommand;
#pragma pack()
typedef char WlibPathNameSize[(sizeof(PathName) == 10) ? 1 : -1];
typedef char WlibImportCommandSize[(sizeof(ImportCommand) == 22) ? 1 : -1];
typedef char WlibMemberCommandSize[(sizeof(MemberCommand) == 10) ? 1 : -1];
typedef char WlibFileNameSize[(sizeof(FileName) == 22) ? 1 : -1];

#pragma pack(1)
typedef struct SymbolRecord {
    struct SymbolRecord *Next;
    union {
        struct SymbolRecord *Owner;
        unsigned short Page;
    } Value;
    unsigned char Flags;
    unsigned char NameLength;
    char Name[1];
} SymbolRecord;
#pragma pack()

typedef struct ModuleIndex {
    struct ModuleIndex *Next;
    SymbolRecord *Symbol;
} ModuleIndex;
extern ModuleIndex *ModuleIndexTail;

typedef struct OpenFile {
    unsigned char *Buffer;
    unsigned char *Current;
    int Available;
    int Handle;
    unsigned long Position;
    unsigned Mode;
} OpenFile;

typedef char WlibOpenFileSize[(sizeof(OpenFile) == 24) ? 1 : -1];

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
extern FileName *LibraryName, *ListingName, *OutputLibraryName, *TemporaryName;
extern FileName *CurrentName;
extern FileName *CurrentExtractName;
extern int ListingHandle;
extern unsigned short RequestedPageSize, LibraryPageSize;
extern struct WorkItem *CommandWork, *MemberWork, *ExtractWork;
extern unsigned long SymbolCount, DictionarySize;
extern unsigned char UpperCaseMap[256];
extern int (*NamesEqual)( const unsigned char *, const unsigned char *,
                          unsigned, unsigned );
extern unsigned char FirstCommand, InCommand;
#pragma pack(1)
typedef struct CommonBlock {
    struct CommonBlock *Next;
    unsigned short Segment;
} CommonBlock;
#pragma pack()
extern CommonBlock *CurrCommonBlk;
extern WorkItem *NameHash[27], *SymbolHash[27];
extern WorkItem *ModuleIndexHead;
void FreeWorkList( WorkItem *head );
void FreePool( MemoryPoolBlock *head );
void FreeHashTable( WorkItem **buckets );
void FreeLargeBlocks( void );
void FreeCommonBlk( void );
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
unsigned char ParseArguments( void );
unsigned char NextNonwhite( void );
unsigned char ReadArgumentCharacter( void );
unsigned char ParseOptions( unsigned char ch );
unsigned char ParseLibraryName( unsigned char ch );
unsigned char ParseAddCommand( void );
unsigned char ParseImportCommand( void );
unsigned char ReadNamePart( unsigned char ch, char *buffer );
char *DuplicateText( const char *text );
unsigned char ParseMinusCommand( void );
FileName *AddModule( const char *text );
FileName *AddExtract( const char *text );
void AddRemoveCommand( FileName *name, unsigned char owns_name );
unsigned char ParseExtractCommand( void );
typedef struct CommandInput {
    char *Current;
    struct OpenFile *File;
    unsigned char (*Reader)( void );
} CommandInput;
extern struct OpenFile *CommandFile;
extern char FileErrorName[80];
unsigned char ReadStringCharacter( void );
unsigned char ReadFileCommandCharacter( void );
void SetFileName( FileName *name, PathName *defaults, const char *text );
int OpenLibraryFile( FileName *name, unsigned flags );
void BuildFileName( FileName *name );
unsigned char *ReadBuffered( OpenFile *file, int size );
unsigned char BeginIndirect( FileName *name, CommandInput *saved );
unsigned char ParseIndirect( void );
unsigned char SkipComment( void );
unsigned char ParseModuleName( unsigned char ch );
unsigned char SetImportOption( void );
unsigned char SetListingOption( void );
unsigned char SetPageOption( void );
unsigned char ReadToken( unsigned char ch, char *buffer );
unsigned char ScanToken( unsigned char ch, char *buffer, unsigned char stop );
unsigned char SetOutputOption( void );
unsigned char SetExtractionDirectory( void );
void ReportSyntaxError( unsigned message, unsigned context, unsigned char *ch );
int NamesEqualCaseSensitive( const unsigned char *, const unsigned char *, unsigned, unsigned );
extern FileName *ExplodeName;
extern PathName ExtractionPath, LibraryPath, ListingPath;
extern unsigned char (*CommandReader)( void );
int NamesEqualIgnoreCase( const unsigned char *first, const unsigned char *second,
                          unsigned first_length, unsigned second_length );
unsigned char ParseCommandLine( void );
void PrintBanner( void );
unsigned char ValidateCommands( void );
unsigned char ValidateInputLibrary( void );
void OpenListing( void );
unsigned char OpenOutput( void );
int CreateTemporaryLibrary( FileName *name );
void PrepareLibrary( void );
extern unsigned short InputLibraryPageSize;
unsigned char ReadLibraryHeader( void );
void SeekBuffered( OpenFile *file, unsigned long position );
void WriteBufferedByte( OpenFile *file, unsigned char ch );
void WriteBuffered( OpenFile *file, const void *data, int size );
void PadLibraryPage( unsigned size );
unsigned long BufferedPosition( OpenFile *file );
void ProcessAddedModules( void );
void BuildImportMember( ImportCommand *command );
extern unsigned long CurrentMemberOffset;
int AddMemberSymbol( const char *name, unsigned length, unsigned module_name );
extern SymbolRecord *CurrentModuleSymbol;
int InsertSymbol( SymbolRecord *symbol, unsigned module_name );
unsigned HashSymbol( SymbolRecord *symbol );
int IsCommonRef( void );
void ReportDuplicateSymbol( SymbolRecord *symbol, unsigned module_name );
void AddModuleIndex( SymbolRecord *symbol );
void CheckLibrarySize( void );
int ReadBufferedByteStrict( OpenFile *file );
unsigned long NextLibraryPage( void );
int LoadExecutableCache( OpenFile *file );
int OpenFileCache( int handle );
void ProcessModuleRecords( MemberCommand *remove, FileName *extract,
                           unsigned from_library );
extern unsigned long InputModuleOffset;
extern unsigned char CurrentRecordType;
void ReadOmfRecord( unsigned strict );
extern unsigned OmfRecordLength;
extern unsigned char *OmfRecordData, *OmfRecordCursor;
extern unsigned char EasyOMF;
int ReadBufferedByte( OpenFile *file );
void ProcessOneModule( MemberCommand *remove, FileName *extract,
                       unsigned from_library );
extern int SkipCurrentModule, ExtractCurrentModule;
SymbolRecord *ReadRecordName( unsigned extra );
int __is_mangled( const char *name, unsigned length );
void NormalizeModuleName( SymbolRecord *module );
unsigned MatchRemoveCommand( const char *name, unsigned length,
                             MemberCommand *remove );
void MatchExtractCommand( const char *name, unsigned length, FileName *extract );
void ProcessOmfModule( SymbolRecord *module );
void CopyLibraryMembers( void );
void PrepareOutput( void );
void BuildDictionary( void );
void FinishOutput( void );
void ReportUnprocessedMembers( void );
void ListLibrary( void );
void CloseLibraryFile( FileName *name );
void CloseFileCache( int handle );
void WriteFileBlock( OpenFile *file, const void *data, unsigned size );
void CompleteLibrary( void );
void CopyOutputFileName( void );

struct OmfName;
extern struct OmfName *LName_Head;
extern struct OmfName **LName_Owner;
extern unsigned short SegDefCount, CurrSegRef;
extern unsigned char Rec32Bit;
void procsegdef( void );
void getpubdef( void );
void GetComent( void );
void getcomdef( void );
void GetComLen( void );
void getcomdat( void );
void getlname( unsigned local );
void CopyOmfRecord( SymbolRecord *module, unsigned trim_name );
void FreeLNames( void );
void GetIdx( void );
void GetOffset( void );
unsigned short GetIndex( void );
extern unsigned long DictionaryOffset;
extern unsigned short DictionaryBlockCount;
unsigned NextPrime( unsigned candidate );
void InitDictionaryBlocks( unsigned count );
int InsertModuleDictionary( void );
int InsertPublicDictionary( void );
void WriteDictionaryBlocks( void );
void FreeDictionaryBlocks( void );
int isPrime( unsigned candidate );
void InitBlockTable( unsigned short count );
extern int AllocatedDictionaryBlocks;
typedef struct DictionaryBlock {
    unsigned char Buckets[37];
    unsigned char NextFree;
    unsigned char Data[474];
} DictionaryBlock;
typedef char WlibDictionaryBlockSize[(sizeof(DictionaryBlock) == 512) ? 1 : -1];
extern DictionaryBlock **DictionaryBlocks;
extern unsigned short DictionaryBlockSlots;
int InsertDictionarySymbol( SymbolRecord *symbol, unsigned short page );
void HashDictionaryName( SymbolRecord *symbol );
DictionaryBlock *FindDictionarySpace( unsigned words );
void CommitDictionaryBlock( void );
extern unsigned short DictionaryBucket;
extern unsigned short DictionaryBucketStep, DictionaryBlockIndex, DictionaryBlockStep;
DictionaryBlock *GetDictionaryBlock( unsigned short index );
void InitDictionaryBlock( DictionaryBlock *block );
char *CopyCountedName( int length, const char *name );
int CompareSymbols( const void *first, const void *second );
int CompareModuleSymbols( const void *first, const void *second );
void PrintPublicListing( SymbolRecord **symbols, unsigned count );
void PrintModuleListing( SymbolRecord **symbols, unsigned count );
int CompareNames( const unsigned char *first, const unsigned char *second );
void WriteListingText( const char *text, unsigned length, int handle );
void FillListing( int count, unsigned char fill );
void ListingNewline( int handle );
void WriteListingCharacter( unsigned char ch, int handle );
extern int ListingColumn;
void PrintModuleHeading( SymbolRecord *module );
void PrintModuleSymbol( SymbolRecord *symbol );
#endif
