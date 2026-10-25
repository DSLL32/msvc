/* Preserved native-only rejection control; no original/public reach claim. */
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include <recon-tool.h>
#include "cvtres.h"
#include <recon.h>
static int unsupported_depth(void)
{
    int status, child = fork();
    if (child < 0) return 0;
    if (!child) { read_pixel((const BYTE *)0, 0, 3); _exit(99); }
    if (waitpid(child, &status, 0) != child) return 0;
    return WIFSIGNALED(status) && WTERMSIG(status) == SIGTRAP;
}
const ReconTest pixel_tests[1] = {
    { "unvalidated pixel depth remains native INT3", 1, unsupported_depth }
};
