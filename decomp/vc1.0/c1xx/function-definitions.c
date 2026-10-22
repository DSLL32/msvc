/* Entry, hidden parameters and scope setup for function definitions. */
#include "function-definitions.h"
#include "lexer.h"
#include "cpp-context.h"
#include "parameter-scopes.h"
#include "parameters.h"

CppSymbol *qualified_definition_class; /* original 0x9fbc4 */
CType *current_return_type; /* original 0x9f258 */
CType *structure_return_pointer_type; /* original 0x94b04 */
SegmentSymbol *default_code_segment; /* original 0x9f3f0 */
int browser_macro_end_suppressed; /* original 0x94b0c */

void __stdcall merge_declaration_storage(CppSymbol *existing, CppSymbol *declaration);

/* Original 0x339c0..0x33ff3. Resolve an out-of-class member or ordinary
   definition, recover invalid declarators, and append the implicit ABI
   parameters before establishing function and browser scopes. */
void __stdcall begin_function_definition(DeclarationType *context, CppSymbol *declaration)
{
    QualifiedDeclarator *qualifier;
    QualifierBinding *binding;
    CppSymbol *member, *existing, *tag, *parameter, *first_parameter;
    TypeShape *shape;
    unsigned short attributes;
    int saved_browser_state;

    if (token_allocation_flags & 0x40) flush_current_function(1);
    if (current_class_context >= 0) return;
    qualifier = (QualifiedDeclarator *)declaration->next;
    if (!qualifier || (qualifier->resolution && !qualifier->resolution->binding->next)) {
        declaration->flags |= 4;
        existing = resolve_declaration(context, declaration);
        if (!existing) {
            report_declaration_error(244, declaration);
            declaration->declaration_flags |= 2;
            return;
        }
        declaration = existing;
        declaration->flags &= ~8;
    } else {
        token_allocation_flags |= 0x100;
        binding = qualifier->resolution->binding;
        member = (CppSymbol *)binding->target;
        qualified_definition_class = (CppSymbol *)binding->next->target;
        declaration->kind = 3;
        declaration->storage[0] = context->storage[0];
        declaration->storage[1] = context->storage[1];
        declaration->storage[2] = context->storage[2];
        if ((qualified_definition_class->kind & 31) != 4) {
            binding = binding->next;
            while ((((CppSymbol *)binding->target)->kind & 31) != 4)
                binding = binding->next;
            declaration->next = 0;
            qualified_definition_class = (CppSymbol *)binding->target;
            report_syntax_error(509, declaration->name, qualified_definition_class->name);
            discard_function_definition();
            return;
        }
        if (!(token_allocation_flags & 0x400)) {
            if (context->storage[1] & 2)
                report_syntax_error(723, format_symbol_name(declaration), token_spellings[18]);
            if ((context->storage[0] & 7) == 3) {
                if (options.configuration.extensions_enabled)
                    compiler_warning(1, 222, format_symbol_name(declaration));
                else report_syntax_error(724, format_symbol_name(declaration));
            }
        }
        existing = resolve_declaration(context, declaration);
        if ((member->kind & 31) != 17) {
            if (declaration->cpp.parts.token == 1)
                report_syntax_error(509, declaration->name, qualified_definition_class->name);
            else report_syntax_error(545, token_spellings[declaration->cpp.parts.token]);
            declaration->declaration_flags |= 2;
            token_allocation_flags &= ~0x100UL;
        } else if (!existing) {
            report_syntax_error(511, declaration->name, qualified_definition_class->name);
            declaration->cpp.parts.token = 1;
            declaration->declaration_flags &= ~0x3000;
            declaration->declaration_flags |= 2;
            token_allocation_flags &= ~0x100UL;
            skip_balanced_brace_body();
        } else if ((existing->kind & 31) == 7) {
            merge_declaration_storage(existing, declaration);
            declaration = existing;
            if (existing->flags & 4) {
                report_declaration_error(84, existing);
                existing->declaration_flags |= 2;
            }
            existing->flags = (existing->flags | 4) & ~8;
            activate_qualified_definition_scopes();
        }
    }

    validate_special_function(declaration, 1);
    if ((declaration->kind & 31) == 7 && (declaration->cpp.parts.member_flags & 0x80)) {
        tag = class_tag_for_type(drop_first_type_shape(strip_based_shape(declaration->type)));
        if (!(tag->flags & 4) && !options.prototypes_only)
            report_syntax_error(27, tag->name);
        structure_return_pointer_type = make_class_pointer_type(tag, 0);
        add_declaration_parameter(0, &parsed_parameter_symbols, name_return_udt,
            structure_return_pointer_type);
    }
    if (token_allocation_flags & 0x100) {
        if ((declaration->declaration_flags & 0x1000) &&
            (qualified_definition_class->detail.class_info->alignment_flags & 0x10))
            add_declaration_parameter(1, &parsed_parameter_symbols, name_init_vbases,
                type_4064_first);
        if (!function_declaration_kind) declaration->cpp.parts.member_flags &= ~1;
        if (declaration->cpp.parts.member_flags & 1) {
            shape = declaration->type->related;
            if (shape && !(shape->flags & 0x77)) shape = shape->related;
            add_declaration_parameter(0, &parsed_parameter_symbols, name_this,
                shape->data.parameters->associated_type);
        }
    }
    if ((declaration->kind & 31) != 7) {
        if ((declaration->kind & 31) == 6) report_syntax_error(169, declaration->name);
        else if (parser_lookahead != -1)
            report_syntax_error(239, token_spellings[current_token_code],
                format_symbol_name(declaration));
        else report_syntax_error(54, declaration->name);
        declaration->declaration_flags |= 2;
        discard_function_definition();
        return;
    }
    if (!(declaration->attributes & 0x80)) {
        report_syntax_error(206, declaration->name);
        parsed_parameter_symbols = 0;
    }
    attributes = declaration->type->related->attributes;
    if ((((attributes & 0x70) == 0x40 && (attributes & 7) == 2) ||
         (attributes & 0x200)) &&
        (options.configuration.option_Gw ||
         ((options.configuration.option_GA || options.configuration.option_GD) &&
          ((attributes & 0x400) || options.configuration.option_GEf))))
        report_syntax_error(80);
    if ((declaration->storage[2] & 4) && !(declaration->storage[2] & 8) &&
        !(declaration->storage[1] & 1) && !(token_allocation_flags & 0x400))
        report_syntax_error(491, format_symbol_name(declaration), "function");
    if ((declaration->type->attributes & 7) == 3)
        report_syntax_error(703, declaration->name, token_spellings[54]);
    if (default_code_segment) {
        if (!(declaration->attributes & 2))
            assign_symbol_segment(declaration, default_code_segment);
        else if (declaration->type->related->data.based_symbol != default_code_segment)
            report_syntax_error(193, declaration->name);
    }
    function_symbol = prepare_function_definition(declaration);
    active_parameter_symbols = parsed_parameter_symbols;
    active_function_declaration_kind = function_declaration_kind;
    if (current_return_type->flags != 0x80 && !type_size(current_return_type)) {
        tag = cpp_context_stack->items[current_return_type->field08];
        report_syntax_error(79, function_symbol->name, tag->name);
    }
    if (!browser_filename) return;
    saved_browser_state = browser_macro_end_suppressed;
    browser_macro_end_suppressed = 0;
    browser_begin_tag();
    if (!function_symbol->index) {
        if (next_symbol_index >= 0x7fffffffU) fatal_error(55);
        if (next_symbol_index >= browser_symbol_limit && browser_filename)
            browser_symbol_rollover();
        function_symbol->index = next_symbol_index++;
    }
    browser_declaration(function_symbol);
    browser_enter_tag(function_symbol);
    browser_macro_end_suppressed = saved_browser_state;
    parameter = active_parameter_symbols;
    while (parameter && (parameter->kind & 31) != 10 && (parameter->kind & 31) != 11)
        parameter = parameter->next;
    if (!parameter) return;
    /* The declaration list owns a circular parameter chain. The native
       walker enters its recorded first node without a separate NULL check. */
    first_parameter = parameter = parameter->detail.parameter_next;
    do {
        if (parameter->type) browser_declaration(parameter);
        if (!parameter) return;
        parameter = parameter->detail.parameter_next;
    } while (parameter != first_parameter);
}
