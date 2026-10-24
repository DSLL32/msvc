/* Native-only unsupported-mode guard probe, outside the target graph. */
#define RECON_SHIM_IMPL
#include <stdlib.h>
#include "command.h"
#include "directives.h"
#include <recon.h>

int main(int count,char **arguments)
{
    if(count!=2)return 2;
    rc_number_buffer_active=atoi(arguments[1]);
    return rc_evaluate_defined(L"defined");
}
