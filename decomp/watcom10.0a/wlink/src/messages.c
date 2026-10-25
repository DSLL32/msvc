/* Message/resource startup, VA424850..4248d6. The later wlnkmsg.c
 * identifies the resource operations; this target accepts the self filename
 * directly and also initializes its150-byte diagnostic-enable bitset. */
#include <stdio.h>
#include <string.h>
#include "link.h"
#include <recon.h>

unsigned char MsgFlags[150];       /* original450224 */
static HANDLE_INFO hInstance;      /* original441f88 */
static int Res_Flag;               /* original450220 */

int InitMsg( char *program )
{
    int initerror;

    memset( MsgFlags, 0xff, sizeof( MsgFlags ) );
    hInstance.filename = program;
    OpenResFile( &hInstance );
    if( hInstance.handle == -1 ) {
        initerror = 1;
    } else {
        initerror = FindResources( &hInstance );
    }
    if( !initerror ) {
        initerror = InitResources( &hInstance );
    }
    if( initerror ) {
        Res_Flag = -1;
        QWrite( fileno( stdout ),
                "Error(1000): could not open message resource file.\r\n", 52, 0 );
    } else {
        Res_Flag = 0;
    }
    return Res_Flag;
}
