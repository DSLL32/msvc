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











int op_new( output_desc *data, state_desc *state ) { WLIB_FRONTIER(); return( 0 ); }
int assignment_function( output_desc *data, state_desc *state ) { WLIB_FRONTIER(); return( 0 ); }
int relational_function( output_desc *data, state_desc *state ) { WLIB_FRONTIER(); return( 0 ); }
int special_type_names( output_desc *data, state_desc *state ) { WLIB_FRONTIER(); return( 0 ); }
int check_recurse( void ) { WLIB_FRONTIER(); return( 0 ); }
int recursive_mangled_name( output_desc *data, state_desc *state ) { WLIB_FRONTIER(); return( 0 ); }
int base_32_num( output_desc *data, int *value ) { WLIB_FRONTIER(); return( 0 ); }
int based_encoding( output_desc *data, state_desc *state ) { WLIB_FRONTIER(); return( 0 ); }


void forceSuppression( output_desc *data ) { WLIB_FRONTIER(); }




char *CopyCountedName( int length, const char *name )
{ WLIB_FRONTIER(); return( NULL ); }


void WriteListingCharacter( unsigned char ch, int handle ) { WLIB_FRONTIER(); }

int ReadImportModuleName( unsigned wide_ordinal ) { WLIB_FRONTIER(); return( 0 ); }
int ConvertNlmExports( int handle, unsigned long count ) { WLIB_FRONTIER(); return( 0 ); }
int ConvertNameTable( int handle, unsigned by_name ) { WLIB_FRONTIER(); return( 0 ); }
