#include "rcdll.h"
#include <recon.h>

HWND rc_window;
RC_DIAGNOSTIC rc_diagnostic;
RC_RESOURCE rc_resource;

int WINAPI RC(HWND window, DWORD unused, RC_DIAGNOSTIC diagnostic,
    RC_RESOURCE resource, int count, char **arguments)
{
    rc_window = window;
    rc_diagnostic = diagnostic;
    rc_resource = resource;
    return main(count, arguments);
}
