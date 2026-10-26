/* The older librarian reads environment/command-tail strings through
   an explicit character callback, switched while processing command files. */
#include <ctype.h>
#include <stdlib.h>
#include "toolstate.h"
#include <recon.h>

unsigned char (*CommandReader)( void ) = ReadArgumentCharacter; /*0x420308*/

/* Original entry0x410e52 has a displaced dispatcher at0x4107a1 and
   shares the RET at0x410e51. It returns the final zero character in AL. */
unsigned char ParseArguments( void )
{
    unsigned char ch;

    ch = NextNonwhite();
    if( ParseStage < 2 ) {
        ch = ParseOptions( ch );
        if( ch == 0 ) return( ch );
        ch = ParseLibraryName( ch );
        if( ch == 0 ) return( ch );
        ch = ParseOptions( ch );
        if( ch == 0 ) return( ch );
        ParseStage = 2;
    }
    for( ;; ) {
        switch( ch ) {
        case 0: return( 0 );
        case '#': ch = SkipComment(); break;
        case '+': ch = ParseAddCommand(); break;
        case '-': ch = ParseMinusCommand(); break;
        case '*': ch = ParseExtractCommand(); break;
        case '@': ch = ParseIndirect(); break;
        default: ch = ParseModuleName( ch ); break;
        }
    }
}

/* Original0x410089..0x4100aa; the callback consumes each character. */
unsigned char NextNonwhite( void )
{
    unsigned char ch;

    do {
        ch = (*CommandReader)();
    } while( ch != 0 && isspace( ch ) );
    return( ch );
}

/* Original0x410010..0x41004e. Adjacent argument strings are separated
   by a synthetic space; the final NUL still decrements the argument count. */
unsigned char ReadArgumentCharacter( void )
{
    unsigned char ch;

    ch = *CommandCurrent;
    if( ch == 0 ) {
        --CommandArgumentCount;
        if( CommandArgumentCount > 0 ) {
            ++CommandArguments;
            CommandCurrent = *CommandArguments;
            ch = ' ';
        }
    } else {
        ++CommandCurrent;
    }
    return( ch );
}

FileName *ExplodeName;              /* Original0x4302fc. */

/* Original0x41034b..0x4104cc shares the7-byte terminal epilogue at
   0x41083a with several option helpers. Before the library name, '-' and
   '/' introduce options; after it only '/' does. */
unsigned char ParseOptions( unsigned char ch )
{
    for( ;; ) {
        if( ParseStage >= 2 ) return( ch );
        if( ch == '@' ) {
            ch = ParseIndirect();
            continue;
        }
        if( ch == '#' ) {
            ch = SkipComment();
            continue;
        }
        if( ch != '/' && (ParseStage >= 1 || ch != '-') ) return( ch );
        ch = NextNonwhite();
        switch( toupper( ch ) ) {
        case 'I': ch = SetImportOption(); continue;
        case 'L': ch = SetListingOption(); continue;
        case 'P': ch = SetPageOption(); continue;
        case 'O': ch = SetOutputOption(); continue;
        case 'D': ch = SetExtractionDirectory(); continue;
        case 'C': NamesEqual = NamesEqualCaseSensitive; break;
        case 'B': ToolFlags |= 0x100; break;
        case 'N': ToolFlags |= 0x200; break;
        case 'Q': ToolFlags |= 0x400; break;
        case 'S': ToolFlags |= 0x1000; break;
        case 'T': ToolFlags |= 0x2000; break;
        case 'X':
            ToolFlags |= 0x4000;
            ExplodeName = AllocPoolItem( &NamePool, sizeof(FileName) );
            ExplodeName->Next = NULL;
            break;
        case 'M': ToolFlags |= 0x8000; break;
        default: ReportSyntaxError( 100, 90, &ch ); break;
        }
        ch = NextNonwhite();
    }
}

/* Original0x4105c4..0x410675 plus the shared7-byte return epilogue.
   The requested size is a16-bit value, rounded down to a power of two,
   with a minimum page bound of16. */
