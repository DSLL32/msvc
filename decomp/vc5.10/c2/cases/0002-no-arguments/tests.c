/* Exercise the installed executable, not a fixture responder or entry shortcut. */
#define RECON_SHIM_IMPL
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <recon-tool.h>
#include <recon.h>

static int installed_no_arguments(void)
{
    static const char diagnostic[] =
        "fatal error C1083: Cannot open compiler intermediate file: 'in': "
        "No such file or directory\r\n\r\n";
    char actual[sizeof(diagnostic)];
    char *arguments[] = { C2_CANONICAL_NATIVE_EXE, NULL };
    char *environment[] = { NULL };
    FILE *input, *output, *error;
    pid_t child;
    int status, ok;
    /* The case workspace is isolated; the missing input is intentional. */
    if (access("in", F_OK) == 0) return 0;
    input = fopen("/dev/null", "rb");
    output = tmpfile();
    error = tmpfile();
    if (!input || !output || !error) return 0;
    child = fork();
    if (child == 0) {
        if (dup2(fileno(input), 0) < 0 || dup2(fileno(output), 1) < 0 ||
            dup2(fileno(error), 2) < 0) _exit(126);
        execve(arguments[0], arguments, environment);
        _exit(127);
    }
    ok = child > 0 && waitpid(child, &status, 0) == child &&
        WIFEXITED(status) && WEXITSTATUS(status) == 1;
    rewind(output);
    rewind(error);
    ok = ok && fgetc(output) == EOF &&
        fread(actual, 1, sizeof(actual), error) == sizeof(diagnostic) - 1 &&
        memcmp(actual, diagnostic, sizeof(diagnostic) - 1) == 0;
    fclose(input);
    fclose(output);
    fclose(error);
    return ok;
}

const ReconTest noargs_tests[] = {
    { "installed C2.EXE no arguments retains missing-intermediate diagnostic and POSIX exit1", 1,
      installed_no_arguments }
};
