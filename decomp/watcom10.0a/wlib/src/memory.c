/* Original 0x411fdf..0x411fe0 is exactly RET. This remains an App/support
   vertex because no CRT identity is proved for this hook. */
#include <stddef.h>
#include <string.h>
#include "wlib.h"
#include "toolstate.h"
#include <recon.h>

void CheckMemoryHook( void )
{
}

/* Original 0x41203d..0x412086. Pools allocate thirty fixed-size
   objects at a time; UsedIndex names the last object returned. */
void *AllocPoolItem( MemoryPoolBlock **pool, unsigned item_size )
{
    MemoryPoolBlock *block, *newblock;

    block = *pool;
    ++AllocationCount;
    if( block == NULL || block->UsedIndex >= 29 ) {
        newblock = AllocMemory( 30 * item_size + offsetof(MemoryPoolBlock, Items) );
        newblock->Next = block;
        newblock->UsedIndex = 0;
        block = newblock;
        *pool = block;
        --AllocationCount;
    } else {
        ++block->UsedIndex;
    }
    return( block->Items + block->UsedIndex * item_size );
}

/* Original 0x411fe0..0x412021. This is an application allocation
   wrapper: it reports allocation failure and zeroes successful storage. */
void *AllocMemory( unsigned size )
{
    void *memory;

    memory = malloc( size );
    if( memory == NULL && size > 0 ) {
        ReportError( 2, 4 );
        ToolFlags |= 0x40;
        AbortCommand();
    }
    memset( memory, 0, size );
    ++AllocationCount;
    return( memory );
}

/* Original0x412086..0x4120a5. Each released pool block accounts
   for its last used index plus the underlying allocation itself. */
void FreePool( MemoryPoolBlock *head )
{
    MemoryPoolBlock *next;

    while( head != NULL ) {
        next = head->Next;
        FreeCount += head->UsedIndex;
        FreeMemory( head );
        head = next;
    }
}

/* Original0x412031..0x41203d; the public free boundary remains CRT. */
void FreeMemory( void *memory )
{
    free( memory );
    ++FreeCount;
}

/* Original0x412021..0x412031. Name descriptive: unlike AllocMemory,
   this nullable allocation hook neither zeroes storage nor aborts on failure. */
void *TryAllocMemory( unsigned size )
{
    void *memory;
    memory = malloc( size );
    if( memory != NULL ) ++AllocationCount;
    return( memory );
}
