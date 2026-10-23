/* C++ return statements and implicit constructor returns. */
#include "return-statements.h"
#include "binary-expressions.h"
#include "lexer.h"
#include <recon.h>

/* Original 0x50250..0x50764, stdcall 8. The return expression, temporary
   cleanup and function-exit branch remain separate native operations. */
void __stdcall emit_return_statement(IrToken *expression, int mode)
{
    IrToken *converted, *arguments, *cleanup;
    CType *saved_type;
    CppSymbol *tag;
    int void_result;
    if (!current_return_type) return;
    if (function_symbol->declaration_flags & 2) {
        cpp_parser_flags |= 0x8000;
        return;
    }
    if (expression && (expression->flags & 8)) {
        cpp_parser_flags |= 0x8000;
        return;
    }
    if (!declaration_mode) emit_pending_function_cleanup();
    void_result = 0;
    if (function_symbol->declaration_flags & 0x1000) {
        if (expression) report_syntax_error(534, qualified_definition_class->name);
        if (!mode) begin_return_expression();
        cleanup = finish_return_expression(0, 0);
        expression = adjusted_this_expression(0);
        mode = 0;
        if (cleanup)
            expression = new_ir_expression(0x40, cleanup, expression, expression->type);
        emit_expression_tree(expression);
        write_ir_record(0x41, expression);
        goto return_branch;
    }
    if ((function_symbol->declaration_flags & 0x2000) && expression) {
        expression = 0;
        report_syntax_error(577, qualified_definition_class->name);
    }
    if ((function_symbol->storage[2] & 1) && mode) {
        report_syntax_error(490, token_spellings[0x87]);
        return;
    }
    void_result = current_return_type->flags == 0x80 && !current_return_type->related;
    if (!void_result) {
        if (!expression) {
            if (!options.configuration.extensions_enabled ||
                (function_symbol->declaration_flags & 0x4400) ||
                (!(token_allocation_flags & 0x02000000UL) &&
                 ((function_symbol->name != name_main && function_symbol->name != name_winmain &&
                   function_symbol->name != name_dllmain) || (function_symbol->storage[0] & 7) != 1))) {
                if (mode == 2 || (function_symbol->storage[2] & 1))
                    cpp_parser_flags |= 0x8000;
                else
                    report_syntax_error(561, format_symbol_name(function_symbol));
                goto finished;
            }
            if (!(cpp_parser_flags & 4)) {
                cpp_parser_flags |= 4;
                current_return_type = type_0080;
                void_result = 1;
                compiler_warning(1, 508, format_symbol_name(function_symbol));
            } else {
                report_syntax_error(618, format_symbol_name(function_symbol));
            }
        } else if (options.configuration.extensions_enabled &&
                   !(function_symbol->declaration_flags & 0x4000) &&
                   (token_allocation_flags & 0x02000000UL) && !(cpp_parser_flags & 4)) {
            cpp_parser_flags |= 4;
            current_return_type = type_0064;
        }
        if (expression) {
            converted = resolve_assignment_operand(current_return_type, expression);
            if (!converted) goto finished;
            if (!(function_symbol->cpp.parts.member_flags & 0x80)) {
                if (!can_initialize(current_return_type, converted)) {
                    report_type_conversion(440, 0, 0x87, converted->type, current_return_type);
                    goto finished;
                }
                converted = convert_initializer(0x87, current_return_type, converted);
                expression = prepare_reference_initializer(current_return_type, converted);
                if (!expression) goto finished;
            } else {
                tag = cpp_context_stack->items[current_return_type->field08];
                if (!(tag->flags & 4)) return;
                if (!(tag->declaration_flags & 0x1000)) {
                    report_syntax_error(553, tag->name);
                    goto finished;
                }
                expression = identifier_expression(name_return_udt);
                tag = cpp_context_stack->items[current_return_type->field08];
                arguments = append_expression(0, converted, 2);
                converted = make_constructor_call(tag, expression, arguments, 0, 1);
                if (!converted) {
                    tag = cpp_context_stack->items[current_return_type->field08];
                    report_syntax_error(553, tag->name);
                    goto finished;
                }
                if (mode) {
                    converted = finish_return_expression(converted, 0);
                    mode = 0;
                }
                emit_expression_statement(converted);
            }
            saved_type = expression->type;
            if ((token_allocation_flags & 0x800) && saved_type->related &&
                (saved_type->related->attributes & 0xc))
                expression = convert_based_expression(saved_type, expression);
            if (mode) {
                expression = finish_return_expression(expression, 1);
                mode = 0;
            }
            emit_expression_tree(expression);
            expression->type = saved_type;
            write_ir_record(0x41, expression);
            cpp_parser_flags |= 0x8000;
            goto return_branch;
        }
    } else if (expression) {
        if (!options.configuration.extensions_enabled ||
            (function_symbol->declaration_flags & 0x4400) ||
            !(token_allocation_flags & 0x02000000UL) || !(cpp_parser_flags & 4))
            report_syntax_error(562, format_symbol_name(function_symbol));
        else
            report_syntax_error(617, function_symbol->name);
        goto finished;
    }
    if (mode || declaration_mode == 2) {
        begin_return_expression();
        expression = finish_return_expression(0, 0);
        mode = 0;
        if (expression) emit_expression_statement(expression);
    }
return_branch:
    if (mode && !void_result) {
        cleanup = finish_return_expression(expression, 0);
        mode = 0;
        if (cleanup) emit_expression_statement(cleanup);
    }
    write_ir_record(0x3a, function_exit_label);
finished:
    if (mode) finish_return_expression(0, 1);
}

/* Original 0x61260..0x6127a. The right operand supplies the complete
   result type; the ordinary binary constructor owns the node allocation. */
IrToken *__stdcall new_right_typed_expression(unsigned char operation, IrToken *left, IrToken *right)
{
    return new_ir_expression(operation, left, right, right->type);
}
