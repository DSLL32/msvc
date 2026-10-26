/* Object input buffer initialization. */
#include "link.h"
#include <recon.h>

unsigned TokSize; /* original4500d8 */
char *TokBuff;     /* original4500e4 */

void InitTokBuff( void )
{
    TokSize = 4096;
    TokBuff = ChkLAlloc( 4096 );
}


void FreeTokBuffs( void )
{
    if( TokBuff != 0 ) {
        _LnkFree( TokBuff );
        TokBuff = 0;
    }
}
