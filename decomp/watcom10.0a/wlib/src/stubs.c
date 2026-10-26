/* Named application/support frontiers. These bodies have no accepted code.
   INT3 must stop before any invented return or application side effect. */
#include "wlib.h"
#include "toolstate.h"
#include "demangle.h"
#include "imports.h"
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


void AbortCommand( void )
{
    WLIB_FRONTIER();
}





int WResIDNameCmp( const WResIDName *first, const WResIDName *second )
{
    WLIB_FRONTIER();
    return( 0 );
}





void ReportSyntaxError( unsigned message, unsigned context, unsigned char *ch )
{
    WLIB_FRONTIER();
}













void forceSuppression( output_desc *data ) { WLIB_FRONTIER(); }




char *CopyCountedName( int length, const char *name )
{ WLIB_FRONTIER(); return( NULL ); }


void WriteListingCharacter( unsigned char ch, int handle ) { WLIB_FRONTIER(); }

