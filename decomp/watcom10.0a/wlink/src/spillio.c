/* Spill-file state initialization. */
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include "cmdline.h"
#include <recon.h>
#ifdef __WATCOMC__
#pragma intrinsic(strlen,memcpy)
#endif

int TempFile;             /* original4500b8 */
unsigned long TmpFSize;    /* original4502c4 */

void InitSpillFile( void )
{
    TempFile = -1;
    TmpFSize = 0;
    SetBreak();
}

static char *TFileName;    /* original441fb4 */

void CloseSpillFile( void )
{
    if( TempFile != -1 ) {
        /* Both old break hooks have the same empty entry in this build. */
        SetBreak();
        QClose( TempFile, TFileName );
        QDelete( TFileName );
        _LnkFree( TFileName );
        TFileName = 0;
        TempFile = -1;
    }
}

char *MakeTempName( char *name )
{
    /* Actual1994 spelling, unlike the later xx extension. */
    memcpy( name, "WLK02112.--`", 13 );
    return name + 11;
}
extern int TempFileOpen( char * );
unsigned long SpillAlloc( unsigned amount )
{
    char *path, *tail;
    unsigned length, tries;
    unsigned long start;
    int handle;
    if( TempFile == -1 ) {
        path = getenv( "WLINKTMP" );
        if( path == 0 ) path = getenv( "TMP" );
        if( path == 0 ) path = getenv( "TMPDIR" );
        if( path == 0 ) {
            TFileName = ChkLAlloc( 13 );
            tail = TFileName;
        } else {
            length = strlen( path );
            TFileName = ChkLAlloc( length + 14 );
            memcpy( TFileName, path, length );
            switch( TFileName[length-1] ) {
            case ':': case '\\': case '/': break;
            default: TFileName[length++] = '\\'; break;
            }
            tail = TFileName + length;
        }
        tail = MakeTempName( tail );
        for( tries = 0;; ++tries ) {
            if( tries >= 26 ) { TempFile = -1; LnkMsg( 0x3483, 0 ); }
            ++*tail;
            handle = TempFileOpen( TFileName );
            TempFile = handle;
            if( handle == -1 ) break;
            QClose( handle, TFileName );
        }
        TempFile = QOpenRW( TFileName );
        LnkMsg( 0x1007, 0 );
    }
    TmpFSize = (TmpFSize + 511UL) & ~511UL;
    start = TmpFSize;
    TmpFSize += amount;
    return start + 1;
}
void SpillWrite( unsigned long base, unsigned long off, void *memory, unsigned size )
{
    QSeek( TempFile, base + off - 1, TFileName );
    QWrite( TempFile, memory, size, TFileName );
}
void SpillRead( unsigned long base, unsigned long off, void *memory, unsigned size )
{
    QSeek( TempFile, base + off - 1, TFileName );
    QRead( TempFile, memory, size, TFileName );
}
