/* Native-only dependency rejection probe, outside the target function graph. */
#include "rc.h"
#include "native-fixture.h"
#include <recon.h>

int WINAPI rc_diagnostic(HWND window, unsigned code, char *message)
{
    printf("UNEXPECTED_CALLBACK\n");
    exit(126);
    return 0;
}
int main(int argc, char **argv)
{
    const char *mode = getenv("RC_REJECTION");
    HWND window = NULL;
    unsigned flags = 0;
    ResourceCallback resource = NULL;
    DiagnosticCallback callback = rc_diagnostic;
    char **arguments;
    char *other[3];
    int count;

    if (!strcmp(mode, "host-case"))
        argv[1] = "/unknown";
    rc_fixture_prepare(argc, argv);
    arguments = rc_fixture_argv;
    count = rc_fixture_argc;
    if (!strcmp(mode, "window")) window = (HWND)1;
    if (!strcmp(mode, "flags")) flags = 1;
    if (!strcmp(mode, "resource")) resource = (void *)1;
    if (!strcmp(mode, "callback")) callback = NULL;
    if (!strcmp(mode, "argc")) ++count;
    if (!strcmp(mode, "argv-alias")) {
        other[0] = arguments[0]; other[1] = arguments[1]; other[2] = NULL;
        arguments = other;
    }
    if (!strcmp(mode, "argv-mutation")) arguments[1][1] = 'X';
    if (!strcmp(mode, "argv-terminator")) arguments[2] = arguments[0];
    return RC(window, flags, callback, resource, count, arguments);
}
