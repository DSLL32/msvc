/* Native unsupported-data-domain control; no original-invalid-input claim. */
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <recon-tool.h>
#include "asm.h"
#include "cpp-context.h"
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
static int check_keyword_initial_domain(void)
{
    int child, status;
    if (find_asm_keyword("$") != -1) return 0;
    child = fork();
    if (child < 0) return 0;
    if (!child) { find_asm_keyword(""); _exit(99); }
    if (waitpid(child, &status, 0) != child) return 0;
    return WIFSIGNALED(status) && WTERMSIG(status) == SIGTRAP;
}
/* This operator bypasses every argument dereference and all side effects.
   Only the native unsupported return is tested, not stale original stack data. */
static int check_address_operator_return_guard(void)
{
    int child, status;
    child = fork();
    if (child < 0) return 0;
    if (!child) { combine_asm_addresses(0, 0, 0); _exit(99); }
    if (waitpid(child, &status, 0) != child) return 0;
    return WIFSIGNALED(status) && WTERMSIG(status) == SIGTRAP;
}
/* A valid empty owner tests only the unsupported native control path;
   no original stale-stack equality or public reachability is claimed. */
extern IrToken *__stdcall make_most_derived_test(IrToken *object);
static int check_most_derived_empty_guard(void)
{
    int child, status;
    child = fork();
    if (child < 0) return 0;
    if (!child) {
        CppSymbol tag = {0};
        ClassInfo info = {0};
        Vector empty = {0};
        InlineVector contexts = {0};
        CType type = {0};
        IrToken object = {0};
        contexts.items[0] = &tag;
        cpp_context_stack = &contexts;
        tag.detail.class_info = &info;
        info.virtual_base_tables = &empty;
        info.virtual_table_groups = &empty;
        object.type = &type;
        make_most_derived_test(&object);
        _exit(99);
    }
    if (waitpid(child,&status,0) != child) return 0;
    return WIFSIGNALED(status) && WTERMSIG(status) == SIGTRAP;
}
const ReconTest packet_tests[] = {
    { "mnemonic initial data-domain guard", 1, check_mnemonic_initial_domain },
    { "keyword initial data-domain guard", 1, check_keyword_initial_domain },
    { "address operator uninitialized return guard", 1, check_address_operator_return_guard },
    { "empty virtual-base identity guard", 1, check_most_derived_empty_guard }
};
