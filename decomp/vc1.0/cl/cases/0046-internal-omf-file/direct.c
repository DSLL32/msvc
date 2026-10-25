/* Existing internal-state case: mutate at real link_pass entry, then resume.
 * This does not claim that a public command-line option selects OMF. */
#include <recon-native.h>
#include "../../src/cl.h"
const void *cl_case_state_entry(void) { return (const void *)link_pass; }
void cl_case_state_mutation(void *unused)
{
    (void)unused;
    coff_format = 0;
}

struct StateInvocation { int argc; char **argv; int status; };
extern int main(int, char **, char **);
static void run_main(void *argument)
{
    struct StateInvocation *invocation = argument;
    invocation->status = main(invocation->argc, invocation->argv, (char **)0);
}
int cl_case_invoke(int argc, char **argv)
{
    struct StateInvocation invocation;
    int observed;
    invocation.argc = argc; invocation.argv = argv; invocation.status = 125;
    observed = recon_native_observe_entry(cl_case_state_entry(),
        cl_case_state_mutation, NULL, run_main, &invocation);
    return observed == 1 ? invocation.status : 125;
}
