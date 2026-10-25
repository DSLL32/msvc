/* Native case metadata. This support is uninstrumented and never part of the
 * reconstructed Win32 application. Paths are not environment variables. */
#define _GNU_SOURCE
#include "recon-fixture.h"
#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

static const char *case_cwd, *case_logical, *case_assets;
static void *storage;
static size_t storage_size;
static pid_t owner;

int recon_case_configure_paths(const char *cwd, const char *logical,
                               const char *assets)
{
    const char *inputs[3];
    const char **outputs[3];
    size_t lengths[3], size = 0, i;
    char *memory, *cursor;
    if (!cwd || cwd[0] != '/' || (assets && assets[0] != '/') ||
        (storage && owner == getpid()))
    {
        errno = EINVAL;
        return 0;
    }
    inputs[0] = cwd;
    inputs[1] = logical;
    inputs[2] = assets;
    outputs[0] = &case_cwd;
    outputs[1] = &case_logical;
    outputs[2] = &case_assets;
    for (i = 0; i < 3; ++i)
    {
        lengths[i] = inputs[i] ? strlen(inputs[i]) + 1 : 0;
        if (lengths[i] > SIZE_MAX - size)
        {
            errno = EOVERFLOW;
            return 0;
        }
        size += lengths[i];
    }
    memory = mmap(NULL, size, PROT_READ | PROT_WRITE,
                  MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (memory == MAP_FAILED)
        return 0;
    cursor = memory;
    for (i = 0; i < 3; ++i)
    {
        if (lengths[i])
        {
            memcpy(cursor, inputs[i], lengths[i]);
            *outputs[i] = cursor;
            cursor += lengths[i];
        }
        else
            *outputs[i] = NULL;
    }
    if (mprotect(memory, size, PROT_READ))
    {
        munmap(memory, size);
        return 0;
    }
    if (storage)
        munmap(storage, storage_size);
    storage = memory;
    storage_size = size;
    owner = getpid();
    return 1;
}
const char *recon_case_assets(void) { return case_assets; }
const char *recon_case_workspace_cwd(void) { return case_cwd; }
const char *recon_case_logical_cwd(void) { return case_logical; }
