/* Original ProcessCommands0x412c40..0x412cd0. Descriptive names retain
   the older librarian's actual orchestration and flag tests. */
#include <stdlib.h>
#include <process.h>
#include "toolstate.h"
#include <recon.h>

unsigned char ProcessCommands( void )
{
    unsigned char error;

    error = ParseCommandLine();
    if( ToolFlags & 0x400 ) {
        ToolFlags |= 0x800;
    } else if( !(ToolFlags & 0x800) ) {
        ToolFlags |= 0x800;
        PrintBanner();
    }
    if( error ) {
        return( error );
    }
    error = ValidateCommands();
    if( !error ) {
        PrepareLibrary();
        if( !(ToolFlags & 2) ) {
            PrepareOutput();
            BuildDictionary();
            FinishOutput();
        }
        ReportUnprocessedMembers();
        if( ToolFlags & 1 ) {
            ListLibrary();
        }
        if( !(ToolFlags & 2) ) {
            CloseLibraryFile( BackupName );
        }
        CompleteLibrary();
        return( 0 );
    }
    return( error );
}

/* Original0x410eb9..0x410f93. Help is recognized before environment
   processing. Keep the complete older command-tail workflow in this body. */
unsigned char ParseCommandLine( void )
{
    char *buffer;
    char **saved_arguments;
    int saved_count;

    if( CommandArgumentCount == 0 || CommandArguments[0][0] == '?' ) {
        PrintUsage();
        return( 1 );
    }
    ParseStage = 0;
    buffer = getenv( "WLIB" );
    if( buffer != NULL ) {
        saved_arguments = CommandArguments;
        saved_count = CommandArgumentCount;
        CommandArguments = &buffer;
        CommandArgumentCount = 1;
        CommandCurrent = buffer;
        ParseArguments();
        CommandArguments = saved_arguments;
        CommandArgumentCount = saved_count;
    }
    buffer = AllocMemory( 0x2800 );
    getcmd( buffer );
    CommandArguments = &buffer;
    CommandArgumentCount = 1;
    CommandCurrent = buffer;
    ParseArguments();
    if( ToolFlags & 4 ) {
        ToolFlags &= ~0x200UL;
    }
    FreeMemory( buffer );
    return( (ToolFlags & 0x40) ? 10 : 0 );
}

/* Original0x41331a..0x413329: make the normal banner visible, then
   print resource-backed usage. The original text argument is empty. */
void PrintUsage( void )
{
    PrintTextLine( "" );
    Usage();
}
