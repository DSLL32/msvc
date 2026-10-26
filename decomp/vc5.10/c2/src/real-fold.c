#include "code-input.h"
#include "real-operations.h"
#include <recon.h>

typedef struct C2RealOperationEntry {
    unsigned opcode;
    C2RealOperation operation;
} C2RealOperationEntry;
const C2RealOperationEntry global_real_operations[] = {
    {0x141, c2_real_add},
    {0x142, c2_real_subtract},
    {0x143, c2_real_multiply},
    {0x149, c2_real_divide},
    {0, NULL}
};
typedef char C2RealOperationEntrySize[sizeof(C2RealOperationEntry) == 8 ? 1 : -1];
extern int extra_function_passes, expression_simplification_mode, expression_warning_issued;
int c2_real_compare(int condition, C2Real *left, C2Real *right);

C2Expression *__fastcall fold_real_operation(C2Expression *expression,
    unsigned short type, C2Expression *left, C2Expression *right)
{
    C2Real *result;
    C2Expression *value;
    const C2RealOperationEntry *entry;
    int status;
    if ((type & 0xf000) != 0x4000 && expression->opcode != 0x135) return NULL;
    if (left->kind != 8 || (right && right->kind != 8)) return NULL;
    switch (expression->opcode) {
    case 0x130: case 0x131: case 0x133: case 0x140: case 0x151: case 0x15f:
        return NULL;
    case 0x135: case 0x138:
        return fold_input_constant(left, type, expression);
    case 0x137:
        result = c2_pool_allocate(0, sizeof(C2Real));
        c2_real_subtract(&c2_real_zero, left->value.real, result);
        value = new_real_expression(type, result);
        attach_input_expression(expression, left, value);
        return fold_input_constant(value, type, expression);
    case 0x149:
        /* The original checks the first operand here, before its table call. */
        if (c2_real_compare(1, left->value.real, &c2_real_zero)) return NULL;
    case 0x141: case 0x142: case 0x143:
        result = c2_pool_allocate(0, sizeof(C2Real));
        for (entry = global_real_operations; entry->opcode &&
             entry->opcode != expression->opcode; ++entry) ;
        status = entry->operation(left->value.real, right->value.real, result);
        if (!status) {
            value = new_real_expression(type, result);
            attach_input_expression(expression, left, value);
            return fold_input_constant(value, type, expression);
        }
        if (!extra_function_passes && expression_simplification_mode == 2 &&
            !expression_warning_issued) {
            compiler_warning(2, 0x2f4);
            expression_warning_issued = 1;
        }
        return NULL;
    default:
        return fold_input_constant(right, type, expression);
    }
}
