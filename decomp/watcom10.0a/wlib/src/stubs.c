/* Named application/support frontiers. These bodies have no accepted code.
   INT3 must stop before any invented return or application side effect. */
#include "wlib.h"
#include "toolstate.h"
#include <recon.h>

void HandleSignal( int signal_number )
{
    WLIB_FRONTIER();
}

void WresRecordError( WResStatus status )
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

int NamesEqualIgnoreCase( const unsigned char *first, const unsigned char *second,
                          unsigned first_length, unsigned second_length )
{
    WLIB_FRONTIER();
    return( 0 );
}

void __cdecl ReportError( unsigned destination, unsigned message, ... )
{
    WLIB_FRONTIER();
}

void AbortCommand( void )
{
    WLIB_FRONTIER();
}

unsigned char ValidateCommands( void )
{
    WLIB_FRONTIER();
    return( 0 );
}

void PrepareLibrary( void )
{
    WLIB_FRONTIER();
}

void PrepareOutput( void )
{
    WLIB_FRONTIER();
}

void BuildDictionary( void )
{
    WLIB_FRONTIER();
}

void FinishOutput( void )
{
    WLIB_FRONTIER();
}

void ReportUnprocessedMembers( void )
{
    WLIB_FRONTIER();
}

void ListLibrary( void )
{
    WLIB_FRONTIER();
}

void CloseLibraryFile( FileName *name )
{
    WLIB_FRONTIER();
}

void CompleteLibrary( void )
{
    WLIB_FRONTIER();
}

void ParseArguments( void )
{
    WLIB_FRONTIER();
}

int WResIDNameCmp( const WResIDName *first, const WResIDName *second )
{
    WLIB_FRONTIER();
    return( 0 );
}

void __cdecl PrintMessage( unsigned message, ... )
{
    WLIB_FRONTIER();
}
