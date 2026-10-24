/* The original's default builtin operation reads an uninitialized local.
 * This native-only rejection test claims no equivalence or public reachability;
 * the ordinary QuickC case is the genuine valid control. */
#include <sys/wait.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <fcntl.h>
#include <recon-tool.h>
#include "../../src/initializers.h"
#include <recon.h>

static int rejects_uninitialized_builtin_default(void)
{
    int child = fork(), status, sink;
    if (child < 0)
        return 0;
    if (!child)
    {
        sink = syscall(SYS_open, "/dev/null", O_WRONLY, 0);
        if (sink < 0 || syscall(SYS_dup2, sink, 2) != 2)
            syscall(SYS_exit_group, 90);
        emit_initializer_builtin_call(0, 2, 0);
        syscall(SYS_exit_group, 99);
    }
    if (waitpid(child, &status, 0) != child)
        return 0;
    return WIFEXITED(status) && WEXITSTATUS(status) == 125;
}

const ReconTest c1_quickc_tests[] = {
    {"reject-uninitialized-builtin-default", 1, rejects_uninitialized_builtin_default}};
