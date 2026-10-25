#ifndef RECON_OUTPUT_H
#define RECON_OUTPUT_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
#define RECON_OUTPUT_POLICY_VERSION 1u
#define RECON_OUTPUT_EXIT 1u
#define RECON_OUTPUT_SIGNAL 2u
/* Borrowed paths/context are valid only during this callback. The checker runs
 * in the original post-setup snapshot, after all target descendants have exited.
 * It may inspect files, never target post-exit globals. Raw streams are immutable;
 * comparison copies may change only after validating case-specific substitutions.
 * Frozen generic stream/file assertions still run, and false always fails. */
typedef struct ReconOutputContext {
    uint32_t struct_size;
    uint32_t executor_pid;
    uint32_t terminal_kind;
    uint32_t terminal_value; /* Full DWORD exit status, or POSIX signal number. */
    const char *workspace_cwd;
    const char *assets_root;
    const char *stdout_raw;
    const char *stderr_raw;
    const char *stdout_compare;
    const char *stderr_compare;
} ReconOutputContext;
typedef int (*ReconOutputCallback)(const ReconOutputContext *);
typedef struct ReconOutputPolicy {
    uint32_t abi_version;
    uint32_t struct_size;
    ReconOutputCallback check_outputs;
} ReconOutputPolicy;
/* Optional native-only export from case.so. */
extern const ReconOutputPolicy case_output_policy;
#ifdef __cplusplus
}
#endif
#endif
