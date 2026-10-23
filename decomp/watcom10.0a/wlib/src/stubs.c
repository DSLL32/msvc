/* Named application/support frontiers. These bodies have no accepted code.
   INT3 must stop before any invented return or application side effect. */
#include "wlib.h"
#include <recon.h>

void CheckMemoryHook( void )
{
    WLIB_FRONTIER();
}

int InitMsg( const char *image_name )
{
    WLIB_FRONTIER();
    return( 0 );
}

void HandleSignal( int signal_number )
{
    WLIB_FRONTIER();
}

void InitToolState( int argc, char **argv )
{
    WLIB_FRONTIER();
}

unsigned char ProcessCommands( void )
{
    WLIB_FRONTIER();
    return( 0 );
}

void FiniToolState( void )
{
    WLIB_FRONTIER();
}

int FiniMsg( void )
{
    WLIB_FRONTIER();
    return( 0 );
}
