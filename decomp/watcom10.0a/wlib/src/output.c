/* Direct descriptor output in the older librarian. Text-mode translation
   belongs to write, not these ordinary point-of-use string literals. */
#include <string.h>
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
