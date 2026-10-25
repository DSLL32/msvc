/* This target links the ordinary, uninstrumented memory configuration.
 * LnkMemInit and LnkMemFini share the same one-byte RET at VA433300.
 * The released mem.c confirms both operations are empty without TRACKER,
 * _INT_DEBUG or the other platform-specific reserve-memory configurations. */
#include "link.h"
#include <recon.h>

void LnkMemFini( void )
{
}
