/* Recursive expression serialization, original 0x59b90..0x59e9d. */
#include "expression-records.h"
#include "initialization.h"
#include "parameter-scopes.h"
#include "unary-expressions.h"
#include "source-input.h"
#include <recon.h>

typedef char StatementValueOffsetMustBe12[(offsetof(IrToken, value) == 12) ? 1 : -1];
typedef char StatementQualifierResolutionOffsetMustBe12[
    (offsetof(QualifiedDeclarator, resolution) == 12) ? 1 : -1];
typedef char StatementResolutionBindingOffsetMustBe0[
    (offsetof(QualifierResolution, binding) == 0) ? 1 : -1];

/* Only the switch-case chain link is consumed here. Case payload layout
   belongs to its record writer. */
typedef struct SwitchCaseLink { struct SwitchCaseLink *next; } SwitchCaseLink;

void __stdcall emit_expression_tree(IrToken *expression)
{
    IrToken *operand, *argument;
    SwitchCaseLink *cases;
    CppSymbol *function, *parameter, *hidden_parameter;
    unsigned rule, record;
    unsigned char saved_flags;

    if (error_count) return;
    while (expression) {
        /* The native shared token flags select this same set of special
           handlers. Other flagged tokens fall through to the ordinary case. */
        switch (expression->kind) {
        case 3:
            expression = materialize_string_literal(expression);
            WRITE_EXPRESSION_RECORD(expression);
            return;
        case OP_DEREFERENCE:
            expression = fold_symbol_constant(expression);
            break;
        case 0x67:
            if (!expression->tail.right)
                expression->tail.right = new_typed_unary_expression(0x1b,
                    type_4064_first, 0);
            break;
        case 0x6e:
            if (expression->type->flags == 0x80 && !expression->type->related) {
                expression = expression->value.pointer;
                if (error_count) return;
                continue;
            }
            break;
        case 0x76:
            emit_expression_tree(expression->value.pointer);
            operand = expression->value.pointer;
            if (operand->kind == 0xa4) operand = operand->value.pointer;
            function = operand->value.pointer;
            --function->references;
            WRITE_EXPRESSION_RECORD(expression);
            hidden_parameter = 0;
            if ((function->declaration_flags & 0x400) &&
                (function->cpp.parts.member_flags & 1)) {
                parameter = active_parameter_symbols;
                while (parameter && (parameter->kind & 0x1f) != 10 &&
                       (parameter->kind & 0x1f) != 11)
                    parameter = parameter->next;
                if (parameter) {
                    hidden_parameter = parameter->detail.parameter_next;
                    saved_flags = hidden_parameter->flags;
                    hidden_parameter->flags |= 8;
                }
            }
            for (parameter = active_parameter_symbols; parameter; parameter = parameter->next)
                if (parameter->type && !(parameter->flags & 8) &&
                    ((parameter->kind & 0x1f) == 10 || (parameter->kind & 0x1f) == 11))
                    write_ir_record(0x2d, symbol_reference(parameter));
            if (hidden_parameter)
                hidden_parameter->flags ^= (hidden_parameter->flags ^ saved_flags) & 8;
            write_ir_record(0x4c, 0);
            return;
        case 0x81:
            if (emit_string_intrinsic(expression)) return;
            /* Otherwise retain ordinary call/argument-list encoding. */
        case 0x7a:
        case 0xa8:
            emit_expression_tree(expression->value.pointer);
            WRITE_EXPRESSION_RECORD(expression);
            for (argument = expression->tail.right; argument; argument = argument->tail.right) {
                emit_expression_tree(argument->value.pointer);
                write_ir_record(0x55, argument);
            }
            write_ir_record(0x4c, 0);
            return;
        case 0x8f:
            emit_expression_tree(expression->value.pointer);
            WRITE_EXPRESSION_RECORD(expression);
            for (cases = (SwitchCaseLink *)expression->tail.right; cases; cases = cases->next)
                write_ir_record(0x3d, cases);
            write_ir_record(0x4c, 0);
            return;
        }
        switch (expression->flags & 7) {
        case 0:
            if (expression->flags & 0x40) { report_syntax_error(124); return; }
            break;
        case 1:
            emit_expression_tree(expression->value.pointer);
            break;
        case 2:
            emit_expression_tree(expression->value.pointer);
            emit_expression_tree(expression->tail.right);
            break;
        }
        WRITE_EXPRESSION_RECORD(expression);
        return;
    }
}

/* Original 0x59ae0..0x59b83: typed statements emit a line marker, tree
   and terminator. Untyped overload names retain their diagnostic path. */
void __stdcall emit_expression_statement(IrToken *expression)
{
    IrToken *root;
    QualifiedDeclarator *qualified;
    CppSymbol *overload;
    if (error_count) return;
    if (!expression || expression->type) {
        record_line_information(1);
        emit_expression_tree(expression);
        write_ir_record(0x4b, 0);
        return;
    }
    root = strip_address_expression(expression);
    overload = 0;
    if (root->kind == 0x5c) {
        qualified = root->tail.right->value.pointer;
        overload = (CppSymbol *)qualified->resolution->binding->target;
    } else if (root->kind == 2) {
        overload = root->value.pointer;
    }
    if (overload && ((overload->kind & 0x1f) == 16 || (overload->kind & 0x1f) == 17) &&
        !has_single_function(overload))
        report_syntax_error(568, token_spellings[root->kind], overload->name);
}
