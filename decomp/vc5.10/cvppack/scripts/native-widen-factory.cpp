/* Native host binding for the approved external-DLL case provider, outside
 * the reconstructed target's function graph. */
#define RECON_SHIM_IMPL
#include "widen.h"
#include <recon.h>
extern "C" int cvpack_widen_factory(WidenTi **, unsigned, int);
int WidenTi::fCreate(WidenTi *&object, unsigned capacity, int flags)
{
    return cvpack_widen_factory(&object, capacity, flags);
}
