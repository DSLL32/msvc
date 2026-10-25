/* Original OpenResFile, 0x418ac8..0x418ad7. WRes resource support is part
   of this application's function graph, rather than a CRT exemption. */
#include <fcntl.h>
#include <io.h>
#include "wlib.h"
#include <recon.h>

struct WResRoutines WResRtns = { open }; /* Original table at 0x42044c. */

int OpenResFile( HANDLE_INFO *instance )
{
    return( instance->handle = ResOpenFileRO( instance->filename ) );
}

/* Original 0x419c40..0x419c65 passes binary mode directly to open.
   The newer released source adds a setmode call absent from this binary. */
int ResOpenFileRO( const char *filename )
{
    int handle;

    handle = (*WResRtns.open)( filename, O_RDONLY | O_BINARY );
    if( handle == -1 ) {
        WresRecordError( WRS_OPEN_FAILED );
    }
    return( handle );
}
