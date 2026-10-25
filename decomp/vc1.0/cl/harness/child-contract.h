#ifndef CL_CHILD_CONTRACT_H
#define CL_CHILD_CONTRACT_H
#include <stdint.h>
#include <recon-process.h>
/* CL's frozen compiler/linker boundary; bytes are capture-owned assets. */
typedef struct ClCapturedFile {
    const char *name, *asset;
    uint32_t size;
} ClCapturedFile;
typedef struct ClCapturedChild {
    const char *file, *logical_cwd;
    const char *const *argv;
    uint32_t argc;
    const char *const *environment;
    uint32_t environment_count;
    int explicit_environment;
    const ClCapturedFile *before, *after;
    uint32_t before_count, after_count;
    const char *image_asset, *image_path, *source_asset;
    uint32_t image_size, source_size;
    const char *stdout_asset, *stderr_asset;
    uint32_t stdout_size, stderr_size;
    int status, failure_errno, signal_number, missing;
} ClCapturedChild;
int cl_captured_child(const ReconProcessRequest *, ReconProcessResponse *,
                      const ClCapturedChild *, uint32_t);
#endif