unsigned char SetPageOption( void )
{
    unsigned char ch;
    char buffer[260];
    unsigned bit, page;

    ch = NextNonwhite();
    if( ch != '=' ) ReportSyntaxError( 66, 83, &ch );
    ch = ReadToken( 0, buffer );
    if( !isdigit( (unsigned char)buffer[0] ) ) {
        ReportSyntaxError( 56, 83, &ch );
    }
    RequestedPageSize = atoi( buffer );
    ToolFlags |= 0x20;
    if( RequestedPageSize <= 16 ) {
        RequestedPageSize = 16;
    } else {
        page = 0;
        for( bit = 16; bit < 0x10000UL; bit += bit ) {
            if( RequestedPageSize & bit ) page = bit;
        }
        RequestedPageSize = page;
    }
    return( ch );
}

/* Original0x41019d..0x4101aa: the ordinary filename/token wrapper. */
unsigned char ReadToken( unsigned char ch, char *buffer )
{
    return( ScanToken( ch, buffer, 0 ) );
}

/* Original0x4100aa..0x41019d. Single quotes protect punctuation, but
   this older parser still rejects whitespace within the quoted token. */
unsigned char ScanToken( unsigned char ch, char *buffer, unsigned char stop )
{
    if( ch == 0 ) ch = NextNonwhite();
    if( ch == '\'' ) {
        ch = NextNonwhite();
        while( ch != '\'' && ch != 0 && !isspace( ch ) ) {
            *buffer++ = ch;
            ch = (*CommandReader)();
        }
        if( ch != '\'' ) {
            ReportSyntaxError( 73, 98, &ch );
            return( ch );
        }
        *buffer = 0;
        ch = NextNonwhite();
    } else {
        while( ch != '\'' && ch != 0 && ch != '#' && ch != '@' &&
               ch != '+' && ch != '-' && ch != '=' && ch != '*' &&
               ch != '/' && !isspace( ch ) && ch != stop ) {
            *buffer++ = ch;
            ch = (*CommandReader)();
        }
        *buffer = 0;
        if( isspace( ch ) ) ch = NextNonwhite();
    }
    return( ch );
}

/* Original0x4102f9..0x41034b. The following character belongs to the
   outer stream, so retain it across the recursive indirect parse. */
struct OpenFile *CommandFile;       /* Original0x430000. */

unsigned char ParseIndirect( void )
{
    FileName name;
    CommandInput saved;
    unsigned char ch;

    name.File = NULL;
    ch = BeginIndirect( &name, &saved );
    if( !(ToolFlags & 0x40) ) {
        ParseArguments();
        if( name.File != NULL ) CloseLibraryFile( &name );
    }
    CommandReader = saved.Reader;
    CommandFile = saved.File;
    CommandCurrent = saved.Current;
    return( ch );
}

/* Original0x410238..0x4102f9. An environment variable takes priority
   over a response file with the same token. Save the stream only after
   consuming that token, including its following delimiter. */
static PathName ResponsePath = { 0, "", 3, "lbc" }; /* Original0x420334. */

unsigned char BeginIndirect( FileName *name, CommandInput *saved )
{
    char token[260];
    char *text;
    unsigned char ch;

    ch = ReadToken( 0, token );
    saved->Current = CommandCurrent;
    saved->File = CommandFile;
    saved->Reader = CommandReader;
    if( token[0] == 0 ) ReportSyntaxError( 67, 86, &ch );
    text = getenv( token );
    if( text != NULL ) {
        CommandCurrent = text;
        CommandReader = ReadStringCharacter;
    } else {
        SetFileName( name, &ResponsePath, token );
        if( !OpenLibraryFile( name, 0x200 ) ) {
            ReportError( 2, 52, FileErrorName );
            ToolFlags |= 0x40;
        } else {
            CommandReader = ReadFileCommandCharacter;
            CommandFile = name->File;
        }
    }
    return( ch );
}

/* Original0x410d0a..0x410e52. The old cache stores counted components,
   with no trailing NUL in any allocated directory/base/extension string.
   A default path object is shared when no explicit path/extension occurs. */
