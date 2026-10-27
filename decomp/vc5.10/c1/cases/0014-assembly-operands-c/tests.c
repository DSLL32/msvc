#include <string.h>
#include "compiler-state.h"
#include <recon-tool.h>
#include <recon.h>
extern CompilerAssemblyExpression *__cdecl make_assembly_integer(unsigned);
extern CompilerAssemblyExpression *__cdecl make_assembly_address(short, CompilerAssemblyExpression *, CompilerAssemblyExpression *);
extern CompilerAssemblyExpression *__cdecl make_assembly_binary(BYTE, CompilerAssemblyExpression *, CompilerAssemblyExpression *);
extern CompilerAssemblyOperand *__cdecl initialize_assembly_operand(unsigned);
extern unsigned __cdecl assembly_scale_code(unsigned);
extern int __cdecl assembly_immediate_fits(unsigned, unsigned);
extern void __cdecl select_assembly_width(BYTE);
static int expression_extent(void)
{
    unsigned storage[16];
    BYTE *bytes = (BYTE *)storage;
    CompilerAssemblyExpression *a, *b, *sum, *difference;
    memset(storage, 0xa5, sizeof(storage));
    c1_bss.arenas[3].cursor = bytes;
    c1_bss.arenas[3].committed_limit = bytes + sizeof(storage);
    a = make_assembly_integer(0xffffffffU);
    b = make_assembly_integer(2);
    sum = make_assembly_address(22, a, b);
    difference = make_assembly_address(23, b, a);
    if ((BYTE *)a != bytes || (BYTE *)b != bytes + 12 ||
        (BYTE *)sum != bytes + 24 || (BYTE *)difference != bytes + 36 ||
        c1_bss.arenas[3].cursor != bytes + 48 || a->kind || b->kind ||
        sum->kind || difference->kind || sum->payload != 1 || difference->payload != 3) return 0;
    {
        unsigned i, j;
        for (i = 0; i != 4; ++i) {
            if (bytes[i * 12 + 2] != 0xa5 || bytes[i * 12 + 3] != 0xa5) return 0;
            for (j = 8; j != 12; ++j) if (bytes[i * 12 + j] != 0xa5) return 0;
        }
        for (i = 48; i != sizeof(storage); ++i) if (bytes[i] != 0xa5) return 0;
    }
    return 1;
}
static int third_operand_and_scale(void)
{
    CompilerAssemblyExpression left = { 1, {0xa5, 0xa5}, 0, 0x12345678 };
    CompilerAssemblyExpression right = { 0, {0x5a, 0x5a}, 4, 0x87654321 };
    CompilerAssemblyOperand *operand;
    CompilerAssemblyOperandValue *value;
    unsigned i;
    memset(c1_bss.assembly.operands, 0xa5, sizeof(c1_bss.assembly.operands));
    memset(c1_bss.assembly.values, 0x5a, sizeof(c1_bss.assembly.values));
    operand = initialize_assembly_operand(2);
    if (operand != &c1_bss.assembly.operands[2] || operand->value != &c1_bss.assembly.values[2] ||
        operand->form != 15 || operand->modifiers || operand->reset_08 || operand->reset_0c ||
        operand->unmodeled_06[0] != 0xa5 || operand->unmodeled_06[1] != 0xa5) return 0;
    for (i = 0; i != 32; ++i) if (((BYTE *)c1_bss.assembly.operands)[i] != 0xa5) return 0;
    for (i = 0; i != 24; ++i) if (((BYTE *)c1_bss.assembly.values)[i] != 0x5a) return 0;
    for (i = 0; i != 12; ++i) if (c1_bss.assembly.values[2].unmodeled[i]) return 0;
    c1_bss.assembly.current_operand = operand;
    c1_bss.assembly.reset_9fc4 = 1;
    value = &operand->value->operand;
    value->kind_flags = 4;
    value->register_code = 2;
    value->segment_and_scale = 27;
    if (make_assembly_binary(24, &left, &right) != &left || value->segment_and_scale != (128 | 27) ||
        value->register_code || value->index_register != 2 || value->kind_flags != 4 ||
        left.register_code != 0x12345678 || right.register_code != 0x87654321) return 0;
    c1_bss.assembly.reset_04 = 1;
    select_assembly_width(2);
    return operand->form == 2 && operand->modifiers == 1 &&
        operand->unmodeled_06[0] == 0xa5 && operand->unmodeled_06[1] == 0xa5 &&
        value->segment_and_scale == (128 | 27) && value->index_register == 2;
}
static int range_edges(void)
{
    return assembly_scale_code(0) == 0 && assembly_scale_code(1) == 0 &&
        assembly_scale_code(2) == 1 && assembly_scale_code(4) == 2 && assembly_scale_code(8) == 3 &&
        assembly_immediate_fits(8, 255) == 1 && assembly_immediate_fits(8, 256) == 0 &&
        assembly_immediate_fits(8, (unsigned)-255) == 1 && assembly_immediate_fits(8, (unsigned)-256) == 0 &&
        assembly_immediate_fits(8, 0x80000000U) == 1 && assembly_immediate_fits(31, 0) == 0 &&
        assembly_immediate_fits(31, 0x80000000U) == 0 && assembly_immediate_fits(0, 0) == 1 &&
        assembly_immediate_fits(0, 1) == 0 && assembly_immediate_fits(40, 255) == 1;
}
const ReconTest assembly_operand_tests[3] = {
    { "assembly nodes preserve unwritten bytes and wrap DWORD address arithmetic", 1, expression_extent },
    { "third operand construction and scaled-index updates preserve neighbors and padding", 1, third_operand_and_scale },
    { "assembly scale and signed immediate-range edge behavior", 1, range_edges }
};
