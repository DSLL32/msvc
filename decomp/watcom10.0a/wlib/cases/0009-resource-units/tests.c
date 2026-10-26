/* Unit expectations follow the1994 instructions and released resource
   algorithms. This suite does not simulate signal delivery or low-stack state. */
#include <string.h>
#include <fcntl.h>
#include <io.h>
#include <unistd.h>
#include <recon-tool.h>
#include "toolstate.h"
#include "demangle.h"
#include <recon.h>
#define CHECK(x) do { if( !(x) ) return( 0 ); } while(0)
int signal_callback_unit( void )
{
    HandleSignal( SIGINT );
    CHECK( BreakRequested == 1 );
    return( 1 );
}
static int suppression_levels( void )
{
    output_desc data;
    static const int before[] = { -1, 0, 1, 2, 3, 7 };
    static const int after[] = { 2, 3, 4, 5, 3, 7 };
    unsigned i;
    for( i = 0; i < sizeof(before)/sizeof(before[0]); ++i ) {
        memset( &data, 0, sizeof(data) );
        data.count = 19;
        data.suppress_output = before[i];
        forceSuppression( &data );
        CHECK( data.suppress_output == after[i] && data.count == 19 );
    }
    return( 1 );
}
static int error_hook( void )
{
    WresRecordError( WRS_READ_FAILED );
    WresRecordError( WRS_READ_INCOMPLETE );
    return( 1 );
}
static int listing_byte( void )
{
    int file;
    unsigned char bytes[3];
    file = open( "character.bin", O_RDWR|O_CREAT|O_TRUNC|O_BINARY, 0666 );
    CHECK( file >= 0 );
    WriteListingCharacter( 'A', file );
    WriteListingCharacter( 255, file );
    CHECK( lseek( file, 0, SEEK_SET ) == 0 );
    CHECK( read( file, bytes, sizeof(bytes) ) == 2 );
    CHECK( bytes[0] == 'A' && bytes[1] == 255 );
    CHECK( close( file ) == 0 && unlink( "character.bin" ) == 0 );
    return( 1 );
}
static int counted_name_records( void )
{
    struct { WResResInfo info; char extra[3]; } owner;
    int file;
    memset( &owner, 0, sizeof(owner) );
    file = open( "record.bin", O_BINARY ); CHECK( file >= 0 );
    CHECK( WResReadFixedResRecord( &owner.info, file ) == 0 );
    CHECK( owner.info.NumResources == 2 && owner.info.ResName.IsName == 1 );
    CHECK( owner.info.ResName.ID.Name.NumChars == 4 );
    CHECK( WResReadExtraWResID( &owner.info.ResName, file ) == 0 );
    CHECK( memcmp( owner.info.ResName.ID.Name.Name, "Name", 4 ) == 0 );
    CHECK( close( file ) == 0 );
    file = open( "empty.bin", O_BINARY ); CHECK( file >= 0 );
    CHECK( WResReadFixedResRecord( &owner.info, file ) == 1 );
    CHECK( close( file ) == 0 );
    return( 1 );
}
static int counted_name_order( void )
{
    struct { unsigned char length; char name[8]; } first = {4,"Name"}, same = {4,"nAME"}, prefix = {3,"Nam"}, other = {4,"Naze"};
    CHECK( WResIDNameCmp( (WResIDName *)&first, (WResIDName *)&same ) == 0 );
    CHECK( WResIDNameCmp( (WResIDName *)&prefix, (WResIDName *)&first ) == -1 );
    CHECK( WResIDNameCmp( (WResIDName *)&first, (WResIDName *)&prefix ) == 1 );
    CHECK( WResIDNameCmp( (WResIDName *)&first, (WResIDName *)&other ) == 'm'-'z' );
    return( 1 );
}
static int language_nodes( void )
{
    WResResNode resource;
    WResLangNode *first, *second;
    int file;
    memset( &resource, 0, sizeof(resource) );
    resource.Info.NumResources = 2;
    file = open( "languages.bin", O_BINARY ); CHECK( file >= 0 );
    CHECK( readLangInfoList( file, &resource ) == 0 );
    first = resource.Head; CHECK( first != NULL ); second = first->Next;
    CHECK( first != NULL && second != NULL && resource.Tail == second );
    CHECK( first->Prev == NULL && second->Prev == first && second->Next == NULL );
    CHECK( first->data == NULL && second->data == NULL );
    CHECK( first->Info.lang.lang == 0x409 && first->Info.lang.sublang == 0 );
    CHECK( first->Info.Offset == 0x11223344UL && first->Info.Length == 5 );
    CHECK( second->Info.lang.lang == 0x407 && second->Info.lang.sublang == 1 );
    CHECK( second->Info.Offset == 0x55667788UL && second->Info.Length == 6 );
    CHECK( close( file ) == 0 );
    __FreeLangList( &resource );
    return( 1 );
}
static int named_wres_directory( void )
{
    WResDir dir;
    WResDirWindow window;
    WResID *type, *name;
    WResLangType lang;
    int file;
    char bytes[8];
    file = open( "named-v2.res", O_BINARY ); CHECK( file >= 0 );
    dir = WResInitDir(); CHECK( dir != NULL );
    CHECK( WResReadDir( file, dir, NULL ) == 0 );
    CHECK( dir->NumTypes == 1 && dir->NumResources == 1 && dir->TargetOS == 1 );
    CHECK( dir->Head == dir->Tail && dir->Head->Info.NumResources == 1 );
    type = WResIDFromStr( "custom" ); name = WResIDFromStr( "greeting" );
    CHECK( type != NULL && name != NULL );
    lang.lang = 0x407; lang.sublang = 1;
    window = WResFindResource( type, name, dir, &lang );
    CHECK( window.CurrLang != NULL );
    CHECK( window.CurrRes->Info.NumResources == 2 );
    CHECK( window.CurrLang->Info.Offset == 35 && window.CurrLang->Info.Length == 7 );
    CHECK( lseek( file, window.CurrLang->Info.Offset, SEEK_SET ) == 35 );
    CHECK( read( file, bytes, 7 ) == 7 && memcmp( bytes, "Deutsch", 7 ) == 0 );
    free( type ); free( name );
    WResFreeDir( dir ); CHECK( close( file ) == 0 );
    return( 1 );
}
static int win16_mres_directory( void )
{
    WResDir dir;
    WResID *type, *name;
    WResDirWindow window;
    int file, duplicate;
    char bytes[5];
    file = open( "win16.res", O_BINARY ); CHECK( file >= 0 );
    dir = WResInitDir(); CHECK( dir != NULL );
    CHECK( WResReadDir( file, dir, &duplicate ) == 0 && duplicate == 1 );
    CHECK( dir->TargetOS == 1 && dir->NumTypes == 2 && dir->NumResources == 2 );
    type = WResIDFromNum( 6 ); name = WResIDFromNum( 1 );
    CHECK( type != NULL && name != NULL );
    window = WResFindResource( type, name, dir, NULL );
    CHECK( window.CurrLang != NULL && window.CurrRes->Info.NumResources == 1 );
    CHECK( window.CurrLang->Info.Offset == 12 && window.CurrLang->Info.Length == 5 );
    CHECK( window.CurrLang->Info.MemoryFlags == 0x10 );
    CHECK( lseek( file, window.CurrLang->Info.Offset, SEEK_SET ) == 12 );
    CHECK( read( file, bytes, 5 ) == 5 && memcmp( bytes, "first", 5 ) == 0 );
    free( type ); free( name );
    type = WResIDFromStr( "CUSTOM" ); name = WResIDFromStr( "Greeting" );
    CHECK( type != NULL && name != NULL );
    window = WResFindResource( type, name, dir, NULL );
    CHECK( window.CurrLang != NULL && window.CurrLang->Info.Length == 5 );
    free( type ); free( name ); WResFreeDir( dir );
    CHECK( close( file ) == 0 );
    return( 1 );
}
static int variable_string_owner( void )
{
    VarString *list;
    char *text;
    unsigned i;
    int length;
    list = VarStringStart(); CHECK( list != NULL );
    for( i = 0; i < 137; ++i ) VarStringAddChar( list, 'a' + i % 26 );
    CHECK( list->next != NULL && list->next->next != NULL );
    CHECK( ComputeVarStringLen( list ) == 137 );
    text = VarStringEnd( list, &length );
    CHECK( text != NULL && length == 137 && text[137] == '\0' );
    for( i = 0; i < 137; ++i ) CHECK( text[i] == 'a' + i % 26 );
    free( text );
    CHECK( WResIDFromNum( -1 ) == NULL && WResIDFromNum( 65536L ) == NULL );
    return( 1 );
}

const ReconTest resource_tests[] = {
    { "signal callback stores deferred break (no delivery claim)", 1, signal_callback_unit },
    { "typed suppression levels", 1, suppression_levels },
    { "empty resource error hook", 1, error_hook },
    { "listing byte descriptor output", 1, listing_byte },
    { "fixed/count-extra records and EOF", 1, counted_name_records },
    { "case-folded counted-name ordering", 1, counted_name_order },
    { "owned language-list links and metadata", 1, language_nodes },
    { "named WRes v2 directory and language lookup", 1, named_wres_directory },
    { "Win16 MRes numeric/named resources and duplicates", 1, win16_mres_directory },
    { "multi-piece variable-string ownership", 1, variable_string_owner }
};
