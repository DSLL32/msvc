#include <stdint.h>
#include <string.h>
#include "compiler-state.h"
#include <recon-tool.h>
#include <recon.h>
extern void __cdecl set_warning_state(unsigned, unsigned);
extern void __cdecl apply_option_flags(unsigned);
extern unsigned __cdecl pack_option_flags(void);
extern void *__cdecl copy_control_option(const void *, unsigned, unsigned);
static int warning_controls(void)
{
    c1_data.options.warning_levels[99] = 7;
    c1_data.options.warning_levels[101] = 9;
    c1_data.options.lowest_warning = 1000;
    c1_data.options.highest_warning = 700;
    set_warning_state(4100, 13);
    if (c1_data.options.warning_levels[100] != 13 ||
        c1_data.options.warning_levels[99] != 7 || c1_data.options.warning_levels[101] != 9 ||
        c1_data.options.lowest_warning != 1000 || c1_data.options.highest_warning != 700) return 0;
    set_warning_state(4799, 5);
    set_warning_state(4700, 3);
    set_warning_state(4999, 14);
    return c1_data.options.lowest_warning == 700 && c1_data.options.highest_warning == 999 &&
        c1_data.options.warning_levels[799] == 5 && c1_data.options.warning_levels[700] == 3 &&
        c1_data.options.warning_levels[999] == 14;
}
static int option_bits(void)
{
    c1_data.options.debug_info = 37;
    c1_data.options.warning_level = 3;
    apply_option_flags(0xffffffffU);
    if (pack_option_flags() != 0x83a46800U || c1_data.normalized_stack_checks != 1 ||
        c1_data.options.global_optimize != 1 || c1_data.options.omit_frame_pointer != 1) return 0;
    apply_option_flags(0);
    return !pack_option_flags() && !c1_data.normalized_stack_checks &&
        !c1_data.options.global_optimize && !c1_data.options.omit_frame_pointer &&
        c1_data.options.debug_info == 37 && c1_data.options.warning_level == 3;
}
static int option_copy_extent(void)
{
    unsigned aligned[16];
    const char source[6] = "abcde";
    unsigned char *bytes = (unsigned char *)aligned;
    memset(aligned, 0xa5, sizeof(aligned));
    c1_bss.arenas[0].cursor = bytes;
    c1_bss.arenas[0].committed_limit = bytes + sizeof(aligned);
    if (copy_control_option(source, 0, 5) != bytes ||
        c1_bss.arenas[0].cursor != bytes + 8 || memcmp(bytes, source, 5)) return 0;
    return bytes[5] == 0xa5 && bytes[7] == 0xa5 && bytes[8] == 0xa5 && bytes[63] == 0xa5;
}
const ReconTest pragma_tests[3] = {
    { "warning-number normalization, neighbors and scan bounds", 1, warning_controls },
    { "sparse option-bit restoration leaves unrelated options untouched", 1, option_bits },
    { "control-option copy alignment and unwritten tail", 1, option_copy_extent }
};
