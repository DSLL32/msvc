/* Spill-file state initialization. */
#include "link.h"
#include <recon.h>

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
