/* Original OpenResFile, 0x418ac8..0x418ad7. WRes resource support is part
   of this application's function graph, rather than a CRT exemption. */
#include "wlib.h"
#include <recon.h>

int OpenResFile( HANDLE_INFO *instance )
{
    return( instance->handle = ResOpenFileRO( instance->filename ) );
}
