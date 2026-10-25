/* Only CL's captured compiler/linker contracts live here. Shared recon-tool owns
 * process transport, isolation, request sequencing, stream delivery and cleanup.
 * This code never launches a dependency or discovers an alternative response. */
#define _GNU_SOURCE
#define RECON_SHIM_IMPL
#include "child-contract.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <limits.h>
#include <errno.h>
#include <signal.h>
#include <sys/stat.h>
#include <recon.h>

static void *previous_stdout, *previous_stderr;
static int reject(const char *reason)
{
    fprintf(stderr, "CL child contract: %s\n", reason);
    return 0;
}
static int joined(char *out, const char *root, const char *name)
{
    int length;
    if (!root || !name) return 0;
    length = snprintf(out, PATH_MAX, "%s/%s", root, name);
    return length >= 0 && length < PATH_MAX;
}
static int regular_size(int fd, uint32_t size)
{
    struct stat st;
    return fd >= 0 && !fstat(fd, &st) && S_ISREG(st.st_mode) && st.st_size == size;
}
static int same_file(const char *actual, const char *asset, uint32_t size)
{
    int left = -1, right = -1, result = 0;
    unsigned char a[8192], b[8192];
    size_t offset = 0, count;
    ssize_t n, m;
    left = open(actual, O_RDONLY | O_NOFOLLOW);
    right = open(asset, O_RDONLY | O_NOFOLLOW);
    if (!regular_size(left, size) || !regular_size(right, size)) goto done;
    while (offset < size) {
        count = size - offset;
        if (count > sizeof(a)) count = sizeof(a);
        n = read(left, a, count); m = read(right, b, count);
        if (n != (ssize_t)count || m != n || memcmp(a, b, count)) goto done;
        offset += count;
    }
    result = 1;
done:
    if (left >= 0) close(left);
    if (right >= 0) close(right);
    return result;
}
static const ClCapturedFile *named_file(const ClCapturedFile *files,
                                       uint32_t count, const char *name)
{
    uint32_t i;
    for (i = 0; i < count; ++i) if (!strcmp(files[i].name, name)) return files + i;
    return NULL;
}
static int tree(const char *root, const char *relative, const char *assets,
                const ClCapturedFile *files, uint32_t count, uint32_t *visited)
{
    char path[PATH_MAX], name[PATH_MAX], asset[PATH_MAX];
    DIR *directory;
    struct dirent *entry;
    struct stat st;
    const ClCapturedFile *expected;
    int result = 1;
    if (!joined(path, root, relative)) return 0;
    directory = opendir(path);
    if (!directory) return 0;
    while ((entry = readdir(directory)) != NULL) {
        if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, "..")) continue;
        if (snprintf(name, sizeof(name), "%s%s%s", relative,
                     *relative ? "/" : "", entry->d_name) >= sizeof(name) ||
            !joined(path, root, name) || lstat(path, &st)) { result = 0; break; }
        if (S_ISDIR(st.st_mode)) {
            if (!tree(root, name, assets, files, count, visited)) { result = 0; break; }
        } else {
            expected = named_file(files, count, name);
            if (!S_ISREG(st.st_mode) || !expected ||
                !joined(asset, assets, expected->asset) ||
                !same_file(path, asset, expected->size)) { result = 0; break; }
            ++*visited;
        }
    }
    if (closedir(directory)) result = 0;
    return result;
}
static int exact_tree(const char *root, const char *assets,
                      const ClCapturedFile *files, uint32_t count)
{
    uint32_t visited = 0;
    return tree(root, "", assets, files, count, &visited) && visited == count;
}
static int parent_directories(char *path)
{
    char *p;
    struct stat st;
    for (p = path + 1; *p; ++p) if (*p == '/') {
        *p = 0;
        if ((mkdir(path, 0777) && errno != EEXIST) || lstat(path, &st) ||
            !S_ISDIR(st.st_mode)) { *p = '/'; return 0; }
        *p = '/';
    }
    return 1;
}
static int apply_files(const char *root, const char *assets, const ClCapturedChild *child)
{
    uint32_t i;
    char path[PATH_MAX], asset[PATH_MAX];
    unsigned char buffer[8192];
    int source, target, ok;
    ssize_t n, offset, written;
    for (i = 0; i < child->before_count; ++i) {
        if (named_file(child->after, child->after_count, child->before[i].name)) continue;
        if (!joined(path, root, child->before[i].name) || unlink(path)) return 0;
    }
    for (i = 0; i < child->after_count; ++i) {
        if (!joined(path, root, child->after[i].name) || !parent_directories(path) ||
            !joined(asset, assets, child->after[i].asset)) return 0;
        source = open(asset, O_RDONLY | O_NOFOLLOW);
        if (!regular_size(source, child->after[i].size)) {
            if (source >= 0) close(source);
            return 0;
        }
        target = open(path, O_WRONLY | O_CREAT | O_TRUNC | O_NOFOLLOW, 0666);
        if (target < 0) { close(source); return 0; }
        ok = 1;
        while ((n = read(source, buffer, sizeof(buffer))) > 0) {
            offset = 0;
            while (offset < n) {
                written = write(target, buffer + offset, n - offset);
                if (written <= 0) { ok = 0; break; }
                offset += written;
            }
            if (!ok) break;
        }
        if (n < 0) ok = 0;
        close(source);
        if (close(target)) ok = 0;
        if (!ok) return 0;
    }
    return exact_tree(root, assets, child->after, child->after_count);
}
static void *asset_bytes(const char *root, const char *name, uint32_t size)
{
    char path[PATH_MAX];
    int fd;
    void *bytes;
    size_t offset = 0;
    ssize_t n;
    if (!joined(path, root, name)) return NULL;
    fd = open(path, O_RDONLY | O_NOFOLLOW);
    if (!regular_size(fd, size)) { if (fd >= 0) close(fd); return NULL; }
    bytes = malloc(size ? size : 1);
    if (!bytes) { close(fd); return NULL; }
    while (offset < size) {
        n = read(fd, (char *)bytes + offset, size - offset);
        if (n <= 0) { free(bytes); close(fd); return NULL; }
        offset += n;
    }
    close(fd);
    return bytes;
}
int cl_captured_child(const ReconProcessRequest *request, ReconProcessResponse *response,
                      const ClCapturedChild *children, uint32_t count)
{
    const ClCapturedChild *child;
    uint32_t i;
    char host[PATH_MAX], asset[PATH_MAX];
    struct stat st;
    void *image, *source;
    free(previous_stdout); free(previous_stderr);
    previous_stdout = previous_stderr = NULL;
    if (request->struct_size != sizeof(*request) || request->index >= count ||
        request->protocol != RECON_PROCESS_RSP1 || request->mode != 0 ||
        !request->workspace_cwd || !request->assets_root || !request->logical_cwd)
        return reject("unexpected request/header");
    child = children + request->index;
    if (strcmp(request->cwd, request->workspace_cwd) ||
        strcmp(request->logical_cwd, child->logical_cwd) ||
        recon_host_path(child->logical_cwd, host, sizeof(host)) ||
        strcmp(host, request->workspace_cwd)) return reject("child cwd differs");
    if (strcmp(request->file, child->file) || request->argc != child->argc)
        return reject("child program/argument count differs");
    for (i = 0; i < child->argc; ++i)
        if (strcmp(request->argv[i], child->argv[i])) return reject("child argv differs");
    if (child->explicit_environment) {
        if (request->environment_count != child->environment_count)
            return reject("explicit child environment count differs");
        for (i = 0; i < child->environment_count; ++i)
            if (strcmp(request->environment[i], child->environment[i]))
                return reject("explicit child environment differs");
    } else {
        if (request->environment_count != request->prepared_environment_count)
            return reject("inherited child environment count differs");
        for (i = 0; i < request->environment_count; ++i)
            if (strcmp(request->environment[i], request->prepared_environment[i]))
                return reject("inherited child environment differs");
    }
    if (child->missing) {
        if (recon_host_path(child->file, host, sizeof(host)) ||
            !lstat(host, &st) || errno != ENOENT) return reject("missing child now exists");
    } else {
        image = asset_bytes(request->assets_root, child->image_asset, child->image_size);
        if (!image || child->image_size < 2 || memcmp(image, "MZ", 2)) {
            free(image); return reject("captured child image differs");
        }
        free(image);
        if (child->image_path) {
            if (recon_host_path(child->image_path, host, sizeof(host)) ||
                !joined(asset, request->assets_root, child->image_asset) ||
                !same_file(host, asset, child->image_size)) return reject("child image input differs");
        }
        if (child->source_asset) {
            source = asset_bytes(request->assets_root, child->source_asset, child->source_size);
            if (!source) return reject("captured helper source differs");
            free(source);
        }
    }
    if (!exact_tree(request->workspace_cwd, request->assets_root, child->before,
                    child->before_count)) return reject("complete child input tree differs");
    previous_stdout = asset_bytes(request->assets_root, child->stdout_asset, child->stdout_size);
    previous_stderr = asset_bytes(request->assets_root, child->stderr_asset, child->stderr_size);
    if (!previous_stdout || !previous_stderr) return reject("captured child streams differ");
    if (!apply_files(request->workspace_cwd, request->assets_root, child))
        return reject("captured child file effects differ");
    if (child->signal_number && kill(request->pid, child->signal_number))
        return reject("child-running signal delivery failed");
    response->action = RECON_PROCESS_CAPTURED;
    response->status = child->status;
    response->failure_errno = child->failure_errno;
    response->stdout_data = previous_stdout; response->stdout_size = child->stdout_size;
    response->stderr_data = previous_stderr; response->stderr_size = child->stderr_size;
    return 1;
}
