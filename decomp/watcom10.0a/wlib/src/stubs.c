/* Named application/support frontiers. These bodies have no accepted code.
   INT3 must stop before any invented return or application side effect. */
#include "wlib.h"
#include "toolstate.h"
#include "demangle.h"
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


void CloseFileCache( int handle )
{
    WLIB_FRONTIER();
}

void CopyOutputFileName( void )
{
    WLIB_FRONTIER();
}


int WResIDNameCmp( const WResIDName *first, const WResIDName *second )
{
    WLIB_FRONTIER();
    return( 0 );
}


unsigned char SkipComment( void )
{
    WLIB_FRONTIER();
    return( 0 );
}

unsigned char ParseModuleName( unsigned char ch )
{
    WLIB_FRONTIER();
    return( 0 );
}

unsigned char SetImportOption( void )
{
    WLIB_FRONTIER();
    return( 0 );
}

void ReportSyntaxError( unsigned message, unsigned context, unsigned char *ch )
{
    WLIB_FRONTIER();
}

int NamesEqualCaseSensitive( const unsigned char *first, const unsigned char *second,
                             unsigned first_length, unsigned second_length )
{
    WLIB_FRONTIER();
    return( 0 );
}


int CreateTemporaryLibrary( FileName *name )
{
    WLIB_FRONTIER();
    return( 0 );
}


int LoadExecutableCache( OpenFile *file )
{
    WLIB_FRONTIER();
    return( 0 );
}


void getcomdef( void )
{
    WLIB_FRONTIER();
}

void getcomdat( void )
{
    WLIB_FRONTIER();
}


void do_demangle( output_desc *data ) { WLIB_FRONTIER(); }


void setEmitIndex( output_desc *data, unsigned index ) { WLIB_FRONTIER(); }
void resetEmitIndex( output_desc *data, unsigned offset ) { WLIB_FRONTIER(); }
void zapSpace( output_desc *data ) { WLIB_FRONTIER(); }
void emitStr( output_desc *data, const char *text ) { WLIB_FRONTIER(); }
void forceSuppression( output_desc *data ) { WLIB_FRONTIER(); }
void unforceSuppression( output_desc *data ) { WLIB_FRONTIER(); }


int isPrime( unsigned candidate ) { WLIB_FRONTIER(); return( 0 ); }


char *CopyCountedName( int length, const char *name )
{ WLIB_FRONTIER(); return( NULL ); }


void WriteListingCharacter( unsigned char ch, int handle ) { WLIB_FRONTIER(); }
