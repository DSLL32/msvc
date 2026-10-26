#include <stdlib.h>
#include <string.h>
#include <recon-tool.h>
#include "demangle.h"
#include <recon.h>
#define CHECK(x) do { if( !(x) ) return( 0 ); } while(0)
int mangled_classification_unit( void )
{
    CHECK( __is_mangled_internal( "plain", 0 ) == 0 );
    CHECK( __is_mangled_internal( "W?a$ni", 0 ) == 1 );
    CHECK( __is_mangled_internal( "W?.internal", 0 ) == 2 );
    CHECK( __is_mangled_internal( "W?$wxx", 0 ) == 2 );
    CHECK( __is_mangled_internal( "W?$CT:S$n()_", 0 ) == 3 );
    CHECK( __is_mangled_internal( "W?$dt:S$n()_", 0 ) == 4 );
    CHECK( __is_mangled_internal( "T?1234$CT:S$n()_", 0 ) == 3 );
    return( 1 );
}
static int name_and_scope_queries( void )
{
    const char *base, *scope;
    unsigned length;
    static const char symbol[] = "W?a$ni";
    static const char constructor[] = "W?$CT:S$n()_";
    CHECK( __unmangled_name( symbol, 0, &base, &length ) == 1 );
    CHECK( base == symbol + 2 && length == 1 && base[0] == 'a' );
    CHECK( __unmangled_name( constructor, 0, &base, &length ) == 1 );
    CHECK( base == constructor + 2 && length == 3 && memcmp( base, "$CT", 3 ) == 0 );
    CHECK( __unmangled_name( "plain", 3, &base, &length ) == 0 && length == 3 );
    CHECK( __scope_name( symbol, 0, 0, &scope, &length ) == 1 );
    CHECK( scope == symbol + 2 && length == 1 && scope[0] == 'a' );
    CHECK( __scope_name( symbol, 0, 8, &scope, &length ) == 0 && scope == NULL && length == 0 );
    CHECK( __scope_name( "plain", 0, 0, &scope, &length ) == 0 && scope == NULL && length == 0 );
    return( 1 );
}
static unsigned reallocations;
static char *resize_output( char *old, unsigned size )
{
    ++reallocations;
    return( realloc( old, size ) );
}
static int owned_output_and_operators( void )
{
    char *output;
    char basename[32], op[4] = {'?','?','?','!'};
    unsigned length;
    output = malloc( 2 ); CHECK( output != NULL );
    reallocations = 0;
    length = __demangle_r( "W?a$ni", 0, &output, 2, resize_output );
    CHECK( length == 10 && strcmp( output, "int near a" ) == 0 && reallocations > 0 );
    free( output );
    length = __demangled_basename( "W?a$ni", 0, basename, sizeof(basename) );
    CHECK( length == 1 && strcmp( basename, "a" ) == 0 );
    length = __demangled_basename( "plain", 0, basename, sizeof(basename) );
    CHECK( length == 5 && strcmp( basename, "plain" ) == 0 );
    CHECK( __mangle_operator( ">>", 0, op ) == 3 && memcmp( op, "$oa", 3 ) == 0 && op[3] == '!' );
    CHECK( __mangle_operator( "==", 2, op ) == 3 && memcmp( op, "$ra", 3 ) == 0 );
    CHECK( __mangle_operator( "+=", 0, op ) == 3 && memcmp( op, "$ac", 3 ) == 0 );
    memset( op, '?', sizeof(op) );
    CHECK( __mangle_operator( "not-an-operator", 0, op ) == 0 && op[0] == '?' && op[3] == '?' );
    return( 1 );
}
typedef struct parse_events { unsigned events, copies, identifiers; const char *copy; int length; } parse_events;
static void record_event( void **cookie, dm_pts event, int value, const char *text )
{
    parse_events *state = *cookie;
    ++state->events;
    if( event == DM_COPY_STRING ) { ++state->copies; state->copy = text; state->length = value; }
    if( event == DM_IDENTIFIER && value == 1 && text[0] == 'a' ) ++state->identifiers;
}
static int callback_parser( void )
{
    parse_events state;
    static const char plain[] = "plain";
    memset( &state, 0, sizeof(state) );
    __parse_mangled_name( plain, 0, &state, record_event );
    CHECK( state.events == 1 && state.copies == 1 && state.copy == plain && state.length == 5 );
    memset( &state, 0, sizeof(state) );
    __parse_mangled_name( "W?a$ni", 0, &state, record_event );
    CHECK( state.events > 1 && state.copies == 0 && state.identifiers == 1 );
    return( 1 );
}
const ReconTest api_tests[] = {
    { "mangled/internal/constructor/destructor classification", 1, mangled_classification_unit },
    { "counted original-name and scope slices", 1, name_and_scope_queries },
    { "reallocated output, basename and operator encoding", 1, owned_output_and_operators },
    { "typed parser callback events and source ownership", 1, callback_parser }
};
