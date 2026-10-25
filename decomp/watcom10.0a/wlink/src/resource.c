/* SDK WRes handle adapter, VA4348b8..4348c7. */
#include "link.h"
#include <recon.h>

int OpenResFile( HANDLE_INFO *instance )
{
    return instance->handle = ResOpenFileRO( instance->filename );
}
