/* Native test host, outside the reconstructed target function graph. */
#include "rc.h"
#include "native-fixture.h"
#include <recon.h>

int main(int argc, char **argv, char **environment)
{
    rc_fixture_prepare(argc, argv);
    recon_crt_bind_arguments(rc_fixture_argc, rc_fixture_argv, rc_fixture_environment);
    mainCRTStartup();
    return 0;
}
