/* Native host smoke only. execve never invokes a shell or a Wine launcher. */
#define RECON_SHIM_IMPL
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <recon-fixture.h>
#include <recon.h>

static FILE *oracle(const char *name)
{
    const char *root = recon_case_assets();
    char path[4096];
    if (!root || snprintf(path, sizeof(path), "%s/%s", root, name) >= sizeof(path))
        return NULL;
    return fopen(path, "rb");
}
static int same_stream(FILE *actual, const char *name)
{
    FILE *expected = oracle(name);
    int a, b, equal = 1;
    if (!expected || fseek(actual, 0, SEEK_SET)) {
        if (expected) fclose(expected);
        return 0;
    }
    do {
        a = fgetc(actual);
        b = fgetc(expected);
        if (a != b) { equal = 0; break; }
    } while (a != EOF);
    equal = equal && !ferror(actual) && !ferror(expected);
    fclose(expected);
    return equal;
}
int installed_no_arguments(void)
{
    FILE *status_file = oracle("original-status-dword");
    FILE *output, *errors;
    unsigned char bytes[4];
    uint32_t original_status;
    pid_t child, waited;
    int status, ok;
    if (!status_file) return 0;
    ok = fread(bytes, 1, sizeof(bytes), status_file) == sizeof(bytes) &&
         fgetc(status_file) == EOF && !ferror(status_file);
    fclose(status_file);
    if (!ok) return 0;
    original_status = (uint32_t)bytes[0] | ((uint32_t)bytes[1] << 8) |
        ((uint32_t)bytes[2] << 16) | ((uint32_t)bytes[3] << 24);
    output = tmpfile();
    errors = tmpfile();
    if (!output || !errors) {
        if (output) fclose(output);
        if (errors) fclose(errors);
        return 0;
    }
    child = fork();
    if (child == 0) {
        char *arguments[] = {RC_CANONICAL_EXE, NULL};
        char *environment[] = {NULL};
        if (dup2(fileno(output), STDOUT_FILENO) < 0 ||
            dup2(fileno(errors), STDERR_FILENO) < 0) _exit(126);
        execve(RC_CANONICAL_EXE, arguments, environment);
        _exit(127);
    }
    if (child < 0) { fclose(output); fclose(errors); return 0; }
    do { waited = waitpid(child, &status, 0); } while (waited < 0 && errno == EINTR);
    /* Original keeps its full DWORD oracle; POSIX exposes only its low byte. */
    ok = waited == child && WIFEXITED(status) &&
         WEXITSTATUS(status) == (original_status & 255) &&
         same_stream(output, "original.stdout") &&
         same_stream(errors, "original.stderr");
    fclose(output);
    fclose(errors);
    return ok;
}