void SetFileName( FileName *name, PathName *defaults, const char *text )
{
    const char *scan, *base, *dot;
    unsigned directory_length, base_length, suffix_length, i;
    char *part;
    PathName *path;

    name->Flags = 0;
    base = text;
    dot = NULL;
    for( scan = text; *scan != 0; ++scan ) {
        if( *scan == '/' || *scan == '\\' || *scan == ':' ) {
            base = scan + 1;
            dot = NULL;
        } else if( *scan == '.' ) {
            dot = scan;
        }
    }
    if( dot == NULL ) dot = scan;
    directory_length = base - text;
    base_length = dot - base;
    suffix_length = scan - dot;
    if( defaults != NULL && (directory_length != 0 || suffix_length != 0) ) {
        path = AllocPoolItem( &PathPool, sizeof(PathName) );
        name->Path = path;
        if( directory_length != 0 ) {
            part = AllocMemory( directory_length );
            for( i = 0; i < directory_length; ++i ) part[i] = text[i];
            path->Directory = part;
            path->DirectoryLength = directory_length;
        } else {
            path->Directory = "";
            path->DirectoryLength = 0;
        }
        if( suffix_length != 0 ) {
            --suffix_length;
            part = AllocMemory( suffix_length );
            for( i = 0; i < suffix_length; ++i ) part[i] = dot[i + 1];
            path->Extension = part;
            path->ExtensionLength = suffix_length;
        } else {
            path->Extension = defaults->Extension;
            path->ExtensionLength = defaults->ExtensionLength;
        }
    } else {
        name->Path = defaults;
    }
    part = AllocMemory( base_length );
    for( i = 0; i < base_length; ++i ) part[i] = base[i];
    name->Name = part;
    name->NameLength = base_length;
}

/* Original0x410058..0x410089. A DOS control-Z terminates a command
   file; the buffered read's final partial block remains consumable. */
unsigned char ReadFileCommandCharacter( void )
{
    unsigned char ch;

    ch = *ReadBuffered( CommandFile, 1 );
    if( (CommandFile->Current == NULL && CommandFile->Available == 0) ||
        ch == 0x1a ) return( 0 );
    return( ch );
}

PathName ListingPath = { 0, "", 3, "lst" }; /* Original0x420316. */

/* Original0x41054e..0x4105c4 plus the shared7-byte option epilogue.
   Bare-l selects the existing listing destination; -l=name supplies one. */
unsigned char SetListingOption( void )
{
    unsigned char ch;
    char token[260];

    ch = NextNonwhite();
    if( ch == '=' ) {
        ch = ReadToken( 0, token );
        if( token[0] == 0 ) ReportSyntaxError( 55, 82, &ch );
        SetFileName( ListingName, &ListingPath, token );
        ToolFlags |= 1;
    } else {
        ToolFlags |= 9;
    }
    return( ch );
}

PathName ExtractionPath = { 0, "", 3, "obj" }; /* Original0x42032a. */

/* Original0x4106e5..0x4107a1. The extraction prefix is a counted
   directory ending in a backslash, unless it already ends in ':'/'\\'. */
unsigned char SetExtractionDirectory( void )
{
    unsigned char ch;
    char token[264];
    unsigned length, i;
    char *directory;

    ch = NextNonwhite();
    if( ch != '=' ) ReportSyntaxError( 66, 85, &ch );
    ch = ReadToken( 0, token );
    for( length = 0; token[length] != 0; ++length ) {}
    if( length == 0 ) ReportSyntaxError( 58, 85, &ch );
    if( token[length - 1] != ':' && token[length - 1] != '\\' ) {
        token[length++] = '\\';
        token[length] = 0;
    }
    directory = AllocMemory( length );
    for( i = 0; i < length; ++i ) directory[i] = token[i];
    ExtractionPath.Directory = directory;
    ExtractionPath.DirectoryLength = length;
    return( ch );
}

PathName LibraryPath = { 0, "", 3, "lib" }; /* Original0x42030c. */

/* Original0x410675..0x4106e5 plus the common option-return suffix. */
unsigned char SetOutputOption( void )
{
    unsigned char ch;
    char token[260];

    ch = NextNonwhite();
    if( ch != '=' ) ReportSyntaxError( 66, 84, &ch );
    ch = ReadToken( 0, token );
    if( token[0] == 0 ) ReportSyntaxError( 57, 84, &ch );
    SetFileName( OutputLibraryName, &LibraryPath, token );
    ToolFlags |= 4;
    return( ch );
}

/* Original0x4101b5..0x41021f consumes the library name only once,
   leaving subsequent module tokens to the command dispatcher. */
unsigned char ParseLibraryName( unsigned char ch )
{
    char token[260];

    if( ParseStage < 1 ) {
        if( ch != 0 ) {
            ch = ReadToken( ch, token );
            if( token[0] != 0 ) {
                SetFileName( LibraryName, &LibraryPath, token );
                ParseStage = 1;
                return( ch );
            }
        }
        ReportError( 2, 41 );
        ToolFlags |= 0x40;
        return( 0 );
    }
    return( ch );
}

