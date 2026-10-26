/* Original ProcessCommands0x412c40..0x412cd0. Descriptive names retain
   the older librarian's actual orchestration and flag tests. */
#include <stdlib.h>
#include <process.h>
#include <fcntl.h>
#include <io.h>
#include <stdio.h>
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
            CloseLibraryFile( OutputLibraryName );
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

/* Original0x41262b..0x412716 checks whether an explicit output is the
   same counted pathname, then chooses the library/listing/output setup. */
unsigned char ValidateCommands( void )
{
    unsigned char error;
    PathName *input_path, *output_path;

    if( ToolFlags & 4 ) {
        if( NamesEqualIgnoreCase( (unsigned char *)OutputLibraryName->Name,
                                 (unsigned char *)LibraryName->Name,
                                 (signed char)OutputLibraryName->NameLength,
                                 (signed char)LibraryName->NameLength ) ) {
            input_path = LibraryName->Path;
            output_path = OutputLibraryName->Path;
            if( NamesEqualIgnoreCase( (unsigned char *)output_path->Directory,
                                     (unsigned char *)input_path->Directory,
                                     output_path->DirectoryLength,
                                     input_path->DirectoryLength ) &&
                NamesEqualIgnoreCase( (unsigned char *)output_path->Extension,
                                     (unsigned char *)input_path->Extension,
                                     output_path->ExtensionLength,
                                     input_path->ExtensionLength ) ) {
                ToolFlags &= ~4UL;
            }
        }
    }
    if( CommandWork == NULL && MemberWork == NULL &&
        !(ToolFlags & 0x20) && !(ToolFlags & 4) ) {
        ToolFlags |= ExtractWork != NULL ? 0x82 : 3;
    }
    error = ValidateInputLibrary();
    if( error == 0 ) {
        OpenListing();
        error = OpenOutput();
    }
    return( error );
}

/* Original0x412716..0x4127ea. A missing input can create a new library;
   explicit new/list-only modes determine its diagnostics and destinations. */
unsigned char ValidateInputLibrary( void )
{
    char *extension;

    if( OpenLibraryFile( LibraryName, 0x200 ) ) {
        ToolFlags &= ~0x10UL;
        if( !(ToolFlags & 6) ) {
            extension = LibraryName->Path->Extension;
            if( extension[0] == 'b' && extension[1] == 'a' &&
                extension[2] == 'k' ) {
                ReportError( 2, 21 );
                ToolFlags |= 0x40;
                return( 4 );
            }
        }
    } else {
        ToolFlags |= 0x10;
        CopyOutputFileName();
        if( !(ToolFlags & 2) ) {
            if( !(ToolFlags & 0x200) ) ReportError( 3, 22, FileErrorName );
            if( ToolFlags & 4 ) ReportError( 3, 23 );
        } else {
            ReportError( 3, 24, FileErrorName );
            ToolFlags &= ~0x49UL;
            ListingName->NameLength = 0;
        }
    }
    return( 0 );
}

/* Original0x412836..0x4128cb. Bare-l derives the listing name from
   the selected library; explicit-l=name retains its own counted path. */
void OpenListing( void )
{
    if( ToolFlags & 8 ) {
        if( ToolFlags & 4 ) *ListingName = *OutputLibraryName;
        else *ListingName = *LibraryName;
        ListingName->Path = &ListingPath;
    }
    if( ListingName->NameLength != 0 ) {
        if( !OpenLibraryFile( ListingName, 0x162 ) ) {
            ReportError( 3, 50, FileErrorName );
            ToolFlags &= ~1UL;
        } else {
            if( ListingName->Flags == 1 ) {
                ListingHandle = ListingName->File->Handle;
            } else {
                ListingHandle = (int)ListingName->File;
            }
        }
    }
}

/* The actual temporary basename is mutable: CreateTemporaryLibrary bumps
   its final digits on collisions. This object is separate from cached names. */
static char TemporaryBase[] = "_T-00001"; /* Original0x420021. */
static PathName TemporaryPath = { 0, "", 3, "tmp" }; /*0x42033e*/
static FileName ScratchLibraryName = {
    NULL, 8, TemporaryBase, &TemporaryPath, NULL, 0, 0
}; /* Original0x420348. */

