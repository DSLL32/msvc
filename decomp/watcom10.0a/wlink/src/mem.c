/* This target links the ordinary, uninstrumented memory configuration.
 * LnkMemInit and LnkMemFini share the same one-byte RET at VA433300.
 * The released mem.c confirms both operations are empty without TRACKER,
 * _INT_DEBUG or the other platform-specific reserve-memory configurations. */
#include <stdlib.h>
#include <malloc.h>
#include "link.h"
#include <recon.h>

void LnkMemFini( void )
{
}

void *ChkLAlloc( unsigned size )
{
    void *ptr;

    ptr = LAlloc( size );
    if( ptr == 0 ) {
        LnkMsg( 0x3409, 0 );
    }
    return ptr;
}

void *LAlloc( unsigned size )
{
    void *ptr;

    do {
        ptr = malloc( size );
        if( ptr != 0 ) {
            break;
        }
    } while( FreeUpMemory() );
    return ptr;
}

void _LnkFree( void *ptr )
{
    if( ptr != 0 ) free( ptr );
}

void *LnkExpand( void *memory, unsigned size )
{
    return _expand( memory, size );
}

extern unsigned char CacheRelease( void );
extern unsigned char SwapOutVirt( void );
extern unsigned char SwapOutRelocs( void );
void *LnkReAlloc( void *memory, unsigned size )
{
    void *ptr;

    for( ;; ) {
        ptr = realloc( memory, size );
        if( ptr != 0 ) return ptr;
        if( !CacheRelease() && !SwapOutVirt() && !SwapOutRelocs() ) {
            LnkMsg( 0x3409, 0 );
        }
    }
}

extern unsigned char DumpObjCache( void );
extern unsigned char DiscardDicts( void );
extern unsigned char PermShrink( void );
unsigned char CacheRelease( void )
{
    unsigned char freed;
    freed = DumpObjCache();
    if( !freed ) freed = DiscardDicts();
    return freed;
}
unsigned char FreeUpMemory( void )
{
    return PermShrink() || CacheRelease() || SwapOutVirt() || SwapOutRelocs();
}
