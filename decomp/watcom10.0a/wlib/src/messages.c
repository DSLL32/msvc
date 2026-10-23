/* Original InitMsg: 0x414bfe..0x414c67. The 2002 nwlib source changed
   argument acquisition, resource cleanup, language selection and error
   handling; the 1994 instructions establish the flow retained here. */
#include <stdio.h>
#include <io.h>
#include "wlib.h"
#include <recon.h>

static HANDLE_INFO hInstance;       /* Original data at 0x420444. */
static int Res_Flag;                /* Original data at 0x43034c. */

int InitMsg( const char *image_name )
{
    int error;

    hInstance.filename = image_name;
    error = OpenResFile( &hInstance ) == -1;
    if( !error ) {
        error = FindResources( &hInstance );
    }
    if( !error ) {
        error = InitResources( &hInstance );
    }
    if( error ) {
        Res_Flag = 255;
        write( fileno( stdout ),
               "Error: could not open message resource file.\r\n", 46 );
    } else {
        Res_Flag = 0;
    }
    return( Res_Flag );
}