/* Original0x4128cb..0x41294f. An explicit-o uses its chosen output;
   otherwise an update writes a temporary library alongside its input. */
unsigned char OpenOutput( void )
{
    if( !(ToolFlags & 0x16) ) {
        TemporaryPath.Directory = LibraryName->Path->Directory;
        TemporaryPath.DirectoryLength = LibraryName->Path->DirectoryLength;
        if( !CreateTemporaryLibrary( &ScratchLibraryName ) ) {
            ReportError( 2, 25 );
            ToolFlags |= 0x40;
            return( 4 );
        }
        *OutputLibraryName = ScratchLibraryName;
    }
    if( !(ToolFlags & 2) ) {
        if( !OpenLibraryFile( OutputLibraryName,
                             O_RDWR | O_CREAT | O_TRUNC | O_BINARY ) ) {
            ReportError( 2, 25 );
            ToolFlags |= 0x40;
            return( 4 );
        }
    }
    return( 0 );
}

/* Original0x412cd0..0x412d55. Warn for requested member operations
   that never matched, using an independent NUL-terminated spelling. */
void ReportUnprocessedMembers( void )
{
    MemberCommand *remove;
    FileName *extract;
    char *name;

    for( remove = (MemberCommand *)MemberWork; remove != NULL; remove = remove->Next ) {
        if( !(remove->Flags & 1) && (remove->Flags & 2) ) {
            name = CopyCountedName( (signed char)remove->NameLength, remove->Name );
            if( !(ToolFlags & 0x210) ) ReportError( 3, 42, name );
            FreeMemory( name );
        }
    }
    for( extract = (FileName *)ExtractWork; extract != NULL; extract = extract->Next ) {
        if( (signed char)extract->NameLength != -1 ) {
            name = CopyCountedName( (signed char)extract->NameLength, extract->Name );
            ReportError( 3, 49, name );
            FreeMemory( name );
        }
    }
}

/* Original0x41294f..0x412a0a, with its shared return at0x412625.
   Explicit output and read-only modes leave the input library untouched. */
void CompleteLibrary( void )
{
    char *original;
    unsigned length, i;

    if( !(ToolFlags & 0x16) ) {
        CopyOutputFileName();
        BuildFileName( LibraryName );
        length = 0;
        while( FileErrorName[length] != '\0' ) ++length;
        original = AllocMemory( length + 1 );
        for( i = 0; i < length; ++i ) original[i] = FileErrorName[i];
        original[length] = '\0';
        if( ToolFlags & 0x100 ) {
            unlink( original );
        } else {
            LibraryName->Path->Extension = "bak";
            LibraryName->Path->ExtensionLength = 3;
            BuildFileName( LibraryName );
            unlink( FileErrorName );
            rename( original, FileErrorName );
        }
        BuildFileName( &ScratchLibraryName );
        rename( FileErrorName, original );
        FreeMemory( original );
    }
}

/* Original0x412d55..0x412d76. Member commands retain counted names;
   diagnostics take an independently allocated terminated copy. */
char *CopyCountedName( int length, const char *name )
{
    char *copy;
    unsigned index;

    copy = AllocMemory( length + 1 );
    for( index = 0; index < (unsigned)length; ++index ) copy[index] = name[index];
    copy[length] = '\0';
    return( copy );
}

/* Original0x412d76..0x412e06. Aborted construction discards its open
   output without flushing buffered data, then resumes main's saved context. */
void AbortCommand( void )
{
    PrintMessage( 6 );
    if( ListingName->File != NULL ) {
        CloseLibraryFile( ListingName );
        BuildFileName( ListingName );
        unlink( FileErrorName );
    }
    if( !(ToolFlags & 2) ) {
        if( OutputLibraryName->File != NULL &&
            OutputLibraryName->File->Handle != 0 ) {
            close( OutputLibraryName->File->Handle );
            OutputLibraryName->File->Handle = 0;
        }
        if( OutputLibraryName->File != NULL ) {
            BuildFileName( OutputLibraryName );
            unlink( FileErrorName );
        }
    }
    FreeDictionaryBlocks();
    longjmp( Env, 1 );
}
