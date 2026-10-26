/* Message/resource startup, VA424850..4248d6. The later wlnkmsg.c
 * identifies the resource operations; this target accepts the self filename
 * directly and also initializes its150-byte diagnostic-enable bitset. */
#include <stdio.h>
#include <string.h>
#include "cmdline.h"
#include <recon.h>

#ifdef __WATCOMC__
#pragma intrinsic(strlen)
#endif

unsigned char BannerPrinted;        /* original441ee4 */

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

void WLPrtBanner( void )
{
    char *msg;

    if( !BannerPrinted ) {
        msg = "WATCOM Linker Version 10.0";
        WriteStdOut( msg, strlen(msg) );
        msg = "Copyright by WATCOM International Corp. 1985, 1994. All rights reserved.";
        WriteStdOut( msg, strlen(msg) );
        msg = "WATCOM is a trademark of WATCOM International Corp.";
        WriteStdOut( msg, strlen(msg) );
        BannerPrinted = 1;
    }
}

void WriteStdOut( char *text, unsigned length )
{
    QWrite( fileno(stdout), text, length, 0 );
    QWriteNL( fileno(stdout), 0 );
}

void Msg_Get( unsigned id, char *buffer )
{
    if( Res_Flag != 0 || WResLoadString( &hInstance, id, buffer, 128 ) != 0 ) {
        buffer[0] = '\0';
    }
}

unsigned char FiniMsg( void )
{
    if( Res_Flag == 0 ) {
        if( CloseResFile( &hInstance ) == -1 ) return 255;
        Res_Flag = 255;
    }
    return 0;
}
