/* This target links the ordinary, uninstrumented memory configuration.
 * LnkMemInit and LnkMemFini share the same one-byte RET at VA433300.
 * The released mem.c confirms both operations are empty without TRACKER,
 * _INT_DEBUG or the other platform-specific reserve-memory configurations. */
#include <stdlib.h>
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
