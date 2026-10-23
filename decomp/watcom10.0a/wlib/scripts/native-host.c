/* Native host setup only; this is outside reconstructed application code.
   The referenced proof establishes the first-call empty target namespace.
   No application main is renamed or replaced. */
#define RECON_SHIM_IMPL
#include <recon.h>

static void __attribute__((constructor(104))) bind_first_main_heap( void )
{
    recon_bind_watcom_wlib_first_main_heap(
        "1b1f029451106e1e055c63ec21a78dac740984dcebe66bc212b73fbbaff1b625" );
}
