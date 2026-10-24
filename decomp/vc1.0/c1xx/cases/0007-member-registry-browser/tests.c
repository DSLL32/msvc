/* A real missing virtual-base group must reject the unwritten-output path.
   Synthetic owners establish native rejection, not original-input equivalence. */
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <recon-tool.h>
#include "member-expressions.h"
#include "cpp-context.h"
#include <recon.h>
IrToken *__stdcall make_virtual_method_pointer(CType *type, IrToken *function,
    IrToken *adjustment, BaseClassLink *virtual_base);

static int check_virtual_method_missing_group(void)
{
    CppSymbol owner = {0}, base_tag = {0};
    ClassInfo info = {0};
    VirtualTableGroup group = {0};
    Vector groups = {0};
    void *entries[1];
    InlineVector contexts = {0};
    BaseClassLink base = {0};
    CType type = {0};
    TypeShape shape = {0};
    unsigned offset = 0x1234;
    int child, status;
    owner.detail.class_info = &info;
    info.virtual_table_groups = &groups;
    group.tag = &base_tag;
    group.virtual_base_table_offset = 12;
    entries[0] = &group;
    groups.items = entries; groups.count = groups.capacity = 1;
    if (!query_virtual_base_pointer(&base_tag, &owner, 0, &offset, 0) || offset != 12) return 0;
    groups.count = 0; offset = 0x1234;
    if (query_virtual_base_pointer(&base_tag, &owner, 0, &offset, 0) || offset != 0x1234) return 0;
    contexts.count = contexts.capacity = 1; contexts.items[0] = &base_tag;
    cpp_context_stack = &contexts;
    shape.flags = 0x20; shape.data.tag_symbol = &owner; type.related = &shape;
    child = fork();
    if (child < 0) return 0;
    if (!child) { make_virtual_method_pointer(&type, 0, 0, &base); _exit(99); }
    if (waitpid(child, &status, 0) != child) return 0;
    return WIFSIGNALED(status) && WTERMSIG(status) == SIGTRAP;
}
const ReconTest browser_tests[] = {
    { "missing virtual-method base group guard", 1, check_virtual_method_missing_group }
};
