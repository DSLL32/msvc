/* Contextual meanings of the expression IR vocabulary at original 0xa19f4.
   The frontend shares this vocabulary with declaration_codes. Keep only the
   expression overrides here, rather than reproducing the packed data area. */
#include "expression-records.h"
#include <recon.h>
ExpressionRecordRule expression_record_rules[] = {
    { 0, 0 }, { 2, 0x26 }, /* end and symbol reference */
    { 3, 0x33 }, /* string constant */
    { 4, 0x33 }, { 5, 0x33 }, { 6, 0x33 }, /* floating constants */
    { 7, 0x33 }, { 8, 0x33 }, { 9, 0x33 }, { 10, 0x33 }, /* integers */
    { OP_BIT_AND, 0x0b }, { OP_LOGICAL_AND, 0x1c }, { OP_AND_ASSIGN, 0x17 },
    { OP_ASSIGN, 0x32 }, { 0x3e, 0x37 }, /* bit field */
    { 0x3f, 0x43 }, { 0x40, 0x44 }, /* conditional and comma */
    { OP_POST_DECREMENT, 0x36 }, { OP_DIVIDE, 0x05 }, { OP_DIVIDE_ASSIGN, 0x12 },
    { OP_EQUAL, 0x1f }, { OP_NOT, 0x1a }, { OP_DEREFERENCE, 0x30 },
    { OP_GREATER, 0x24 }, { OP_GREATER_EQUAL, 0x23 }, { OP_POST_INCREMENT, 0x35 },
    { OP_SUBSCRIPT, 0x28 }, { OP_SHIFT_LEFT_ASSIGN, 0x15 }, { OP_SHIFT_LEFT, 0x09 },
    { OP_LESS, 0x22 }, { OP_LESS_EQUAL, 0x21 }, { OP_SUBTRACT, 0x03 },
    { OP_SUBTRACT_ASSIGN, 0x10 }, { OP_REMAINDER, 0x06 }, { OP_REMAINDER_ASSIGN, 0x13 },
    { OP_MULTIPLY, 0x04 }, { OP_MULTIPLY_ASSIGN, 0x11 }, { OP_NOT_EQUAL, 0x20 },
    { OP_BIT_OR, 0x0c }, { OP_OR_ASSIGN, 0x19 }, { OP_LOGICAL_OR, 0x1b },
    { 0x59, 0x27 }, { OP_ADD, 0x02 }, { OP_ADD_ASSIGN, 0x0f }, { OP_ARROW, 0x27 },
    { OP_PRE_DECREMENT, 0x10 }, { OP_PRE_INCREMENT, 0x0f }, { 0x5f, 0x42 },
    { OP_SHIFT_RIGHT_ASSIGN, 0x16 }, { OP_SHIFT_RIGHT, 0x0a }, { OP_COMPLEMENT, 0x0e },
    { OP_NEGATE, 0x08 }, { OP_BIT_XOR, 0x0d }, { OP_XOR_ASSIGN, 0x18 },
    { 0x66, 0x86 }, /* segment */
    { 0x67, 0x68 }, { 0x68, 0x8a }, { 0x69, 0x8b }, /* based and segment forms */
    { 0x6a, 0x55 }, { TOK_CASE, 0x3d }, { 0x6e, 0x2c }, /* argument and cast */
    { 0x76, 0x46 }, /* function entry */
    { 0x7a, 0x3e }, { 0x7e, 0x32 }, /* argument list and initialization */
    { OP_CALL, 0x59 }, { 0x81, 0x40 }, /* call and intrinsic */
    { 0x89, 0x85 }, { 0x90, 0x3b }, { TOK_SWITCH, 0x3c },
    { OP_UNARY_PLUS, 0x59 },
    { 0xa4, 0x99 }, { 0xa5, 0x9a }, { 0xa7, 0x9b }, /* member references */
    { 0xa8, 0x9d }, { 0xa9, 0x77 },
    { 0xab, 0x04 }, { 0xac, 0x0b }, /* converted multiply and bit-and */
    { 0xb1, 0x5a }, /* implicit-this indirection */
    { TOK_SEH_TRY, 0x8f }, { TOK_EXCEPT, 0x8d }, { TOK_FINALLY, 0x8e },
    { 0xce, 0 }, { 0xcf, 0 }, { 0xd4, 0 } /* reserved terminals */
};
unsigned expression_record_rule_count = sizeof(expression_record_rules) / sizeof(expression_record_rules[0]);

/* The i386 specialization test and ordinary declaration fallback are distinct
   original functions, still trapping until a genuine case reaches them. */
int __stdcall type_needs_intrinsic_fallback(CType *type);
CppSymbol *__stdcall find_memory_intrinsic(unsigned operation);

/* Original0x56af0..0x56c7f, stdcall4; ten-way switch at0x56c80..0x56ca8.
   Literal string intrinsics become fixed-length memory operations. The normal
   fallback retains all argument evaluation when a target type requests it. */
int __stdcall emit_string_intrinsic(IrToken *expression)
{
    IrToken *callee = expression->value.pointer;
    IrToken *arguments, *argument, *literal, *call;
    char *text;
    unsigned operation = callee->value.integer, length;
    switch (operation) {
    case 0xa4: /* strcpy */
    case 0xa5: /* strcmp */
    case 0xa7: /* strlen */
        arguments = expression->tail.right;
        if (!arguments || !arguments->value.pointer) return 0;
        literal = arguments->value.pointer;
        if (literal->kind != 3) {
            if (operation != 0xa5) return 0;
            argument = arguments->tail.right;
            if (!argument || !argument->value.pointer) return 0;
            literal = argument->value.pointer;
            if (literal->kind != 3) return 0;
        }
        if (literal->tail.string.auxiliary_flags & 1) return 0;
        text = literal->value.text;
        for (length = 0; text[length]; ++length) ;
        if (operation == 0xa7) {
            emit_expression_tree(make_integer_constant(length));
            return 1;
        }
        expression->tail.right = append_expression(arguments,
            make_integer_constant(length + 1), 2);
        operation = operation == 0xa4 ? 0xac : 0xaa;
        callee->value.integer = operation;
        /* Continue with the resulting memory intrinsic. */
    case 0xaa: /* memcmp */
    case 0xac: /* memcpy */
    case 0xad: /* memset */
        for (argument = expression->tail.right; argument;
             argument = argument->tail.right) {
            if (type_needs_intrinsic_fallback(argument->type)) goto fallback;
            literal = argument->value.pointer;
            if (type_needs_intrinsic_fallback(literal->type)) goto fallback;
        }
    }
    return 0;
fallback:
    expression->kind = 0x7a;
    expression->value.pointer = symbol_reference(find_memory_intrinsic(operation));
    if (!expression->value.pointer) return 1;
    call = build_call_expression(expression->value.pointer,expression->tail.right);
    if (!call) return 1;
    expression->value.pointer = call->value.pointer;
    return 0;
}