/* Original0x410ae0..0x410be0. Combined -+ and -* keep both operations
   on the same counted member name; plain- records a standalone deletion. */
unsigned char ParseMinusCommand( void )
{
    unsigned char ch;
    char token[260];
    FileName *name;

    ch = NextNonwhite();
    if( ch == '+' ) {
        ch = ReadToken( 0, token );
        if( token[0] == 0 ) ReportSyntaxError( 68, 89, &ch );
        name = AddModule( token );
        AddRemoveCommand( name, 0 );
    } else if( ch == '*' ) {
        ch = ReadToken( 0, token );
        if( token[0] == 0 ) ReportSyntaxError( 68, 101, &ch );
        name = AddExtract( token );
        AddRemoveCommand( name, 0 );
    } else {
        name = AllocMemory( sizeof(FileName) );
        ch = ReadToken( ch, token );
        if( token[0] == 0 ) ReportSyntaxError( 68, 101, &ch );
        SetFileName( name, NULL, token );
        AddRemoveCommand( name, 1 );
        FreeMemory( name );
    }
    return( ch );
}

static PathName ModulePath = { 0, "", 3, "obj" }; /* Original0x420320. */

/* Original0x410c72..0x410ca6. Commands are prepended for the later
   ordering pass, with their original separate object-file default path. */
FileName *AddModule( const char *text )
{
    FileName *name;

    name = AllocPoolItem( &NamePool, sizeof(FileName) );
    SetFileName( name, &ModulePath, text );
    name->Next = (FileName *)CommandWork;
    CommandWork = (WorkItem *)name;
    return( name );
}

/* Original0x410ca6..0x410cda stores a borrowed module name for combined
   commands, or owns its allocated name for a standalone removal. */
void AddRemoveCommand( FileName *name, unsigned char owns_name )
{
    MemberCommand *command;

    command = AllocMemory( sizeof(MemberCommand) );
    command->NameLength = name->NameLength;
    command->Flags &= ~3;
    command->Flags |= (owns_name & 1) << 1;
    command->Name = name->Name;
    command->Next = (MemberCommand *)MemberWork;
    MemberWork = (WorkItem *)command;
}

/* Original0x410be0..0x410c72 plus the7-byte return suffix410ad9.
   *- both extracts and deletes; plain* leaves the original library member. */
unsigned char ParseExtractCommand( void )
{
    unsigned char ch;
    char token[260];
    FileName *name;

    ch = NextNonwhite();
    if( ch == '-' ) {
        ch = ReadToken( 0, token );
        if( token[0] == 0 ) ReportSyntaxError( 68, 102, &ch );
        name = AddExtract( token );
        AddRemoveCommand( name, 0 );
    } else {
        ch = ReadToken( ch, token );
        if( token[0] == 0 ) ReportSyntaxError( 68, 102, &ch );
        AddExtract( token );
    }
    return( ch );
}

/* Original0x410cda..0x410d0a shares AddModule's6-byte return suffix
   at410ca0. Extraction names use the separately configurable path. */
FileName *AddExtract( const char *text )
{
    FileName *name;

    name = AllocPoolItem( &NamePool, sizeof(FileName) );
    SetFileName( name, &ExtractionPath, text );
    name->Next = (FileName *)ExtractWork;
    ExtractWork = (WorkItem *)name;
    return( name );
}

/* Original0x410a27..0x410ae0. ++ defines an import; +- replaces a
   module while plain+ appends it to the pending module list. */
unsigned char ParseAddCommand( void )
{
    unsigned char ch;
    char token[260];
    FileName *name;

    ch = NextNonwhite();
    if( ch == '+' ) return( ParseImportCommand() );
    if( ch == 0 ) {
        ReportSyntaxError( 69, 88, &ch );
    } else if( ch == '-' ) {
        ch = ReadToken( 0, token );
        if( token[0] == 0 ) ReportSyntaxError( 68, 87, &ch );
        name = AddModule( token );
        AddRemoveCommand( name, 0 );
    } else {
        ch = ReadToken( ch, token );
        if( token[0] == 0 ) ReportSyntaxError( 69, 88, &ch );
        AddModule( token );
    }
    return( ch );
}

