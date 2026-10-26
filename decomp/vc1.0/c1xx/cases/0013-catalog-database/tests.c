/* Narrow controls for the separate-debug unsupported native domains. */
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <recon-tool.h>
#include "declarations.h"
#include "debug-types.h"
#include "debug-info.h"
#include <recon.h>

static int check_scalar_domain(void)
{
    int child, status;
    if (line_record_primitive_type(0x64) != 0x74) return 0;
    child = fork();
    if (child < 0) return 0;
    if (!child) { line_record_primitive_type(0x4f); _exit(99); }
    if (waitpid(child, &status, 0) != child) return 0;
    return WIFSIGNALED(status) && WTERMSIG(status) == SIGTRAP;
}
static CppSymbol *guard_tag;
static unsigned short *guard_leaf;
static void class_trap(int signal_number)
{
    /* The trap must follow the real clear-bit and zero-count effects. */
    _exit(signal_number == SIGTRAP && !(guard_tag->flags & 8) &&
        guard_leaf[1] == 0 ? 73 : 74);
}
static int check_class_fallback(void)
{
    CppSymbol tag = {0};
    CType type = {0};
    unsigned short leaf[8] = {0};
    int child, status;
    tag.name = "unsupported-type-control";
    tag.type = &type;
    type.debug_index = 0x1234;
    if (line_record_tag_type(&tag) != 0x1234) return 0;
    type.debug_index = 0;
    tag.flags = 8;
    leaf[1] = 0xabcd;
    child = fork();
    if (child < 0) return 0;
    if (!child) {
        line_record_leaf = (unsigned char *)leaf;
        guard_tag = &tag;
        guard_leaf = leaf;
        signal(SIGTRAP, class_trap);
        line_record_tag_type(&tag);
        _exit(99);
    }
    if (waitpid(child, &status, 0) != child) return 0;
    return WIFEXITED(status) && WEXITSTATUS(status) == 73;
}

/* DeviceRoot subsequently establishes the encountered nine-WORD prefix. */
extern unsigned short line_record_scratch[9];
extern unsigned char *line_record_cursor;
unsigned char *line_record_cursor_position(void);
void line_record_restore_cursor(unsigned char *position);
void line_record_push_type(unsigned short index);
unsigned short line_record_read_type(void);
unsigned short line_record_method_list(OverloadSet *methods);
unsigned short line_record_virtual_base_data(int pass, unsigned char *destination,
    CppSymbol *tag, CppSymbol *owner);

static unsigned expected_cursor;
static void cursor_trap(int signal_number)
{
    _exit(signal_number == SIGTRAP && (unsigned)line_record_cursor == expected_cursor ? 73 : 74);
}
static int check_scratch_prefix(void)
{
    unsigned char *mark = line_record_cursor_position();
    int child, status, i;
    for (i = 0; i < 9; ++i) line_record_push_type((unsigned short)(0x1200 + i));
    line_record_restore_cursor(mark);
    for (i = 0; i < 9; ++i)
        if (line_record_read_type() != 0x1200 + i) return 0;
    child = fork();
    if (child < 0) return 0;
    if (!child) {
        expected_cursor = (unsigned)line_record_scratch + sizeof(line_record_scratch);
        signal(SIGTRAP, cursor_trap);
        line_record_push_type(0xffff);
        _exit(99);
    }
    if (waitpid(child, &status, 0) != child || !WIFEXITED(status) || WEXITSTATUS(status) != 73)
        return 0;
    child = fork();
    if (child < 0) return 0;
    if (!child) {
        expected_cursor = (unsigned)line_record_scratch + sizeof(line_record_scratch) + 2;
        signal(SIGTRAP, cursor_trap);
        line_record_read_type();
        _exit(99);
    }
    if (waitpid(child, &status, 0) != child || !WIFEXITED(status) || WEXITSTATUS(status) != 73)
        return 0;
    line_record_restore_cursor(mark);
    return 1;
}
static int check_null_method_list(void)
{
    int child = fork(), status;
    if (child < 0) return 0;
    if (!child) { line_record_method_list(0); _exit(99); }
    if (waitpid(child, &status, 0) != child) return 0;
    return WIFSIGNALED(status) && WTERMSIG(status) == SIGTRAP;
}
static int check_complete_class_fallback(void)
{
    CppSymbol tag = {0};
    ClassInfo info = {0};
    CType type = {0};
    unsigned short leaf[16] = {0};
    int child, status;
    tag.name = "unsupported-complete-type-control";
    tag.type = &type;
    tag.detail.class_info = &info;
    info.debug_index = 0x1234;
    if (prepare_line_record_tag(&tag) != 0x1234) return 0;
    info.debug_index = 0;
    type.debug_index = 1;
    tag.flags = 4 | 8;
    leaf[1] = 0xabcd;
    child = fork();
    if (child < 0) return 0;
    if (!child) {
        line_record_leaf = (unsigned char *)leaf;
        guard_tag = &tag;
        guard_leaf = leaf;
        signal(SIGTRAP, class_trap);
        prepare_line_record_tag(&tag);
        _exit(99);
    }
    if (waitpid(child, &status, 0) != child) return 0;
    return WIFEXITED(status) && WEXITSTATUS(status) == 73;
}
static int check_virtual_base_outputs(void)
{
    CppSymbol base = {0}, owner = {0}, pointer = {0};
    CType base_type = {0}, pointer_type = {0};
    ClassInfo info = {0};
    VirtualTableGroup group = {0};
    void *items[1];
    Vector groups = {0};
    unsigned short leaf[8];
    int child, status, i;
    base.type = &base_type;
    base_type.debug_index = 0x1234;
    pointer.type = &pointer_type;
    pointer_type.debug_index = 0x2345;
    pointer_type.flags = 4;
    owner.detail.class_info = &info;
    info.virtual_table_groups = &groups;
    group.tag = &base;
    group.virtual_base_pointer = &pointer;
    group.virtual_pointer_offset = 12;
    group.virtual_base_table_offset = 8;
    items[0] = &group;
    groups.items = items;
    groups.count = groups.capacity = 1;
    for (i = 0; i < 8; ++i) leaf[i] = 0xa5a5;
    if (line_record_virtual_base_data(2, (unsigned char *)leaf, &base, &owner) != 12 ||
        leaf[0] != 0xa5a5 || leaf[1] != 0x1234 || leaf[2] != 0x2345 ||
        leaf[3] != 0xa5a5 || leaf[4] != 12 || leaf[5] != 2 || leaf[6] != 0xa5a5)
        return 0;
    groups.count = 0;
    child = fork();
    if (child < 0) return 0;
    if (!child) { line_record_virtual_base_data(2, (unsigned char *)leaf, &base, &owner); _exit(99); }
    if (waitpid(child, &status, 0) != child) return 0;
    return WIFSIGNALED(status) && WTERMSIG(status) == SIGTRAP;
}
const ReconTest catalog_database_tests[6] = {
    { "separate-debug scalar table domain", 1, check_scalar_domain },
    { "separate-debug class unwritten length", 1, check_class_fallback },
    { "separate-debug scratch prefix and cursor order", 1, check_scratch_prefix },
    { "separate-debug null method-list guard", 1, check_null_method_list },
    { "separate-debug complete class unwritten length", 1, check_complete_class_fallback },
    { "separate-debug virtual-base output ownership", 1, check_virtual_base_outputs }
};
