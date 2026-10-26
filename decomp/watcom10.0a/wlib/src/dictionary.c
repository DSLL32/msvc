/* OMF library dictionary construction in the1994 librarian. */
#include <string.h>
#include "toolstate.h"
#include <recon.h>

unsigned long DictionaryOffset;       /* Original0x4302ec. */
unsigned short DictionaryBlockCount;  /* Original0x430318. */

/* Original0x412f52..0x412fe1. Start from both bucket and byte-density
   estimates; a failed insertion retries with a larger prime block count. */
void BuildDictionary( void )
{
    unsigned count, required;
    int full;

    DictionaryOffset = BufferedPosition( OutputLibraryName->File );
    count = SymbolCount / 36 + 1;
    required = (3 * SymbolCount + DictionarySize) / 474 + 1;
    if( (int)required > (int)count ) count = required;
    do {
        DictionaryBlockCount = NextPrime( count );
        InitDictionaryBlocks( DictionaryBlockCount );
        full = InsertModuleDictionary();
        if( !full ) full = InsertPublicDictionary();
        if( !full ) WriteDictionaryBlocks();
        FreeDictionaryBlocks();
        if( full ) count = DictionaryBlockCount + 1;
    } while( full );
}

/* Original0x41313e..0x41315c keeps the single-block dictionary as1;
   larger candidates use an odd prime block count. */
unsigned NextPrime( unsigned candidate )
{
    if( (int)candidate > 1 ) {
        candidate |= 1;
        while( !isPrime( candidate ) ) candidate += 2;
    }
    return( candidate );
}

int AllocatedDictionaryBlocks;        /* Original0x430048. */

/* Original0x41315c..0x41316f. */
void InitDictionaryBlocks( unsigned count )
{
    InitBlockTable( (unsigned short)count );
    AllocatedDictionaryBlocks = count;
}

DictionaryBlock **DictionaryBlocks;   /* Original0x430344. */
unsigned short DictionaryBlockSlots;  /* Original0x430348. */

/* Original0x414b16..0x414b41. Dictionary blocks are allocated lazily;
   the initial table contains only NULL owning pointers. */
void InitBlockTable( unsigned short count )
{
    unsigned bytes;

    bytes = count * sizeof(*DictionaryBlocks);
    DictionaryBlocks = AllocMemory( bytes );
    memset( DictionaryBlocks, 0, bytes );
    DictionaryBlockSlots = count;
}

/* Original0x412fe1..0x413038. Module dictionary names use a final'!'
   discriminator; restore the in-memory name even when the table is full. */
int InsertModuleDictionary( void )
{
    SymbolRecord *symbol;
    unsigned bucket, length;
    int full;

    for( bucket = 0; bucket < 27; ++bucket ) {
        for( symbol = (SymbolRecord *)NameHash[bucket]; symbol != NULL;
             symbol = symbol->Next ) {
            length = symbol->NameLength;
            ++symbol->NameLength;
            symbol->Name[length] = '!';
            full = InsertDictionarySymbol( symbol, symbol->Value.Page );
            --symbol->NameLength;
            symbol->Name[symbol->NameLength] = '\0';
            if( full ) return( full );
        }
    }
    return( 0 );
}

unsigned short DictionaryBucket;     /* Original0x430310. */

/* Original0x413071..0x413103. Each dictionary slot contains a counted
   name followed by its little-endian module page, padded to two-byte units. */
int InsertDictionarySymbol( SymbolRecord *symbol, unsigned short page )
{
    DictionaryBlock *block;
    unsigned length, words, next, i;
    unsigned char *record;

    HashDictionaryName( symbol );
    length = symbol->NameLength;
    words = (length + 4) >> 1;
    block = FindDictionarySpace( words );
    if( block == NULL ) return( 1 );
    CommitDictionaryBlock();
    block->Buckets[DictionaryBucket] = block->NextFree;
    record = (unsigned char *)block + 2 * block->NextFree;
    next = block->NextFree + words;
    block->NextFree = next > 255 ? 255 : next;
    *record++ = length;
    for( i = 0; i < length; ++i ) *record++ = symbol->Name[i];
    *record++ = page;
    *record = page >> 8;
    return( 0 );
}

unsigned short DictionaryBucketStep; /* Original0x43030c. */
unsigned short DictionaryBlockIndex; /* Original0x430316. */
unsigned short DictionaryBlockStep;  /* Original0x430312. */

/* Original0x412e49..0x412f34. OMF's dictionary hash folds with OR0x20
   and uses four independent16-bit rotate/XOR accumulators. */
void HashDictionaryName( SymbolRecord *symbol )
{
    const unsigned char *left, *right;
    unsigned count;
    unsigned short block, block_step, bucket, bucket_step, ch;

    count = symbol->NameLength;
    left = (const unsigned char *)symbol->Name;
    right = left + count;
    block = bucket_step = count | 0x20;
    block_step = bucket = 0;
    for( ;; ) {
        ch = *--right | 0x20;
        block_step = ch ^ ((block_step << 2) | (block_step >> 14));
        bucket = ch ^ ((bucket >> 2) | (bucket << 14));
        if( --count == 0 ) break;
        ch = *left++ | 0x20;
        block = ch ^ ((block << 2) | (block >> 14));
        bucket_step = ch ^ ((bucket_step >> 2) | (bucket_step << 14));
    }
    DictionaryBucket = bucket % 37;
    DictionaryBucketStep = bucket_step % 37;
    if( DictionaryBucketStep == 0 ) DictionaryBucketStep = 1;
    DictionaryBlockIndex = block % DictionaryBlockCount;
    DictionaryBlockStep = block_step % DictionaryBlockCount;
    if( DictionaryBlockStep == 0 ) DictionaryBlockStep = 1;
}

