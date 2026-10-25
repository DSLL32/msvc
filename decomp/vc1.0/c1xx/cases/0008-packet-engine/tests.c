/* Native unsupported-data-domain control; no original-invalid-input claim. */
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <recon-tool.h>
#include "asm.h"
#include <recon.h>
static int check_mnemonic_initial_domain(void)
{
    int child, status;
    if (find_asm_mnemonic("$") != -1) return 0;
    child = fork();
    if (child < 0) return 0;
    if (!child) { find_asm_mnemonic(""); _exit(99); }
    if (waitpid(child, &status, 0) != child) return 0;
    return WIFSIGNALED(status) && WTERMSIG(status) == SIGTRAP;
}
const ReconTest packet_tests[] = {
    { "mnemonic initial data-domain guard", 1, check_mnemonic_initial_domain }
};
