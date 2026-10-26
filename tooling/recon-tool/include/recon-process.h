#ifndef RECON_PROCESS_H
#define RECON_PROCESS_H
/* Native fixture glue only; no process provider runs original code or Wine. */
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
#define RECON_PROCESS_POLICY_VERSION 1u
#define RECON_PROCESS_RSP1 1u
#define RECON_PROCESS_RSP2 2u
#define RECON_PROCESS_CAPTURED 0u
#define RECON_PROCESS_SELF 1u

typedef struct ReconProcessRequest {
    uint32_t struct_size, index;
    int32_t mode;
    uint32_t pid, protocol;
    const char *file, *cwd;
    uint32_t argc;
    const char *const *argv;
    uint32_t environment_count;
    const char *const *environment;
    int32_t parent_errno;
    uint32_t parent_doserrno, parent_last_error, environment_source;
    /* Trusted runner paths; cwd above is the actual shim's untrusted request. */
    const char *workspace_cwd, *logical_cwd, *assets_root;
    /* Requesting executor's full post-setup host environment. Explicit target-OS
     * environments and application mutations require their own case contract. */
    uint32_t prepared_environment_count;
    const char *const *prepared_environment;
    /* Optional trusted initial target OS block retained in capture.bin. Empty
     * without that opt-in; distinct from both host prepared_environment and
     * this request's actual (possibly mutated) environment. */
    uint32_t target_os_environment_count;
    const char *const *target_os_environment;
} ReconProcessRequest;

typedef struct ReconProcessResponse {
    uint32_t struct_size, action;
    int32_t status; /* captured: -1 or 0..255; self: exactly zero */
    uint32_t failure_errno; /* captured -1: 1..4095; otherwise zero */
    const void *stdout_data;
    uint32_t stdout_size;
    const void *stderr_data;
    uint32_t stderr_size;
} ReconProcessResponse;

typedef int (*ReconProcessCallback)(const ReconProcessRequest *,
                                    ReconProcessResponse *);
typedef struct ReconProcessPolicy {
    uint32_t abi_version, struct_size, expected_request_count;
    ReconProcessCallback request;
} ReconProcessPolicy;
/* Optional case export. Absent means deny all process requests. A callback must
 * validate the complete invocation and all inputs before applying declared
 * captured file effects and accepting. Zero rejects the whole fixture.
 * Request pointers are borrowed for this call. Response buffers are borrowed
 * until the next callback; the runner copies them immediately after return.
 * Callback state belongs to the isolated, untraced broker snapshot. */
extern const ReconProcessPolicy case_process_policy;
#ifdef __cplusplus
}
#endif
#endif
