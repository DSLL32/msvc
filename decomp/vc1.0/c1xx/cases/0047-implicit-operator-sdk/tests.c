/* Whole-body/ABI qualification is unit-only: this SDK's original CLI
   concretizes operator patterns before reaching these abstract matchers. */
#include <recon-tool.h>
#include "constructor-initializers.h"
#include <recon.h>

static int check_scalar_patterns(void)
{
    CType target = {0}, source = {0};
    TypeShape reference = {0}, pointer = {0};
    target.flags = 0x40; source.flags = 0x64;
    if (classify_special_scalar_conversion(&target, &source, 0, 0) != 4) return 0;
    reference.flags = 0x40; source.related = &reference;
    if (classify_special_scalar_conversion(&target, &source, 1, 0) != 4) return 0;
    pointer.flags = 1; reference.related = &pointer;
    if (classify_special_scalar_conversion(&target, &source, 0, 0) != 0x2000) return 0;
    source.related = &pointer;
    if (classify_special_scalar_conversion(&target, &source, 0, 0) != 0x2000) return 0;
    source.related = 0; source.flags = 0x200;
    if (classify_special_scalar_conversion(&target, &source, 0, 0) != 4) return 0;
    target.flags = 0x140;
    if (classify_special_scalar_conversion(&target, &source, 0, 0) != 4) return 0;
    source.flags = 0x80;
    return classify_special_scalar_conversion(&target, &source, 0, 0) == 0x2000;
}
static int check_member_pointer_shapes(void)
{
    CType type = {0};
    TypeShape prefix = {0}, member = {0};
    if (has_member_pointer_shape(0) || has_member_pointer_shape(&type)) return 0;
    type.related = &member; member.flags = 1;
    if (has_member_pointer_shape(&type)) return 0;
    member.flags = 0x10;
    if (!has_member_pointer_shape(&type)) return 0;
    member.flags = 0x20;
    if (!has_member_pointer_shape(&type)) return 0;
    type.related = &prefix; prefix.related = &member;
    if (!has_member_pointer_shape(&type)) return 0;
    prefix.flags = 0x40;
    return !has_member_pointer_shape(&type);
}
const ReconTest implicit_operator_tests[] = {
    {"abstract scalar conversion patterns (unit-only)", 1, check_scalar_patterns},
    {"member-pointer shape recognition (unit-only)", 1, check_member_pointer_shapes}
};
