/* Direct descriptor output in the older librarian. Text-mode translation
   belongs to write, not these ordinary point-of-use string literals. */
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <io.h>
#include <stdio.h>
#include <conio.h>
#include "toolstate.h"
#include <recon.h>

#if defined(__WATCOMC__)
/* The original computes the length inline with REPNE SCASB. */
#pragma intrinsic(strlen)
#endif

/* Original0x4120f0..0x412147. Banner emission is guarded before the
   recursive call back into this output function. */
void PrintTextLine( const char *text )
{
    unsigned length;

    CheckBreak();
    if( !(ToolFlags & 0x800) ) {
        ToolFlags |= 0x800;
        PrintBanner();
    }
    length = strlen( text );
    write( 1, text, length );
    if( ListingHandle != 1 ) {
        write( ListingHandle, text, length );
    }
}

int BreakRequested;                /* Original0x430010, initially zero. */

/* Original0x412a43..0x412a5c. Consume a deferred break once, then
   enter the application's ordinary abort/cleanup path. */
void CheckBreak( void )
{
    if( BreakRequested ) {
        BreakRequested = 0;
        AbortCommand();
    }
}

/* Original0x413329..0x413333; ordinary banner literal stays at use. */
void PrintBanner( void )
{
    PrintTextLine( "WATCOM Library Manager Version 3.0\n"
                   "Copyright by WATCOM International Corp. 1988, 1994. All rights reserved.\n"
                   "WATCOM is a trademark of WATCOM International Corp.\n" );
}

/* Original0x414e4a..0x414ec6. Resource strings are grouped by empty
   entries; a single dot terminates usage and two empty entries stop it. */
void Usage( void )
{
    char buffer[128];
    unsigned message;
    int newpage;

    newpage = 0;
    for( message = 1000; ; ++message ) {
        MsgGet( message, buffer );
        if( buffer[0] == '.' && buffer[1] == '\0' ) {
            break;
        }
        if( newpage ) {
            if( buffer[0] == '\0' ) {
                break;
            }
            PauseOnConsole();
            PrintTextLine( buffer );
            PrintTextLine( "\n" );
            newpage = 0;
        } else if( buffer[0] == '\0' ) {
            newpage = 1;
        } else {
            PrintTextLine( buffer );
            PrintTextLine( "\n" );
        }
    }
}

/* Original0x412147..0x412185. Captured help redirects stdout to a
   regular file, so this case does not enter the console/FILE branch. */
void PauseOnConsole( void )
{
    if( isatty( 1 ) ) {
        PrintMessage( 5 );
        fputc( '\n', stdout );
        getch();
        fputc( '\n', stdout );
    }
}

/* Original0x414db9..0x414dd7 and the shared formatting tail414c9c.
   A diagnostic consists of a severity resource followed by its body. */
void __cdecl ReportError( unsigned destination, unsigned message, ... )
{
    va_list arguments;

    va_start( arguments, message );
    PrintMessage( destination );
    FormatMessage( message, &arguments );
    va_end( arguments );
}

/* Original0x414c8e..0x414cae. The shared tail consumes this call's
   own va_list; it is not a separate function or a forwarded public ABI. */
void __cdecl PrintMessage( unsigned message, ... )
{
    va_list arguments;

    va_start( arguments, message );
    FormatMessage( message, &arguments );
    va_end( arguments );
}

/* Original0x414ceb..0x414db9. Resource templates support numbered
   string arguments; uppercase S requests the librarian's symbol spelling. */
void FormatMessage( unsigned message, va_list *arguments )
{
    char buffer[128];
    char *values[9];
    char *scan, *start;
    unsigned current, index;

    MsgGet( message, buffer );
    current = 1;
    start = scan = buffer;
    while( *scan != '\0' ) {
        if( *scan == '%' ) {
            if( scan > start ) {
                *scan = '\0';
                PrintTextLine( start );
            }
            ++scan;
            if( isdigit( (unsigned char)*scan ) ) index = *scan++ - '0';
            else index = current;
            while( current <= index ) {
                values[current - 1] = va_arg( *arguments, char * );
                ++current;
            }
            start = scan + 1;
            if( *scan == 'S' ) PrintTextLine( FormatSymbolName( values[index - 1] ) );
            else if( *scan == 's' ) PrintTextLine( values[index - 1] );
            else --start;
        }
        ++scan;
    }
    PrintTextLine( start );
}

/* Original0x414cae..0x414ceb. Its shared mutable result buffer is
   consumed immediately by the resource formatter. */
const char *FormatSymbolName( const char *name )
{
    static char spelling[128];       /* Original0x430350. */
    unsigned i;

    if( ToolFlags & 0x8000 ) {
        i = 0;
        do { spelling[i] = name[i]; } while( name[i++] != '\0' );
    } else {
        __demangle_l( name, 0, spelling, sizeof(spelling) );
    }
    return( spelling );
}

/* Original0x414dd7..0x414e4a. Syntax errors bypass main's ordinary
   return path after releasing application and resource state. */
void ReportSyntaxError( unsigned message, unsigned context, unsigned char *ch )
{
    char explanation[128];
    char option[128];
    char found[128];

    MsgGet( message, explanation );
    MsgGet( context, option );
    if( *ch == '\0' ) {
        MsgGet( 65, found );
    } else {
        found[0] = *ch;
        found[1] = '\0';
    }
    ReportError( 2, 40, explanation, option, found );
    FiniToolState();
    FiniMsg();
    CheckMemoryHook();
    exit( 1 );
}
