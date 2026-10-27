/* The oracle assets come from the untouched RC.EXE IO capture, SHA256
 * b5b376ec49a1cc5337916ebc87d5f89d5b66a72406eaecf9cd1f99ba29d6cbe9.
 * Its original IO report remains separate from this DIRECT host regression;
 * no original DIRECT invocation or child-entry trace is claimed. */
/* Installed-executable smoke; not a new captured-DLL library case. */
#include <recon-tool.h>
extern int installed_no_arguments(void);
RECON_EXPORT const ReconCase case_descriptor = {
    .abi_version = RECON_ABI_VERSION, .struct_size = sizeof(ReconCase),
    .name = "no-arguments",
    .description = "Installed native RC no-argument stdout/stderr and POSIX status against the genuine original oracle; no new library or child trace claim.",
    .execution = RECON_DIRECT, .terminal = RECON_EXIT,
    .direct = installed_no_arguments
};
