/* Named application/support frontiers. These bodies have no accepted code.
   INT3 must stop before any invented return or application side effect. */
#include "wlib.h"
#include <recon.h>

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



void WresRecordError( WResStatus status )
{
    WLIB_FRONTIER();
}

void __FreeTypeList( WResDir dir )
{
    WLIB_FRONTIER();
}

int readMResDir( int handle, WResDir dir, int *dup_discarded )
{
    WLIB_FRONTIER();
    return( 0 );
}




int WResReadExtraWResID( WResID *id, int handle )
{
    WLIB_FRONTIER();
    return( 0 );
}



int WResReadFixedResRecord1( WResResInfo1 *info, int handle )
{
    WLIB_FRONTIER();
    return( 0 );
}

int WResReadFixedResRecord( WResResInfo *info, int handle )
{
    WLIB_FRONTIER();
    return( 0 );
}

int readLangInfoList( int handle, WResResNode *node )
{
    WLIB_FRONTIER();
    return( 0 );
}