/* Original entry41004e shares410044..41004e with the argv reader.
   Unlike that reader, a string callback advances even past its final NUL. */
unsigned char ReadStringCharacter( void )
{
    return( (unsigned char)*CommandCurrent++ );
}

/* Original0x41086c..0x410a27. Synthetic import entries reuse the22-byte
   command-node slot with pointer/ordinal fields specific to Flags3. */
unsigned char ParseImportCommand( void )
{
    ImportCommand *command;
    char token[260];
    unsigned char ch;
    char *scan;

    command = AllocPoolItem( &NamePool, sizeof(ImportCommand) );
    command->Next = (FileName *)CommandWork;
    CommandWork = (WorkItem *)command;
    command->Flags = 3;
    command->Ordinal = 0;
    command->Entry = NULL;
    ch = ReadNamePart( 0, token );
    if( ch != '.' || token[0] == 0 ) ReportSyntaxError( 70, 97, &ch );
    command->Symbol = DuplicateText( token );
    ch = ReadNamePart( 0, token );
    if( token[0] == 0 ) ReportSyntaxError( 71, 97, &ch );
    command->Module = DuplicateText( token );
    if( ch == '.' ) {
        ch = ReadNamePart( 0, token );
        if( !isdigit( (unsigned char)token[0] ) ) {
            if( token[0] != 0 ) {
                FreeMemory( command->Symbol );
                command->Symbol = DuplicateText( token );
            }
            if( ch != '.' ) return( ch );
            ch = ReadNamePart( 0, token );
            if( token[0] == 0 ) ReportSyntaxError( 72, 97, &ch );
            if( !isdigit( (unsigned char)token[0] ) ) {
                command->Entry = DuplicateText( token );
                if( ch != '.' ) return( ch );
                ch = ReadNamePart( 0, token );
            }
        }
        for( scan = token; *scan != 0; ++scan ) {
            if( !isdigit( (unsigned char)*scan ) ) {
                ReportSyntaxError( 72, 97, &ch );
            }
        }
        if( command->Entry == NULL ) command->Ordinal = atol( token );
    }
    return( ch );
}

/* Original entry4101aa..4101b5 joins ReadToken's actual CALL/return
   tail4101a3..4101aa. This wrapper treats '.' as an import separator. */
unsigned char ReadNamePart( unsigned char ch, char *buffer )
{
    return( ScanToken( ch, buffer, '.' ) );
}

/* Original0x410841..0x41086c keeps import strings beyond the parser's
   automatic token buffer. Allocation stays through the App wrapper. */
char *DuplicateText( const char *text )
{
    unsigned length, i;
    char *copy;

    for( length = 0; text[length] != 0; ++length ) {}
    copy = AllocMemory( length + 1 );
    for( i = 0; i < length; ++i ) copy[i] = text[i];
    copy[length] = 0;
    return( copy );
}

/* Original0x41021f..0x410238. A newline resumes tokenization through
   the ordinary whitespace reader; EOF remains the zero return value. */
unsigned char SkipComment( void )
{
    unsigned char ch;

    do {
        ch = (*CommandReader)();
    } while( ch != '\n' && ch != 0 );
    if( ch != 0 ) return( NextNonwhite() );
    return( ch );
}

/* Original0x4107f3..0x410841: a bare object name means replace/add,
   borrowing its counted member name for the paired deletion request. */
unsigned char ParseModuleName( unsigned char ch )
{
    char token[260];
    FileName *name;

    ch = ReadToken( ch, token );
    if( token[0] == 0 ) ReportSyntaxError( 68, 87, &ch );
    name = AddModule( token );
    AddRemoveCommand( name, 0 );
    return( ch );
}

/* Original0x4104cc..0x41054e. N/R selects the nonresident/resident
   export-name table; N/O selects importing by name or by ordinal. */
unsigned char SetImportOption( void )
{
    unsigned char ch;
    unsigned char *option;

    ch = NextNonwhite();
    switch( toupper( ch ) ) {
    case 'N': option = &InCommand; break;
    case 'R': option = &FirstCommand; break;
    default: ReportSyntaxError( 74, 99, &ch ); break;
    }
    switch( toupper( NextNonwhite() ) ) {
    case 'N': *option = 1; break;
    case 'O': *option = 0; break;
    default: ReportSyntaxError( 81, 99, &ch ); break;
    }
    return( NextNonwhite() );
}
