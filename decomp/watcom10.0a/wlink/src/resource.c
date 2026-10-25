/* SDK WRes handle adapters. Actual1994graph omits the later setmode edge. */
#include <fcntl.h>
#include <io.h>
#include "link.h"
#include <recon.h>

struct WResRoutines {
    int (*open)( const char *, int, ... );
};
struct WResRoutines WResRtns = { open }; /* original441f94 */

int OpenResFile( HANDLE_INFO *instance )
{
    return instance->handle = ResOpenFileRO( instance->filename );
}

int ResOpenFileRO( const char *filename )
{
    int handle;

    handle = (*WResRtns.open)( filename, O_RDONLY | O_BINARY );
    if( handle == -1 ) {
        WresRecordError( 5 );
    }
    return handle;
}
