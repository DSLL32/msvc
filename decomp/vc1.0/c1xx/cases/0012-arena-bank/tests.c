/* Narrow native rejection of the original unwritten based-value return. */
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <recon-tool.h>
#include "types.h"
#include <recon.h>
IrToken *__stdcall based_type_expression(CType *type);
static int check_based_fallback(void)
{
    CType type = {0};
    TypeShape shape = {0};
    int child, status;
    type.related = &shape;
    shape.attributes = 4;
    if (based_type_expression(&type) != 0) return 0;
    shape.attributes = 1;
    child = fork();
    if (child < 0) return 0;
    if (!child) { based_type_expression(&type); _exit(99); }
    if (waitpid(child, &status, 0) != child) return 0;
    return WIFSIGNALED(status) && WTERMSIG(status) == SIGTRAP;
}
void __stdcall emit_runtime_initializer_expression(unsigned operation,
    unsigned mode, IrToken *arguments);
static int check_runtime_initializer_fallback(void)
{
    int child = fork(), status;
    if (child < 0) return 0;
    if (!child) { emit_runtime_initializer_expression(0, 2, 0); _exit(99); }
    if (waitpid(child, &status, 0) != child) return 0;
    return WIFSIGNALED(status) && WTERMSIG(status) == SIGTRAP;
}
const ReconTest arena_bank_tests[2] = {
    { "based-value unwritten fallback guard", 1, check_based_fallback },
    { "runtime-initializer unwritten fallback guard", 1, check_runtime_initializer_fallback }
};
