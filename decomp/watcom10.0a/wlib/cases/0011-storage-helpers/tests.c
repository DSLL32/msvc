#include <string.h>
#include <recon-tool.h>
#include "toolstate.h"
#include <recon.h>
#define CHECK(x) do { if( !(x) ) return( 0 ); } while(0)
int allocation_unit( void )
{
    unsigned long allocated = AllocationCount, freed = FreeCount;
    unsigned char *owner;
    owner = TryAllocMemory( 64 ); CHECK( owner != NULL );
    CHECK( AllocationCount == allocated + 1 && FreeCount == freed );
    memset( owner, 0xa5, 64 );
    CHECK( owner[0] == 0xa5 && owner[63] == 0xa5 );
    FreeMemory( owner ); CHECK( FreeCount == freed + 1 );
    return( 1 );
}
static int dictionary_lifecycle( void )
{
    unsigned long allocated = AllocationCount, freed = FreeCount;
    DictionaryBlock *first, *second;
    CHECK( DictionaryBlocks == NULL );
    FiniDictionary(); CHECK( DictionaryBlocks == NULL && FreeCount == freed );
    InitDictionaryBlocks( 2 );
    CHECK( DictionaryBlocks != NULL && DictionaryBlockSlots == 2 );
    first = GetDictionaryBlock( 0 ); second = GetDictionaryBlock( 1 );
    CHECK( first != NULL && second != NULL && first != second );
    CHECK( GetDictionaryBlock( 0 ) == first );
    CHECK( AllocationCount == allocated + 3 );
    FiniDictionary();
    CHECK( DictionaryBlocks == NULL && FreeCount == freed + 3 );
    FiniDictionary(); CHECK( FreeCount == freed + 3 );
    return( 1 );
}
const ReconTest storage_tests[] = {
    { "nullable allocation success owns requested storage", 1, allocation_unit },
    { "typed dictionary creation/lazy blocks/finalization", 1, dictionary_lifecycle }
};
