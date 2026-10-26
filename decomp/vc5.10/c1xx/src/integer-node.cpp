#include "frontend.h"
#include "ir.h"
#include "parser-actions.h"
#include <recon.h>

extern "C" CompilerExpression *__cdecl allocate_integer_constant(int value);

extern "C" void *__cdecl make_integer_constant(int value)
{
    CompilerExpression *constant = allocate_integer_constant(value);
    if (!value)
        constant->flags |= 0x80;
    return constant;
}
extern "C" CompilerExpression *__cdecl allocate_integer_constant(int value)
{
    CompilerArena *arena = &frontend_bss.arenas[frontend_data.constant_arena_index];
    CompilerExpression *constant = (CompilerExpression *)arena->cursor;
    arena->cursor += 24;
    if (arena->cursor > arena->committed_limit)
        arena->CommitExtent();
    constant->flags = 3;
    constant->kind = 0x65;
    constant->type = frontend_data.builtin_types.type_0063;
    constant->value.integer = (__int64)value;
    return constant;
}

/* The comparer consumes the x87 ten-byte value at these static owners.
 * Original classification/tail bytes beyond that prefix are not represented. */
struct CompilerExtendedConstant {
    DWORD significand_low, significand_high;
    WORD sign_exponent;
};
typedef char CompilerExtendedConstant_prefix[
    offsetof(CompilerExtendedConstant, sign_exponent) == 8 ? 1 : -1];
extern "C" int __cdecl compare_extended_constants(unsigned,
    const CompilerExtendedConstant *, const CompilerExtendedConstant *); /*5025c*/
extern "C" CompilerExpression *__cdecl make_typed_constant(
    CompilerType *type, const void *value, unsigned arena_kind)
{
    static const CompilerExtendedConstant zero = { 0, 0, 0 }; /* original cd5a0 */
    static const CompilerExtendedConstant one = { 0, 0x80000000, 0x3fff }; /* cd5b0 */
    CompilerArena *arena;
    CompilerExpression *constant;
    CompilerType *qualified;
    if (arena_kind >= 9) { __asm { int 3 } }
    arena = &frontend_bss.arenas[arena_kind];
    constant = (CompilerExpression *)arena->cursor;
    arena->cursor += 24;
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    constant->flags = 3;
    constant->kind = 0x65;
    if (type->flags & 0x40) {
        constant->value.integer = *(const __int64 *)value;
        if (!constant->value.integer) {
            constant->flags = 0x83;
            constant->constant_flags |= 1;
        } else if (constant->value.integer == 1) constant->constant_flags |= 1;
    } else if (type->flags & 0x100) {
        constant->value.pointer = *(void *const *)value;
        if (!compare_extended_constants(6, (const CompilerExtendedConstant *)constant->value.pointer, &zero)) {
            constant->flags = 0x83;
            constant->constant_flags |= 1;
        } else if (!compare_extended_constants(6, (const CompilerExtendedConstant *)constant->value.pointer, &one))
            constant->constant_flags |= 1;
    }
    qualified = make_parser_type_scratch(type);
    qualified->attributes = 0x100;
    constant->type = intern_type(qualified);
    --frontend_bss.type_scratch_index;
    return constant;
}
extern "C" int __cdecl compare_extended_constants(unsigned,
    const CompilerExtendedConstant *, const CompilerExtendedConstant *)
{ __asm { int 3 } return 0; }

extern "C" CompilerExpression *__cdecl resolve_parser_expression(CompilerExpression *expression)
{
    CompilerExpression *node = expression;
    CompilerSymbol *symbol, *owner;
    if (!expression) return NULL;
    if (expression->flags & 8) return expression;
    while ((node->flags & 7) == 7) {
        if (node->kind == 0xbb || node->kind == 0xbc ||
            node->kind == 0xbd || node->kind == 0xcb)
            node = node->value.operands.first;
    }
    if (!(node->flags & 7) && node->kind == 0x98 && (short)node->type->auxiliary >= 0) {
        compiler_warning(0x113, node->value.symbol);
        node->flags |= 8;
        expression->flags |= 8;
        return expression;
    }
    if (node->kind == 6) {
        symbol = node->value.symbol;
        if (symbol->kind == 0x12) {
            owner = ((CompilerBoundSymbol *)symbol)->binding->owner->symbol;
            if (owner->kind == 0x11 || owner->kind == 8)
                return make_unary_expression(0x31, expression);
            compiler_warning(0x255, symbol);
            node->flags |= 8;
            expression->flags |= 8;
        }
    }
    return expression;
}

extern "C" CompilerExpression *__cdecl convert_parser_rvalue(CompilerExpression *expression)
{
    if (!expression) return NULL;
    if (!(expression->flags & 8)) {
        if (!expression->type) report_language_diagnostic(1, 0x227);
        else if (expression->type->shape && (expression->type->shape->flags & 8))
            report_language_diagnostic(1, 0x226);
    }
    return expression;
}

extern "C" CompilerExpression *__cdecl resolve_constant_reference(CompilerExpression *); /*7a5d*/
extern "C" __int64 __cdecl constant_integer_value(CompilerExpression *expression)
{
    if (!expression) {
        compiler_warning(0x38);
        return 0;
    }
    expression = resolve_constant_reference(expression);
    if ((expression->flags & 7) != 3) {
        if (!(frontend_data.source_mode_flags & 0x100000)) compiler_warning(0x39);
        return 1;
    }
    if (expression->type->flags & 0x240) return expression->value.integer;
    compiler_warning(0x3a);
    return 0;
}
extern "C" CompilerExpression *__cdecl make_sized_integer_constant(__int64, int); /*5df6*/
extern "C" CompilerExpression *__cdecl resolve_constant_reference(CompilerExpression *expression)
{
    CompilerExpression *reference, *constant;
    CompilerStoredConstantSymbol *symbol;
    CompilerType *type;
    unsigned flags;
    int bytes;
    /* These two operators have the original indirection classification3d;
     * all other BYTE opcodes return unchanged before a payload read. */
    if (expression->kind != 0x3d && expression->kind != 0xa6) return expression;
    reference = expression->value.operands.first;
    if (reference->kind != 6) return expression;
    symbol = (CompilerStoredConstantSymbol *)reference->value.symbol;
    if (!(symbol->qualifiers & 0x40)) return expression;
    type = reference->type;
    if (type->shape) {
        constant = (CompilerExpression *)make_integer_constant((int)symbol->value.pointer_bits);
        constant->type = symbol->type;
        return constant;
    }
    flags = type->flags;
    bytes = flags & 15;
    if ((flags & 0x340) || !(flags & 0xfff0)) bytes = compiler_primitive_type_bytes[bytes];
    constant = make_sized_integer_constant(symbol->value.integer, bytes);
    if (!symbol->value.integer || symbol->value.integer == 1) constant->constant_flags |= 1;
    constant->type = symbol->type;
    return constant;
}
extern "C" CompilerExpression *__cdecl make_sized_integer_constant(__int64, int)
{ __asm { int 3 } return NULL; }
