#ifndef RECON_FIXTURE_H
#define RECON_FIXTURE_H
#ifdef __cplusplus
extern "C" {
#endif
/* Native harness metadata, never inserted into the application's environment.
 * Borrowed immutable paths are configured before case setup. */
const char *recon_case_assets(void);
const char *recon_case_workspace_cwd(void);
const char *recon_case_logical_cwd(void);
#ifdef __cplusplus
}
#endif
#endif
