/* Native host binding only, outside target source and tracing. */
#define RECON_SHIM_IMPL
#include <recon.h>

static void __attribute__((constructor(104))) bind_first_main_heap( void )
{
    recon_bind_watcom_wlib_first_main_heap(
        "43a88e9b5f61d369a862f6eecd56c6aef78074dd20e005ae754cc96d9a333d5b" );
}
