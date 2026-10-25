/* Exact unsupported native NULL cursor; no original stale-stack claim. */
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <recon-tool.h>
#include "declarations.h"
#include "diagnostics.h"
#include "source-input.h"
#include "initialization.h"
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

static void call_diagnostic(unsigned category, char *format, ...)
{
    va_list arguments;
    va_start(arguments,format);
    emit_diagnostic(category,69,format,arguments);
    va_end(arguments);
}
static int check_diagnostic_category_guard(void)
{
    int child, status, control;
    for (control = 0; control < 2; ++control) {
        child = fork();
        if (child < 0) return 0;
        if (!child) {
            source_line_number = 0;
            source_filename = 0;
            diagnostic_trace_file = 0;
            call_diagnostic(control ? 4000 : 1234,"%s","defined");
            _exit(0);
        }
        if (waitpid(child,&status,0) != child) return 0;
        if (control) {
            if (!WIFEXITED(status) || WEXITSTATUS(status)) return 0;
        } else if (!WIFSIGNALED(status) || WTERMSIG(status) != SIGTRAP) return 0;
    }
    return 1;
}

const ReconTest dsp_tests[] = {
    { "C-linkage NULL cursor guard and defined chain", 1, check_c_linkage_cursor_guard },
    { "Diagnostic category guard and defined warning", 1, check_diagnostic_category_guard }
};
