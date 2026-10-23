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
int main(int argc, char **argv, char **environment)
{
    const char *mode = recon_host_getenv("RC_REJECTION");
    HWND window = NULL;
    unsigned flags = 0;
    ResourceCallback resource = NULL;
    DiagnosticCallback callback = rc_diagnostic;
    char **arguments;
    char *other[3];
    char *original[6];
    int count, n, result;

    if (!strcmp(mode, "host-case"))
        argv[1] = "/unknown";
    rc_fixture_prepare(argc, argv);
    arguments = rc_fixture_argv;
    count = rc_fixture_argc;
    for (n = 0; n < count; ++n) original[n] = arguments[n];
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
    if (!strcmp(mode, "argv-terminator")) arguments[count] = arguments[0];
    if (!strcmp(mode, "environment-mutation")) rc_fixture_environment[0][0] = 'X';
    if (!strcmp(mode, "handler")) Handler(0);
    result = RC(window, flags, callback, resource, count, arguments);
    if (!strcmp(mode, "rich-positive")) {
        if (count != 6 || arguments[0] != original[0] || arguments[1] != original[1] ||
            arguments[2] != original[2] + 2 || arguments[3] != original[4] ||
            arguments[4] != original[4] + 3 || arguments[5] != original[5] ||
            arguments[6] || strcmp(arguments[2], "CASE_FROM_COMMAND=7") ||
            strcmp(arguments[3], "/foresources.res") || strcmp(arguments[4], "resources.res")) {
            fprintf(stderr, "Observed post-RC argv aliases differ\n");
            return 127;
        }
    }
    return result;
}
