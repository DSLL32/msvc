/* Original main: VA 0x412a67..0x412af5, 13 calls to nine physical callees.
   Recovered from the 1994 body. The 2002 nwlib entry has a different subsystem
   architecture and is not substituted here. CRT bodies remain library calls;
   native Watcom compatibility is supplied by lib/recon, not local CRT code. */
#include "wlib.h"
#include <recon.h>

jmp_buf Env;
unsigned long ToolFlags;

int main( int argc, char **argv )
{
    _heapchk();
    CheckMemoryHook();
    InitMsg( argv[0] );
    signal( SIGABRT, HandleSignal );
    signal( SIGBREAK, HandleSignal );
    signal( SIGTERM, HandleSignal );
    signal( SIGINT, HandleSignal );
    /* The original tests AL. Keep the consumed width explicit even when a
       later longjmp supplies a value outside the usual zero/one pair. */
    if( (unsigned char)setjmp( Env ) == 0 ) {
        InitToolState( argc, argv );
        ProcessCommands();
    }
    FiniToolState();
    FiniMsg();
    CheckMemoryHook();
    return( (ToolFlags & 0x40) != 0 );
}
