/* Existing internal-state case: clear the live IDE buffer at ide_message
 * entry, then resume its original assertion/diagnostic/exit path. */
#include <stddef.h>
#include <recon-native.h>
#include "../../src/cl.h"
typedef struct CaseIdeCallbacks {
    int (__cdecl *descriptor)(int);
    int (__cdecl *write)(int, char *, int);
    int (__cdecl *abort)(char *, int);
    char *buffer;
    unsigned capacity;
} CaseIdeCallbacks;
typedef char CaseIdeLayout[(sizeof(CaseIdeCallbacks) == 20 &&
    offsetof(CaseIdeCallbacks, buffer) == 12) ? 1 : -1];
extern CaseIdeCallbacks *ide_callbacks;
const void *cl_case_state_entry(void) { return (const void *)ide_message; }
void cl_case_state_mutation(void *unused)
{
    (void)unused;
    ide_callbacks->buffer = NULL;
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
