#include <string.h>
#include "compiler-state.h"
#include "case.h"
#include <recon.h>
extern CompilerConditionValue *__cdecl fold_condition_binary(unsigned, CompilerConditionValue *, CompilerConditionValue *);
extern void __cdecl layout_bitfield_member(CompilerIrSymbol *, WORD);

static int conditional_arithmetic(void)
{
    CompilerConditionValue first, second, saved;
    memset(&first, 0xa5, sizeof(first));
    memset(&second, 0x5a, sizeof(second));
    first.flags = second.flags = 0x63;
    first.value = 13; second.value = 4; saved = second;
    if (fold_condition_binary(70, &first, &second) != &first || first.value != 9 ||
        first.flags != 0x44 || memcmp(&second, &saved, sizeof(second))) return 0;
    first.value = 13;
    if (fold_condition_binary(57, &first, &second)->value != 3) return 0;
    first.value = 13;
    if (fold_condition_binary(72, &first, &second)->value != 1) return 0;
    first.value = 3;
    if (fold_condition_binary(67, &first, &second)->value != 48) return 0;
    first.value = -32; second.value = 3;
    if (fold_condition_binary(88, &first, &second)->value != -4 || first.flags != 0x44) return 0;
    first.value = -32; second.value = 65;
    if (fold_condition_binary(88, &first, &second)->value != -1) return 0;
    return first.unmodeled_02[0] == 0xa5 && first.unmodeled_02[5] == 0xa5;
}
static int conditional_promotions(void)
{
    CompilerConditionValue first, second;
    unsigned i;
    memset(&first, 0xa5, sizeof(first)); memset(&second, 0x5a, sizeof(second));
    first.flags = 0x4044; second.flags = 0x44;
    first.value = -1; second.value = 1;
    if (fold_condition_binary(62, &first, &second)->value != 1 || first.flags != 0x44) return 0;
    first.flags = 0x4061; second.flags = 0x61;
    first.value = 1; second.value = -1;
    if (fold_condition_binary(68, &first, &second)->value || first.flags != 0x44) return 0;
    first.flags = 0x4044; first.value = -32; second.value = 3;
    if ((unsigned __int64)fold_condition_binary(88, &first, &second)->value !=
        (unsigned __int64)0x1ffffffffffffffc || first.flags != 0x4044) return 0;
    first.flags = 0x63; first.value = 17;
    if (fold_condition_binary(134, &first, NULL) != &first || first.value != 17 || first.flags != 0x44) return 0;
    for (i = 0; i < sizeof(first.unmodeled_02); ++i)
        if (first.unmodeled_02[i] != 0xa5 || second.unmodeled_02[i] != 0x5a) return 0;
    return 1;
}
extern int c1_test_packed_bitfields(void);
extern int c1_test_extended_range_boundaries(void);
extern int c1_test_statement_label_storage(void);
extern void __cdecl append_literal_text(CompilerStringValue *, const CompilerStringValue *);
static int adjacent_literal_storage(void)
{
    BYTE storage[24];
    CompilerStringValue first = { "packed", 7, 0x40 };
    CompilerStringValue second = { "\tcase\n", 7, 0x40 };
    CompilerStringValue saved = second;
    unsigned index;
    memset(storage, 0xa5, sizeof(storage));
    c1_data.expression_arena_kind = 3;
    c1_bss.arenas[0].cursor = storage + 4;
    c1_bss.arenas[0].committed_limit = storage + 20;
    append_literal_text(&first, &second);
    if (first.text != (const char *)storage + 4 || first.length != 13 ||
        first.flags != 0x40 || memcmp(first.text, "packed\tcase\n", 13) ||
        c1_bss.arenas[0].cursor != storage + 20 ||
        memcmp(&second, &saved, sizeof(second))) return 0;
    for (index = 0; index < sizeof(storage); ++index)
        if ((index < 4 || index >= 17) && storage[index] != 0xa5) return 0;
    return 1;
}
const ReconTest declaration_tests[6] = {
    { "conditional arithmetic preserves operand order, shift boundaries and input ownership", 1, conditional_arithmetic },
    { "conditional promotion distinguishes unsigned comparisons and retains opaque bytes", 1, conditional_promotions },
    { "packed bitfields retain ordered offsets, list ownership and unwritten bytes", 1, c1_test_packed_bitfields },
    { "adjacent literals retain exact content, terminator, input and allocation tails", 1, adjacent_literal_storage },
    { "finite extended range edges retain signed values, status, stack depth and scratch tails", 1, c1_test_extended_range_boundaries },
    { "statement labels retain exact constructor writes, unknown bytes and lookup identity", 1, c1_test_statement_label_storage }
};
