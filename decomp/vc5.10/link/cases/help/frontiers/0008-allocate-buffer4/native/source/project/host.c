/* Native/Win32 executable host; target source starts at link_main. */
#include <windows.h>
#include <stdlib.h>
#include <float.h>
#include "link.h"
#include "initializers.h"
#include <recon.h>

#if defined(RECON_NATIVE_TEST)
static int target_startup(int argc, char **argv, char **environment)
{
    Initializer c_initializers[1];
    c_initializers[0] = NULL;
    /* Native host for the verified shipped CRT startup boundary. */
    argv[0] = _pgmptr;
    _fmode = 0;
    _commode = 0;
    _controlfp(_PC_53, _MCW_PC);
    _initterm(c_initializers, c_initializers + 1);
    _initterm(application_initializers, application_initializers + 5);
    return link_main(argc, argv, environment);
}
#endif

#if defined(RECON_NATIVE_TEST)
int main(int argc, char **argv, char **environment)
{
    exit(target_startup(argc, argv, environment));
    return 255;
}
#endif
