/* Exact unsupported native NULL cursor; no original stale-stack claim. */
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <recon-tool.h>
#include "declarations.h"
#include <recon.h>
extern CppSymbol *__stdcall find_c_linkage_function(CppSymbol *functions);
static int check_c_linkage_cursor_guard(void)
{
    CppSymbol first = {0}, second = {0};
    int child, status;
    first.kind = 6;
    first.next = &second;
    second.kind = 7;
    second.declaration_flags = 0x200;
    if (find_c_linkage_function(&first) != &second) return 0;
    child = fork();
    if (child < 0) return 0;
    if (!child) { find_c_linkage_function(0); _exit(99); }
    if (waitpid(child,&status,0) != child) return 0;
    return WIFSIGNALED(status) && WTERMSIG(status) == SIGTRAP;
}
const ReconTest dsp_tests[] = {
    { "C-linkage NULL cursor guard and defined chain", 1, check_c_linkage_cursor_guard }
};