/* Original0x4131a4..0x413243. Double hashing tries every bucket of
   each candidate block; mark a block full when its data area cannot fit. */
DictionaryBlock *FindDictionarySpace( unsigned words )
{
    DictionaryBlock *block;
    unsigned blocks, buckets, available;

    blocks = DictionaryBlockCount;
    do {
        block = GetDictionaryBlock( DictionaryBlockIndex );
        buckets = 37;
        do {
            if( block->Buckets[DictionaryBucket] == 0 ) {
                available = 256 - block->NextFree;
                if( (int)available >= (int)words ) return( block );
                block->NextFree = 255;
                CommitDictionaryBlock();
                break;
            }
            DictionaryBucket += DictionaryBucketStep;
            if( DictionaryBucket >= 37 ) DictionaryBucket -= 37;
        } while( --buckets );
        DictionaryBlockIndex += DictionaryBlockStep;
        if( DictionaryBlockIndex >= DictionaryBlockCount )
            DictionaryBlockIndex -= DictionaryBlockCount;
    } while( --blocks );
    return( NULL );
}

/* Original0x414b8a..0x414bcb. Final RET414bca is also the genuine
   empty commit-hook entry. Each table slot owns its lazily allocated block. */
DictionaryBlock *GetDictionaryBlock( unsigned short index )
{
    DictionaryBlock *block;

    if( DictionaryBlocks[index] == NULL ) {
        block = AllocMemory( sizeof(DictionaryBlock) );
        DictionaryBlocks[index] = block;
        InitDictionaryBlock( block );
    }
    return( DictionaryBlocks[index] );
}

/* Original0x414bca is one shared C3 byte. This supported platform's
   commit hook has no state effects; preserve its distinct callable identity. */
void CommitDictionaryBlock( void )
{
}

/* Original0x414afd..0x414b16. The37 buckets and free-byte marker
   occupy38 bytes, so the first available two-byte unit is19. */
void InitDictionaryBlock( DictionaryBlock *block )
{
    memset( block, 0, sizeof(*block) );
    block->NextFree = 19;
}

/* Original0x413038..0x413071. Superseded weak definitions are flagged
   rather than removed, and do not enter the final dictionary. */
int InsertPublicDictionary( void )
{
    SymbolRecord *symbol;
    unsigned bucket;
    int full;

    for( bucket = 0; bucket < 27; ++bucket ) {
        for( symbol = (SymbolRecord *)SymbolHash[bucket]; symbol != NULL;
             symbol = symbol->Next ) {
            if( symbol->Flags & 0x80 ) continue;
            full = InsertDictionarySymbol( symbol, symbol->Value.Owner->Value.Page );
            if( full ) return( full );
        }
    }
    return( 0 );
}

/* Original0x41316f..0x4131a4. Allocated blocks are emitted in physical
   dictionary order after all names have been inserted successfully. */
void WriteDictionaryBlocks( void )
{
    OpenFile *output;
    DictionaryBlock *block;
    int index;

    output = OutputLibraryName->File;
    for( index = 0; index < AllocatedDictionaryBlocks; ++index ) {
        block = GetDictionaryBlock( (unsigned short)index );
        WriteBuffered( output, block, sizeof(*block) );
    }
}

/* Original0x414b41..0x414b8a. NULL slots were never allocated; the
   owning table is cleared after its lazily allocated blocks are released. */
void FreeDictionaryBlocks( void )
{
    unsigned index;

    if( DictionaryBlocks != NULL ) {
        for( index = 0; index < DictionaryBlockSlots; ++index ) {
            if( DictionaryBlocks[index] != NULL ) FreeMemory( DictionaryBlocks[index] );
        }
        FreeMemory( DictionaryBlocks );
        DictionaryBlocks = NULL;
    }
}

/* Original0x420368: trial divisors for the sixteen-bit dictionary block
   count. The named const object also gives the period MAP its data boundary. */
const unsigned DictionaryPrimes[] = {
    2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53,
    59, 61, 67, 71, 73, 79, 83, 89, 97, 101, 103, 107, 109, 113,
    127, 131, 137, 139, 149, 151, 157, 163, 167, 173, 179, 181,
    191, 193, 197, 199, 211, 223, 227, 229, 233, 239, 241, 251, 0
};

/* Original0x413103..0x41313e. Stop once a divisor exceeds the square
   root, or the finite table of possible sixteen-bit divisors is exhausted. */
int isPrime( unsigned candidate )
{
    const unsigned *divisor;

    for( divisor = DictionaryPrimes; *divisor != 0; ++divisor ) {
        if( *divisor * *divisor > candidate ) break;
        if( candidate % *divisor == 0 ) return( 0 );
    }
    return( 1 );
}
