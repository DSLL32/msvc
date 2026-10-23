/* Native host setup only; this is outside reconstructed application code.
   The referenced proof establishes the first-call empty target namespace.
   No application main is renamed or replaced. */
#define RECON_SHIM_IMPL
#include <recon.h>

static void __attribute__((constructor(104))) bind_first_main_heap( void )
{
    recon_bind_watcom_wlib_first_main_heap(
        "6f6ec24c1ce17f7f04e3b13e6d62cf283c1e87a663fb8ab4048a9e9c6a0650e4" );
}
