/* Cached original invocation; no original execution during migration. */
#include <dirent.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <recon-tool.h>
#include <recon-process.h>
extern int main(int, char **, char **);
static int invoke(int argc, char **argv) { return main(argc, argv, (char **)0); }
static const uint32_t order[] = {
    0x7000, /* main */
    0x74c0, /* parse_options */
    0xed60, /* fatal_error */
    0x76c0, /* print_banner */
};
RECON_EXPORT const ReconCase case_descriptor = {
    RECON_ABI_VERSION, sizeof(ReconCase),
    "help", "Help invocation: exact missing QH viewer dependency, 219-byte stdout and exit 1.",
    order, RECON_COUNT(order), NULL, 0,
    RECON_IO, RECON_EXIT, 0,
    NULL, invoke, NULL, NULL, 0
};

/* Frozen original missing-viewer request: no child, file inputs or streams.
 * The prepared vector is the runner's trusted native environment at the
 * broker checkpoint. This target does not mutate it before _spawnlp. */
static int environment_value(const ReconProcessRequest *request,
                             const char *name, const char *expected)
{
    uint32_t i, count = 0;
    size_t length = strlen(name);
    const char *value = NULL;
    for (i = 0; i < request->environment_count; ++i) {
        const char *entry = request->environment[i];
        if (!strncmp(entry, name, length) && entry[length] == '=') {
            value = entry + length + 1;
            ++count;
        }
    }
    return expected ? count == 1 && !strcmp(value, expected) : count == 0;
}

static int qh_request(const ReconProcessRequest *request,
                      ReconProcessResponse *response)
{
    uint32_t i;
    DIR *directory;
    struct dirent *entry;
    int empty = 1;
    if (!request || request->struct_size != sizeof(*request) || !response ||
        request->mode != 0 || request->protocol != RECON_PROCESS_RSP1 ||
        request->environment_source != 0 || !request->file ||
        strcmp(request->file, "qh.exe") || request->argc != 3 || !request->argv ||
        !request->argv[0] || strcmp(request->argv[0], "qh") ||
        !request->argv[1] || strcmp(request->argv[1], "-u") ||
        !request->argv[2] || strcmp(request->argv[2], "cvpack.exe") ||
        !request->cwd || !request->workspace_cwd ||
        strcmp(request->cwd, request->workspace_cwd) ||
        request->environment_count != request->prepared_environment_count ||
        !request->environment || !request->prepared_environment)
        return 0;
    for (i = 0; i < request->environment_count; ++i)
        if (!request->environment[i] || !request->prepared_environment[i] ||
            strcmp(request->environment[i], request->prepared_environment[i]))
            return 0;
    if (!environment_value(request, "PATH", "") ||
        !environment_value(request, "_MSC_IDE_FLAGS", NULL) ||
        !environment_value(request, "LIB", "Z:\\root\\msvc2\\toolchains\\1.0-nt\\lib") ||
        !environment_value(request, "RECON_PROGRAM_PATH",
                           "Z:\\root\\msvc2\\toolchains\\1.0-nt\\bin\\CVPACK.EXE"))
        return 0;
    directory = opendir(request->workspace_cwd);
    if (!directory) return 0;
    errno = 0;
    while ((entry = readdir(directory)) != NULL)
        if (strcmp(entry->d_name, ".") && strcmp(entry->d_name, "..")) empty = 0;
    if (errno) empty = 0;
    if (closedir(directory)) empty = 0;
    if (!empty) return 0;
    memset(response, 0, sizeof(*response));
    response->struct_size = sizeof(*response);
    response->action = RECON_PROCESS_CAPTURED;
    response->status = -1;
    response->failure_errno = 2;
    return 1;
}

RECON_EXPORT const ReconProcessPolicy case_process_policy = {
    RECON_PROCESS_POLICY_VERSION, sizeof(ReconProcessPolicy), 1, qh_request
};
